#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include <zmq.hpp>

#include "cpp_plot.hpp"

#include "ipython_protocol.hpp"
#include "ReqRepConnection.hpp"

const uint32_t NAME_LEN = 16;

// Reads an entire file into a string
std::string LoadFile(std::string filename) {
  std::ifstream infile(filename);
  std::stringstream buffer;
  buffer << infile.rdbuf();
  return buffer.str();
}

// Randomly generates a name.
std::string GetName(void) {
  std::default_random_engine generator;
  std::uniform_int_distribution<int> letters_distribution('A', 'Z');
  std::string salt{"cppmpl_"};
  std::string random_name(NAME_LEN - salt.size(), 'a');
  for (size_t i = 0; i != NAME_LEN - salt.size(); ++i) {
    random_name[i] = letters_distribution(generator);
  }
  std::string name = salt + random_name;
  return name;
}


//======================================================================
void NumpyArray::SetData (const dtype *data, size_t rows, size_t cols) {
  dtype *tmp = new dtype[rows*cols];
  std::memcpy(tmp, data, sizeof(dtype)*rows*cols);
  data_ = std::unique_ptr<dtype[]>(tmp);
  rows_ = rows;
  cols_ = cols;
}

uint32_t NumpyArray::WireSize (void) const {
  return sizeof(rows_) + sizeof(cols_) + 1 + sizeof(dtype)*rows_*cols_
    + name_.size();
}

void NumpyArray::SerializeTo (std::vector<uint8_t> *buffer) const {
  if (buffer->size() < WireSize()) {
    buffer->reserve(WireSize());
  }

  uint8_t *alias = &(*buffer)[0];

  std::memcpy(alias, &rows_, sizeof(rows_));
  alias += sizeof(rows_);

  std::memcpy(alias, &cols_, sizeof(cols_));
  alias += sizeof(cols_);

  uint8_t dtype_size = sizeof(dtype);
  std::memcpy(alias, &dtype_size, 1);
  alias += 1;

  std::memcpy(alias, data_.get(), sizeof(dtype)*rows_*cols_);
  alias += sizeof(dtype)*rows_*cols_;

  std::memcpy(alias, name_.c_str(), name_.size());
}

const NumpyArray::dtype* NumpyArray::DataRef (void) const {
  return data_.get();
}

uint32_t NumpyArray::Cols (void) const {
  return cols_;
}

uint32_t NumpyArray::Rows (void) const {
  return rows_;
}

std::string NumpyArray::Name (void) const {
  return name_;
}

//======================================================================
CppMatplotlib::CppMatplotlib (const std::string &config_filename)
  : upConfig_{new IPyKernelConfig(config_filename)},
  upData_conn_{new ReqRepConnection("tcp://localhost:5555")},
  upSession_{new IPythonSession(*upConfig_)}
{}

CppMatplotlib::~CppMatplotlib (void) {
  return;
}

void CppMatplotlib::Connect () {
  upSession_->Connect();
  upData_conn_->Connect();

  auto &shell = upSession_->Shell();

  if (!shell.HasVariable("data_listener_thread")) {
    shell.RunCode(LoadFile("../src/pyplot_listener.py"));
    shell.RunCode("data_listener_thread = ipython_run(globals())");
  }
}

bool CppMatplotlib::SendData(const NumpyArray &data) {
  std::vector<uint8_t> buffer(data.WireSize());
  data.SerializeTo(&buffer);
  upData_conn_->Send(buffer);
  return true;
}


bool CppMatplotlib::RunCode(const std::string &code) {
  upSession_->Shell().RunCode(code);
  return true;
}
