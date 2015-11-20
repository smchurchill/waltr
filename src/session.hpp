/*
 * serial_session.hpp
 *
 *  Created on: Nov 18, 2015
 *      Author: schurchill
 */

#ifndef SESSION_HPP_
#define SESSION_HPP_

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


#include "session.h"

void dispatcher::hello(basic_session* new_friend) {
		friends.push_back(new_friend);
}

void dispatcher::brag() {
		for(std::vector<basic_session*>::iterator
				it = friends.begin() ; it != friends.end() ; ++it )
			std::cout << (**it).print() << '\n';
}

basic_session::basic_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in) :
				io_service(&io_in),
				logdir_(log_in),
				ref(ref_in) {
		start_ = boost::chrono::steady_clock::now();
		ref->hello(this);
}

#endif /* SESSION_HPP_ */
