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

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base methods
 */


/*=============================================================================
 * December 8, 2015 :: _socket_ methods
 */

/* Currently implemented using asyc_read_some. May need to switch to async_read
 * with timeouts if partial commands are being processed.
 */
void nss::do_read() {
	nssp self (shared_from_this());
	socket_.async_read_some(boost::asio::buffer(request_),bind(
			&nss::handle_read,self,_1,_2));
}

void nss::handle_read(
		boost::system::error_code ec, size_t in_length) {
	if(ec){
		dis_ref->remove_nss(shared_from_this());
		return;
	} else {

	nssp self (shared_from_this());

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
		sentence cmds;
		string in;
		while(ss >> in)
			cmds.push_back(in);



		string msg;
		if(cmds.size()) {
			msg = dis_ref->call_net(cmds,self);
		} else {
			msg = "No command entered.\n";
		}
		out_length = msg.size();


		copy(msg.begin(),msg.end(),reply_.begin());

	}


	boost::asio::async_write(socket_,boost::asio::buffer(reply_,out_length),
			[self](boost::system::error_code ec, size_t in_length) {
				self->do_read();
			});

	}
}


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ methods
 */
void nas::do_accept() {
	auto self (shared_from_this());
	acceptor_.async_accept(socket_,
			[this,self](boost::system::error_code ec)
			{
				if(!ec) {
					dis_ref->make_nss(socket_);
				}
				start();
			});
}









} // dew namespace

#endif /* NETWORK_SESSION_HPP_ */
