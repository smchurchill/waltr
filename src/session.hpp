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
#include <cstdio>
#include <algorithm>
#include <functional>
#include <utility>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>

#include <boost/program_options.hpp>
#include <boost/range/iterator_range.hpp>

#include "utils.h"

#include "session.h"

namespace po = boost::program_options;

namespace dew {

using ::boost::asio::io_service;
using ::boost::chrono::steady_clock;
using ::boost::make_iterator_range;
using ::boost::bind;

using ::std::endl;

using ::std::string;
using ::std::to_string;
using ::std::stringstream;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::make_pair;
using ::std::pair;

using ::std::find;
using ::std::swap;

using ::std::cout;
using ::std::count;


/*-----------------------------------------------------------------------------
 * December 1, 2015 :: dispatcher methods
 */

void dispatcher::hello(basic_session* new_comrade) {
	comrades.push_back(new_comrade);
}

void dispatcher::forget_(basic_session* ex_comrade) {
	vector<basic_session*>::iterator position = find(
				comrades.begin(), comrades.end(), ex_comrade);
	if(position != comrades.end())
		comrades.erase(position);
}

dispatcher::~dispatcher() {
	for(auto comrade : comrades) {
		delete comrade;
	}
}


string dispatcher::brag() {
	stringstream ss;
	ss << "All comrades:\r\n";
	for(auto comrade : comrades)
		ss << '\t' << comrade->get("name") << endl;
	ss << "Port comrades:\r\n";
	for(auto comrade : port_directory)
		ss << '\t' << comrade.first << endl;
	return ss.str();
}
string dispatcher::bark() {
	stringstream ss;
	ss << "get_map:\r\n";
	for(auto comrade : comrades) {
		ss << '\t' << comrade->get("name") << endl;
		for(auto pair : comrade->get_map) {
			ss << '\t' << '\t' << pair.first << " : " << comrade->get(pair.first) << endl;
		}
	}
	return ss.str();
}
string dispatcher::zabbix_ports() {
	string json ("{\"data\":[");
	int not_first = 0;
	for(auto comrade : port_directory) {
			if(not_first)
				json += ",";
			json += "{\"{#DEWDSP}\":\"";
			json += comrade.first;
			json += "\"}";
			++not_first;
	}
	json += "]}";
	return json;
}

string dispatcher::call_net(vector<string> cmds) {
	/* command parsing goes here */
	if(root_map.count(cmds[0]))
		return root_map.at(cmds[0])(cmds[1],cmds[2]);
	else
		return "Query not supported.  \"help\" is also not yet supported.\n";

}

string dispatcher::help(string type, string item) {return "nyet\n";}
string dispatcher::raw(string item, string host) {
	if(port_directory.count(host))
		return port_directory.at(host)->get(item);
	else if(raw_map.count(item))
		return raw_map.at(item)();
	else
		return "-1";
}
string dispatcher::hr(string item, string host) {
	if(hr_map.count(item))
		return hr_map.at(item)();
	else
		return "nyet\n";
}

void dispatcher::build_lists() {
	for(auto comrade : comrades)
		if(!comrade->get("subtype").compare(string("serial-read"))) {
			port_directory.insert(make_pair(comrade->get("name"), comrade));
	}

}

/*-----------------------------------------------------------------------------
 * November 25, 2015 :: basic_session methods
 */
basic_session::basic_session( io_service& io_in, string log_in, dispatcher* ref_in) :
		io_ref(&io_in), logdir_(log_in), dis_ref(ref_in)
{
	dis_ref->hello(this);
}




} //namespace

#endif /* SESSION_HPP_ */
