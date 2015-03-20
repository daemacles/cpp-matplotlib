// 
// Copyright (c) 2015 Jim Youngquist
// under The MIT License (MIT)
// full text in LICENSE file in root folder of this project.
// 

#pragma once

#include <string>
#include <vector>

#include <zmq.hpp>

namespace cppmpl {

//======================================================================
/** \brief This class wraps a ZeroMQ request-response socket connection.
 *
 * It acts as a one-way sink for data.  Responses are not actually returned,
 * only checked that the "request" was successfully transmitted.
 *
 * Usage:
\code
    RequestSink conn{"tcp://hostname:port"};
    conn.Connect();
    conn.Send("oh my goodness!");
\endcode
 */
class RequestSink {
public:
  //--------------------------------------------------
  /** \brief Initializes for a connection to a valid ZeroMQ endpoint, but does
   * not actually connect to it.
   *
   * \param url  the ZeroMQ url to connect to.
   */
  RequestSink(const std::string &url);

  //--------------------------------------------------
  /** \brief Transmits a string.
   *
   * \param buffer  the string to send.
   *
   * \returns whether transmission was successful. 
   */
  bool Send(const std::string &buffer);

  //--------------------------------------------------
  /** \brief Transmits a byte vector
   *
   * \param buffer  the bytes to send.
   *
   * \returns whether transmission was successful. 
   */
  bool Send(const std::vector<uint8_t> &buffer);

  //--------------------------------------------------
  /** \brief Actually connects to a Request socket.
   */
  bool Connect(void);

private:
  zmq::context_t context_;
  zmq::socket_t socket_;
  const std::string url_;
  bool connected_;
};

}
