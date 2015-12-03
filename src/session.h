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

class basic_session; class serial_session; class network_session;
class dispatcher;

class basic_session{
	friend class dispatcher;
public:
	basic_session( io_service& io_in, string log_in, dispatcher* ref_in);
	virtual ~basic_session() {};

	string get(string value) {
		if (get_map.count(value))
				return get_map.at(value)();
		else
			return string("Value not found");
	};

protected:
  time_point<steady_clock> start_ = steady_clock::now();
	io_service* io_ref;
	string logdir_;
	dispatcher* dis_ref;

	map<string,std::function<string()> > get_map;
private:
};



class dispatcher {
	friend class basic_session;
	friend class network_socket_iface_session;

private:
	void init_net_cmd ();

	void hello(basic_session* new_comrade);
	void forget_(basic_session* ex_comrade);
	vector<basic_session*> comrades;
	map<string, basic_session*> port_directory;

	string brag();
	string bark();
	string zabbix_ports();

	map<string, std::function<string(string,string)> > root_map;
	map<string, std::function<string()> > raw_map;
	map<string, std::function<string()> > hr_map;

	string help(string type, string item);
	string raw(string item, string host);
	string hr(string item, string host);

public:
	dispatcher() :
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
		)
	{
	}
	~dispatcher();

	string call_net(vector<string> cmds);

	void build_lists();

	template<class container_type>
	void forward(basic_session* msg_from, container_type* msg) {delete msg;}
};


} // dew namespace

#endif /* SESSION_H_ */
