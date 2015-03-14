#pragma once

#include <cstdio>
#include <cstring>
#include <unistd.h>
extern "C" {
#include <uuid/uuid.h>
}

#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

#include <jsoncpp/json/json.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <zmq.hpp>

struct IPyKernelConfig; // forward def

typedef std::function<std::string(std::vector<Json::Value>)> HmacFn;
enum class PortType {SHELL, IOPUB, STDIN, HB};
const std::string DELIM{"<IDS|MSG>"};


std::string GetUuid(void);

std::string BuildUri(const IPyKernelConfig &config, PortType port);


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


class IPythonMessage {
public:
  IPythonMessage() {}
  std::vector<Json::Value> GetMessage(const std::string ident) const;
  Json::Value GetContent(void) const;
  std::string GetType(void) const;

protected:
  virtual Json::Value GetContent_(void) const = 0;
  virtual std::string GetType_(void) const = 0;
};


class ExecuteCommandMessage : public IPythonMessage {
public:
  ExecuteCommandMessage (const std::string &code)
    : code_{code} 
  {}

protected:
  virtual Json::Value GetContent_(void) const;
  virtual std::string GetType_(void) const;

private:
  const std::string code_;
};


class ShellConnection {
public:
  ShellConnection(const IPyKernelConfig &config, zmq::context_t &context,
                  const HmacFn &hmac_fn)
    : hmac_fn_{hmac_fn},
    ident_{GetUuid()},
    socket_(context, ZMQ_DEALER),
    uri_{BuildUri(config, PortType::SHELL)}
  {}

  void Connect(void);
  void Send(const IPythonMessage &message);

private:
  const HmacFn hmac_fn_;
  const std::string ident_;
  zmq::socket_t socket_; 
  const std::string uri_;
};


class IPythonSession {
public:
  IPythonSession(const IPyKernelConfig &config)
    : config_{config},
    zmq_context_{1},
    hmac_fn_{std::bind(&IPythonSession::ComputeHMAC_, this, 
                       std::placeholders::_1)},
    shell_connection_{config, zmq_context_, hmac_fn_}
  {}

  void RunCode(std::string code);
  void Connect(void);

private:
  std::string ComputeHMAC_(const std::vector<Json::Value> &parts) const;

  const IPyKernelConfig config_;
  zmq::context_t zmq_context_;
  HmacFn hmac_fn_;
  ShellConnection shell_connection_;  
};
