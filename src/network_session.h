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
using ::boost::chrono::milliseconds;

using ::std::stringstream;
using ::std::string;
using ::std::vector;
using ::std::deque;
using ::std::map;
using ::std::pair;
using ::std::cout;
using ::std::size_t;

using ::std::enable_shared_from_this;


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base class
 */
class network_session : public basic_session{
public:
	network_session() : basic_session() {}

	network_session(
			shared_ptr<io_service> const& io_in, tcp::endpoint const& ep_in) :
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

	virtual string get_name() { stringstream ss; ss << endpoint_; return ss.str(); }
	string get_type() { return string("network"); }
	tcp::endpoint endpoint_;
};

/*-----------------------------------------------------------------------------
 * December 8, 2015 :: _socket_ class
 */
class network_socket_session : public network_session,
	public enable_shared_from_this<network_socket_session> {
	friend class network_acceptor_session;
	friend class dispatcher;
public:
	network_socket_session(
			shared_ptr<io_service> const& io_in, tcp::socket& sock_in) :
			network_session(io_in, sock_in.local_endpoint()), socket_(move(sock_in))
	{
		stringstream ss;
		ss << socket_.remote_endpoint();
		name_ = ss.str();

		map<string,std::function<string()> > tmp_map
			{
				{"subtype", bind(&network_socket_session::get_subtype,this)}
			};
		get_map.insert(tmp_map.begin(),tmp_map.end());
	}

	~network_socket_session() { cout << get_name() << " destructed\n"; }

	void start() { do_read(); }


protected:
	void do_read();
	void handle_read( boost::system::error_code ec, size_t in_length);

	string name_;

	const long BUFFER_LENGTH = 8192;
	bBuff request_ = vector<u8>(BUFFER_LENGTH,0);
	bBuff reply_ = vector<u8>(2*BUFFER_LENGTH,0);
	tcp::socket socket_;

	tcp::socket& get_sock() { return socket_; }

	string get_subtype() { return "socket"; }
	string get_name() { return name_; }
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ class
 */
class network_acceptor_session : public network_session,
	public enable_shared_from_this<network_acceptor_session> {
public:
	network_acceptor_session(
			shared_ptr<io_service> const& io_in, tcp::endpoint const& ep_in) :
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


} // dew namespace

#endif /* NETWORK_SESSION_H_ */
