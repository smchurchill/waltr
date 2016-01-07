/*
 * structs.h
 *
 *  Created on: Dec 14, 2015
 *      Author: schurchill
 */

#ifndef STRUCTS_H_
#define STRUCTS_H_

#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>


using ::std::shared_ptr;
using ::boost::asio::io_service;
using ::boost::chrono::time_point;
using ::boost::chrono::steady_clock;
using ::std::vector;

namespace dew {

class maintainer;


struct context_struct_lite {
	context_struct_lite(shared_ptr<io_service> const& io_in) :
		start(steady_clock::now()), service(io_in) {}

	time_point<steady_clock> start;
	const shared_ptr<io_service> service;

	void set_start() { start = steady_clock::now(); }
};



struct context_struct {
	context_struct(const context_struct_lite& context_in, shared_ptr<maintainer> const& dis_in) :
		start(steady_clock::now()), service(context_in.service), dispatch(dis_in) {}

	context_struct(){}

	time_point<steady_clock> start;
	const shared_ptr<io_service> service;
	const shared_ptr<maintainer> dispatch;

	void set_start() { start = steady_clock::now(); }
};


} //namespace dew

#endif /* STRUCTS_H_ */
