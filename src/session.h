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
using ::std::to_string;
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

	virtual string get(string value) =0;

	string get_msg_tot() { return to_string(counts.msg_tot); }
	string get_lost_msg_count() { return to_string(counts.lost_msg_count); }
	string get_frame_too_old() { return to_string(counts.frame_too_old); }
	string get_frame_too_long() { return to_string(counts.frame_too_long); }
	string get_bad_prefix() { return to_string(counts.bad_prefix); }
	string get_bad_crc() { return to_string(counts.bad_crc); }
	string get_bytes_received() { return to_string(counts.bytes_received); }
	string get_msg_bytes_tot() { return to_string(counts.msg_bytes_tot); }
	string get_garbage() { return to_string(counts.garbage); }

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
	string h_tree(string type, );
	string q_tree(vector<string> cmds);
	string z_tree(vector<string> cmds);
	string d_tree(vector<string> cmds);


	void init_net_cmd ();

	void hello(basic_session* new_comrade);
	void forget_(basic_session* ex_comrade);
	vector<basic_session*> comrades;
	map<string, basic_session*> port_directory;

	string brag();
	string zabbix_ports();
	string zbx_sp_x(string ident, int io);

	map<string, std::function<string(string, string)> > root_map;
	map<string, std::function<string()> > query_map;
	map<string, std::function<string()> > zabbix_map;
	map<string, std::function<string()> > dut_map;

	po::options_description root;
	po::positional_options_description root_pos;

public:
	dispatcher() {
		init_net_cmd();
	}
	~dispatcher();

	string call_net(vector<string> cmds);

	template<class container_type>
	void forward(basic_session* msg_from, container_type* msg) {delete msg;}
};


} // dew namespace

#endif /* SESSION_H_ */
