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
  Json::Value header_;
  Json::Value parent_;
  Json::Value metadata_;
  Json::Value content_;

  IPythonMessage(const std::string ident) 
    : header_{Json::objectValue},
    parent_{Json::objectValue},
    metadata_{Json::objectValue},
    content_{Json::objectValue}
  {
    char username[80];
    getlogin_r(username, 80);
    header_["username"] = std::string(username);
    header_["session"] = ident;
    header_["msg_id"] = GetUuid();
  }

  std::vector<Json::Value> GetMessageParts(void) const;
};


class ExecuteRequestMessage : public IPythonMessage {
public:
  ExecuteRequestMessage (const std::string ident, const std::string &code)
    : IPythonMessage{ident},
    code_{code} 
  {
    header_["msg_type"] = "execute_request";
    content_["code"] = code_;
    content_["silent"] = false;
    content_["store_history"] = true;
    content_["user_variables"] = Json::Value(Json::arrayValue);
    content_["user_expressions"] = Json::Value(Json::objectValue);
    content_["allow_stdin"] = false;
  }

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
  void RunCode (const std::string &code);
  void GetVariable (const std::string &variable_name);

private:
  const HmacFn hmac_fn_;
  const std::string ident_;
  zmq::socket_t socket_; 
  const std::string uri_;
};


class IPythonSession {
public:
  IPythonSession (const IPyKernelConfig &config)
    : config_{config},
    zmq_context_{1},
    hmac_fn_{std::bind(&IPythonSession::ComputeHMAC_, this, 
                       std::placeholders::_1)},
    shell_connection_{config, zmq_context_, hmac_fn_}
  {}

  void Connect (void);
  ShellConnection& Shell (void) { return shell_connection_; }

private:
  std::string ComputeHMAC_(const std::vector<Json::Value> &parts) const;

  const IPyKernelConfig config_;
  zmq::context_t zmq_context_;
  HmacFn hmac_fn_;
  ShellConnection shell_connection_;  
};
