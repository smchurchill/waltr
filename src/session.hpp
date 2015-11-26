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

namespace dew {

using ::boost::asio::io_service;
using ::boost::chrono::steady_clock;

using ::std::string;
using ::std::vector;
using ::std::deque;

using ::std::cout;


/*-----------------------------------------------------------------------------
 * November 25, 2015 :: dispatcher methods
 */
void dispatcher::hello(basic_session* new_friend) {
		friends.push_back(new_friend);
}

void dispatcher::brag() {
		for(vector<basic_session*>::iterator
				it = friends.begin() ; it != friends.end() ; ++it )
			cout << (**it).print() << '\n';
}

/*-----------------------------------------------------------------------------
 * November 25, 2015 :: basic_session methods
 */

} //namespace

#endif /* SESSION_HPP_ */
