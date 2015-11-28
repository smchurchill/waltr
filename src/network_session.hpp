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

#include "utils.h"
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

using ::std::size_t;

using ::std::copy;
using ::std::copy_n;

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
			[this](boost::system::error_code ec, size_t in_length)
			{
				if(ec){ return; }

				debug(make_iterator_range(request_.begin(), request_.begin()+in_length));

				size_t out_length = 0;

				if(valid_request(in_length)) {
					out_length = process_request(in_length);
				}
				else if (in_length >= BUFFER_LENGTH) {
					string exceeds ("Request exceeds length.\r\n");
					out_length = exceeds.size();
					copy(exceeds.begin(),exceeds.end(),reply_.begin());
				}
				else if (in_length <= 2) {
					string nomsg ("No request received.\r\n");
					out_length = nomsg.size();
					copy(nomsg.begin(), nomsg.end(),reply_.begin());
				}
				else {
					string invalid_message;
					stringstream ss;
					bBuff::iterator otter = request_.begin();
					bBuff badend = {'\r','\n'};

					if((otter = search(request_.begin(),request_.begin()+in_length,badend.begin(),badend.end()))
							== request_.begin()+in_length)
						otter = find(request_.begin(),request_.begin()+in_length,'\n');

					ss << "From " << socket_.local_endpoint() << ": " << '"';
					for(auto c : make_iterator_range(request_.begin(),otter))
						ss << filter_unprintable(c);
					ss<< '"';

					string command = ss.str();

					otter = copy(command.begin(),command.end(),reply_.begin());

					string msg = string (" is not a valid request.\n");
					otter = copy(msg.begin(),msg.end(),otter);
					out_length = otter - reply_.begin();
				}

				debug(make_iterator_range(reply_.begin(), reply_.begin()+out_length));

				boost::asio::async_write(socket_,boost::asio::buffer(reply_,out_length),
						[this](boost::system::error_code ec, size_t)
						{
							do_read();
						});
			});
}


void network_socket_iface_session::do_write(size_t length){


}

bool network_socket_iface_session::valid_request(size_t length){ return false;}

size_t network_socket_iface_session::process_request(size_t length) { return 0;}




} // dew namespace

#endif /* NETWORK_SESSION_HPP_ */
