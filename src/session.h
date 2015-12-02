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

using ::std::string;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::pair;
using ::std::make_pair;

class basic_session; class serial_session; class network_session;
class dispatcher;

class basic_session{
public:
	basic_session( io_service& io_in, string log_in, dispatcher* ref_in);
	virtual ~basic_session() {};

	virtual string print() =0;
	virtual string get_type() =0;
	virtual string get_rx() =0;
	virtual string get_tx() =0;

	struct counter_struct {
		/* For the serial_read_parse_session class */
		int msg_tot;
		int bytes_received;

		int last_msg;
		int curr_msg;
		int lost_msg_count;

		int frame_too_long;
		int frame_too_old;
		int bad_prefix;
		int bad_crc;

		int msg_bytes_tot;
		int garbage;
	};
	struct counter_struct counts {0};

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
	void init_net_cmd ();

	void hello(basic_session* new_comrade);
	void forget_(basic_session* ex_comrade);
	vector<basic_session*> comrades;

	string rx_name;
	string tx_name;

	string brag();
	string zabbix_ports();
	string zbx_sp_x(string ident, int io);

	std::map<std::string, std::function<std::string()> > network_cmd;

public:
	dispatcher() : cmd_options("Network Command Options") {
		init_net_cmd();
	}
	~dispatcher();


	po::options_description cmd_options;
	string call_net(vector<string> cmds);

	template<class container_type>
	void forward(basic_session* msg_from, container_type* msg) {delete msg;}


};


} // dew namespace

#endif /* SESSION_H_ */
