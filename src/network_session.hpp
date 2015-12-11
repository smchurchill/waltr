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
	if(ec){ die(); } else {

	stime req_rec = sc::now(); stime postwrite;


	stime preproc, postproc;
	stime presend, postsend;
	stime precop, postcop;
	stime prewrite;
	if(0) {
	cout << "request: ";
	debug(make_iterator_range(request_.begin(),request_.begin()+in_length));
	}




	size_t out_length = 0;

	/* Cut out edge cases */
	if (in_length >= BUFFER_LENGTH) {
		string exceeds ("Request exceeds length.\r\n");
		out_length = exceeds.size();
		copy(exceeds.begin(),exceeds.end(),reply_.begin());
	} else {
		preproc = sc::now();
		stringstream ss;
		for(auto c : make_iterator_range(request_.begin(),request_.begin()+in_length))
		ss << c;
		vector<string> cmds;
		string in;
		while(ss >> in)
			cmds.push_back(in);
		postproc = sc::now();

		presend = sc::now();
		string msg;
		if(cmds.size() && cmds.size() < 4) {
			while(cmds.size() < 3)
				cmds.push_back("0");
				msg = dis_ref->call_net(cmds);
		} else {
			msg = "Adhere to command format: query-type query-subtype query-option.\n";
		}
		out_length = msg.size();
		postsend = sc::now();

		precop = sc::now();
		copy(msg.begin(),msg.end(),reply_.begin());
		postcop = sc::now();
	}


	prewrite = sc::now();
	nssp self (shared_from_this());
	boost::asio::async_write(socket_,boost::asio::buffer(reply_,out_length),
			[self,this](boost::system::error_code ec, size_t in_length) {
				do_read();
			});
	postwrite = sc::now();

	if(0){
	cout << "preprocessing: " << (preproc - req_rec).count()/1000 << endl;
	cout << "processing: " << (postproc - preproc).count()/1000 << endl;
	cout << "sending: " << (postsend - presend).count()/1000 << endl;
	cout << "copying: " << (postcop - precop).count()/1000 << endl;
	cout << "writing: " << (postwrite - prewrite).count()/1000 << endl;
	cout << "total: " << (postwrite - req_rec).count()/1000 << endl;
	cout << "response: " << string(reply_.begin(), reply_.begin()+out_length) << endl;
	}
	cout << (postwrite - req_rec).count()/1000 << endl;

	}
}


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ methods
 */
void nas::do_accept() {
	assert(is_alive());
	acceptor_.async_accept(socket_,
			[this](boost::system::error_code ec)
			{
				if(!ec) {
					dis_ref->make_session(socket_);
				}
				start();
			});
}









} // dew namespace

#endif /* NETWORK_SESSION_HPP_ */
