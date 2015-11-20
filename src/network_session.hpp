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

#include "network_session.h"


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base methods
 */
network_session::network_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in,
			boost::asio::ip::tcp::endpoint ep_in) :
				basic_session(io_in, log_in, ref_in),
				endpoint_(ep_in) {
	}

std::string network_session::print() {
		std::stringstream ss;
		ss << endpoint_;
		return ss.str();
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_ methods
 */
network_socket_session::network_socket_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		boost::asio::ip::tcp::endpoint ep_in) :
			network_session(io_in, log_in, ref_in, ep_in),
			socket_(*io_service) {
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ methods
 */
network_acceptor_session::network_acceptor_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		boost::asio::ip::tcp::endpoint ep_in) :
			network_session(io_in, log_in, ref_in, ep_in),
			acceptor_(*io_service, endpoint_){
	start();
}

void network_acceptor_session::do_accept() {
	network_socket_echo_session* sock_ =	new network_socket_echo_session(
			*io_service, logdir_, ref, endpoint_);

	acceptor_.async_accept(sock_->get_sock(),
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
network_socket_echo_session::network_socket_echo_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		boost::asio::ip::tcp::endpoint ep_in) :
			network_socket_session(io_in, log_in, ref_in, ep_in) {
}

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


#endif /* NETWORK_SESSION_HPP_ */
