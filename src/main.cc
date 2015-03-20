//
// Copyright (c) 2015 Jim Youngquist
// under The MIT License (MIT)
// full text in LICENSE file in root folder of this project.
//

#include <cmath>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "cpp_plot.hpp"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " /path/to/kernel-PID.json"
      << std::endl;
    exit(-1);
  }

  CppMatplotlib mpl{argv[1]};
  mpl.Connect();

  std::vector<NumpyArray::dtype> raw_data;

  double x = 0.0;
  while (x < 3.14159 * 4) {
    raw_data.push_back(std::sin(x));
    x += 0.05;
  }

  NumpyArray data("A", raw_data);
  mpl.SendData(data);
  mpl.RunCode("plot(A)\n"
              "title('f(x) = sin(x)')\n"
              "xlabel('x')\n"
              "ylabel('f(x)')\n");

  return 0;
}
