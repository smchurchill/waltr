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

private:
	string help(string type, string item, nssp reference);
	string raw(string item, string host, nssp reference);
	string hr(string item, string host, nssp reference);
	string sub(string channel, string output_type, nssp reference);

	string zabbix_ports();
	string zabbix_help();

	string brag();
	string bark();
	string hr_help();

	map<string, std::function<string(string,string,nssp)> > root_map;
	map<string, std::function<string()> > raw_map;
	map<string, std::function<string()> > hr_map;

	map<string, weak_ptr<srs> > srs_map;
	map<string, weak_ptr<swps> > sws_map;
	map<string, weak_ptr<nas> > nas_map;
	map<string, nssp > nss_map;

	list<nssw> waveform_raw_subs;
	map<string, list<nssw> > channel_sub_map = {
			{"waveform_raw", waveform_raw_subs}
	};

	bool local_logging_enabled = false;
	shared_ptr<io_service> io_ref;
	vector<shared_ptr<basic_session> > comrades;
	string logdir_;

	void forward_handler(boost::system::error_code ec, size_t in_length,
			shared_ptr<bBuff> ptr, nssp ptr2);

public:
	dispatcher(shared_ptr<io_service> const& io_in, string log_in) :
		root_map(
			{
				{"help", bind(&dispatcher::help,this,_1,_2,_3)},
				{"zabbix", bind(&dispatcher::raw,this,_1,_2,_3)},
				{"query", bind(&dispatcher::hr,this,_1,_2,_3)},
				{"subscribe", bind(&dispatcher::sub,this,_1,_2,_3)}
			}
		),
		raw_map(
			{
				{"ports", bind(&dispatcher::zabbix_ports,this)},
				{"help", bind(&dispatcher::zabbix_help,this)}
			}
		),
		hr_map(
			{
				{"comrades", bind(&dispatcher::brag, this)},
				{"map", bind(&dispatcher::bark,this)},
				{"help", bind(&dispatcher::hr_help,this)}
			}
		),
		io_ref(io_in),
		logdir_(log_in)
	{
	}
	~dispatcher() {}

	string call_net(vector<string> cmds, nssp reference);
	string get_logdir() { return logdir_; }
	void set_logdir(string logdir_in) { logdir_ = logdir_in; }

	void forward(string* msg);

	void make_session (tcp::endpoint& ep_in);
	void make_session (tcp::socket& sock_in);
	void make_session (string device_name, string type);

	const time_point<steady_clock> start_ = steady_clock::now();

};


} // dew namespace

#endif /* SESSION_H_ */
