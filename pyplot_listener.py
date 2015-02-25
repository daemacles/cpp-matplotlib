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


def decode(message):
    rows, cols, size = struct.unpack('IIB', message[:9])
    length = rows*cols*size
    dtype = {4 : np.float32,
             8 : np.float64}[size]
    data = np.fromstring(message[9:9+length], dtype=dtype)
    data = data.reshape(rows, cols)
    name = message[9+length:]
    return data, name


class ListenerThread(QtCore.QThread):
    newData = QtCore.pyqtSignal(name='newData')

    def __init__(self, dataQ):
        super(ListenerThread, self).__init__()
        self.running = True
        self.dataQ = dataQ

    def stop(self):
        self.running = False

    def run(self):
        context = zmq.Context()
        socket = context.socket(zmq.REP)
        socket.bind("tcp://*:5555")

        print "Listener started"

        while self.running:
            try:
                message = socket.recv()
            except zmq.error.ZMQError as e:
                self.running = False
                continue

            data, name = decode(message)

            self.dataQ.put((data, name))
            self.newData.emit()

            try:
                socket.send(b"Success" + chr(0))
            except zmq.error.ZMQError as e:
                running = False
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


if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)

    # Queue to receive data objects upon
    dataQ = Queue.Queue()

    plotWindow = PlotWindow()
    plotWindow.show()

    listenerThread = ListenerThread(dataQ)
    listenerThread.start()

    def newData():
        data, name = dataQ.get()
        plotWindow.addData(data, name)
        plotWindow.plot()

    listenerThread.newData.connect(newData)


    sys.exit(app.exec_())
