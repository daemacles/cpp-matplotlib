//
// Copyright (c) 2015 Jim Youngquist
// under The MIT License (MIT)
// full text in LICENSE file in root folder of this project.
//

#include <exception>

#include "RequestSink.hpp"

RequestSink::RequestSink(const std::string &url) :
      context_{1},
      socket_{context_, ZMQ_REQ},
      url_{url},
      connected_{false}
{}

bool RequestSink::Send(const std::string &buffer) {
  std::vector<uint8_t> byte_buffer(buffer.begin(), buffer.end());
  return Send(byte_buffer);
}

bool RequestSink::Send(const std::vector<uint8_t> &buffer) {
  zmq::message_t request((void*)&buffer[0], buffer.size(), nullptr);
  socket_.send(request);

  // Get the reply
  zmq::message_t reply;
  socket_.recv(&reply);
  std::string value{reinterpret_cast<char*>(reply.data()), reply.size()};
  if (value != "Success") {
    std::runtime_error(value.c_str());
  }

  return true;
}

bool RequestSink::Connect(void) {
  socket_.connect(url_.c_str());
  connected_ = true;

  return true;
}

