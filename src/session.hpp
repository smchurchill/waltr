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

/*=============================================================================
 * December 14, 2015 :: Network command parsing
 */

string dispatcher::call_net(sentence to_parse, nssp ref) {
	auto init = make_shared<command>{
		"get", //Do
		"help", //WhatTo
		"dewd", //From
		"me", //For
	};

	do_parse(to_parse,init);

}

void dipatcher::do_parse(sentence to_parse, shared_ptr<command> cmd) {

}



/*-----------------------------------------------------------------------------
 * December 14, 2015 :: Network communications, Layer 0
 */

/* December 14, 2015
 * call_net simply checks whether we may pass to Layer 1 of communications.  It
 * strips away the first command, and sends the rest through the root map.
 *

string dispatcher::call_net(sentence c, nssp reference) {
	auto iter = root_map.find(c[0]);
	c.pop_front();
	if(iter != root_map.end())
		return iter->second(c,reference);
	else
		return "Query not supported.\n";
}*/

string dispatcher::help(sentence c, nssp reference) {
	auto iter = nullptr;
	if(c.empty() ||
		 ((iter = root_map.find(c[0])) == root_map.end()) ||
		 !c[0].compare("help")) {
		string msg ("List of commands:\n");
		for(auto opt : root_map) {
			msg+='\t';
			msg+=opt.first;
			msg+='\n';
		}
		msg+="Pair \'help\' with any of the above for more information.\n";
		return msg;
	} else {
		c.push_front(c[0]);
		c[1]="help";
		return root_map.find(c[0])->second(c, reference);
	}
}

string dispatcher::raw(sentence c, nssp reference) {
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

string dispatcher::hr(sentence vec, nssp reference) {
	auto iter = hr_map.find(item);
	if(iter != hr_map.end())
		return iter->second();
	else
		return "nyet\n";
}
string dispatcher::sub(sentence vec, nssp reference) {
	if((!(option.compare("help"))) || (!channel.compare("help"))) {
		string help("Channels:\n");
		for(auto channel_pr : channel_sub_map) {
			help +='\t';
			help += channel_pr.first;
			help += '\n';
		}
		return help;
	}

	auto iter = channel_sub_map.find(channel);
	if(iter==channel_sub_map.end())
		return "Channel not recognized.  Use \'help subscribe\' to see a list of"
				" channels.\n";

	for(auto subscriber : iter->second)
		if(subscriber.lock() == reference)
			return string("Already subscribed to " + channel + '\n');

	iter->second.emplace_back(nssw(reference));

	return string("Successfully subscribed to channel "+channel+'\n');
}

string dispatcher::unsub(sentence vec, nssp reference) {
	if((!(option.compare("help"))) || (!channel.compare("help")))
		return "subscription help\n";

	auto iter = channel_sub_map.find(channel);
	if(iter == channel_sub_map.end())
		return "Channel not recognized.  Use \'help unsubscribe\' to see a list of"
				" channels.\n";

	bool removed = false;

	for(auto sub_iter = iter->second.begin() ;
			sub_iter != iter->second.end() ;)
		if(sub_iter->lock() == reference){
			iter->second.erase(sub_iter);
			sub_iter = iter->second.begin();
			removed=true;
		} else
			 ++sub_iter;

	if(removed)
		return "Successfully unsubscribed from channel "+channel+'\n';
	else
		return "You were not subscribed to channel "+channel+'\n';
}




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





string dispatcher::zabbix_help(vector<string> vec, nssp reference) {
	return "zabbix help\n";
}

string dispatcher::hr_help(vector<string> vec, nssp reference) {
	return "human readable help\n";
}


void dispatcher::forward(shared_ptr<string> msg) {
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

			for(auto pr : channel_sub_map)
				if(pr.first.find("waveform") != string::npos)
					for(auto sub : pr.second)
						async_write(sub.lock()->get_sock(),
								boost::asio::buffer(*buffer_, buffer_->size()),
								bind(&dispatcher::forward_handler,self,_1,_2,
										buffer_,sub.lock()));

			}
	}
}

void dispatcher::forward_handler(boost::system::error_code ec, size_t in_length,
		shared_ptr<bBuff> ptr1, nssp ptr2) {
	ptr2.reset();
	ptr1.reset();
}



/*	December 14, 2015 --  Session creation.
 *
 * Under the new object management model, the dispatcher handles the creation
 * and destruction of all sessions.  The only sessions with lifetime less than
 * the run time of the program are the network sockets.
 *
 */

void dispatcher::make_nas (tcp::endpoint& ep_in) {
	auto pt = make_shared<nas>(io_ref, ep_in);

	pt->set_ref(shared_from_this());
	pt->start();

	comrades.push_back(move(pt));
}

void dispatcher::make_nss (tcp::socket& sock_in) {

	auto pt = make_shared<network_socket_session>(io_ref, sock_in);

	pt->set_ref(shared_from_this());
	pt->start();

	nss_map.insert({pt->get("name"),pt});
}

void dispatcher::remove_nss (nssp to_remove) {

	to_remove->get_sock().cancel();

	for(auto channel : channel_sub_map)
		unsub(channel.first, string("disconnect"), to_remove);

	nss_map.erase(to_remove->get("name"));

}


void dispatcher::make_ss (string device_name, string type) {
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
