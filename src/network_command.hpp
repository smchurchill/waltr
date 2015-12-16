/*
 * network_command.hpp
 *
 *  Created on: Dec 14, 2015
 *      Author: schurchill
 */

#ifndef NETWORK_COMMAND_HPP_
#define NETWORK_COMMAND_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <utility>
#include <deque>
#include <cstdio>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <iterator>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/duration.hpp>

#include <boost/range/iterator_range.hpp>

#include "structs.h"
#include "types.h"
#include "utils.h"
#include "session.h"
#include "network_command.h"

namespace dew {

using ::boost::asio::io_service;
using ::boost::chrono::steady_clock;
using ::boost::asio::basic_waitable_timer;
using ::boost::chrono::time_point;
using ::boost::chrono::milliseconds;
using ::boost::asio::mutable_buffers_1;

using ::boost::make_iterator_range;

using ::boost::bind;

using ::boost::system::error_code;

using ::std::to_string;
using ::std::string;
using ::std::vector;
using ::std::deque;

using ::std::search;
using ::std::find;
using ::std::copy;
using ::std::reverse_copy;





void network_command::read(sentence to_read) {
	sentence::iterator iter = find(to_read.begin(),to_read.end(),"do");
	if(iter == to_read.end()) {
		sentence::iterator iter = find(to_read.begin(),to_read.end(),"what");
		if(iter == to_read.end()) {


}


} // dew namespace



#endif /* NETWORK_COMMAND_HPP_ */
