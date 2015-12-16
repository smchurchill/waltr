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

#include <memory>
#include <utility>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>

#include <boost/function.hpp>

#include "types.h"
#include "utils.h"
#include "session.h"
#include "network_command.h"
#include "network_session.h"


namespace dew {

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::chrono::steady_clock;
using ::boost::chrono::milliseconds;
using ::boost::chrono::nanoseconds;
using ::boost::bind;
using ::boost::make_iterator_range;

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

using ::std::make_shared;
using ::std::shared_ptr;

void ns::do_accept() {
	auto self (shared_from_this());
	acceptor_.async_accept(socket_,
			[this,self](boost::system::error_code ec)
			{
				if(!ec) {
					context_.dispatch->make_ns(socket_);
				}
				start_accept();
			});
}


void ns::do_read() {
	auto self (shared_from_this());
	socket_.async_read_some(boost::asio::buffer(request),bind(
			&ns::handle_read,self,_1,_2));
}

void ns::handle_read(
		boost::system::error_code ec, size_t in_length) {
	if(ec){
		context_.dispatch->remove_ns(shared_from_this());
		return;
	} else {
	auto self (shared_from_this());
	size_t out_length = 0;

	/* Cut out edge cases */
	if (in_length >= BUFFER_LENGTH) {
		string exceeds ("Request exceeds length.\r\n");
		out_length = exceeds.size();
		copy(exceeds.begin(),exceeds.end(),reply.begin());
	} else {
		sentence s = buffer_to_sentence(in_length);
		string message;
		if(s.empty()) {
			message = "No command entered.\n";
		} else {
			network_command command (s);
			message = context_.dispatch->execute_network_command(command, self);
		}
		out_length = message.size();
		copy(message.begin(),message.end(),reply.begin());
	}

	boost::asio::async_write(socket_,boost::asio::buffer(reply,out_length),
			[self](boost::system::error_code ec, size_t in_length) {
				self->do_read();
			});
	}
}

sentence ns::buffer_to_sentence(int len) {
	stringstream ss;
	for(auto c : make_iterator_range(request.begin(),request.begin()+len))
	ss << c;
	sentence s;
	string in;
	while(ss >> in)
		s.push_back(in);

	return s;
}

} // dew namespace

#endif /* NETWORK_SESSION_HPP_ */
