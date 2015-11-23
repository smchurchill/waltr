/*
 * serial_sessions.hpp
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef SERIAL_SESSION_HPP_
#define SERIAL_SESSION_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <deque>
#include <cstdio>
#include <algorithm>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/duration.hpp>

#include "serial_session.h"

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base methods
 */
serial_session::serial_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in,
			std::string device_in) :
				basic_session(io_in, log_in, ref_in),
				name_(device_in),
				port_(*io_service, name_) {
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _read_ methods
 */
serial_read_session::serial_read_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		std::string device_in) :
			serial_session(io_in, log_in, ref_in, device_in),
			timer_(*io_service) {
}

void serial_read_session::set_timer() {
	boost::chrono::time_point<boost::chrono::steady_clock> now_ =
		boost::chrono::steady_clock::now();

	while(dead_< now_)
		dead_ = dead_ + timeout_;

	timer_.expires_at(dead_);
	timer_.async_wait(
  	boost::bind(&serial_read_session::handle_timeout, this, _1));
}

void serial_read_session::handle_timeout(boost::system::error_code ec) {
	if(!ec)
		port_.cancel();

	handle_timeout_extra();

	set_timer();
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _read_parse_ methods
 */
serial_read_parse_session::serial_read_parse_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		std::string device_in) :
			serial_read_session(io_in, log_in, ref_in, device_in),
			front_last(boost::chrono::steady_clock::now()), front_count(0) {
	garbage_file = logdir_ + name_.substr(name_.find_last_of("/\\")+1);
	this->start();
}
void serial_read_parse_session::start() {
	start_read();
	dead_ = boost::chrono::steady_clock::now() + timeout_;
	set_timer();
}
void serial_read_parse_session::start_read() {
	std::vector<char>* buffer_ = new std::vector<char>;
	buffer_->assign (BUFFER_LENGTH,0);
	boost::asio::mutable_buffers_1 Buffer_ = boost::asio::buffer(*buffer_);
	boost::asio::async_read(port_,Buffer_,boost::bind(
			&serial_read_parse_session::handle_read, this, _1, _2, buffer_));
}
void serial_read_parse_session::handle_read(boost::system::error_code ec,
		size_t length, std::vector<char>* buffer_) {

	/* First, make sure we add more work to the io_service.
	 */
	start_read();

	if(!buffer_->empty()) {
		if(to_parse.empty())
			front_last = boost::chrono::steady_clock::now();
		for(std::vector<char>::iterator
				it = buffer_->begin(); it!=buffer_->end() ; ++it)
			to_parse.emplace_back(*it);
	}
	delete buffer_;

	/* immediately check for a frame.  this should be very fast */
	check_the_deque();
}
void serial_read_parse_session::check_the_deque() {
	/* every time we check the deque, we first scrub to an 'FF' or empty the
	 * deque if no 'FF' exists.
	 */
	bool scrubbed;
	while(!to_parse.empty() && to_parse.front()!=0xFF) {
		to_parse.pop_front();
		scrubbed = true;
	}

	if(scrubbed)
		front_last = boost::chrono::steady_clock::now();

	/* the front and back delimiters of a frame take up 12 characters.  If we
	 * have fewer than 12 characters in to_parse then we cannot succeed.
	 *
	 * With exactly 12 characters there is an edge case of an empty frame.  We
	 * would want to find and remove that.
	 */
	if(to_parse.size() < 12)
		return;

	/* Guaranteed at this point that to_parse has at least 12 characters and the
	 * first character is 'FF'.
	 *
	 * First, we make sure that our 'FF' is part of a valid prefix 'FF' 'FE'.
	 * if not, we get rid of the first 'FF' and try again.
	 */

	if(to_parse[1]!=0xFE) {
		to_parse.pop_front();
		check_the_deque();
	}

	/* GATP that to_parse has at least 12 characters and the first two characters
	 * are 'FF' then 'FE'.
	 *
	 * We now have a valid prefix, so we gather the rest of the frame delimiter,
	 * which is of the form
	 * 	'FF' + 'FE' + <nonce>
	 * where nonce is 4 character identifier occuring in positions 2 through 5 of
	 * to_parse.
	 *
	 * the matching delimiter we search for is:
	 * 	<nonce> + 'FE' + 'FF'
	 */

	std::deque<char> delim;
	std::copy(to_parse.begin()+2,to_parse.begin()+6,delim.begin());
	std::copy(to_parse.begin(),to_parse.begin()+2,delim.end());
	std::deque<char>::iterator endframe =
		std::search(to_parse.begin(), to_parse.end(), delim.begin(), delim.end());
	if(endframe != to_parse.end()) {
			 /* then we found a match that begins at endframe.  we can copy the frame
		    * into to_send and send it off to the dispatcher.  they'll know what to
		    * do with it...
		    */
		std::vector<char>* to_send = new std::vector<char>;
		copy(to_parse.begin(),endframe+6,to_send->begin());
	//	io_service->post(bind(&(dispatcher::forward),this,to_send));
	}

}
inline void serial_read_parse_session::handle_timeout_extra() {
	/* if the first character in to_parse is too old, we pop it. */
	++front_count;
	if(!front_count%5) {
		front_count=0;
		if(
				front_last <= boost::chrono::steady_clock::now()-boost::chrono::milliseconds(500)
					&&
				!to_parse.empty()
			) {
			to_parse.pop_front();
		}
	}
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _read_log_ methods
 */
serial_read_log_session::serial_read_log_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		std::string device_in) :
			serial_read_session(io_in, log_in, ref_in, device_in) {
	filename_ = logdir_ + name_.substr(name_.find_last_of("/\\")+1);
	this->start();
}

void serial_read_log_session::start() {
	start_read();
	dead_ = boost::chrono::steady_clock::now() + timeout_;
	set_timer();
}

void serial_read_log_session::start_read() {
	std::vector<char>* buffer_ = new std::vector<char>;
	buffer_->assign (BUFFER_LENGTH,0);
	boost::asio::mutable_buffers_1 Buffer_ = boost::asio::buffer(*buffer_);
	boost::asio::async_read(port_,Buffer_,boost::bind(
			&serial_read_log_session::handle_read, this, _1, _2, buffer_));
}
void serial_read_log_session::handle_read(boost::system::error_code ec,
		size_t length,	std::vector<char>* buffer_) {
	this->start_read();
	if (length > 0) {
		FILE* file_ = fopen(filename_.c_str(), "a");
		std::fwrite(buffer_->data(), sizeof(char), length, file_);
		fclose(file_);
	}
	delete buffer_;
}

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _write_methods
 */
serial_write_session::serial_write_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		std::string device_in) :
			serial_session(io_in, log_in, ref_in, device_in) {
}

#endif /* SERIAL_SESSION_HPP_ */
