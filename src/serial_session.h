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

using ::std::string;
using ::std::vector;
using ::std::pair;
using ::std::deque;
using ::std::size_t;

/*-----------------------------------------------------------------------------
 * December 2, 2015 :: base class
 */
class serial_session : public basic_session{
	friend class dispatcher;
public:
	serial_session(
			io_service& io_in, string log_in, dispatcher* ref_in, string device_in) :
				basic_session(io_in, log_in, ref_in),
				name_(device_in),	port_(*io_ref, name_)
	{
		map<string,std::function<string()> > tmp_map {
			{"name", bind(&serial_session::get_name,this)},
			{"type", bind(&serial_session::get_type,this)},
			{"tx",bind(&serial_session::get_tx,this)},
			{"rx",bind(&serial_session::get_rx,this)}
		};
		get_map.insert(tmp_map.begin(),tmp_map.end());
		fd = port_.native_handle();
	}

//	string get(string value) {return get_map[value]();}

protected:
	string get_name() { return name_; }
	string get_type() { return string("serial"); }

	string get_tx();
	string get_rx();

	int pop_counters() {
		memset(&counters, 0, sizeof(serial_icounter_struct));
		return ioctl(fd, TIOCGICOUNT, &counters);
	}

	string name_;
	serial_port port_;
	int fd;
	struct serial_icounter_struct counters {0};
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _read_ class
 */
class serial_read_session :	public serial_session {
	friend class dispatcher;
public:
	serial_read_session(
			io_service& io_in, string log_in, dispatcher* ref_in, string device_in) :
				serial_session(io_in, log_in, ref_in, device_in), timer_(*io_ref)
	{
		map<string,std::function<string()> > tmp_map
			{
				{"subtype", bind(&serial_read_session::get_subtype,this)}
			};
		get_map.insert(tmp_map.begin(),tmp_map.end());
	}

	virtual ~serial_read_session() {}

protected:

	string get_subtype() { return string("serial-read"); }

	/* November 18, 2015
	 * AJS says that pang has a 4k kernel buffer.  We want our buffer to be
	 * bigger than that, and we can be greedy with what "bigger" means.
	 */
	const size_t BUFFER_LENGTH = 16384;

	/* Timer specific members/methods */

	/* November 13, 2015
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
	 *
	 *
	 */
  basic_waitable_timer<steady_clock> timer_;
  time_point<steady_clock> dead_;

  /* November 18, 2015
	 * This value gives us a polling rate of 10Hz.  This is the only place where
	 * this polling rate is set.
	 */
	milliseconds timeout_ = milliseconds(100);

	/* November 20, 2015
	 * This timeout handling attempts to set a stopwatch that expires every 100ms
	 * so that the read/handle cycle of a serial_read_session operates at 10Hz.
	 * 10Hz timers are available in all serial_read_sessions, but are not
	 * necessary. The set_timer() function must be called by the inheriting class
	 * if the class wants to use the timer.
	 */
	void set_timer();
	void handle_timeout(boost::system::error_code ec);
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _read_parse_ class
 */
class serial_read_parse_session :	public serial_read_session {
	friend class dispatcher;
public:
	serial_read_parse_session(
			io_service& io_in, string log_in, dispatcher* ref_in, string device_in) :
				serial_read_session(io_in, log_in, ref_in, device_in)
	{
		map<string,std::function<string()> > tmp_map {
				{"msg_tot",bind(&serial_read_parse_session::get_msg_tot,this)},
				{"lost_msg_count",bind(&serial_read_parse_session::get_lost_msg_count,this)},
				{"frame_too_old", bind(&serial_read_parse_session::get_frame_too_old,this)},
				{"frame_too_long", bind(&serial_read_parse_session::get_frame_too_long,this)},
				{"bad_prefix", bind(&serial_read_parse_session::get_bad_prefix,this)},
				{"bad_crc", bind(&serial_read_parse_session::get_bad_crc,this)},
				{"bytes_received", bind(&serial_read_parse_session::get_bytes_received,this)},
				{"wrapper_bytes_tot", bind(&serial_read_parse_session::get_wrapper_bytes_tot,this)},
				{"msg_bytes_tot", bind(&serial_read_parse_session::get_msg_bytes_tot,this)},
				{"garbage", bind(&serial_read_parse_session::get_garbage,this)}
		};

		get_map.insert(tmp_map.begin(),tmp_map.end());

		this->start();
	}
	~serial_read_parse_session() {}

