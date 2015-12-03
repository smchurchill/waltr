/*
 * network_sessions.hpp
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef NETWORK_SESSION_HPP_
#define NETWORK_SESSION_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdio>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>

#include <boost/function.hpp>

#include "utils.h"
#include "session.hpp"
#include "network_session.h"


namespace dew {

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::chrono::steady_clock;

using ::std::istream;
using ::std::stringstream;
using ::std::string;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::pair;

using ::std::size_t;

using ::std::copy;
using ::std::copy_n;

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base methods
 */


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_ methods
 */

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ methods
 */
void network_acceptor_session::do_accept() {
	network_socket_iface_session* sock_ =	new network_socket_iface_session(
			*io_ref, logdir_, dis_ref, endpoint_);

	acceptor_.async_accept(*(sock_->get_sock()), endpoint_,
			[this,sock_](boost::system::error_code ec)
			{
				if(!ec) {
					sock_->start();
				}
				this->start();
			});
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_echo_ methods
 */
void network_socket_echo_session::do_read() {
	socket_.async_read_some(boost::asio::buffer(request_),
			[this](boost::system::error_code ec, size_t length)
			{
				if(!ec)
					do_write(length);
				else {
					delete this;
				}
			});
}

void network_socket_echo_session::do_write(size_t length)
{
	boost::asio::async_write(socket_,boost::asio::buffer(request_,length),
			[this](boost::system::error_code ec, size_t)
			{
				do_read();
			});
}

/*-----------------------------------------------------------------------------
 * November 27, 2015 :: _socket_iface_ methods
 */


/* Currently implemented using asyc_read_some. May need to switch to async_read
 * with timeouts if partial commands are being processed.
 */
void network_socket_iface_session::do_read() {
	socket_.async_read_some(boost::asio::buffer(request_),
			boost::bind(&network_socket_iface_session::handle_read,this,_1,_2));
}

void network_socket_iface_session::handle_read(
		boost::system::error_code ec, size_t in_length) {

				if(ec){ dis_ref->forget_(this); delete this; return; }

				size_t out_length = 0;

				/* Cut out edge cases */
				if (in_length >= BUFFER_LENGTH) {
					string exceeds ("Request exceeds length.\r\n");
					out_length = exceeds.size();
					copy(exceeds.begin(),exceeds.end(),reply_.begin());
				} else {
					stringstream ss;
					for(auto c : make_iterator_range(request_.begin(),request_.begin()+in_length))
						ss << c;
					vector<string> cmds;
					string in;
					while(ss >> in)
						cmds.push_back(in);

					string msg;
					if(cmds.size() && cmds.size() < 4) {
						while(cmds.size() < 3)
							cmds.push_back("0");
						msg = dis_ref->call_net(cmds);
					} else {
						msg = "Adhere to command format: query-type query-subtype query-option.\n";
					}

					out_length = msg.size();

					copy(msg.begin(),msg.end(),reply_.begin());
				}

				boost::asio::async_write(socket_,boost::asio::buffer(reply_,out_length),
						[this](boost::system::error_code ec, size_t)
						{
							do_read();
						});
}





} // dew namespace

#endif /* NETWORK_SESSION_HPP_ */
