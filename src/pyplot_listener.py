# 
# Copyright (c) 2015 Jim Youngquist
# under The MIT License (MIT)
# full text in LICENSE file in root folder of this project.
# 

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
  def __init__(self, global_env):
    super(ListenerThread, self).__init__()
    self.running = True
    self.global_env = global_env
    self.port = None


  def decodeData(self, message):
    if len(message) < 9:
        return False, "Message is not long enough"

    rows, cols, size = struct.unpack('IIB', message[:9])
    length = rows*cols*size
    if len(message) < 9 + length:
        return False, "Message contains insufficient data"

    dtype = {4 : np.float32,
         8 : np.float64}[size]
    data = np.fromstring(message[9:9+length], dtype=dtype)
    data = data.reshape(rows, cols)

    if len(message[9+length:]) == 0:
        return False, "Message has zero length name field"

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


  def sendFailure(self, socket, message):
    return self.sendString(socket, message)


  def processData(self, data_message):
    data, name = self.decodeData(data_message)
    if data is False:
        return data, name

    self.global_env[name] = data
    return True, name


  def run(self):
    context = zmq.Context()

    data_socket = context.socket(zmq.REP)
    self.port = data_socket.bind_to_random_port("tcp://*")

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
            success, message = processors[socket](message)
            if success:
              self.sendSuccess(socket)
            else:
              self.sendFailure(socket, message)
        except zmq.error.ZMQError as e:
          # there was a transmit error...oops...die
          self.running = False
          continue


def cpp_ipython_start_thread(global_env):
  listener_thread = ListenerThread(global_env)
  listener_thread.start()
  global_env["cpp_ipython_listener_thread"] = listener_thread
  global_env["cpp_ipython_listener_thread_port"] = listener_thread.port
  return True

