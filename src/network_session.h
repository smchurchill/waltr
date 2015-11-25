/*
 * network_sessions.h
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef NETWORK_SESSION_H_
#define NETWORK_SESSION_H_

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base class
 */
class network_session :
		public basic_session {
public:
	network_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in,
			boost::asio::ip::tcp::endpoint ep_in);

	std::string print();

protected:
	boost::asio::ip::tcp::endpoint endpoint_;
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_ class
 */
class network_socket_session :
		public network_session {
	friend class network_acceptor_session;

public:
	network_socket_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in,
			boost::asio::ip::tcp::endpoint ep_in);

protected:
	boost::asio::ip::tcp::socket* get_sock() { return &socket_; }

	const long BUFFER_LENGTH = 8192;
	std::vector<char> data_ = std::vector<char>(BUFFER_LENGTH,0);
	boost::asio::ip::tcp::socket socket_;
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _acceptor_ class
 */
class network_acceptor_session :
		public network_session {
public:
	network_acceptor_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in,
			boost::asio::ip::tcp::endpoint ep_in);

	void start() { do_accept(); }

private:
	void do_accept();

	boost::asio::ip::tcp::acceptor acceptor_;
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _socket_echo_ class
 *
 * Simple echoing protocol modeled on the tcp echo server example in boost asio
 * library.
 */
class network_socket_echo_session :
		public network_socket_session {

public:
	network_socket_echo_session(
			boost::asio::io_service& io_in,
			std::string log_in,
			dispatcher* ref_in,
			boost::asio::ip::tcp::endpoint ep_in);

	void start() { do_read();	}

private:
	void do_read();
	void do_write(std::size_t length);
};

#endif /* NETWORK_SESSION_H_ */
