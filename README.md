# Contents

[About](#about)
[Usage] (#usage)
[Prereqs](#prereqs)
[Building](#building)
[Example](#example)


# About

An easy-to-use library for simple plotting from C++ via a ZeroMQ bridge to
an [IPython](http://ipython.org/) kernel. 

It provides the ability to send [NumPy](http://www.numpy.org/) array
compatible data to an IPython kernel session as well as execute arbitrary
code.

Execution of arbitrary code is "protected" by IPython's kernel HMAC signing
mechanism: only code signed by a secret key provided by IPython will run.  As
of 2015-03-19 it has been tested with IPython version 1.2.1 on Ubuntu 14.04.


# Usage

Here we create some 1D data and plot it.  The variable "A" will be available
for working with in the IPython session, even after the C++ program finishes.

    CppMatplotlib mpl{"/path/to/kernel-NNN.json"};
    mpl.Connect();

    // Create a nice curve  
    std::vector<NumpyArray::dtype> raw_data;
    double x = 0.0;
    while (x < 3.14159 * 4) {
      raw_data.push_back(std::sin(x));
      x += 0.05;
    }

    // Send it to IPython for plotting
    NumpyArray data("A", raw_data);
    mpl.SendData(data);
    mpl.RunCode("plot(A)\n"
                "title('f(x) = sin(x)')\n"
                "xlabel('x')\n"
                "ylabel('f(x)')\n");

And the result will be ![Screenshot](screenshot.png?raw=true "Screenshot of
sin(x)")

See [src/main.cc](src/main.cc) for a complete program.


# Prereqs

## Ubuntu 14.04

    sudo apt-get install ipython python-matplotlib libzmq3-dev \
                         libjsoncpp-dev uuid-dev libssl-dev


# Building

    git clone https://bitbucket.org/james_youngquist/cpp-matplotlib.git
    cd cpp-matplotlib
    mkdir build
    cd build
    cmake ..
    make

## Generating the documentation

    cd cpp-matplotlib
    doxygen Doxyfile
    # open html/index.html

# Example
    
In terminal 1:

    ipython kernel --pylab
    # this will print out the kernel PID to connect to, NNN below.

In terminal 2:

    # Once per kernel invocation:
    export KERNEL_CONFIG=`find ~/ -name kernel-NNN.json`

    # Each time you run the program
    build/cpp-matplotlib-example $KERNEL_CONFIG
    

In terminal 3 (if desired):

    ipython console --existing kernel-NNN.json

