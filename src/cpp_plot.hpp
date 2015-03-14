#pragma once

#include <exception>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "ipython_protocol.hpp"

class NumpyArray; // Forward definition

std::string GetName(void);
std::string LoadFile(std::string filename);
bool SendCode(const std::string &code);
bool SendData(const NumpyArray &data);

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

  void doit() {
    std::string jsonConfigFile("CHANGEME");
    IPyKernelConfig config(jsonConfigFile);

    IPythonSession session(config);
    session.Connect();

    session.Shell().RunCode(LoadFile("../src/pyplot_listener.py"));
    session.Shell().RunCode(R"(
      try:
      lt.running
      except:
      lt = ipython_run(globals())
      )");

    std::vector<NumpyArray::dtype> raw_data;

    double x = 0.0;
    while (x < 3.14159 * 4) {
      raw_data.push_back(std::sin(x));
      x += 0.05;
    }

    NumpyArray data("A", raw_data);
    SendData(data);

    session.Shell().RunCode("plot(A)");
  }
private:
};
