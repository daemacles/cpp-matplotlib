#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <unistd.h>

#include <zmq.hpp>

#include "cpp_plot.hpp"
#include "ReqRepConnection.hpp"

const uint32_t NAME_LEN = 16;

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

void NdArray::SetData (const dtype *data, size_t rows, size_t cols) {
  dtype *tmp = new dtype[rows*cols];
  std::memcpy(tmp, data, sizeof(dtype)*rows*cols);
  data_ = std::unique_ptr<dtype[]>(tmp);
  rows_ = rows;
  cols_ = cols;
}

uint32_t NdArray::WireSize (void) const {
  return sizeof(rows_) + sizeof(cols_) + 1 + sizeof(dtype)*rows_*cols_
    + name_.size();
}

void NdArray::SerializeTo (std::vector<uint8_t> *buffer) const {
  if (buffer->size() < WireSize()) {
    throw std::runtime_error{"supplied buffer is not large enough"};
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

const NdArray::dtype* NdArray::DataRef (void) const {
  return data_.get();
}

uint32_t NdArray::Cols (void) const {
  return cols_;
}

uint32_t NdArray::Rows (void) const {
  return rows_;
}

std::string NdArray::Name (void) const {
  return name_;
}

bool SendData(const NdArray &data) {
  ReqRepConnection data_conn("tcp://localhost:5555");
  data_conn.Connect();

  std::vector<uint8_t> buffer(data.WireSize());
  data.SerializeTo(&buffer);

  std::cout << "data sending " << data.Name() << "..." << std::endl;
  data_conn.Send(buffer);

  return true;
}

bool SendCode(const std::string &code) {
  ReqRepConnection code_conn("tcp://localhost:5556");
  code_conn.Connect();

  std::cout << "code sending ..." << std::endl;
  code_conn.Send(code);

  return true;
}
