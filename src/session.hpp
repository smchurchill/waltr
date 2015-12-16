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


#include "structs.h"
#include "types.h"
#include "utils.h"
#include "session.h"
#include "serial_session.h"
#include "network_session.h"
#include "network_command.h"


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

nsp dispatcher::make_ns (tcp::endpoint& ep_in) {
	auto pt = make_shared<ns>(context_.service, shared_from_this(), ep_in);
	pt->start_accept();
	network.emplace_back(pt);
	return pt;
}

nsp dispatcher::make_ns (tcp::socket& sock_in) {
	auto pt = make_shared<ns>(context_.service, shared_from_this(), sock_in);
	pt->start_read();
	network.emplace_back(pt);
	return pt;
}

void dispatcher::remove_ns (nsp to_remove) {
	to_remove->cancel_socket();

	for(auto channel : channel_subscribers)
		unsub(channel.first, to_remove);

	network.remove(to_remove);
}

ssp dispatcher::make_r_ss(string device_name, unsigned short timeout) {
	auto pt = make_ss(device_name, timeout);
	pt->start_read();
	return pt;
}

ssp dispatcher::make_rw_ss(string device_name) {
	auto pt = make_ss(device_name);
	pt->start_read();
	return pt;
}

ssp dispatcher::make_rwt_ss(string device_name) {
	auto pt = make_ss(device_name);
	pt->start_read();
	pt->start_write();
	return pt;
}

ssp dispatcher::make_wt_ss(string device_name) {
	auto pt = make_ss(device_name);
	pt->start_write();
	return pt;
}

ssp dispatcher::make_w_ss(string device_name) {
	return make_ss(device_name);
}

ssp dispatcher::make_ss(string device_name, unsigned short timeout) {
	auto pt = make_shared<ss>(context_.service, shared_from_this(), device_name,
			milliseconds(timeout));
	serial.emplace_back(pt);
	return pt;
}

ssp dispatcher::make_ss(string device_name) {
	auto pt = make_shared<ss>(context_.service, shared_from_this(), device_name);
	serial.emplace_back(pt);
	return pt;
}

/* December 15, 2015 :: network communications */

string dispatcher::execute_network_command(network_command command, nsp reference) {
	assert(false);
	return "\n";
}

void dispatcher::receive(string message_in) {
	auto self (shared_from_this());
	auto message = make_shared<string>(message_in);
	context_.service->post(bind(&dispatcher::forward,self,message));
}

void dispatcher::forward(string);
void dispatcher::forward_handler(const error_code&,size_t, bBuffp, nsp);
void dispatcher::unsub(string channel, nsp to_remove) {
	assert(false);
}

/* December 15, 2015 :: basic information */
shared_ptr<const ss> dispatcher::get_ss_from_name(string name) {
	for(auto i : serial)
		if(!i->get_name().compare(name))
			return i;

	return make_shared<const ss>(context_);
}
shared_ptr<const ns> dispatcher::get_ns_from_name(string name) {
	for(auto i : network)
		if(!i->get_name().compare(name))
			return i;

	return make_shared<const ns>(context_);
}



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






/*-----------------------------------------------------------------------------
 * November 25, 2015 :: basic_session methods
 */




} //namespace

#endif /* SESSION_HPP_ */
