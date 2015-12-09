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
#include <memory>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>

#include <boost/program_options.hpp>
#include <boost/range/iterator_range.hpp>

#include "utils.h"

#include "session.h"
#include "serial_session.h"
#include "network_session.h"

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

using ::std::unique_ptr;
using ::std::shared_ptr;
using ::std::make_shared;
using ::std::move;

using ::std::enable_shared_from_this;




/*-----------------------------------------------------------------------------
 * December 1, 2015 :: dispatcher methods
 */

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
	for(auto comrade : comrades) {
		if(!comrade->get("subtype").compare("serial-read")) {
			if(not_first)
				json += ",";
			json += "{\"{#DEWDSP}\":\"";
			json += comrade->get("name");
			json += "\"}";
			++not_first;
		}
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
	for(auto comrade : comrades)
		if(!comrade->get("name").compare(host))
			return comrade->get(item);
	else if(raw_map.count(item))
		return raw_map.at(item)();

	return "-1";
}

string dispatcher::hr(string item, string host) {
	if(hr_map.count(item))
		return hr_map.at(item)();
	else
		return "nyet\n";
}

void dispatcher::forward(basic_session* msg_from, string* msg) {
	flopointpb::FloPointWaveform fpwf;

	if(local_logging_enabled){
		if(!(fpwf.ParseFromString(*msg))) {
			FILE * log = fopen((logdir_ + "dispatch.failure.log").c_str(),"a");
			string s (to_string(steady_clock::now()) + ": Could not parse string.\n");
			std::fwrite(s.c_str(), sizeof(u8), s.length(), log);
			fclose(log);
		} else {
			FILE * log = fopen((logdir_ + "dispatch.message.log").c_str(),"a");
			string s (to_string(steady_clock::now()) + ": Message received:\n");
			s += "\tName: " + fpwf.name() + '\n';
			s += "\tWaveform: ";
			for(int i = 0; i < fpwf.waveform().wheight().size(); ++i)
				s += to_string(fpwf.waveform().wheight(i)) + '\n';
			std::fwrite(s.c_str(), sizeof(u8), s.length(), log);
			fclose(log);
		}
	}
	delete msg;
}

void dispatcher::make_session (tcp::endpoint& ep_in) {
	auto pt = make_shared<network_acceptor_session>(io_ref, ep_in);

	pt->set_ref(shared_from_this());
	pt->start();

	comrades.push_back(move(pt));
}

void dispatcher::make_session (tcp::socket sock_in) {
	auto pt = make_shared<network_socket_session>(io_ref, move(sock_in));

	pt->set_ref(shared_from_this());
	pt->start();

	comrades.push_back(move(pt));

	for(auto comrade = comrades.begin() ; comrade != comrades.end() ; ++comrade )
		if((*comrade)->is_dead()) {
			comrades.erase(comrade);
		}
}


void dispatcher::make_session (string device_name, string type) {
	if(type.compare("read")) {
		auto pt = make_shared<serial_read_parse_session>(io_ref, device_name);

		pt->set_ref(shared_from_this());
		pt->start();

		comrades.push_back(move(pt));
	}	else if(type.compare("write-test")) {
		auto pt = make_shared<serial_write_pb_session>(io_ref, device_name);

		pt->set_ref(shared_from_this());
		pt->start();

		comrades.push_back(move(pt));
	}
}


/*-----------------------------------------------------------------------------
 * November 25, 2015 :: basic_session methods
 */




} //namespace

#endif /* SESSION_HPP_ */
