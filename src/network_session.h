/*
 * network_sessions.h
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef NETWORK_SESSION_H_
#define NETWORK_SESSION_H_

namespace dew {

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::chrono::steady_clock;

using ::std::stringstream;
using ::std::string;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::pair;

using ::std::size_t;

using ::std::enable_shared_from_this;


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base class
 */
class network_session : public basic_session{
public:
	network_session(
			shared_ptr<io_service> io_in, tcp::endpoint const ep_in) :
				basic_session(io_in), endpoint_(ep_in)
	{
		map<string,std::function<string()> > tmp_map
			{
				{"name", bind(&network_session::get_name,this)},
				{"type", bind(&network_session::get_type,this)}
			};
		get_map.insert(tmp_map.begin(),tmp_map.end());
	}

	~network_session() {};

protected:

	string get_name() { stringstream ss; ss << endpoint_; return ss.str(); }
	string get_type() { return string("network"); }
	tcp::endpoint endpoint_;
};

/*-----------------------------------------------------------------------------
 * December 8, 2015 :: _socket_ class
 */
class network_socket_session : public network_session,
	public enable_shared_from_this<network_socket_session> {
	friend class network_acceptor_session;

public:
	network_socket_session(
			shared_ptr<io_service> io_in, tcp::socket sock_in) :
			network_session(io_in, sock_in.local_endpoint()), socket_(move(sock_in))
	{
		map<string,std::function<string()> > tmp_map
			{
				{"subtype", bind(&network_socket_session::get_subtype,this)}
			};
		get_map.insert(tmp_map.begin(),tmp_map.end());
	}

	void start() { do_read(); }

protected:
	void do_read();
	void handle_read( boost::system::error_code ec, size_t in_length);

	string get_subtype() { return "socket"; }

	const long BUFFER_LENGTH = 8192;
	bBuff request_ = vector<u8>(BUFFER_LENGTH,0);
	bBuff reply_ = vector<u8>(2*BUFFER_LENGTH,0);
	tcp::socket socket_;
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ class
 */
class network_acceptor_session : public network_session,
	public enable_shared_from_this<network_acceptor_session> {
public:
	network_acceptor_session(
			shared_ptr<io_service> io_in, tcp::endpoint & ep_in) :
			network_session(io_in, ep_in),
			acceptor_(*io_ref, endpoint_),
			socket_(*io_ref)
	{
		map<string,std::function<string()> > tmp_map
			{
				{"subtype", bind(&network_acceptor_session::get_subtype,this)}
			};
		get_map.insert(tmp_map.begin(),tmp_map.end());
	}

	void start() { do_accept(); }


private:

	void do_accept();

	string get_subtype() { return "acceptor"; }

	tcp::acceptor acceptor_;
	tcp::socket socket_;
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_echo_ class
 *
 * Simple echoing protocol modeled on the tcp echo server example in boost asio
 * library.
 */
class network_socket_echo_session :	public network_socket_session {

public:
	network_socket_echo_session(
			shared_ptr<io_service> io_in, tcp::socket sock_in) :
				network_socket_session(io_in, move(sock_in))
	{
	}

	void start() { do_read();	}

private:
	void do_read();
	void do_write(size_t length);
};

} // dew namespace

#endif /* NETWORK_SESSION_H_ */
