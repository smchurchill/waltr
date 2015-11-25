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
#include <time.h>
#include <iterator>

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
 * November 24, 2015 :: _read_parse_ methods
 */
serial_read_parse_session::serial_read_parse_session(
		boost::asio::io_service& io_in,
		std::string log_in,
		dispatcher* ref_in,
		std::string device_in) :
			serial_read_session(io_in, log_in, ref_in, device_in),
			front_last(boost::chrono::steady_clock::now()) {
	garbage_file = logdir_ + name_.substr(name_.find_last_of("/\\")+1);
	this->start();
}
void serial_read_parse_session::start() {
	start_read();
	dead_ = boost::chrono::steady_clock::now() + timeout_;
	set_timer();
}
void serial_read_parse_session::start_read() {
	bBuff* buffer_ = new bBuff;
	buffer_->assign (BUFFER_LENGTH,0);
	boost::asio::mutable_buffers_1 Buffer_ = boost::asio::buffer(*buffer_);
	boost::asio::async_read(port_,Buffer_,boost::bind(
			&serial_read_parse_session::handle_read, this, _1, _2, buffer_));
}
void serial_read_parse_session::handle_read(boost::system::error_code ec,
		size_t length, bBuff* buffer_) {

	/* First, make sure we add more work to the io_service.
	 */
	start_read();

	if(!buffer_->empty()) {
		if(to_parse.empty())
			front_last = boost::chrono::steady_clock::now();
		for(bBuff::iterator
				it = buffer_->begin(); it - buffer_->begin() < (signed)length ; ++it)
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
	bool scrubbed = false;
	while(!to_parse.empty() && to_parse.front()!=0xff) {
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

	if(to_parse[1]!=0xfe) {
		to_parse.pop_front();
		check_the_deque();
		return;
	}

	/* GATP that to_parse has at least 12 characters and the first two characters
	 * are 'FF' then 'FE'.
	 *
	 * The last check of whether we have a valid prefix is to compute the crc.
	 * The crc is located at byte location 11, counting from 0, of to_parse.
	 */
	assert(to_parse.size()>=12);
	assert(to_parse[0]==0xff);
	assert(to_parse[1]==0xfe);

	if(crc8(&*to_parse.begin(),11)!=to_parse[11])
		return;

	/* We now have a valid prefix, so we gather the rest of the frame delimiter,
	 * which is of the form
	 * 	'FF' + 'FE' + <nonce>
	 * where nonce is 4 character identifier occuring in positions 2 through 5 of
	 * to_parse.
	 *
	 * the matching delimiter we search for is:
	 * 	<nonce> + 'FE' + 'FF'
	 *
	 * We search to_parse for a matching delimiter and, if found, cut it away
	 * from the rest of to_parse and ...
	 *  - currently printing to console
	 *  - later, will send to dispatcher
	 */



	pBuff delim (to_parse.begin()+2, to_parse.begin()+6);
	delim.push_back(0xfe); delim.push_back(0xff);

	pBuff::iterator match_point =
			std::search(to_parse.begin(), to_parse.end(), delim.begin(), delim.end());

	if(match_point!=to_parse.end()) {
		++internal_count;
		pBuff* to_send = new pBuff;

		/* We have a match and match_point points to the beginning of the endframe
		 * delimiter.  endframe delimiter is 6 characters long, so match_point + 6
		 * is less than to_parse.end().
		 */
		assert(match_point+6 <= to_parse.end());
		std::copy(to_parse.begin(), match_point+6, back_inserter(*to_send));
		to_parse.erase(to_parse.begin(), match_point+6);
		front_last = boost::chrono::steady_clock::now();

		assert(to_send->size()>=11);
		std::cout << "message number marked [" <<
				(int)to_send->at(10) << "] and is internal count [256*" << internal_count/256 << "]+[" << internal_count%256 << "] arrived"
						" at port " << name_ << '\n';
		//printi(to_send);
		//std::cout << '\n';
	}
}
inline void serial_read_parse_session::handle_timeout_extra() {
	/* if the first character in to_parse is too old, we pop it. */
	++tenths_count;
	if(!(tenths_count%5)) {
		if(
				front_last <= boost::chrono::steady_clock::now()-boost::chrono::milliseconds(500)
					&&
				!to_parse.empty()
			) {
			to_parse.pop_front();
		}
		if(0)
		std::cout << "messages received at " << name_ << ": " << internal_count << '\n';
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
	bBuff* buffer_ = new bBuff;
	buffer_->assign (BUFFER_LENGTH,0);
	boost::asio::mutable_buffers_1 Buffer_ = boost::asio::buffer(*buffer_);
	boost::asio::async_read(port_,Buffer_,boost::bind(
			&serial_read_log_session::handle_read, this, _1, _2, buffer_));
}
void serial_read_log_session::handle_read(boost::system::error_code ec,
		size_t length,	bBuff* buffer_) {
	this->start_read();
	if (length > 0) {
		FILE* file_ = fopen(filename_.c_str(), "a");
		std::fwrite(buffer_->data(), sizeof(u8), length, file_);
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

/*-----------------------------------------------------------------------------
 * November 24, 2015 :: _write_methods
 */
serial_write_nonsense_session::serial_write_nonsense_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in,
			std::string device_in) :
					serial_write_session(io_in, log_in, ref_in, device_in),
					timer_(*io_service) {
	this->start();

}
void serial_write_nonsense_session::start() {
	std::srand(time(NULL)*time(NULL));
	start_write();
}
void serial_write_nonsense_session::start_write() {
	bBuff nonsense = generate_nonsense();
	boost::asio::mutable_buffers_1 bnonsense = boost::asio::buffer(nonsense);
	//std::cout << "writing to port: " << name_ << '\n';
	//printi(&nonsense);
	//std::cout << '\n';

	++internal_counter;
	if(0)
	if(!(internal_counter%5))
		std::cout << "messages sent from " << name_ << ": " << internal_counter << '\n';

	boost::asio::async_write(port_, bnonsense, boost::bind(
			&serial_write_nonsense_session::handle_write, this, _1, _2));

}
void serial_write_nonsense_session::handle_write(
		const boost::system::error_code& error, std::size_t bytes_transferred) {
	unsigned long int wait = 150 + std::rand()%50;
	timer_.expires_from_now(boost::chrono::milliseconds(wait));
	timer_.async_wait(
	//io_service->post(
			boost::bind(&serial_write_nonsense_session::start_write,this));
}
bBuff serial_write_nonsense_session::generate_nonsense() {
		bBuff msg,  nonce1, nonce2, payload;
		for(int i=0;i<4;++i) {
			nonce1.push_back(32+std::rand()%224);
			nonce2.push_back(32+std::rand()%224);
		}

		for(int i=2000 ; i ; --i)
			payload.push_back(32+std::rand()%224);

		msg.push_back(0xff); msg.push_back(0xfe);
		std::copy(nonce1.begin(), nonce1.end(), back_inserter(msg));
		std::copy(nonce2.begin(), nonce2.end(), back_inserter(msg));
		msg.push_back(internal_counter%256);

		assert(msg.size()==11);
		msg.push_back(crc8(&*msg.begin(),msg.size()));


		std::copy(payload.begin(), payload.end(), back_inserter(msg));
		std::copy(nonce1.begin(), nonce1.end(), back_inserter(msg));
		msg.push_back(0xfe); msg.push_back(0xff);
		return msg;
}
bBuff serial_write_nonsense_session::generate_some_sense(
		bBuff payload) {
	return payload;
}

#endif /* SERIAL_SESSION_HPP_ */
