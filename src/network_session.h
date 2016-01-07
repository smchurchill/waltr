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
using ::std::move;

using ::std::enable_shared_from_this;

class network_session : public enable_shared_from_this<network_session> {
public:
	network_session(context_struct);

	network_session(
			context_struct,
			tcp::endpoint const&,
			string
	);

	~network_session() {};

	shared_ptr<network_session> get_ns();

	void start_connect() { do_connect(); }
	void do_write(stringp);
	void do_write(string&);

	void cancel_socket() { if(socket_.is_open()) socket_.cancel(); }

/* December 15, 2015
 *
 * These variables are named by whether they are initialized by the constructor
 */
private:
	context_struct context_;
	tcp::endpoint dewd_;
	tcp::socket socket_;
	string channel_;

	const size_t MAX_FRAME_LENGTH = 4096;
	const long BUFFER_LENGTH = 8192;
	pBuff the_deque = pBuff (4*BUFFER_LENGTH);
	time_point<steady_clock> front_last = steady_clock::now();

	void do_connect();
	void do_read();
	void handle_read(boost::system::error_code, size_t, bBuffp);
	void handle_write(boost::system::error_code, size_t, bBuffp);
	void handle_connect(boost::system::error_code);

	void set_a_check();
	void check_the_deque();
	int scrub(pBuff::iterator);

public:
};

} // dew namespace

#endif /* NETWORK_SESSION_H_ */
