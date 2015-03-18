#include <fstream>
#include <sstream>

#include <jsoncpp/json/json.h>

#include "ipython_protocol.hpp"

/// Delimeter used by the iPython messaging protocol to separate ZMQ
/// identities from message data.
static const std::string DELIM{"<IDS|MSG>"};

std::string GetUuid (void) {
  uuid_t uuid;
  char uuid_str[37] = {'\0'};
  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  return std::string{uuid_str};
}


std::string BuildUri (const IPyKernelConfig &config, PortType port) {
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


IPyKernelConfig::IPyKernelConfig (const std::string &jsonConfigFile) {
  // Parse the config file into JSON
  std::ifstream infile(jsonConfigFile);
  std::stringstream buffer;
  buffer << infile.rdbuf();
  Json::Value config;
  buffer >> config;

  // Assign JSON values to ourselves
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
  return {header, parent, metadata, content};
}


IPythonMessage MessageBuilder::BuildExecuteRequest (
    const std::string &code) const {
  IPythonMessage message{ident_};

  message.header["msg_type"] = "execute_request";
  message.content["code"] = code;
  message.content["silent"] = false;
  message.content["store_history"] = true;
  message.content["user_variables"] = Json::Value(Json::arrayValue);
  message.content["user_expressions"] = Json::Value(Json::objectValue);
  message.content["allow_stdin"] = false;

  return message;
}

void ShellConnection::Connect (void) {
  socket_.setsockopt(ZMQ_DEALER, ident_.data(), ident_.size());
  socket_.connect(uri_.c_str());
}

IPythonMessage ShellConnection::Send (const IPythonMessage &message) {
  if (!socket_.connected()) {
    // TODO throw an exception?
    return IPythonMessage("None");
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

  // Send the message parts
  for (size_t i = 0; i != parts.size()-1; ++i) {
    socket_.send(parts[i].data(), parts[i].size(), ZMQ_SNDMORE);
  }
  socket_.send(parts.back().data(), parts.back().size());

  // Receive the response
  // 1) Strip out the leading stuff
  zmq::message_t response;
  while (true) {
    socket_.recv(&response);
    std::string serialized(reinterpret_cast<char*>(response.data()),
                           response.size());
    if (serialized == DELIM || !response.more()) {
      break;
    }
  }

  // 2) Get the HMAC signature
  // TODO verify contents via HMAC
  std::string hmac_signature{"NONE"};
  if (response.more()) {
    socket_.recv(&response);
    hmac_signature = std::string(reinterpret_cast<char*>(response.data()),
                                 response.size());
  }

  // 3) Get the header, parent, metadata, and content
  std::vector<Json::Value> response_message_parts;
  while (response.more()) {
    socket_.recv(&response);
    std::string serialized(reinterpret_cast<char*>(response.data()),
                           response.size());
    std::stringstream json_stream(serialized);
    Json::Value root;
    json_stream >> root;
    response_message_parts.push_back(root);
  }

  return IPythonMessage{response_message_parts};
}

void ShellConnection::RunCode (const std::string &code) {
  IPythonMessage command = message_builder_.BuildExecuteRequest(code);
  Send(command);
}

bool ShellConnection::HasVariable (const std::string &variable_name) {
  IPythonMessage command = message_builder_.BuildExecuteRequest("None");
  command.content["user_variables"].append(variable_name);
  IPythonMessage response = Send(command);
  return response.content["user_variables"][variable_name]["status"]
    .asString() == "ok";
}


void IPythonSession::Connect (void) {
  shell_connection_.Connect();
}

std::string IPythonSession::ComputeHMAC_ (
    const std::vector<Json::Value> &parts) const {
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
