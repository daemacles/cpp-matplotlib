#pragma once

#include <string>
#include <vector>

#include <zmq.hpp>

class ReqRepConnection {
public:
  ReqRepConnection(const std::string &url) :
      context_{1},
      socket_{context_, ZMQ_REQ},
      url_{url},
      connected_{false}
  {}

  bool Send(const std::string &buffer);
  bool Send(const std::vector<uint8_t> &buffer);
  bool Connect(void);

private:
  zmq::context_t context_;
  zmq::socket_t socket_;
  const std::string url_;
  bool connected_;
};
