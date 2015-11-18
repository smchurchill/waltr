/* The dewd server
 *
 *
 */

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>

#include <boost/version.hpp>
#include <boost/asio.hpp>

#include <boost/program_options.hpp>

/* For std::make_shared and std::enable_shared_from_this */
#include <memory>

/* For std::move */
#include <utility>


#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/asio/steady_timer.hpp>
#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/duration.hpp>

#include <session.hpp>

namespace
{
	const size_t SUCCESS = 0;
	const size_t ERROR_IN_COMMAND_LINE = 1;
	const size_t ERROR_UNHANDLED_EXCEPTION = 2;
} //namespace

class serial_read_session;
class serial_write_session;
class fp_em_session;
class network_acceptor;

/* November 17, 2015
 *
 *
 *
 */

template<typename handler_wrapper, typename initializer_list >
std::vector<handler_wrapper*> make_asio_work(
		boost::asio::io_service& io_service, initializer_list i,
		std::string logging_directory ){

	std::vector<handler_wrapper*>* v = new std::vector<handler_wrapper*>;

	for(typename initializer_list::iterator it = i.begin() ;
			it != i.end() ; ++it ) {
		handler_wrapper* h = new handler_wrapper(
				io_service, *it, logging_directory);
		v->push_back(h);
	}

	for(typename std::vector<handler_wrapper*>::iterator it = v->begin() ;
			it != v->end() ; ++it ) {
		(**it).start();
	}

	return *v;
}

void graceful_exit(const boost::system::error_code& error, int signal_number)
{
	exit(1);
}