	void start();

private:
	void start_read();
	void handle_read(boost::system::error_code ec, size_t length,
			bBuff* buffer_);
	void check_the_deque();
	int scrub(pBuff::iterator iter);

	time_point<steady_clock> front_last = steady_clock::now();

	const size_t MAX_FRAME_LENGTH = 4096;
	pBuff to_parse;

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

		int wrapper_bytes_tot;
		int msg_bytes_tot;
		int garbage;
	};
	struct counter_struct counts {0};

	string get_msg_tot() { return to_string(counts.msg_tot); }
	string get_lost_msg_count() { return to_string(counts.lost_msg_count); }
	string get_frame_too_old() { return to_string(counts.frame_too_old); }
	string get_frame_too_long() { return to_string(counts.frame_too_long); }
	string get_bad_prefix() { return to_string(counts.bad_prefix); }
	string get_bad_crc() { return to_string(counts.bad_crc); }
	string get_bytes_received() { return to_string(counts.bytes_received); }
	string get_msg_bytes_tot() { return to_string(counts.msg_bytes_tot); }
	string get_wrapper_bytes_tot() {return to_string(counts.wrapper_bytes_tot); }
	string get_garbage() { return to_string(counts.garbage); }
};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _read_log_ class
 */
class serial_read_log_session :	public serial_read_session {
public:
		serial_read_log_session(
			io_service& io_in, string log_in,	dispatcher* ref_in,	string device_in) :
				serial_read_session(io_in, log_in, ref_in, device_in)
	{
		filename_ = logdir_ + name_.substr(name_.find_last_of("/\\")+1);
		this->start();
	}

	void start();

	/* November 20, 2015
	 * This kicks off the reading work loop, and the loop returns here every time
	 * through.  This is the only place that the asyc_read work is added to the
	 * io_service.
	 *
	 * The new vector made in this method is freed when handle_read is called,
	 * once the async_read work breaks.
	 */
	void start_read();

	/* November 20, 2015
	 * The first thing we do is kick off another read work cycle by calling the
	 * start_read() method. If $length > 0 then there are characters to write, so
	 * we write $length characters from buffer_ to file_.  Otherwise, $length=0
	 * and there are no characters ready to write in buffer_.
	 */
	void handle_read(boost::system::error_code ec, size_t length,
		bBuff* buffer_);

private:
		string filename_;

};

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _write_ class
 */
class serial_write_session : public serial_session {
public:
	serial_write_session(
			io_service& io_in, string log_in,	dispatcher* ref_in,	string device_in) :
				serial_session(io_in, log_in, ref_in, device_in)
	{
	}

	string get_type() { return string("serial-write"); }
};

/*=============================================================================
 * December 7, 2015 :: _write_pb_ class
 */

class serial_write_pb_session : public serial_write_session {
public:
	serial_write_pb_session(
			io_service& io_in, string log_in, dispatcher* ref_in, string device_in) :
				serial_write_session(io_in, log_in, ref_in, device_in)
	{
		this->start();
	}
	~serial_write_pb_session(){}

	void start();

private:
	void start_write();
	void handle_write(
			const boost::system::error_code& error, size_t bytes_transferred,
			bBuff* message);
	bBuff* generate_message();

	int internal_counter = 0;
};

/*-----------------------------------------------------------------------------
 * November 24, 2015 :: _write_nonsense_ class
 */
class serial_write_nonsense_session : public serial_write_session {
public:
	serial_write_nonsense_session(
			io_service& io_in, string log_in, dispatcher* ref_in, string device_in) :
				serial_write_session(io_in, log_in, ref_in, device_in), timer_(*io_ref)
	{
		this->start();
	}
	~serial_write_nonsense_session(){}

	void start();

private:
	void start_write();
	void handle_write(
			const boost::system::error_code& error, size_t bytes_transferred,
			std::size_t message_size, bBuff* nonsense);
	bBuff* generate_nonsense();
	bBuff generate_some_sense(bBuff payload);

  time_point<steady_clock> dead_ = steady_clock::now();
  basic_waitable_timer<steady_clock> timer_;

  int internal_counter = 0;
  long int bytes_sent = 0;
  long int bytes_intended = 0;

};

} // dew namespace

#endif /* SERIAL_SESSION_H_ */
