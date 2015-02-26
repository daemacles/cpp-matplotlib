#pragma once

#include <exception>
#include <memory>
#include <random>
#include <string>
#include <vector>

std::string GetName(void);

class NdArray {
public:
  typedef double dtype;

  NdArray (void) : name_{GetName()}, data_{nullptr}, rows_{0}, cols_{0} {}

  NdArray (const std::vector<dtype> &row_data) : 
    NdArray {row_data, row_data.size(), 1} {}

  NdArray (const std::vector<dtype> data, size_t rows, size_t cols) : 
      NdArray{}
  {
    if (data.size() < rows*cols) {
      throw std::runtime_error("data.size() must not be less than rows*cols");
    }
    SetData(&data[0], rows, cols);
  }
  
  NdArray (const dtype *data, size_t rows, size_t cols) : NdArray{} {
    SetData(data, rows, cols);
  }

  void SetData (const dtype *data, size_t rows, size_t cols);

  uint32_t WireSize (void) const;
  void SerializeTo (std::vector<uint8_t> *buffer) const;
  
  // Getters
  const dtype* DataRef (void) const;
  uint32_t Rows (void) const;
  uint32_t Cols (void) const;
  std::string Name (void) const;

private:
  const std::string name_;
  std::unique_ptr<dtype[]> data_;
  uint32_t rows_;
  uint32_t cols_;
};


bool SendCode(const std::string &code);
bool SendData(const NdArray &data);
