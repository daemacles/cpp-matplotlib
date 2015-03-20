//
// Copyright (c) 2015 Jim Youngquist
// under The MIT License (MIT)
// full text in LICENSE file in root folder of this project.
//

#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include <zmq.hpp>

#include "cpp_plot.hpp"

#include "ipython_protocol.hpp"
#include "RequestSink.hpp"

// Names of the python variables
static const std::string THREAD_VAR_NAME{"cpp_ipython_listener_thread"};
static const std::string PORT_VAR_NAME{"cpp_ipython_listener_thread_port"};

std::string LoadFile(std::string filename) {
  std::ifstream infile(filename);
  std::stringstream buffer;
  buffer << infile.rdbuf();
  return buffer.str();
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
  upData_conn_{nullptr}, // don't know what port listener thread will be on
  upSession_{new IPythonSession(*upConfig_)}
{}

CppMatplotlib::~CppMatplotlib (void) {
  return;
}

void CppMatplotlib::Connect () {
  upSession_->Connect();

  auto &shell = upSession_->Shell();

  if (!shell.HasVariable(THREAD_VAR_NAME)) {
    shell.RunCode(LoadFile("../src/pyplot_listener.py"));
    shell.RunCode("cpp_ipython_start_thread(globals())");
  }
  std::string port_str = shell.GetVariable(PORT_VAR_NAME);

  upData_conn_.reset(new RequestSink("tcp://localhost:" + port_str)),
  upData_conn_->Connect();
}

bool CppMatplotlib::SendData(const NumpyArray &data) {
  std::vector<uint8_t> buffer(data.WireSize());
  data.SerializeTo(&buffer);
  return upData_conn_->Send(buffer);
}

void CppMatplotlib::RunCode(const std::string &code) {
  upSession_->Shell().RunCode(code);
}
