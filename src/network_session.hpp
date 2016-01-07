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

#include "structs.h"
#include "types.h"
#include "utils.h"

#include "network_session.h"
#include "session.h"


namespace dew {

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::chrono::steady_clock;
using ::boost::chrono::milliseconds;
using ::boost::chrono::nanoseconds;
using ::boost::bind;
using ::boost::make_iterator_range;
using ::boost::system::error_code;

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

using ::std::enable_shared_from_this;

/* December 16, 2015 :: constructors */

ns::network_session(context_struct context_in) :
		context_(context_in),
		dewd_(),
		socket_(*context_.service)
{
}

ns::network_session(
		context_struct context_in,
		tcp::endpoint const& ep_in,
		string channel_in
) :
		context_(context_in),
		dewd_(ep_in),
		socket_(*context_.service),
		channel_(channel_in)
{
}

nsp ns::get_ns() {
	return shared_from_this();
}

void ns::do_write(stringp message) {
	auto self (shared_from_this());
	if(socket_.is_open()) {
		auto messagep = make_shared<bBuff>(message->begin(),message->end());
		auto Message = boost::asio::buffer(*messagep);
		boost::asio::async_write(
					socket_, Message, bind(&ns::handle_write, self, _1, _2, messagep));
	}
}

void ns::do_write(string& message) {
	auto messagep = make_shared<string>(message);
	do_write(messagep);
}


void ns::do_connect() {
	auto self (shared_from_this());
	socket_.async_connect(dewd_, bind(&ns::handle_connect, self, _1));
	do_read();
}

void ns::do_read() {
	auto self (shared_from_this());
	auto buffer = make_shared<bBuff> (BUFFER_LENGTH);
	auto Buffer = boost::asio::buffer(*buffer);
	auto handler = bind(&ns::handle_read, self, _1, _2, buffer);
	socket_.async_read_some(Buffer, handler);
}

void ns::handle_read(
		boost::system::error_code ec, size_t in_length, bBuffp buffer) {
	if(ec){
		assert(false);
		context_.dispatch->remove_ns(shared_from_this());
		return;
	} else {
		do_read();
		auto self (shared_from_this());
		if(!buffer->empty()) {
			if(the_deque.empty())
				front_last = steady_clock::now();
			copy(buffer->begin(),buffer->begin()+in_length, back_inserter(the_deque));
		}
		set_a_check();
	}
}

void ns::handle_write(
		boost::system::error_code ec, size_t in_length, bBuffp buffer) {
	buffer.reset();
	return;
}

void ns::handle_connect(boost::system::error_code ec) {
	if(ec) {
		assert(false);
		//oh no!
	} else {
		auto sub_cmd = make_shared<string>("subscribe to "+channel_);
		do_write(sub_cmd);
	}
}

/* This whole section has pretty similar logic to the checking in
 * the dewd's serial_session class.
 */

void ns::set_a_check() {
	auto self (shared_from_this());
	context_.service->post(boost::bind(&ns::check_the_deque,self));
}

void ns::check_the_deque() {
	auto self (shared_from_this());
	if(the_deque.size()<7)
		return;

	if(the_deque[0] != 0xff || the_deque[1]!=0xfe) {
		scrub(the_deque.begin()+2);
		set_a_check();
		return;
	}

	pBuff end = {0xfe, 0xff};
	pBuff::iterator match_point =
			search(the_deque.begin()+2, the_deque.end(), end.begin(), end.end());

	if(match_point == the_deque.end()) {
		if(the_deque.size()>MAX_FRAME_LENGTH) {
			scrub(the_deque.begin()+1);
			set_a_check();
			return;
		} else if (steady_clock::now() - front_last > milliseconds(100)) {
			scrub(the_deque.begin()+1);
			set_a_check();
			return;
		}
	} else { /* We have a message.  Remember: ff, fe, msg, crc, fe, ff */
			pBuff to_send;
			assert(match_point+2 <= the_deque.end());
			copy(the_deque.begin()+2,match_point,back_inserter(to_send));
			scrub(match_point+2);
			if(crc8(make_iterator_range(to_send.begin(),to_send.end()-1))
					!= to_send.back()) {// uh oh!  crc doesn't match!
				assert(false);
			} else {
				auto str_to_send = make_shared<string>(to_send.begin(),to_send.end()-1);
				context_.dispatch->delivery(str_to_send);
			}

			/* Loop checks until to_parse has no more messages waiting for us. */
			set_a_check();
	}
}

int ns::scrub(pBuff::iterator iter) {
	pBuff::iterator oter = find(iter,the_deque.end(),0xff);
	int bytes = oter - the_deque.begin();
	the_deque.erase(the_deque.begin(), oter);
	front_last = steady_clock::now();
	return bytes;
}


} // dew namespace

#endif /* NETWORK_SESSION_HPP_ */
