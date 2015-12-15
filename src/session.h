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
using ::boost::bind;

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

class basic_session;
class dispatcher;
class network_acceptor_session;
class network_socket_session;
class serial_read_session;
class serial_write_pb_session;


class basic_session {
	friend class dispatcher;
public:
	basic_session() {}
	basic_session( shared_ptr<io_service> const& io_in ) :
		io_ref(io_in) {}
	virtual ~basic_session() {}

	string get(string value) {
		if (get_map.count(value))
				return get_map.at(value)();
		else
			return string("Value \"" + value +"\" not found");
	};

protected:
	bool alive = true;
	bool is_alive() { return alive; }
	bool is_dead() { return !alive; }
	void live() { alive = true; }
	void die() {alive = false; }

	void set_ref(shared_ptr<dispatcher> dis_in) { dis_ref = dis_in;}

  time_point<steady_clock> start_ = steady_clock::now();
	shared_ptr<io_service> io_ref;
	shared_ptr<dispatcher> dis_ref;

	map<string,std::function<string()> > get_map;
private:
};



class dispatcher : public enable_shared_from_this<dispatcher> {

	/* Layer 0 of network command parsing.
	 * Some other function distributes a list of commands to call_net for
	 * parsing.  Call_net will delegate through root_map to the approriate
	 * resource.
	 */
public:
	string call_net(sentence c, nssp reference);

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

	// Layer 1






	map<string, weak_ptr<srs> > srs_map;
	map<string, weak_ptr<swps> > sws_map;
	map<string, weak_ptr<nas> > nas_map;
	map<string, nssp > nss_map;

	list<nssw> raw_waveforms;
	map<string, list<nssw> > channel_sub_map = {
			{"waveform_raw", raw_waveforms}
	};

	bool local_logging_enabled = false;
	shared_ptr<io_service> io_ref;
	vector<shared_ptr<basic_session> > comrades;
	string logdir_;

	void forward_handler(boost::system::error_code ec, size_t in_length,
			shared_ptr<bBuff> ptr, nssp ptr2);

public:
	dispatcher(shared_ptr<io_service> const& io_in, string log_in) :

		io_ref(io_in),
		logdir_(log_in)
	{
	}
	~dispatcher() {}


	string get_logdir() { return logdir_; }
	void set_logdir(string logdir_in) { logdir_ = logdir_in; }

	void forward(shared_ptr<string> msg);

	void make_nas (tcp::endpoint& ep_in);

	void make_nss (tcp::socket& sock_in);
	void remove_nss (nssp to_remove);

	void make_ss (string device_name, string type);

	const time_point<steady_clock> start_ = steady_clock::now();

};


} // dew namespace

#endif /* SESSION_H_ */
