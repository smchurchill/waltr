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

using ::std::string;
using ::std::vector;
using ::std::deque;

using ::std::size_t;


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base class
 */
class network_session : public basic_session{
public:
	network_session(
			io_service& io_in, string log_in, dispatcher* ref_in, tcp::endpoint ep_in) :
				basic_session(io_in, log_in, ref_in), endpoint_(ep_in)
	{
	}

	string print();

protected:
	tcp::endpoint endpoint_;
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_ class
 */
class network_socket_session : public network_session {
	friend class network_acceptor_session;

public:
	network_socket_session(
			io_service& io_in, string log_in, dispatcher* ref_in, tcp::endpoint ep_in) :
				network_session(io_in, log_in, ref_in, ep_in), socket_(*io_ref)
	{
	}

protected:
	tcp::socket* get_sock() { return &socket_; }

	const long BUFFER_LENGTH = 8192;
	bBuff request_ = vector<u8>(BUFFER_LENGTH,0);
	bBuff reply_ = vector<u8>(2*BUFFER_LENGTH,0);
	tcp::socket socket_;
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ class
 */
class network_acceptor_session : public network_session {
public:
	network_acceptor_session(
			io_service& io_in, string log_in, dispatcher* ref_in, tcp::endpoint ep_in) :
				network_session(io_in, log_in, ref_in, ep_in),
				acceptor_(*io_ref, endpoint_)
	{
		start();
	}

	void start() { do_accept(); }

private:
	void do_accept();

	tcp::acceptor acceptor_;
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
			io_service& io_in, string log_in, dispatcher* ref_in, tcp::endpoint ep_in) :
				network_socket_session(io_in, log_in, ref_in, ep_in)
	{
	}

	void start() { do_read();	}

private:
	void do_read();
	void do_write(size_t length);
};

/*-----------------------------------------------------------------------------
 * November 27, 2015 :: _socket_iface_ class
 */

class network_socket_iface_session : public network_socket_session {
public:
	network_socket_iface_session(
			io_service& io_in, string log_in, dispatcher* ref_in, tcp::endpoint ep_in) :
				network_socket_session(io_in, log_in, ref_in, ep_in)
	{
	}

	void start() { do_read(); }

protected:

	void do_read();
	void do_write(size_t length);
	bool valid_request(size_t length);
	size_t process_request(size_t length);
};

} // dew namespace

#endif /* NETWORK_SESSION_H_ */
