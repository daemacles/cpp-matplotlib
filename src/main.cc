#include <cmath>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "ipython_protocol.hpp"
#include "cpp_plot.hpp"

std::string LoadFile(std::string filename) {
  std::ifstream infile(filename);
  std::stringstream buffer;
  buffer << infile.rdbuf();
  return buffer.str();
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " /path/to/kernel-PID.json"
      << std::endl;
    exit(-1);
  }

  std::string jsonConfigFile(argv[1]);
  IPyKernelConfig config(jsonConfigFile);

  IPythonSession session(config);
  session.Connect();

  session.RunCode(LoadFile("../src/pyplot_listener.py"));
  session.RunCode(R"(
try:
  lt.running
except:
  lt = ipython_run(globals())
)");

  std::vector<NumpyArray::dtype> raw_data;

  double x = 0.0;
  while (x < 3.14159 * 4) {
    raw_data.push_back(std::sin(x));
    x += 0.05;
  }

  NumpyArray data("A", raw_data);
  SendData(data);

  session.RunCode("plot(A)");

  return 0;
}
