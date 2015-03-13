#include <fstream>
#include <sstream>

#include <jsoncpp/json/json.h>

#include "ipython_protocol.hpp"

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
