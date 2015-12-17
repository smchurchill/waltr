/*
 * network_command.hpp
 *
 *  Created on: Dec 14, 2015
 *      Author: schurchill
 */

#ifndef COMMAND_GRAPH_HPP_
#define COMMAND_GRAPH_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <deque>
#include <cstdio>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <iterator>
#include <functional>

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
#include "command_graph.h"

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
using ::std::map;
using ::std::pair;
using ::std::make_pair;

using ::std::search;
using ::std::find;
using ::std::copy;
using ::std::reverse_copy;


void node::spawn(pair<string,nodep> child) {
	if(child.second->is_owned())
		return;
	else {
		child.second->own();
		children.emplace(child);
	}
}
void node::spawn(string str_in, nodep node_in) {
	if(node_in->is_owned())
		return;
	else {
		node_in->own();
		children.emplace(make_pair(str_in,node_in));
	}
}

void node::spawn(map<string,nodep> children_in) {
	for(auto child : children_in) {
		spawn(child);
	}
}

void node::purge() {
	if(is_leaf())
		return;
	else {
		for(auto child : children)
			child.second->purge();
		children.clear();
	}
}

void node::operator()(nsp in) const {
	if(fn)
		fn(in);
}

string node::descendants(const int ancestors) {
	string d;
	if(!ancestors)
		d+="root";

	for(auto child : children) {
		for(int i = ancestors ; i ; --i)
			d+='\t';
		d+='\t';
		d+=child.first;
		d+='\n';
		d+=child.second->descendants(ancestors+1);
	}

	return d;
}




} // dew namespace



#endif /* COMMAND_GRAPH_HPP_ */
