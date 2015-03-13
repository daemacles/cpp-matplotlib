#include <cmath>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <cstdio>
#include <cstring>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <unistd.h>
#include <jsoncpp/json/json.h>
#include <zmq.hpp>

extern "C" {
#include <uuid/uuid.h>
}

#include "ipython_protocol.hpp"
#include "cpp_plot.hpp"


std::vector<uint8_t> ToLineFormat(const std::string &msg) {
  std::vector<uint8_t> buffer(1 + msg.size());

  buffer[0] = static_cast<uint8_t>(msg.length());

  memcpy(&buffer[1], msg.data(), msg.length());

  return buffer;
}


std::string ComputeHMAC(const IPyKernelConfig &config,
                        const std::vector<Json::Value> &parts) {
  ENGINE_load_builtin_engines();
  ENGINE_register_all_complete();

  HMAC_CTX hmac_ctx;
  HMAC_CTX_init(&hmac_ctx);
  HMAC_Init_ex(&hmac_ctx, config.key.data(), config.key.size(), EVP_sha256(),
               nullptr);

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


int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " /path/to/kernel-PID.json"
      << std::endl;
    exit(-1);
  }

  std::string jsonConfigFile(argv[1]);
  IPyKernelConfig config(jsonConfigFile);

//  std::vector<NumpyArray::dtype> raw_data;
//
//  double x = 0.0;
//  while (x < 3.14159 * 4) {
//    raw_data.push_back(std::sin(x));
//    x += 0.05;
//  }
//
//  NumpyArray data(raw_data);
//  SendData(data);
//  SendCode("a=1+3");

  uuid_t uuid;
  char uuid_str[37] = {'\0'};

  // Create the header
  Json::Value header;

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  header["msg_id"] = std::string{uuid_str};

  char username[80];
  getlogin_r(username, 80);
  header["username"] = std::string(username);

  uuid_generate_random(uuid);
  uuid_unparse(uuid, uuid_str);
  //header["session"] = std::string{uuid_str};
  std::string ident(uuid_str);
  header["session"] = ident;

  header["msg_type"] = "execute_request";

  // Empty parent header
  Json::Value parent{Json::objectValue};

  // Empty metadata
  Json::Value metadata(Json::objectValue);

  // And finally, the content
  Json::Value content;
  content["code"] = "plot([1,2,3],[1,4,9])\n";
  content["silent"] = false;
  content["store_history"] = true;
  content["user_variables"] = Json::Value(Json::arrayValue);
  content["user_expressions"] = Json::Value(Json::objectValue);
  content["allow_stdin"] = false;

  Json::Value message;
  message["header"] = header;

  std::vector<std::vector<uint8_t>> parts;
  parts.push_back(std::vector<uint8_t>(DELIM.begin(), DELIM.end()));

  std::string hmac = ComputeHMAC(config, {header, parent, metadata, content});
  parts.push_back(std::vector<uint8_t>(hmac.begin(), hmac.end()));

  Json::FastWriter writer;
  std::string serialized;
  serialized = writer.write(header);
  parts.push_back(std::vector<uint8_t>(serialized.begin(), serialized.end()));
  serialized = writer.write(parent);
  parts.push_back(std::vector<uint8_t>(serialized.begin(), serialized.end()));
  serialized = writer.write(metadata);
  parts.push_back(std::vector<uint8_t>(serialized.begin(), serialized.end()));
  serialized = writer.write(content);
  parts.push_back(std::vector<uint8_t>(serialized.begin(), serialized.end()));

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_DEALER);
  socket.setsockopt(ZMQ_DEALER, ident.data(), ident.size());
  std::stringstream uri;
  uri << config.transport << "://"
    << config.ip << ":" << config.shell_port;
  std::cout << "Connecting to " << uri.str().c_str() << std::endl;
  socket.connect(uri.str().c_str());

  std::cout << "Sending..." << std::endl;
  for (size_t i = 0; i != parts.size()-1; ++i) {
    socket.send(parts[i].data(), parts[i].size(), ZMQ_SNDMORE);
  }
  socket.send(parts.back().data(), parts.back().size());

  std::cout << "Receiving..." << std::endl;
  while(true) {
    zmq::message_t response;
    socket.recv(&response);
    std::fwrite(response.data(), 1, response.size(), stdout);
    if (!response.more()) {
      break;
    }
  }

  return 0;
}
