/*
 * serial_session.hpp
 *
 *  Created on: Nov 18, 2015
 *      Author: schurchill
 */

#ifndef SESSION_HPP_
#define SESSION_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/duration.hpp>

class dispatcher;

/*-----------------------------------------------------------------------------
 * November 18, 2015
 *
 * The initialization of the session classes in dewd.cpp requires that a class
 * has a constructor with signature:

		handler_wrapper(boost::asio::io_service& io_service,
			initializer_definer def,
			std::string logging_directory
			)

 *
 * The function that initializes all sessions also calls the public class
 * function

 	 	 void start()

 * which should cause the instance of handler_wrapper to make some work for the
 * boost::asio::io_service object passed to it.
 */


/*-----------------------------------------------------------------------------
 * November 19, 2015
 *
 * base class io_session
 *
 * The new model for the session classes found in previous versions of dewd is
 * implemented using inheritance.  There is a base class io_session that states
 * the structure needed for initialization.
 *
 * An io_session class constructor takes as input the io_service the session
 * posts work to, a unique identifier such as a tcp endpoint or serial port,
 * a directory to be logged to, and a handler to call when work is completed.
 *
 * Further, the io_session class has a
 	 	 void start()
 * function that must be called in order for the instantiated object to begin
 * work.  A derived class defines their own start() function.
 *
 * Every instance of an io_session can be connected to a central dispatcher.
 */

template< typename session_type, typename unique_identifier >
class io_session{
public:
	io_session(boost::asio::io_service& io_in, unique_identifier uid_in,
			std::string log_in) :
				comm_(io_in, uid_in), logdir_(log_in)
	{
		start_ = boost::chrono::steady_clock::now();
	}

	void start();

protected:
	friend class dispatcher;
	dispatcher* ref = NULL;
	void setRef(dispatcher* toSet) {
		ref = toSet;
	}

	session_type& get_comm() {
		return &comm_;
	}

	session_type comm_;
	std::string logdir_;

  boost::chrono::time_point<boost::chrono::steady_clock> start_;
};

/*-----------------------------------------------------------------------------
 * November 18, 2015
 * class serial_read_session
 * The read and write sessions of the twnn server are separated because we have
 * a timeout implemented on reads.  We would like to rapidly create a trusted
 * communications log for our commands rather than fill our buffer each time.
 * The hope is that the integrity of the communications will be preserved by
 * this method.
 *
 * On the other hand, we would like to ensure that our whole buffer is clear
 * when we write to avoid half commands getting through to a flopoint board.
 * The boost::asio::serial_port.cancel() function cancels all read and write
 * operations in progress when it is called, so our specifications do require
 * the use of two separate boost::asio::serial_port instances: one for reading
 * and one for writing.
 *
 */

