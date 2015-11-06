/* The twnn server
 *
 *
 */

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>

/* For std::make_shared and std::enable_shared_from_this */
#include <memory>

/* For std::move */
#include <utility>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/chrono.hpp>

#define TIMEOUT 100
#define BUFFER_LENGTH 8192

/*-----------------------------------------------------------------------------
 * class serial_read_session
 */

class serial_read_session {
public:
	serial_read_session(boost::asio::io_service& io_service, std::string location, std::string DATAPATH) :
			port_(io_service), timer_(io_service), name_(location) {
		filename_ = DATAPATH + name_.substr(name_.find_last_of("/\\") + 1);
		port_.open(location);
		start();
		std::cout << "[" << name_ << "]: session started" << std::endl;
	}

	/* For now, this function simply begins the read/write loop with a call to
	 * port_.async_read_some with handler handle_read.
	 *
	 * Later, it will take care of starting the timer, making sure that buffer_,
	 * port_, and file_ are sane, etc.
	 */
	void start() {
		timer_.expires_from_now(boost::posix_time::milliseconds(TIMEOUT));
		boost::asio::async_read(port_, boost::asio::buffer(buffer_, BUFFER_LENGTH),
				boost::bind(&serial_read_session::handle_read, this, _1, _2));
		timer_.async_wait(
				boost::bind(&serial_read_session::handle_timeout, this, _1));
	}

private:
	/* This handler first gives more work to the io_service.  The io_service will
	 * terminate if it is ever not executing work or handlers, so passing more
	 * work to the service at the start of this handler will writes $length char-
	 * acters from buffer_ to file_.  If no characters are ready to write in buf-
	 * fer, then the count since last write is increased.  If a nonzero number of
	 * characters are written then that count is set to 0.
	 *
	 * If the handler is invoked with an operation_aborted error, that means we
	 * canceled an async_read operation manually via a timeout.  In this case we
	 * increment count_timed_out. Otherwise we increment count_buff_full.  These
	 * statistics may be used to modify timeouts in the future.
	 *
	 */
	void handle_read(boost::system::error_code ec, size_t length) {
		/* The first thing we do is give more work to the io_service.
		 */
		boost::asio::async_read(port_, boost::asio::buffer(buffer_, BUFFER_LENGTH),
				boost::bind(&serial_read_session::handle_read, this, _1, _2));

		if (length > 0) {
			file_ = fopen(filename_.c_str(), "a");
			fwrite(&buffer_, sizeof(char), length, file_);
			fclose(file_);
			std::cout << "[" << name_ << "]" << length << " chars written" << std::endl;
			count_since_last_write = 0;
		} else
			++count_since_last_write;

		if (ec) {
			++count_timed_out;
//			std::cout << "[" << name_ << "]" << count_timed_out/10 << " times timed out" << std::endl;
		} else
			++count_buff_full;

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
	 *
	 */
	void handle_timeout(boost::system::error_code ec) {
		if (timer_.expires_from_now() < boost::posix_time::seconds(0)) {
			timer_.expires_from_now(boost::posix_time::milliseconds(TIMEOUT));
			timer_.async_wait(bind(&serial_read_session::handle_timeout, this, _1));
			port_.cancel();
			if(hurts)
				--hurts;
			else
			{
				++count_seconds;
				hurts=1000/TIMEOUT;
				std::cout << "[" << name_ << "]" << count_seconds << " seconds elapsed" << std::endl;
			}
		} else {
			timer_.async_wait(
					boost::bind(&serial_read_session::handle_timeout, this, _1));
		}

	}

	boost::asio::serial_port port_;
	boost::asio::deadline_timer timer_;
	std::string name_;
	std::string filename_;
	FILE* file_;
	char buffer_[BUFFER_LENGTH];
	int count_since_last_write = 0;
	int count_timed_out = 0;
	int count_buff_full = 0;
	int hurts = 1000/TIMEOUT;
	int count_seconds;
};

/*-----------------------------------------------------------------------------
 * class network_session
 */

class network_session: public std::enable_shared_from_this<network_session> {
public:
	network_session(boost::asio::ip::tcp::socket sock) :
			socket_(std::move(sock)) {
	}

	void start() {
		socket_.async_read_some(boost::asio::buffer(buffer_, BUFFER_LENGTH),
				boost::bind(&network_session::handle_read, this, _1));
	}

	/* haven't decided what we want to do with what we've read yet */
	void handle_read(boost::system::error_code ec) {

	}

private:

	char buffer_[BUFFER_LENGTH];
	boost::asio::ip::tcp::socket socket_;
};

/*-----------------------------------------------------------------------------
 * class network_server
 *
 * The server is initialized with a port to bind to and an io_service to assign
 * work to.  It sits on an acceptor and a socket, waiting for someone to try to
 * connect.  When a connection is accepted it is assigned to socket_, and a new
 * shared_ptr to socket_ is created and passed to a new network_session which
 * will use communicate through socket_. The server then continues to sit on
 * socket_ and acceptor_ and await a new connection.
 *
 */

class network_server {
public:
	network_server(boost::asio::io_service& io_service, int port) :
			acceptor_(io_service,
					boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)), socket_(
					io_service) {
		do_accept();
	}

private:
	void do_accept() {
		acceptor_.async_accept(socket_, [this](boost::system::error_code ec)
		{
			if(!ec) {
				std::make_shared<network_session>(std::move(socket_))->start();
			}
			do_accept();
		});
	}

	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::ip::tcp::socket socket_;
};

/*-----------------------------------------------------------------------------
 * class serial_write_session
 *
 * We plan to write commands to the serial port that are dictated by commands
 * read from a network socket.
 *
 * Depends:
 * 	- network_session
 */

class serial_write_session {
public:
	serial_write_session();

	/*
	 */
	void start() {
	}

private:
	/*
	 *
	 */

};

/*-----------------------------------------------------------------------------
 * twnn accepts as arguments:
 * argv[1] is the path to a logging folder i.e. /var/log/twnn
 * argv[i] for i in [2,argc-1] are device names i.e. /dev/ttyS5
 */


int main(int argc, char* argv[]) {
	try {
		if (argc < 2) {
			std::cerr << "No serial port names supplied\n";
			return 1;
		}

		boost::asio::io_service io_service;

		/*
		 * Instantiate the serial port sessions
		 */

		std::vector<serial_read_session*> vec;
		for (int i = 2; i < argc; ++i) {
			serial_read_session* s = new serial_read_session(io_service, argv[i], argv[1]);
			vec.push_back(s);
		}

		/*
		 * io_service.run() will run the io_service until there are no jobs or han-
		 * dlers left to be invoked.  Because the handlers as written invoke new
		 * work, run() should never terminate.
		 *
		 */
		io_service.run();

	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
