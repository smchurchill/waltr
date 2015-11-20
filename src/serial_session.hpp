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
#include <cstdio>

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
			serial_read_session(io_in, log_in, ref_in, device_in) {
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
