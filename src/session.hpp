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

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>



#include "types.h"
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
	for(auto wp : srs_map) {
		auto sp = wp.second.lock();
			if(not_first)
				json += ",";
			json += "{\"{#DEWDSP}\":\"";
			json += sp->get("name");
			json += "\"}";
			++not_first;
	}
	json += "]}";
	return json;
}

string dispatcher::call_net(vector<string> cmds, nssp reference) {
	/* command parsing goes here */
	auto iter = root_map.find(cmds[0]);
	if(iter != root_map.end())
		return iter->second(cmds[1],cmds[2],reference);
	else
		return "Query not supported.\n";

}

string dispatcher::help(string type, string item, nssp reference) {
	auto iter = root_map.find(type);
	if((!(type.compare("help"))) || iter==root_map.end())
		return string(
				"Usage: \'help type item\' where \'type\' is one of help, zabbix,"
				" query, subscribe.  Use \'help type\' to see a list of items.\n"
		);
	return iter->second(string("help"),item, reference);
}

string dispatcher::raw(string item, string host, nssp reference) {
	auto iter = srs_map.find(host);
	auto oter = raw_map.find(item);
	if(iter != srs_map.end()){
		auto sp = iter->second.lock();
		return sp->get(item);
	}
	else if(oter != raw_map.end()) {
		return oter->second();
	}

	return "-1";
}

string dispatcher::hr(string item, string host, nssp reference) {
	auto iter = hr_map.find(item);
	if(iter != hr_map.end())
		return iter->second();
	else
		return "nyet\n";
}
string dispatcher::sub(string channel,string output_type, nssp reference) {
	if((!(output_type.compare("help"))) || (!channel.compare("help")))
		return "subscription help\n";

	string channel_name (channel + "_" + output_type);
	auto iter = channel_sub_map.find(channel_name);
	if(iter==channel_sub_map.end())
		return "Channel not recognized.  Use \'help subscribe\' to see a list of"
				" channels.\n";

	iter->second.emplace_back(nssw(reference));

	if(output_type.compare("raw"))
		return string("Successfully subscribed to channel "+channel_name+'\n');

	return string("\n");
}

string dispatcher::zabbix_help() { return "zabbix help\n"; }

string dispatcher::hr_help() { return "human readable help\n"; }


void dispatcher::forward(string* msg) {
	flopointpb::FloPointWaveform fpwf;
	auto self=shared_from_this();

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
	} else {
		if(!(fpwf.ParseFromString(*msg))) {
			string s (to_string(steady_clock::now()) + ": Could not parse string.\n");
			cout << s;
		} else {
			auto buffer_ = make_shared<bBuff>();
			for(auto wheight : fpwf.waveform().wheight()) {
				bBuff bytes_(5);
				bytes_[0] = '\t';
				bytes_[1] = (wheight >> 24 ) & 0xFF;
				bytes_[2] = (wheight >> 16 ) & 0xFF;
				bytes_[3] = (wheight >> 8 ) & 0xFF;
				bytes_[4] = wheight & 0xFF;
				copy(bytes_.begin(),bytes_.end(),back_inserter(*buffer_));
			}
			buffer_->push_back('\n');

			for(auto pr : channel_sub_map) {
				if(pr.first.find("waveform") != string::npos) {
					for(auto sub = pr.second.begin(); sub != pr.second.end() ; ++sub) {
						if(sub->expired()) {
							pr.second.erase(sub);
						} else {
							async_write(sub->lock()->get_sock(),
								boost::asio::buffer(*buffer_, buffer_->size()),
								bind(&dispatcher::forward_handler,self,_1,_2,
										buffer_,sub->lock()));
						}
					}
				}
			}
		}
	}
	delete msg;
}

void dispatcher::forward_handler(boost::system::error_code ec, size_t in_length,
		shared_ptr<bBuff> ptr1, nssp ptr2) {
	ptr2.reset();
	ptr1.reset();
}


void dispatcher::make_session (tcp::endpoint& ep_in) {
	auto pt = make_shared<nas>(io_ref, ep_in);

	pt->set_ref(shared_from_this());
	pt->start();

	comrades.push_back(move(pt));
}

void dispatcher::make_session (tcp::socket& sock_in) {

	auto pt = make_shared<network_socket_session>(io_ref, sock_in);

	pt->set_ref(shared_from_this());
	pt->start();

	nss_map.insert({pt->get("name"),pt});

	if(0)
	for(auto s : nss_map ){
		if(s.second->is_dead()) {
			s.second.reset();
			nss_map.erase(s.first);
		}
	}
}


void dispatcher::make_session (string device_name, string type) {
	if(type.compare("read")) {
		auto pt = make_shared<srs>(io_ref, device_name);

		pt->set_ref(shared_from_this());
		pt->start();

		srs_map.insert({pt->get("name"), pt});
		comrades.push_back(move(pt));
	}	else if(type.compare("write-test")) {
		auto pt = make_shared<swps>(io_ref, device_name);

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
