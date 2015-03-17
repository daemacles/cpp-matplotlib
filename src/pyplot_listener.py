import os
import time
import signal
import struct
import sys
import Queue

import numpy as np
import zmq

import threading 

class ListenerThread(threading.Thread):
  def __init__(self, code_uri, global_env):
    super(ListenerThread, self).__init__()
    self.running = True
    self.global_env = global_env
    self.code_uri = code_uri


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

    poller = zmq.Poller()
    poller.register(data_socket, flags = zmq.POLLIN)

    processors = {
        data_socket : self.processData,
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
          # there was a transmit error...oops...die
          self.running = False
          continue


def ipython_run(global_env):
  cpp_ipython_listener_thread = ListenerThread(global_env)
  cpp_ipython_listener_thread.start()
  return lt

