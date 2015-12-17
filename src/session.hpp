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
#include <set>
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
#include "serial_session.h"
#include "network_session.h"
#include "network_help.h"
#include "command_graph.h"

#include "session.h"

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
using ::std::set;
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

/* December 16, 2015 :: constructors */

dispatcher::dispatcher(shared_ptr<io_service> const& io_in) :
	context_(io_in),
	root(root_nodes)
{
}
dispatcher::dispatcher(shared_ptr<io_service> const& io_in, string log_in) :
	context_(io_in),
	logdir_(log_in),
	root(root_nodes)
{
}

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
	auto pt = make_shared<ns>(context_struct(context_, shared_from_this()),ep_in);
	pt->start_accept();
	network.emplace_back(pt->get_ns());
	return pt->get_ns();
}

nsp dispatcher::make_ns (tcp::socket& sock_in) {
	auto pt = make_shared<ns>(context_struct(context_, shared_from_this()), sock_in);
	pt->start_read();
	network.emplace_back(pt->get_ns());
	return pt->get_ns();
}

void dispatcher::remove_ns (nsp to_remove) {
	to_remove->cancel_socket();

	//for(auto channel : channel_subscribers)
	//	unsub(channel.first, to_remove);

	network.remove(to_remove);
}

ssp dispatcher::make_r_ss(string device_name, unsigned short timeout) {
	auto pt = make_ss(device_name, timeout);
	serial_reading.emplace_back(pt->get_ss());
	pt->start_read();
	return pt->get_ss();
}

ssp dispatcher::make_rw_ss(string device_name) {
	auto pt = make_ss(device_name);
	serial_reading.emplace_back(pt->get_ss());
	serial_writing.emplace_back(pt->get_ss());
	pt->start_read();
	return pt->get_ss();
}

ssp dispatcher::make_rwt_ss(string device_name) {
	auto pt = make_ss(device_name);
	serial_reading.emplace_back(pt->get_ss());
	serial_writing.emplace_back(pt->get_ss());
	pt->start_read();
	pt->start_write();
	return pt->get_ss();
}

ssp dispatcher::make_wt_ss(string device_name) {
	auto pt = make_ss(device_name);
	serial_writing.emplace_back(pt->get_ss());
	pt->start_write();
	return pt->get_ss();
}

ssp dispatcher::make_w_ss(string device_name) {
	auto pt = make_ss(device_name);
	serial_writing.emplace_back(pt->get_ss());
	return pt->get_ss();
}

ssp dispatcher::make_ss(string device_name, unsigned short timeout) {
	auto pt = make_shared<ss>(context_struct(context_, shared_from_this()), device_name,
			milliseconds(timeout));
	return pt->get_ss();
}

ssp dispatcher::make_ss(string device_name) {
	auto pt = make_shared<ss>(context_struct(context_, shared_from_this()), device_name);
	return pt->get_ss();
}

/* December 15, 2015 :: network communications */

void dispatcher::execute_network_command(
		sentence command, nsp reference) {
	assert(false);
}

void dispatcher::receive(string message_in) {
	auto self (shared_from_this());
	auto message = make_shared<string>(message_in);
	context_.service->post(bind(&dispatcher::forward,self,message));
}

void dispatcher::forward(shared_ptr<string> msg) {
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

	if(!(fpwf.ParseFromString(*msg))) {
		string s (to_string(steady_clock::now()) + ": Could not parse string:\n");
		cout << s;
		debug(*msg);
	} else {
		string wf_str;
		for(auto wheight : fpwf.waveform().wheight()) {
			wf_str += '\t';
			wf_str += (wheight >> 24 ) & 0xFF;
			wf_str += (wheight >> 16 ) & 0xFF;
			wf_str += (wheight >> 8 ) & 0xFF;
			wf_str += wheight & 0xFF;
		}
		wf_str += '\n';
		for(auto channel : subscriptions)
			if(channel.first.find("waveform") != string::npos)
				for(auto subscriber : channel.second)
					subscriber->do_write(wf_str);
	}
}

void dispatcher::subscribe(nsp sub, string channel) {
	auto sub_set = subscriptions.find(channel);
	if(sub_set->second.find(sub) == sub_set->second.end()) {
		sub_set->second.emplace(sub);
	}
}

void dispatcher::unsubscribe(nsp sub, string channel) {
	auto sub_set = subscriptions.find(channel);
	auto locator = sub_set->second.find(sub);
	if(locator != sub_set->second.end()) {
		sub_set->second.erase(locator);
	}
}


void dispatcher::ports_for_zabbix(nsp in) {
	string json ("{\"data\":[");
	int not_first = 0;
	for(auto port : serial_reading) {
			if(not_first)
				json += ",";
			json += "{\"{#DEWDSP}\":\"";
			json += port->get_name();
			json += "\"}";
			++not_first;
	}
	json += "]}";
	in->do_write(json);
}


/* December 16, 2015 :: command tree building */

void dispatcher::build_command_tree() {
	if(!added_static_leaves)
		add_static_leaves();

	purge_dynamic_leaves();
	add_dynamic_leaves();
}

void dispatcher::add_static_leaves() {
	auto self (shared_from_this());
	root.child("get")->child("ports_for_zabbix")->set_fn(
			std::function<void(nsp)>(
					bind(&dispatcher::ports_for_zabbix,self,_1)));

	for(auto channel : subscriptions) {
		root.child("subscribe")->child("to")->spawn(
				channel.first,
				make_shared<node>(
						std::function<void(nsp)>(
							bind(&dispatcher::subscribe,self,_1,channel.first))));
		root.child("unsubscribe")->child("from")->spawn(
				channel.first,
				make_shared<node>(
						std::function<void(nsp)>(
							bind(&dispatcher::unsubscribe,self,_1,channel.first))));
	}
	added_static_leaves = true;
}

void dispatcher::purge_dynamic_leaves() {
	root.child("get")->child("rx")->purge();
	root.child("get")->child("tx")->purge();
	root.child("get")->child("messages_received_tot")->purge();
	root.child("get")->child("messages_lost_tot")->purge();
}

void dispatcher::add_dynamic_leaves() {
	for(auto port : serial_reading) {
		root.child("get")->child("rx")->spawn(
			port->get_name(),
			make_shared<node>(
					std::function<void(nsp)>(
							bind(&ss::get_rx,port,_1))));
		root.child("get")->child("messages_received_tot")->spawn(
			port->get_name(),
			make_shared<node>(
					std::function<void(nsp)>(
							bind(&ss::get_messages_received_tot,port,_1))));
		root.child("get")->child("messages_lost_tot")->spawn(
			port->get_name(),
			make_shared<node>(
					std::function<void(nsp)>(
							bind(&ss::get_messages_lost_tot,port,_1))));
	}

	for(auto port : serial_writing) {
		root.child("get")->child("tx")->spawn(
			port->get_name(),
			make_shared<node>(
					std::function<void(nsp)>(
							bind(&ss::get_tx,port,_1))));
	}

}

} //namespace

#endif /* SESSION_HPP_ */
