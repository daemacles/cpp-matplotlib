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
    newData = QtCore.pyqtSignal(float, name='newData')

    def __init__(self):
        super(ListenerThread, self).__init__()
        self.running = True

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

            value = struct.unpack('d', message)[0]

            print("Received request: {}".format(value))

            self.newData.emit(value)

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
        self.button = QtGui.QPushButton('Plot')
        self.button.clicked.connect(self.plot)

        # set the layout
        layout = QtGui.QVBoxLayout()
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)
        layout.addWidget(self.button)
        self.setLayout(layout)

        self.data = []

    def addData(self, datum):
        self.data.append(datum)

    def plot(self):
        # create an axis
        ax = self.figure.add_subplot(111)

        # discards the old graph
        ax.hold(False)

        # plot data
        ax.plot(self.data, '*-')

        # refresh canvas
        self.canvas.draw()


if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)

    plotWindow = PlotWindow()
    plotWindow.show()

    listenerThread = ListenerThread()
    listenerThread.start()

    def newData(data):
        plotWindow.addData(data)
        plotWindow.plot()

    listenerThread.newData.connect(newData)


    sys.exit(app.exec_())
