/*
 * session.h
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef SESSION_H_
#define SESSION_H_

namespace dew {

using ::boost::asio::io_service;
using ::boost::chrono::steady_clock;
using ::boost::chrono::time_point;

using ::std::string;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::pair;
using ::std::make_pair;

class basic_session;
class dispatcher;

class basic_session{
public:
	basic_session( io_service& io_in, string log_in, dispatcher* ref_in);
	virtual string print() =0;
	virtual ~basic_session() {};
	virtual string get_type() =0;

protected:
  time_point<steady_clock> start_ = steady_clock::now();
	io_service* io_ref;
	string logdir_;
	dispatcher* dis_ref;
private:
};



class dispatcher {
	friend class basic_session;
	friend class network_socket_iface_session;

private:
	void hello(basic_session* new_comrade);
	void forget_(basic_session* ex_comrade);
	vector<basic_session*> comrades;

	string check_connect();
	string close_connection();
	string brag();
	string zabbix_ports();

	std::map<std::string, std::function<std::string()> > network_cmd;

public:
	dispatcher() {};
	~dispatcher();



	string call_net(string cmd);
	bool check_net(string cmd) {return (network_cmd.find(cmd) != network_cmd.end());}
	void init_net_cmd ();

	template<class container_type>
	void forward(basic_session* msg_from, container_type* msg) {delete msg;}
};


} // dew namespace

#endif /* SESSION_H_ */
