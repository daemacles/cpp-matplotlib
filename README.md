# About

An easy-to-use library for simple plotting from C++ via a ZeroMQ bridge to
Python's Matplotlib.

# Prereqs

## Ubuntu

    sudo apt-get install libzmq3-dev python-zmq python-matplotlib

Some sort of pyqt 

    sudo apt-get install python-pyside.qtcore python-pyside.qtgui

# Build

    git clone https://bitbucket.org/james_youngquist/cpp-matplotlib.git
    cd cpp-matplotlib
    mkdir build
    cd build
    cmake ..
    make

# Run (alpha demo)
    
In terminal 1:

    python cpp-matplotlib/pyplot_listener.py

In terminal 2:

    cpp-matplotlib/build/cpp-matplotlib

