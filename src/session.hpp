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
#include <map>
#include <set>
#include <cstdio>
#include <algorithm>
#include <functional>

#include <utility>
#include <memory>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

#include <boost/program_options.hpp>
#include <boost/range/iterator_range.hpp>

#include <boost/filesystem.hpp>
#include <stdio.h>

#include "structs.h"
#include "types.h"
#include "utils.h"
#include "network_session.h"

#include "session.h"

namespace dew {

using ::boost::asio::io_service;
using ::boost::chrono::steady_clock;
using ::boost::chrono::system_clock;
using ::boost::chrono::time_fmt;
using	::boost::chrono::timezone;
using ::boost::make_iterator_range;
using ::boost::bind;
using ::boost::filesystem::path;

using ::std::endl;

using ::std::string;
using ::std::to_string;
using ::std::stringstream;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::set;
using ::std::make_pair;
using ::std::pair;

using ::std::find;
using ::std::swap;

using ::std::cout;
using ::std::count;

using ::std::unique_ptr;
using ::std::shared_ptr;
using ::std::make_shared;
using ::std::move;

using ::std::enable_shared_from_this;

/* December 16, 2015 :: constructors */

maintainer::maintainer(
		shared_ptr<io_service> const& io_in
) :
		maintainer(io_in, "/dev/null")
{
}
maintainer::maintainer(
		shared_ptr<io_service> const& io_in,
		string log_in
) :
		dewd_(),
		context_(io_in),
		logdir_(log_in)
{
}
maintainer::maintainer(
		shared_ptr<io_service> const& io_in,
		string log_in,
		tcp::endpoint& ep_in
) :
		dewd_(ep_in),
		context_(io_in),
		logdir_(log_in)
{
}

/*	December 14, 2015 :: Session creation.
 *
 * Under the new object management model, the dispatcher handles the creation
 * and destruction of all sessions.  The only sessions with lifetime less than
 * the run time of the program are the network sockets.
 *
 * This also allows us to include pseudo-default constructors in our session
 * classes that need only be passed a context_struct.
 *
 */

void maintainer::make_subs (vector<string>& channels) {
	for (auto channel : channels) {
		auto pt = make_shared<ns>(context_struct(context_, shared_from_this()),
			dewd_, channel);
		pt->start_connect();
		network.emplace_back(pt->get_ns());
	}
}

void maintainer::remove_ns (nsp to_remove) {
	to_remove->cancel_socket();
	network.remove(to_remove);
}

void maintainer::delivery(stringp message) {
	auto fpm = make_shared<flopointpb::FloPointMessage>();
	bool parse_successful = fpm->ParseFromString(*message);
	if(parse_successful) {
		string logfile_ (logfile(fpm->name()));
		FILE * log = fopen(logfile_.c_str(),"a");
		std::fwrite(message->c_str(), sizeof(u8), message->length(), log);
		fclose(log);
	}
}

string maintainer::logfile(string channel) {
	stringstream ss;
	ss << logdir_ << channel << "/"
			<< ::boost::chrono::time_fmt(
					::boost::chrono::timezone::local, "%F")
			<< system_clock::now();
	path p (ss.str());
	if(!::boost::filesystem::exists(p))
		assert(::boost::filesystem::create_directories(p));

	ss << "/"
			<< ::boost::chrono::time_fmt(::boost::chrono::timezone::local, "%H")
			<< system_clock::now();
	return ss.str();
}


void maintainer::forward(stringp) {


}

void maintainer::forward_handler(const error_code&,size_t, bBuffp, nsp) {


}


} //namespace

#endif /* SESSION_HPP_ */
