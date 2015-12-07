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

/*-----------------------------------------------------------------------------
 * December 3, 2015
 *
 * Network commands.  The dispatcher class receives its network commands from
 * a network_socket_iface_session.  A network command is a vector<string> that
 * contains exactly 3 strings in the order Super-Type Request-Type Option. For
 * example, zabbix rx /dev/ttyS5 will obtain the rx count from port /dev/ttyS5
 * provided there is a serial_read_*_session running on that port.
 *
 * A loose description of how network commands currently work:
 *  1.  There is a root_map from ST to a function that deals with the RT and O
 *  		supplied.
 *  2.  For a RT and O that just requests a specific value, ie everything that
 *  		zabbix does, we usually just invoke a request like:
 *  			port_directory.at(option)->get(request-type)
 *  3.  The super types could tell the dewd to format the output in a certain
 *  		way, eg zabbix supertype, or could just be handy classification for
 *  		command types, eg device-under-test commands.
 *
 */


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
/* Supplies a string to be returned to `zabbix ports` network command.
 * list of ports is delivered in JSON format for zabbix discovery capability. */
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