int main(int argc, char** argv) {
	static_assert(BOOST_VERSION >= 104900,
			"asio waitable timer only supported in boost versions at least 1.49");

	try {
		std::string logging_directory;
		std::vector<std::string> rdev;
		std::vector<std::string> wdev;
		std::vector<std::string> addr;

		boost::program_options::options_description desc("Options");
		desc.add_options()
				("help,h", "Print help messages.")
				("test,t", "Start in testing mode.  No other arguments considered when"
						" starting in test mode.  Test mode configuration is pang-specific"
						" and current as of November 17, 2015.")
				("logging,l",boost::program_options::value<std::string>(
						&logging_directory)->default_value("/tmp/dewd/"),
						"Specify the path to logging folder.  Default is /tmp/dewd/."
						" Permissions are not checked before logging begins -- it is"
						" assumed that dewd can write to the given directory.")
				("read-from,r", boost::program_options::value<
						std::vector<std::string> >(&rdev),
						"Specify serial ports to read from. "
						"Allows multiple entries. Ex. /dev/ttyS0 /dev/ttyS1")
				("write-to,w", boost::program_options::value<
						std::vector<std::string> >(&wdev),
						"Specify serial ports to write to. "
						"Allows multiple entries. Ex. /dev/ttyS0 /dev/ttyS1")
				("network,n",boost::program_options::value<
						std::vector<std::string> >(&addr),
						"Enable networking and specify local ip4 addresses to bind to."
						" It is assumed that these ip addresses can be bound to by dewd." )
				("port-number,p",boost::program_options::value<
						int>()->default_value(2023),
						"Give a port number to bind to.  Used with --network flag.  It is"
						" assumed that dewd can bind to the given port.")
				("fp-em,f", "Start dewd as a flo-point emulator rather than a"
						" communications server.  Not currently supported.")
				;

		/* If you ask for help, you only get help.
		 *
		 * After that, the program exits.
		 *
		 * If there is an error in building the variables map, then we also need to
		 * exit.
		 */

		boost::program_options::variables_map vmap;
		try {
			boost::program_options::store(boost::program_options::parse_command_line(
					argc,argv,desc), vmap);

			if(vmap.count("help")) {
				std::cout << "The dewd DewDrop daemon.\n" << desc << '\n';

				return SUCCESS;
			}

			boost::program_options::notify(vmap);
		}
		catch(boost::program_options::error& poe) {

			std::cerr << "Exception:" << poe.what() << ".\nExiting.\n";
			return ERROR_UNHANDLED_EXCEPTION;
		}

		/* The first thing we'll do is set a port number for network communications
		 *
		 * Even if we don't end up communicating on the network we need to set this
		 * to a const int ASAP.
		 */
		const int port_number = vmap["port-number"].as<int>();

		/* Everything else we're doing requires an asio io_service, so we
		 * initialize that now.
		 *
		 *
		 */

		boost::shared_ptr<boost::asio::io_service> io_service(
				new boost::asio::io_service);

		/* If a log-directory was specified, we set it now.  Otherwise, we use the
		 * default log-directory.
		 *
		 * If an explicit directory is passed to us, then we assume dewd may write
		 * to it.  This is the responsibility of whoever runs dewd.
		 *
		 *
		 */

		std::cout << "Logging to " << logging_directory << '\n';


		/* If dewd is explicitly run in test mode, then we use our test devs of
		 * ttyS[5..12] and test ips of 192.168.16.[0..8].
		 *
		 * If no devs -or- ips are given, then we implicitly start in test mode.
		 * This also covers the case of dewd being run with no options.
		 *
		 * If devs and/or ips are given, then we assume that those are the only
		 * objects we want to consider.  The vectors devs and ips will be assumed
		 * to hold all serial ports/ip addresses we want to communicate through in
		 * the future.
		 */

		std::vector<boost::asio::ip::tcp::endpoint> comm;

		if(vmap.count("test")||(!vmap.count("read-from")&&!vmap.count("write-to")
				&&!vmap.count("network")))
		{
			std::cout << "Starting in test mode.\n";
			for(int i = 0; i < 13 ; ++i) {
				if(i < 9)
					comm.emplace_back(
						boost::asio::ip::address_v4::from_string(
							"192.168.16." + std::to_string(i)), 2023
					);
				if(i > 4)
					rdev.emplace_back("/dev/ttyS" + std::to_string(i));
			}
		}
		else {
			for(std::vector<std::string>::iterator it = addr.begin() ;
					it != addr.end() ; ++it)
						comm.emplace_back(
							boost::asio::ip::address_v4::from_string(*it), port_number );
		}

		std::cout << "Reading from ports:\n";
		for(std::vector<std::string>::iterator it = rdev.begin() ;
				it != rdev.end() ; ++it)
			std::cout << (*it) << '\n';

		std::cout << "Writing to ports:\n";
		for(std::vector<std::string>::iterator it = wdev.begin() ;
				it != wdev.end() ; ++it)
			std::cout << (*it) << '\n';

		std::cout << "Binding to ips:\n";
		for(std::vector<boost::asio::ip::tcp::endpoint>::iterator it = comm.begin()
				;	it != comm.end() ; ++it)
			std::cout << (*it) << '\n';

		std::vector<serial_read_session*> vec_srs =
				make_asio_work<serial_read_session, std::vector<std::string> >(
						*io_service, rdev, logging_directory);
		std::vector<serial_write_session*> vec_sws =
				make_asio_work<serial_write_session, std::vector<std::string> >(
						*io_service, wdev, logging_directory);
		std::vector<network_acceptor*> vec_nas =
				make_asio_work<network_acceptor, std::vector<
					boost::asio::ip::tcp::endpoint> >(
						*io_service, comm, logging_directory);





		/*
		 * Instantiate the serial port sessions
		 *
		if(0){
		std::vector<serial_read_session*> vec_srs;

		for (int i = 2; i < argc; ++i) {
			serial_read_session* s = new serial_read_session(io_service, argv[i], argv[1]);
			vec_srs.push_back(s);
		}

		* November 13, 2015
		 * Instantiate the network acceptors.  As of writing, the ip addresses on
		 * pang assigned to flopoint are 192.168.16.X, for X in [0:8].
		 *
		 * These values should later be part of startup discovery processes instead
		 * of being explicitly stated.
		 *




		std::vector<network_acceptor*> vec_na;
		for (std::vector<std::string>::iterator it = ips.begin() ;
				it != ips.end() ; ++it ) {
			boost::asio::ip::tcp::endpoint ep (
					boost::asio::ip::address_v4::from_string(*it), PORT_NUMBER);
			std::cout << ep << '\n';
			network_acceptor* a = new network_acceptor(io_service, ep);
			vec_na.push_back(a);
		}

		*/

		/*
		 * Set signals to catch for graceful termination.
		 */

		boost::asio::signal_set signals(*io_service, SIGINT, SIGTERM);
		signals.async_wait(&graceful_exit);

		/*
		 * io_service.run() will run the io_service until there are no jobs or han-
		 * dlers left to be invoked.  Because the handlers as written invoke new
		 * work, run() should never terminate.
		 *
		 */

		io_service->run();



	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
