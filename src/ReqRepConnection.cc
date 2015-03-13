#include "ReqRepConnection.hpp"

bool ReqRepConnection::Send(const std::string &buffer) {
  std::vector<uint8_t> byte_buffer(buffer.begin(), buffer.end());
  return Send(byte_buffer);
}

bool ReqRepConnection::Send(const std::vector<uint8_t> &buffer) {
  zmq::message_t request((void*)&buffer[0], buffer.size(), nullptr);
  socket_.send(request);

  // Get the reply
  zmq::message_t reply;
  socket_.recv(&reply);

  return true;
}

bool ReqRepConnection::Connect(void) {
  socket_.connect(url_.c_str());
  connected_ = true;

  return true;
}

