#include <fstream>
#include <sstream>

#include <jsoncpp/json/json.h>

#include "ipython_protocol.hpp"

std::string GetUuid(void) {
  uuid_t uuid;
  char uuid_str[37] = {'\0'};
  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  return std::string{uuid_str};
}


std::string BuildUri(const IPyKernelConfig &config, PortType port) {
  std::stringstream uri;
  uri << config.transport << "://" << config.ip << ":";
  switch (port) {
  case PortType::SHELL: uri << config.shell_port; break;
  case PortType::IOPUB: uri << config.iopub_port; break;
  case PortType::STDIN: uri << config.stdin_port; break;
  case PortType::HB:    uri << config.hb_port;    break;
  default:                                        break;
  }
  return uri.str();
}


IPyKernelConfig::IPyKernelConfig(const std::string &jsonConfigFile) {
  // Parse the config file into JSON
  std::ifstream infile(jsonConfigFile);
  std::stringstream buffer;
  buffer << infile.rdbuf();
  Json::Value config;
  buffer >> config;

  // Assign JSON to ourselves
  shell_port = config["shell_port"].asUInt();
  iopub_port = config["iopub_port"].asUInt();
  stdin_port = config["stdin_port"].asUInt();
  hb_port = config["hb_port"].asUInt();
  ip = config["ip"].asString();
  transport = config["transport"].asString();
  signature_scheme = config["signature_scheme"].asString();
  key = config["key"].asString();
}


std::vector<Json::Value> IPythonMessage::GetMessageParts (void) const {
  return {header_, parent_, metadata_, content_};
}



void ShellConnection::Connect(void) {
  socket_.setsockopt(ZMQ_DEALER, ident_.data(), ident_.size());
  socket_.connect(uri_.c_str());
}

void ShellConnection::Send(const IPythonMessage &message) {
  if (!socket_.connected()) {
    // TODO throw an exception?
    return;
  }

  std::vector<Json::Value> message_parts = message.GetMessageParts();

  std::vector<std::vector<uint8_t>> parts;
  parts.push_back(std::vector<uint8_t>(DELIM.begin(), DELIM.end()));
  std::string hmac = hmac_fn_(message_parts);
  parts.push_back(std::vector<uint8_t>(hmac.begin(), hmac.end()));
  Json::FastWriter writer;
  for (const Json::Value &section : message_parts) {
    std::string serialized = writer.write(section);
    parts.push_back(std::vector<uint8_t>(serialized.begin(),
                                         serialized.end()));
  }

  std::cout << "Sending..." << std::endl;
  for (size_t i = 0; i != parts.size()-1; ++i) {
    socket_.send(parts[i].data(), parts[i].size(), ZMQ_SNDMORE);
  }
  socket_.send(parts.back().data(), parts.back().size());

  std::cout << "Receiving..." << std::endl;
  // TODO actually get and parse this response and return it.
  while(true) {
    zmq::message_t response;
    socket_.recv(&response);
    std::fwrite(response.data(), 1, response.size(), stdout);
    if (!response.more()) {
      break;
    }
  }
}

void ShellConnection::RunCode (const std::string &code) {
  ExecuteRequestMessage command(ident_, code);
  Send(command);
}

void ShellConnection::GetVariable (const std::string &variable_name) {
  ExecuteRequestMessage command(ident_, "None");
  command.content_["user_variables"].append(variable_name);
  Send(command);
}


void IPythonSession::Connect(void) {
  shell_connection_.Connect();
}

std::string IPythonSession::ComputeHMAC_(const std::vector<Json::Value> &parts) const {
  ENGINE_load_builtin_engines();
  ENGINE_register_all_complete();

  HMAC_CTX hmac_ctx;
  HMAC_CTX_init(&hmac_ctx);
  HMAC_Init_ex(&hmac_ctx, config_.key.data(), config_.key.size(),
               EVP_sha256(), nullptr);

  Json::FastWriter writer;
  for (const auto& part : parts) {
    std::string serialized{writer.write(part)};
    HMAC_Update(&hmac_ctx, (const uint8_t*)(serialized.data()),
                serialized.size());
  }

  unsigned int result_len = 32;
  uint8_t result[32];
  HMAC_Final(&hmac_ctx, result, &result_len);
  HMAC_CTX_cleanup(&hmac_ctx);

  std::stringstream hmac;
  for (size_t i = 0; i != 32; ++i) {
    hmac << std::setfill('0') << std::setw(2) << std::hex
      << static_cast<int>(result[i]);
  }

  return hmac.str();
}
