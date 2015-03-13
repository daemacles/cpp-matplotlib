#pragma once

#include <string>
#include <sstream>

#include <zmq.hpp>

const std::string DELIM{"<IDS|MSG>"};

struct IPyKernelConfig {
  size_t shell_port;
  size_t iopub_port;
  size_t stdin_port;
  size_t hb_port;
  std::string ip;
  std::string transport;
  std::string signature_scheme;
  std::string key;

  IPyKernelConfig (const std::string &jsonConfigFile);
}; 

class IPythonConnection {
  public:
    IPythonConnection(const IPyKernelConfig &config, zmq::context_t &context)
      : config_{config},
      shell_socket_{context, ZMQ_DEALER}
    {}

    void connect(void) {
      std::stringstream uri;
      uri << config_.transport << "://" 
        << config_.ip << ":" << config_.shell_port;
      shell_socket_.bind(uri.str().c_str());
    } 

  private:
    const IPyKernelConfig config_;
    zmq::socket_t shell_socket_;
    //zmq::socket_t stdin_socket_;
    //zmq::socket_t iopub_socket_;
};
