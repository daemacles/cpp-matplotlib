import os
import time
import signal
import struct
import sys
import Queue

from PyQt4 import QtGui
from PyQt4 import QtCore
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt4agg import NavigationToolbar2QTAgg as NavigationToolbar
import matplotlib.pyplot as plt
import numpy as np
import zmq


class ListenerThread(QtCore.QThread):
  def __init__(self, global_env):
    super(ListenerThread, self).__init__()
    self.running = True
    self.global_env = global_env


  def decodeData(self, message):
    assert len(message) >= 9, "Message is not long enough"

    rows, cols, size = struct.unpack('IIB', message[:9])
    length = rows*cols*size
    assert len(message) >= 9+length, "Message contains insufficient data"

    dtype = {4 : np.float32,
         8 : np.float64}[size]
    data = np.fromstring(message[9:9+length], dtype=dtype)
    data = data.reshape(rows, cols)

    assert len(message[9+length:]) > 0, "Message has zero length name field"
    name = message[9+length:]

    return data, name


  def stop(self):
    self.running = False


  def sendString(self, socket, string):
    try:
      socket.send(b"Success" + chr(0))
    except zmq.error.ZMQError as e:
      return False
    return True


  def sendSuccess(self, socket):
    return self.sendString(socket, "Success")


  def sendFailure(self, socket):
    return self.sendString(socket, "Failure")


  def processData(self, data_message):
    data, name = self.decodeData(data_message)
    self.global_env[name] = data
    return True


  def processCode(self, code_message):
    print "I'm running:\n", code_message
    exec(code_message, self.global_env)
    return True


  def run(self):
    context = zmq.Context()

    data_socket = context.socket(zmq.REP)
    data_socket.bind("tcp://*:5555")

    code_socket = context.socket(zmq.REP)
    code_socket.bind("tcp://*:5556")

    poller = zmq.Poller()
    poller.register(data_socket, flags = zmq.POLLIN)
    poller.register(code_socket, flags = zmq.POLLIN)

    print "Listener started"

    processors = {
        data_socket : self.processData,
        code_socket : self.processCode
        }

    while self.running:
      for socket, event in poller.poll(timeout=20):
        try:
          if event:
            message = socket.recv()
            success = processors[socket](message)
            if success:
              self.sendSuccess(socket)
            else:
              self.sendFailure(socket)
        except zmq.error.ZMQError as e:
          # transmit error...oops...die
          self.running = False
          continue

    print "Listener stopped"


class PlotWindow(QtGui.QDialog):
  def __init__(self, parent=None):
    super(PlotWindow, self).__init__(parent)

    # a figure instance to plot on
    self.figure = plt.figure()

    # this is the Canvas Widget that displays the `figure`
    # it takes the `figure` instance as a parameter to __init__
    self.canvas = FigureCanvas(self.figure)

    # this is the Navigation widget
    # it takes the Canvas widget and a parent
    self.toolbar = NavigationToolbar(self.canvas, self)

    # Just some button connected to `plot` method
    self.plotButton = QtGui.QPushButton('Plot')
    self.plotButton.clicked.connect(self.plot)

    # Just some button connected to `clear` method
    self.clearButton = QtGui.QPushButton('Clear')
    self.clearButton.clicked.connect(self.clear)
    
    # set the layout
    layout = QtGui.QVBoxLayout()
    layout.addWidget(self.toolbar)
    layout.addWidget(self.canvas)
    layout.addWidget(self.plotButton)
    layout.addWidget(self.clearButton)
    self.setLayout(layout)

    self.data = np.array([1])

  def addData(self, data, name):
    self.data = data

  def clear(self):
    self.figure.clear()
    self.canvas.draw()

  def plot(self):
    # create an axis
    ax = self.figure.add_subplot(111)

    # discards the old graph
    ax.hold(False)

    # plot data
    ax.plot(self.data)

    # refresh canvas
    self.canvas.draw()


def ipython_run(global_env):
  lt = ListenerThread(global_env)
  lt.start()
  return lt


'''
if __name__ == "__main__":
  app = QtGui.QApplication(sys.argv)

  plotWindow = PlotWindow()
  plotWindow.show()

  listenerThread = ListenerThread(globals())
  listenerThread.start()

  sys.exit(app.exec_())
'''
