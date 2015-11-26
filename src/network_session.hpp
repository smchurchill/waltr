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

#include "session.hpp"
#include "network_session.h"


namespace dew {

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::chrono::steady_clock;

using ::std::stringstream;
using ::std::string;
using ::std::vector;
using ::std::deque;

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base methods
 */
string network_session::print() {
		stringstream ss;
		ss << endpoint_;
		return ss.str();
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_ methods
 */

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ methods
 */
void network_acceptor_session::do_accept() {
	network_socket_echo_session* sock_ =	new network_socket_echo_session(
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
	socket_.async_read_some(boost::asio::buffer(data_),
			[this](boost::system::error_code ec, std::size_t length)
			{
				if(!ec)
					do_write(length);
				else {
					delete this;
				}
			});
}

void network_socket_echo_session::do_write(std::size_t length)
{
	boost::asio::async_write(socket_,boost::asio::buffer(data_,length),
			[this](boost::system::error_code ec, std::size_t)
			{
				do_read();
			});
}

} // dew namespace

#endif /* NETWORK_SESSION_HPP_ */
