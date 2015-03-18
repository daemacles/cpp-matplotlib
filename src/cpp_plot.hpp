#pragma once

#include <exception>
#include <memory>
#include <random>
#include <string>
#include <vector>

// Forward declarations
struct IPyKernelConfig;
class IPythonSession;
class ReqRepConnection;

std::string GetName(void);
std::string LoadFile(std::string filename);


//======================================================================
/** \brief Container class to represent a 2D Numpy array.
 *
 * The NumpyArray is named at construction with the name of the python
 * variable into which its data will be placed.  The name is immutable, but
 * the data held by this array is not.  The data must be of type
 * NumpyArray::dtype.
 *
 * Usage:
\code
    // Create some 1D data that could be sent to an iPython kernel.
    std::vector<NumpyArray::dtype> raw_data;
    double x = 0.0;
    while (x < 3.14159 * 4) {
      raw_data.push_back(std::sin(x));
      x += 0.05;
    }

    // Put it in the NumpyArray container.
    NumpyArray data("Data", raw_data);
\endcode
 */
class NumpyArray {
public:
  /** The type expected by numpy when reading the raw data buffer.  The data
   * passed to NumpyArray must be of this type.
   */
  typedef double dtype;

  //--------------------------------------------------
  /** \brief Constructs an empty Numpy compatible array associated with a
   * named variable in the iPython session.
   *
   * \param name  the name the numpy array will have in the ipython session.
   */
  explicit NumpyArray (const std::string &name) 
    : name_{name}, data_{nullptr}, rows_{0}, cols_{0} 
  {}

  //--------------------------------------------------
  /** \brief Constructs a Numpy compatible Nx1 array associated with a named
   * variable in the iPython session and initialized with 1D data.
   *
   * \param name  the name the numpy array will have in the ipython session.
   * \param row_data  the 1D data, will become a column matrix in iPython.
   */
  NumpyArray (const std::string name, const std::vector<dtype> &row_data) 
    : NumpyArray {name, row_data, row_data.size(), 1}
  {}

  //--------------------------------------------------
  /** \brief Constructs an empty Numpy compatible array associated with a
   * named variable in the iPython session.
   *
   * \param name  the name the numpy array will have in the ipython session.
   * \param data  a vector<dtype> buffer in row major order.
   * \param rows  the number of rows in data.
   * \param cols  the number of columns in data.
   */
  NumpyArray (const std::string name, const std::vector<dtype> data, 
              size_t rows, size_t cols)
    : NumpyArray{name}
  {
    if (data.size() < rows*cols) {
      throw std::runtime_error("data.size() must not be less than rows*cols");
    }
    SetData(&data[0], rows, cols);
  }
  
  //--------------------------------------------------
  /** \brief Constructs an empty Numpy compatible array associated with a
   * named variable in the iPython session.
   *
   * \param name  the name the numpy array will have in the ipython session.
   * \param data  pointer to a contiguous memory array in row major order.
   * \param rows  the number of rows in data.
   * \param cols  the number of columns in data.
   */
  NumpyArray (const std::string &name, const dtype *data, 
              size_t rows, size_t cols)
    : NumpyArray{name} 
  {
    SetData(data, rows, cols);
  }

  //--------------------------------------------------
  /** \brief Sets the contents of this container.
   *
   * \param data  pointer to a contiguous memory buffer in row major order.
   * \param rows  the number of rows in data.
   * \param cols  the number of columns in data.
   */
  void SetData (const dtype *data, size_t rows, size_t cols);

  //--------------------------------------------------
  /** \brief Returns the size of this container in bytes when it has been
   * serialized for sending to iPython.
   */
  uint32_t WireSize (void) const;

  //--------------------------------------------------
  /** \brief Serializes this container to a byte buffer.
   *
   * \param buffer  the byte buffer to be serialized into.  Overwrites
   *                previous contents.
   */
  void SerializeTo (std::vector<uint8_t> *buffer) const;
  
  //--------------------------------------------------
  /** \brief Returns a pointer to the row-major order data held by this 
   * container.
   */
  const dtype* DataRef (void) const;

  //--------------------------------------------------
  /** \brief Returns the number of rows in this array.
   */
  uint32_t Rows (void) const;

  //--------------------------------------------------
  /** \brief Returns the number of columns in this array.
   */
  uint32_t Cols (void) const;

  //--------------------------------------------------
  /** \brief Returns the name of the iPython variable this array is associated
   * with.
   */
  std::string Name (void) const;

private:
  const std::string name_;
  std::unique_ptr<dtype[]> data_;
  uint32_t rows_;
  uint32_t cols_;
};

//======================================================================
/** \brief Interface between C++ and an IPython kernel with the pylab
 * environment.
 *
 * Presents a simple way to send code and Numpy compatible data to a
 * seperately running IPython kernel.
 *
 * Example usage:
\code
    // Create the mpl object and connect to an iPython kernel.
    CppMatplotlib mpl{"path/to/my/kernel-NNNN.json"};
    mpl.Connect();

    // Create some 1D data that we want to work with and visualize in iPython.
    std::vector<NumpyArray::dtype> raw_data;
    // ... load values into raw_data ...

    // Put it in the NumpyArray container and send it.
    NumpyArray data("Data", raw_data);
    mpl.SendData(data);

    // Plot the data.  If iPython is running the pylab environment, this will
    // cause a Figure window to pop up.
    mpl.RunCode("plot(Data)");
\endcode
 */
class CppMatplotlib {
public:
  //----------------------------------------------------------------------
  /** \brief Create a CppMatplotlib object that is configured to connect to
   * the iPython kernel as specified in the config_filename JSON file.
   */
  explicit CppMatplotlib (const std::string &config_filename);

  //----------------------------------------------------------------------
  // Note: Must declare destructor here and define it in the .cc file so it is
  // not implicitly inline, which allows the use of unique_ptrs with forward
  // declarations.
  ~CppMatplotlib (void); 

  //----------------------------------------------------------------------
  /** \brief Connects to the ipython kernel according to the configuration
   * given to the constructor.
   */
  void Connect (void);

  //----------------------------------------------------------------------
  /** \brief Runs code in the iPython kernel.
   *
   * The code is eval'd by the iPython kernel as though the user had typed it
   * in by hand at an interactive session.
   *
   * \param code the code as a single string.  If it spans multiple lines, it
   * must explicitly contain newline characters and appropriate whitespace.
   */
  bool RunCode (const std::string &code);

  //----------------------------------------------------------------------
  /** \brief Sends a Numpy compatible array to the iPython kernel's global
   * namespace.
   *
   * \param data the data to send.  After sending, it will immediately be
   * available for working with in the iPython session.
   */
  bool SendData (const NumpyArray &data);

private:
  std::unique_ptr<IPyKernelConfig> upConfig_;
  std::unique_ptr<ReqRepConnection> upData_conn_;
  std::unique_ptr<IPythonSession> upSession_;
};
