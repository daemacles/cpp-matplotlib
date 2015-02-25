#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <unistd.h>

#include <zmq.hpp>

#include "cpp_plot.hpp"

std::string GetName(void) {
  std::default_random_engine generator;
  std::uniform_int_distribution<int> letters_distribution('A', 'Z');
  std::string salt{"cppmpl_"};
  std::string random_name('a', NAME_LEN - salt.length());
  for (size_t i = 0; i != NAME_LEN - salt.size(); ++i) {
    random_name[i] = letters_distribution(generator);
  }
  return salt + random_name;
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
    + NAME_LEN;
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

  std::memcpy(alias, name_.c_str(), NAME_LEN);
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

void plot(const NdArray &data) {
  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);

  std::cout << "Connecting to HW server..." << std::endl;
  socket.connect("tcp://localhost:5555");

  std::vector<uint8_t> buffer(data.WireSize());
  data.SerializeTo(&buffer);
  zmq::message_t request(&buffer[0], buffer.size(), nullptr);

  std::cout << "Sending " << data.Name() << "...";
  socket.send(request);

  // Get the reply
  zmq::message_t reply;
  socket.recv(&reply);
  std::cout << "Received " << std::string((char*)reply.data()) << std::endl;
}