class serial_read_session :
		public io_session<boost::asio::serial_port,std::string> {
public:
	serial_read_session(boost::asio::io_service& io_in, std::string device_in,
			std::string log_in) :
		io_session(io_in, device_in, log_in), //init the standard io_session stuff
		name_(device_in), 										//init the serial_read specific stuff
		timer_(io_in)
	{
		filename_ = logdir_ + name_.substr(name_.find_last_of("/\\")+1);
		dead_ = start_ + timeout_;
	}

	void start() {
		start_read();
		set_timer();
	}

private:

	/* For now, this function begins the read/write loop with a call to
	 * port_.async_read_some with handler handle_read.  It makes a new vector,
	 * where the promise to free the memory is honored at the end of handle_read.
	 *
	 * Later, it will take care of making sure that buffer_, port_, and file_ are
	 * sane.  Other specifications are tbd.
	 */
	void start_read() {
		std::vector<char>* buffer_ = new std::vector<char>;
		buffer_->assign (BUFFER_LENGTH,0);
		boost::asio::mutable_buffers_1 Buffer_ = boost::asio::buffer(*buffer_);
		boost::asio::async_read(comm_,Buffer_,
				boost::bind(&serial_read_session::handle_read, this, _1, _2, buffer_));
	}

	/* This handler first gives more work to the io_service.  The io_service will
	 * terminate if it is ever not executing work or handlers, so passing more
	 * work to the service at the start of this handler will reduce the possible
	 * situations where that could happen.
	 *
	 * If $length > 0 then there are characters to write, and we write $length
	 * characters from buffer_ to file_.  Otherwise, $length=0 and there are no
	 * characters ready to write in buffer_.  We increase the count since last
	 * write.  If a nonzero number of characters are written then that count is
	 * set to 0.
	 *
	 *
	 */
	void handle_read(boost::system::error_code ec, size_t length,
		std::vector<char>* buffer_) {
		/* The first thing we do is give more work to the io_service.  Allocate a
		 * new buffer to pass to our next read operation and for our next handler
		 * to read from.  That is all done in start_read().
		 */
		this->start_read();

		if (length > 0) {
			FILE* file_ = fopen(filename_.c_str(), "a");
			std::fwrite(buffer_->data(), sizeof(char), length, file_);
			fclose(file_);
		}

		delete buffer_;

		/* The following is for debugging purposes only */
		if(0) {
			std::cout << "[" << name_ << "]: " << length << " characters written\n";
		}
	}

  /* This timeout handling is based on the blog post located at:
   *
   * blog.think-async.com/2010/04/timeouts-by-analogy.html
   *
   * The basic idea is that there are two pieces of async work in handled by a
   * serial_read_session: a read and a wait.
   *
   * The async_wait runs the handle_timout handler either when time expires or
   * when the wait operation is canceled.  Nothing will cancel the wait in this
   * program at the moment.
   *
   * The async_read reads characters from the port_ buffer into this session's
   * buffer_ container.  The async_read will exit and run read_handler when
   * either buffer_ is full or timer_ expires and invokes handle_timeout, which
   * cancels all current work operations in progress through port_.
   */
	void set_timer() {
		boost::chrono::time_point<boost::chrono::steady_clock> now_ =
			boost::chrono::steady_clock::now();

		while(dead_< now_)
			dead_ = dead_ + timeout_;

		timer_.expires_at(dead_);
		timer_.async_wait(
    	boost::bind(&serial_read_session::handle_timeout, this, _1));

		/* The following is for debugging purposes only */
		if(0) {
			std::cout << "[" << name_ << "]: deadline set to " << dead_ << '\n';
		}
	}

  void handle_timeout(boost::system::error_code ec) {
  	if(!ec)
  		comm_.cancel();

  	set_timer();
  }

	std::string name_;
	std::string filename_;

	/* November 18, 2015
	 * AJS says that pang has a 4k kernel buffer.  We want our buffer to be
	 * bigger than that, and we can be greedy with what "bigger" means.
	 */
	const long BUFFER_LENGTH = 8192;

	/* Timer specific members */

	/* November 18, 2015
	 * This value gives us a polling rate of 10Hz.  This is the only place where
	 * this polling rate is set.
	 */
	boost::chrono::milliseconds timeout_ = boost::chrono::milliseconds(100);

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

  boost::asio::basic_waitable_timer<boost::chrono::steady_clock> timer_;
  boost::chrono::time_point<boost::chrono::steady_clock> dead_;

};

/*-----------------------------------------------------------------------------
 * November 18, 2015
 * class serial_write_session
 *
 * We plan to write commands to the serial port that are dictated by commands
 * read from a network socket.
 *
 * Depends:
 * 	- network_session
 * 	- dispatch
 */

class serial_write_session :
	public io_session<boost::asio::serial_port,std::string>{
public:
	serial_write_session(boost::asio::io_service& io_in, std::string device_in,
			std::string log_in) :
				io_session(io_in, device_in, log_in),
				name_(device_in)
		{
			filename_ = logdir_ + name_.substr(name_.find_last_of("/\\"));
			std::cout << "Serial write session ready on port located at " << name_ << '\n';
		}

	/*
	 */
	void start() {
	}

private:
	/*
	 *
	 */

	std::string name_;
	std::string filename_;
};





/*-----------------------------------------------------------------------------
 * November 19, 2015
 * class network_socket_session
 */

class network_socket_session :
		public io_session<boost::asio::ip::tcp::socket,
											boost::asio::ip::tcp::endpoint> {
public:
	network_socket_session(boost::asio::io_service& io_in,
			boost::asio::ip::tcp::endpoint ep_in, std::string log_in ) :
			io_session(io_in, ep_in, log_in)
	{
	}

	void start() {
		do_read();
	}



private:
	friend class network_acceptor_session;

	/*
	 * November 16, 2015
	 * Simple echoing protocol modeled on the tcp echo server example in boost
	 * asio library.
	 */
	void do_read() {
		comm_.async_read_some(boost::asio::buffer(data_),
				[this](boost::system::error_code ec, std::size_t length)
				{
					if(!ec)
						do_write(length);
					else {
						std::cerr << ec << '\n';
						delete this;
					}
				});
	}

	void do_write(std::size_t length)
	{
		boost::asio::async_write(comm_,boost::asio::buffer(data_,length),
				[this](boost::system::error_code ec, std::size_t)
				{
					do_read();
				});
	}

	/* November 18, 2015
	 * The buffer for this class probably doesn't need to be this large.
	 */
	const long BUFFER_LENGTH = 8192;

	std::vector<char> data_ = std::vector<char>(BUFFER_LENGTH,0);
};

