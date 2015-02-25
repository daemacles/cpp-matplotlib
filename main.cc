#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "cpp_plot.hpp"

int main(int argc, char **argv) {
  // DELETE THESE.  Used to suppress unused variable warnings.
  (void)argc;
  (void)argv;

  std::vector<NdArray::dtype> raw_data;

  double x = 0.0;
  while (x < 3.14159 * 4) {
    raw_data.push_back(std::sin(x));
    x += 0.05;
  }

  NdArray data(raw_data);
  plot(data);

  return 0;
}
