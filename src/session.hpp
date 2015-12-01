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

#include "session.h"

namespace dew {

using ::boost::asio::io_service;
using ::boost::chrono::steady_clock;

using ::std::string;
using ::std::stringstream;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::make_pair;
using ::std::pair;

using ::std::find;

using ::std::cout;


/*-----------------------------------------------------------------------------
 * November 25, 2015 :: dispatcher methods
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
string dispatcher::check_connect() {
	return "privyet\n";
}

string dispatcher::close_connection() {
	return string("goodbye\n");
}
string dispatcher::brag() {
	stringstream ss;
	for(auto comrade : comrades)
		ss << comrade->print() << '\n';
	return ss.str();
}
string dispatcher::zabbix_ports() {
	string json ("{\"data\":[");
	int not_first = 0;
	for(auto comrade : comrades) {
		if(comrade->get_type() == string("serial")) {
			if(not_first)
				json += ",";
			json += "{\"{#DEWDSP}\":\"";
			json += comrade->print();
			json += "\"}";
		}
	}
	json += "]}";
	return json;
}



string dispatcher::call_net(string cmd) {
	string reply = network_cmd[cmd]();
	return reply;
}

void dispatcher::init_net_cmd () {
	network_cmd.insert(make_pair(
			string("hello"),boost::bind(&dispatcher::check_connect,this)));
	network_cmd.insert(make_pair(
			string("goodbye"),boost::bind(&dispatcher::close_connection,this)));
	network_cmd.insert(make_pair(
			string("greetings"),boost::bind(&dispatcher::brag,this)));

	network_cmd.insert(make_pair(
			string("zabbix-ports"),boost::bind(&dispatcher::zabbix_ports,this)));
	network_cmd.insert(make_pair(
			string("zabbix-")))


	return;
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