/*-----------------------------------------------------------------------------
 * November 19, 2015
 * class network_acceptor_session
 *
 *	The following may be deprecated.
 *
 * The acceptor is initialized with a port to bind to and an io_service to ass-
 * -ign work to.  It sits on an acceptor and a socket, waiting for someone to
 * try to connect.  When a connection is accepted it is assigned to socket_,
 * and a new shared_ptr to socket_ is created and passed to a new network_sess-
 * -ion which will communicate through socket_. The server then continues to
 * sit on socket_ and acceptor_ and waits for a new connection.
 *
 * The intent is to have an acceptor listening on port 50001 over each of the
 * ip addresses on pang that are assigned to flopoint.  As of writing, those
 * are 192.168.16.X for X in [0:8].
 *
 */

class network_acceptor_session :
		public io_session<boost::asio::ip::tcp::acceptor,
											boost::asio::ip::tcp::endpoint> {
public:
	network_acceptor_session(boost::asio::io_service& io_in,
			boost::asio::ip::tcp::endpoint ep_in, std::string log_in ) :
				io_session(io_in, ep_in, log_in) {
	}

	void start() {
		do_accept();
	}

private:
	void do_accept() {
		network_socket_session* sock_ =	new network_socket_session(
				comm_.get_io_service(), comm_.local_endpoint(), logdir_);

		comm_.async_accept(sock_->get_comm(),
				[this,sock_](boost::system::error_code ec)
				{
					if(!ec) {
						sock_->start();
					}
					this->start();
				});
	}
};


/*-----------------------------------------------------------------------------
 * November 10, 2015
 * class dispatcher
 *
 * A single dispatch object should be instantiated in main, before any other
 * object is created.  Other class constructors will be overloaded to accept a
 * pointer to a dispatch object.  That dispatch object will be able to route
 * communications between our other objects.  The desired functionality is des-
 * cribed by the following example:
 *
 * 0. Server twnn is launched on pang, listening on port 50001.
 * 			One dispatch object dispatch_, one server object server_, and ~enough~
 * 			serial_read_session and serial_write_session objects srs_i and sws_i
 * 			are instantiated.
 * 1. Client zabbix connects to pang on port 50001.
 * 2. server_ instantiates a network_session net_j to talk to zabbix.
 * 3.	zabbix sends a command to pang, asking for the GUID of the flopoint board
 * 			connected to serial port i.
 * 4. net_j forwards the command to dispatch which forwards the command to sws_i
 * 5. sws_i asks its board for a GUID.
 * 6. srs_i read a GUID, and sends that to dispatch.
 * 7. dispatch_ heard about a GUID from srs_i.  net_j asked for the GUID of who
 * 			is attached to serial port i, so dispatch sends the GUID to net_j
 * 8. net_j forwards the GUID to zabbix.
 *
 * This level of translation is needed because the commands being issued to and
 * from the flopoint board are intended to be bjson, while the communication
 * across the network is intended to be json.  The different wrappers should be
 * simple, but necessary.
 *
 */

class dispatcher {

private:
	template<typename session_type, typename unique_identifier>
	std::vector<io_session<session_type, unique_identifier>*>* friends;
	boost::asio::io_service* io_service;

	void forward() {


	}

public:
	template<typename session_type, typename unique_identifier>
	dispatcher(boost::asio::io_service& io_in,
		std::vector<io_session<session_type, unique_identifier>*> session_in ) {
		friends = session_in;
		io_service = &io_in;
	}

	template<typename session_type, typename unique_identifier>
	void initialize(){
		for(std::vector<io_session<session_type, unique_identifier>*>::iterator it = friends->begin() ;
				it != friends->end() ; ++it)
			(**it).setRef(this);
	}



};


#endif /* SESSION_HPP_ */
