#include <iostream>
#include <string>
#include <unistd.h>

#include <zmq.hpp>


int main(int argc, char **argv) {
  // DELETE THESE.  Used to suppress unused variable warnings.
  (void)argc;
  (void)argv;

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);

  std::cout << "Connecting to HW server..." << std::endl;
  socket.connect("tcp://localhost:5555");

  for (int req_number = 0; req_number != 10; ++req_number) {
    double value = req_number*2.9;
    value = value / 4.3;

    zmq::message_t request(sizeof(value));
    memcpy((void*) request.data(), &value, sizeof(value));
    std::cout << "Sending value " << value << "...";
    socket.send(request);

    // Get the reply
    zmq::message_t reply;
    socket.recv(&reply);
    std::cout << "Received " << std::string((char*)reply.data()) << " "
      << req_number << std::endl;
  }

  return 0;
}
