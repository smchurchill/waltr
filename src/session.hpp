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

using ::std::string;
using ::std::to_string;
using ::std::stringstream;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::make_pair;
using ::std::pair;

using ::std::find;

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
	for(auto comrade : comrades)
		ss << comrade->print() << '\n';
	return ss.str();
}
string dispatcher::zabbix_ports() {
	string json ("{\"data\":[");
	int not_first = 0;
	for(auto comrade : comrades) {
		if(!comrade->get_type().compare(string("serial-read"))) {
			if(not_first)
				json += ",";
			json += "{\"{#DEWDSP}\":\"";
			json += comrade->print();
			json += "\"}";
			++not_first;
		}
	}
	json += "]}";
	return json;
}
string dispatcher::zbx_sp_x(string ident, int io) {
	for(auto comrade : comrades) {
		if(!((comrade->print()).compare(ident)) &&
			 !(comrade->get_type().compare(string("serial-read")))) {
			if(!io) {
				return comrade->get_rx();
			} else if(io) {
				return comrade->get_tx();
			}
		}
	}

	return string("Port \"" + ident + "\" not found\n");
}



string dispatcher::call_net(vector<string> cmds) {
	/* command parsing goes here */

	po::variables_map vmap;
	po::basic_command_line_parser<char> bclp(cmds);
	bclp.options(cmd_options);
	try{
		po::store(bclp.run(), vmap);
		if(vmap.count("help")) {
			stringstream ss;
			ss << cmd_options;
			return ss.str();
		}
		po::notify(vmap);
	}
	catch (po::error& poe) {
		return string(poe.what()+ '\n');
	}

	string reply;

	if(vmap.count("hello"))
		reply += string("privyet\n");

	if(vmap.count("greetings"))
		reply += brag();

	if(vmap.count("zabbix-ports"))
		return zabbix_ports();

	if(vmap.count("zsp-tx"))
		return zbx_sp_x(tx_name, 1);

	if(vmap.count("zsp-rx"))
		return zbx_sp_x(rx_name, 0);

	if(reply.empty())
		reply += "nyet\n";

	return reply;
}

void dispatcher::init_net_cmd () {
	/* program options stuff goes here */


	cmd_options.add_options()
			("help,h", "Get help messages.")
			("hello", "See basic greeting.")
			("greetings,b", "Returns all of the dewd's endpoints.")
			("zabbix-ports,p", "Returns the names of serial ports that the dewd is"
					" listening on in JSON format.  To be called by Zabbix discovery."
					" Clobbers non-help options.")
			("zsp-tx,t", po::value<string>(&tx_name),
					"Returns the total bytes transmitted through the given"
					" serial port, identified by value returned in zabbix-ports.")
			("zsp-rx,r", po::value<string>(&rx_name),
					"Returns the total bytes received through the given"
					" serial port, identified by value returned in zabbix-ports.")
			;
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
