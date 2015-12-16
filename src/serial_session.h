/*
 * serial_session.h
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef SERIAL_SESSION_H_
#define SERIAL_SESSION_H_

namespace dew {

using ::boost::asio::io_service;
using ::boost::chrono::steady_clock;

using ::boost::asio::basic_waitable_timer;
using ::boost::chrono::time_point;
using ::boost::chrono::milliseconds;

using ::boost::asio::serial_port;

using ::boost::bind;

using ::boost::system::error_code;

using ::std::string;
using ::std::vector;
using ::std::pair;
using ::std::deque;
using ::std::size_t;

/*=============================================================================
 * December 15, 2015 :: serial_session class
 *
 * Contains both read and write capabilities as of December 15, 2015.
 */
class serial_session : public enable_shared_from_this<serial_session> {
public:
	serial_session(context_struct context_in) :
		serial_session(context_in, "/dev/null")
	{
	}

	serial_session(
			context_struct context_in,
			string device_in,
			milliseconds timeout_in
	) :
			context_(context_in),
			port_(context_.service, device_in),
			fd_(port_.native_handle),
			name_(device_in),
			timeout_(timeout_in),
			timer_(context_.service),
			read_type_is_timeout_(true)
	{
	}

	serial_session(
			context_struct context_in,
			string device_in
	) :
			context_(context_in),
			port_(context_.service, device_in),
			fd_(port_.native_handle),
			name_(device_in),
			timeout_(milliseconds(0)),
			timer_(context_.service),
			read_type_is_timeout_(false)
	{
	}

	/* It is recommended to only invoke _timeout_read if the port is not writing
	 * to avoid potential loss or clobbering of data.
	 *
	 * It is also recommended that at most one type of reading/writing loop is
	 * started in a single serial session for similar reasons.
	 */
	void start_write();
	void start_read();



/* December 15, 2015 :: serial_session variables
 *
 * These variables are named by whether they are initialized by the constructor
 */

/* November 13, 2015 :: Timer specific members/methods
 *
 * Using:
 * 	boost::asio::basic_waitable_timer<boost::chrono::steady_clock>
 * 	boost::chrono::time_point<boost::chrono::steady_clock>
 *
 * over:
 * 	boost::asio::steady_timer
 * 	boost::asio::steady_timer::time_point
 *
 * because the latter does not work.  The typedefs of the asio library should
 * make them act the same, but they don't.  The latter doesn't even compile.
 *
 * May be related to the issue located at:
 *	www.boost.org/doc/libs/1_59_0/doc/html/boost_asio/overview/cpp2011/chrono.html
 */
private:
	context_struct context_;
	serial_port port_;
	int fd_;
	string name_;
	milliseconds timeout_;
  basic_waitable_timer<steady_clock> timer_;
  bool read_type_is_timeout_;

	const size_t MAX_FRAME_LENGTH = 4096;
	const size_t BUFFER_LENGTH = 16384;
	serial_icounter_struct ioctl_counters {0};
	message_counter_struct counts {0};
	time_point<steady_clock> front_last = steady_clock::now();
	time_point<steady_clock> dead = steady_clock::now();
	pBuff to_parse;

/* December 15, 2015 :: serial_session methods
 *
 * Method type: prework
 */
	void do_write();
	void do_write(bBuff);
	void do_read();

/* Method type: postwork */
	void handle_write(const error_code&, size_t, bBuffp);
	void handle_read(const error_code&, size_t, bBuffp);

/* Method type: complicated information handling */
	void set_a_check();
	void check_the_deque();
	void deliver(string);
	int scrub(pBuff::iterator);
	int pop_counters();
	bBuff generate_message();

/* Method type: time handling */
	void set_write_timer();
	void set_read_timer();
	void handle_write_timeout(const error_code&);
	void handle_read_timeout(const error_code&);

/* Method type: basic information */
public:
	string get_tx();
	string get_rx();
	string get_message_received_tot() { return to_string(counts.messages_received); }
	string get_message_sent_tot() 		{ return to_string(counts.messages_sent); }
	string get_lost_msg_count() 			{ return to_string(counts.lost_msg_count); }
	string get_frame_too_old() 				{ return to_string(counts.frame_too_old); }
	string get_frame_too_long() 			{ return to_string(counts.frame_too_long); }
	string get_bad_prefix() 					{ return to_string(counts.bad_prefix); }
	string get_bad_crc() 							{ return to_string(counts.bad_crc); }
	string get_bytes_received() 			{ return to_string(counts.bytes_received); }
	string get_msg_bytes_tot() 				{ return to_string(counts.msg_bytes_tot); }
	string get_wrapper_bytes_tot() 		{ return to_string(counts.wrapper_bytes_tot); }
	string get_garbage() 							{ return to_string(counts.garbage); }
	string get_name()									{ return name_; }
	string get_type() 								{ return string("serial"); }
};

} // dew namespace

#endif /* SERIAL_SESSION_H_ */
