/*
 * session.h
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef SESSION_H_
#define SESSION_H_

namespace po = boost::program_options;

namespace dew {

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::asio::serial_port;
using ::boost::chrono::steady_clock;
using ::boost::chrono::time_point;
using ::boost::chrono::milliseconds;
using ::boost::bind;
using ::boost::system::error_code;

using ::std::string;
using ::std::to_string;
using ::std::vector;
using ::std::deque;
using ::std::list;
using ::std::map;
using ::std::pair;
using ::std::make_pair;

using ::std::move;

using ::std::unique_ptr;
using ::std::shared_ptr;
using ::std::weak_ptr;
using ::std::enable_shared_from_this;

class dispatcher;
class network_session;
class serial_session;

class dispatcher : public enable_shared_from_this<dispatcher> {

public:


private:
	map<string, std::function<string(sentence,nssp)> > root_map = {
			{"help", bind(&dispatcher::help,this,_1,_2)},
			{"zabbix", bind(&dispatcher::raw,this,_1,_2)},
			{"query", bind(&dispatcher::hr,this,_1,_2)},
			{"subscribe", bind(&dispatcher::sub,this,_1,_2)},
			{"unsubscribe", bind(&dispatcher::unsub,this,_1,_2)}
		};

	string help(sentence c, nssp reference);
	string raw(sentence c, nssp reference);
	string hr(sentence c, nssp reference);
	string sub(sentence c, nssp reference);
	string unsub(sentence c, nssp reference);
	// Layer 0

	/* Layer 1 of network command parsing.
	 * A resource was called by root_map, and that resource further delegates
	 * down the decision tree.
	 */
private:
	map<string, std::function<string(vector<string>,nssp)> > raw_map = {
			{"ports", bind(&dispatcher::zabbix_ports,this,_1,_2)},
			{"help", bind(&dispatcher::zabbix_help,this,_1,_2)},
			{"get", bind(&dispatcher::zabbix_get,this,_1,_2)}
		};

	string zabbix_ports(vector<string> vec, nssp reference);
	string zabbix_help(vector<string> vec, nssp reference);
	string zabbix_get(vector<string> vec, nssp reference);

	map<string, std::function<string(vector<string>,nssp)> > hr_map = {
			{"comrades", bind(&dispatcher::brag, this, _1, _2)},
			{"map", bind(&dispatcher::bark,this, _1, _2)},
			{"help", bind(&dispatcher::hr_help,this, _1, _2)}
		};

	string brag(vector<string> vec, nssp reference);
	string bark(vector<string> vec, nssp reference);
	string hr_help(vector<string> vec, nssp reference);


public:
	dispatcher(shared_ptr<io_service> const& io_in, string log_in) :
		context_{steady_clock::now(), io_in},
		logdir_(log_in)
	{
	}
	~dispatcher() {}



private:
	context_struct context_;
	string logdir_;

	list<ssp> serial;
	list<nsp> network;

	vector<list<nsp> > subscriptions = {
			{} // Channel 0 is raw waveforms.
	};

	bool local_logging_enabled = false;


/* Method type: creation and destruction of sessions */
public:
	nsp make_ns (tcp::endpoint&);
	nsp make_ns (tcp::socket&);
	void remove_ns (nsp);

	ssp make_r_ss(string, unsigned short);
	ssp make_rw_ss(string);
	ssp make_rwt_ss(string);
	ssp make_wt_ss(string);
	ssp make_w_ss(string);
private:
	ssp make_ss (string, unsigned short);
	ssp make_ss (string);

/* Method type: network communications */
	string execute_network_command(network_command command, nsp reference);
	void receive(string);
	void forward(string);
	void forward_handler(const error_code&,size_t, bBuffp, nsp);






	void unsub(string, nsp);

/* Method type: basic information */
public:
	string get_logdir() { return logdir_; }
	shared_ptr<const ss> get_ss_from_name(string name);
	shared_ptr<const ns> get_ns_from_name(string name);
};


} // dew namespace

#endif /* SESSION_H_ */
