#pragma once

#include <exception>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "ipython_protocol.hpp"
#include "ReqRepConnection.hpp"

class NumpyArray; // Forward definition

std::string GetName(void);
std::string LoadFile(std::string filename);


class NumpyArray {
public:
  typedef double dtype;

  explicit NumpyArray (const std::string &name) 
    : name_{name}, data_{nullptr}, rows_{0}, cols_{0} 
  {}

  NumpyArray (const std::string name, const std::vector<dtype> &row_data) 
    : NumpyArray {name, row_data, row_data.size(), 1}
  {}

  NumpyArray (const std::string name, const std::vector<dtype> data, 
              size_t rows, size_t cols)
    : NumpyArray{name}
  {
    if (data.size() < rows*cols) {
      throw std::runtime_error("data.size() must not be less than rows*cols");
    }
    SetData(&data[0], rows, cols);
  }
  
  NumpyArray (const std::string &name, const dtype *data, 
              size_t rows, size_t cols)
    : NumpyArray{name} 
  {
    SetData(data, rows, cols);
  }

  void SetData (const dtype *data, size_t rows, size_t cols);

  uint32_t WireSize (void) const;
  void SerializeTo (std::vector<uint8_t> *buffer) const;
  
  // Getters
  const dtype* DataRef (void) const;
  uint32_t Rows (void) const;
  uint32_t Cols (void) const;
  std::string Name (void) const;

private:
  const std::string name_;
  std::unique_ptr<dtype[]> data_;
  uint32_t rows_;
  uint32_t cols_;
};


class CppMatplotlib {
public:
  CppMatplotlib (const std::string &config_filename)
    : config_{config_filename},
    data_conn_{"tcp://localhost:5555"},
    session_{config_}
  {}

  void Connect () {
    session_.Connect();
    data_conn_.Connect();

    auto &shell = session_.Shell();

    if (!shell.HasVariable("data_listener_thread")) {
      shell.RunCode(LoadFile("../src/pyplot_listener.py"));
      shell.RunCode("data_listener_thread = ipython_run(globals())");
    }
  }

  void Connect (void) const;
  bool RunCode (const std::string &code);
  bool SendData (const NumpyArray &data);

private:
  IPyKernelConfig config_;
  ReqRepConnection data_conn_;
  IPythonSession session_;
};
