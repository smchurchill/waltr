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
using ::std::map;
using ::std::pair;
using ::std::make_pair;

using ::std::move;

using ::std::unique_ptr;
using ::std::shared_ptr;
using ::std::enable_shared_from_this;

class basic_session;
class dispatcher;
class network_acceptor_session;
class network_socket_session;
class serial_read_parse_session;
class serial_write_pb_session;


class basic_session{
	friend class dispatcher;
public:
	basic_session( shared_ptr<io_service> io_in ) :
		io_ref(io_in) {}
	virtual ~basic_session() {}

	string get(string value) {
		if (get_map.count(value))
				return get_map.at(value)();
		else
			return string("Value not found");
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
	string brag();
	string bark();
	string zabbix_ports();

	string help(string type, string item);
	string raw(string item, string host);
	string hr(string item, string host);

	map<string, std::function<string(string,string)> > root_map;
	map<string, std::function<string()> > raw_map;
	map<string, std::function<string()> > hr_map;

	bool local_logging_enabled = false;
	shared_ptr<io_service> io_ref;
	vector<shared_ptr<basic_session> > comrades;
	string logdir_;

public:
	dispatcher(shared_ptr<io_service> io_in, string log_in) :
		root_map(
			{
				{"help", bind(&dispatcher::help,this,_1,_2)},
				{string("zabbix"), bind(&dispatcher::raw,this,_1,_2)},
				{"query", bind(&dispatcher::hr,this,_1,_2)},
			}
		),
		raw_map(
			{
				{"ports", bind(&dispatcher::zabbix_ports,this)}
			}
		),
		hr_map(
			{
				{"comrades", bind(&dispatcher::brag, this)},
				{"map", bind(&dispatcher::bark,this)}
			}
		),
		io_ref(io_in),
		logdir_(log_in)
	{
	}
	~dispatcher() {}

	string call_net(vector<string> cmds);
	string get_logdir() { return logdir_; }
	void set_logdir(string logdir_in) { logdir_ = logdir_in; }

	void forward(basic_session* msg_from, string* msg);

	void make_session (tcp::endpoint& ep_in);
	void make_session (tcp::socket sock_in);
	void make_session (string device_name, string type);

};


} // dew namespace

#endif /* SESSION_H_ */
