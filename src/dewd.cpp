/* The dewd server
 *
 *
 */

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <cstdio>

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "types.h"
#include "utils.h"

#include "session.hpp"
#include "serial_session.hpp"
#include "network_session.hpp"

namespace
{
	const size_t SUCCESS = 0;
//	const size_t ERROR_IN_COMMAND_LINE = 1;
	const size_t ERROR_UNHANDLED_EXCEPTION = 2;
} //namespace

using namespace dew;
namespace po = boost::program_options;

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::asio::ip::address_v4;

using ::std::string;
using ::std::vector;

using ::std::cout;
using ::std::cerr;
using ::std::to_string;
using ::std::endl;


void graceful_exit(const boost::system::error_code& error, int signal_number, dispatcher* dis)
{
	delete dis;
	exit(0);
}

int main(int argc, char** argv) {
	static_assert(BOOST_VERSION >= 104900,
			"asio waitable timer only supported in boost versions at least 1.49");

	try {
		string logging_directory;
		vector<string> rdev;
		vector<string> wdev;
		vector<string> addr;
		string conf;

		po::options_description modes("Mode options");
		modes.add_options()
				("test,t", "Start in testing mode.  No other arguments considered when"
						" starting in test mode.  Test mode configuration is pang-specific"
						" and current as of November 17, 2015.")
				("fp-comm,c", "Start the dewd in flopoint communication mode.  Need to"
						" specify serial ports to read and write to.")
				("fp-em,m", "Start dewd as a flo-point emulator rather than a"
						" communications server.  Not currently supported.")
				;
		po::options_description ifaces("Interface options");
		ifaces.add_options()
				("read-from,r", po::value<vector<string> >(&rdev)->multitoken(),
					"Specify serial ports to read from, where the input string must"
					" have no trailing '/'. Allows multiple entries. Ex. /dev/ttyS0")
				("write-to,w", po::value<vector<string> >(&wdev)->multitoken(),
					"Specify serial ports to write to, where the input string must"
					" have no trailing '/'. Allows multiple entries. Ex. /dev/ttyS0")
				("network,n",po::value<vector<string> >(&addr)->multitoken(),
					"Enable networking and specify local ip4 addresses to bind to. It"
					" is assumed that these ip addresses can be bound to by dewd." )
				("port-number,p",po::value<int>()->default_value(2023),
					"Give a port number to bind to.  Used with --network flag.  It is"
					" assumed that dewd can bind to the given port.");

		po::options_description general("General options");
		general.add_options()
				("help,h", "Print help messages.")
				("logging,l",
						po::value<string>(&logging_directory)->default_value("/tmp/dewd/"),
						"Specify the path to logging folder, where input string must have"
						" trailing '/'.  Default is /tmp/dewd/. Permissions are not"
						" checked before logging begins -- it is assumed that dewd can"
						" write to the given directory.")
			;

		po::options_description cmdline_options;
		cmdline_options.add(general).add(modes).add(ifaces);


		/* If you ask for help, you only get help.
		 *
		 * After that, the program exits.
		 *
		 * If there is an error in building the variables map, then we also need to
		 * exit.
		 */

		po::variables_map vmap;
		try {
			po::store(po::parse_command_line(argc,argv,cmdline_options), vmap);

			if(vmap.count("help")
					||
					!(vmap.count("test")||vmap.count("fp-comm")||vmap.count("fp-em"))) {
				cout << "The dewd DewDrop daemon.\n" << cmdline_options << '\n';

				return SUCCESS;
			}

			po::notify(vmap);
		}
		catch(po::error& poe) {

			cerr << "Exception:" << poe.what() << ".\nExiting.\n";
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

		boost::shared_ptr<io_service> service(new io_service);

		/* If a log-directory was specified, we set it now.  Otherwise, we use the
		 * default log-directory.
		 *
		 * If an explicit directory is passed to us, then we assume dewd may write
		 * to it.  This is the responsibility of whoever runs dewd.
		 *
		 *
		 */
		if(0)
			cout << "Logging to " << logging_directory << '\n';


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



		vector<tcp::endpoint> ends;

		if(vmap.count("test"))
		{
			std::cout << "Starting in test mode.\n";
			for(int i = 0; i < 13 ; ++i) {
				if(i < 9)
					ends.emplace_back(
							address_v4::from_string(
								"192.168.16." + to_string(i)), 2023);

				if(i > 4)
					rdev.emplace_back("/dev/ttyS" + to_string(i));
			}
		}
		else if (vmap.count("network")){
			for(vector<string>::iterator
					it = addr.begin() ;	it != addr.end() ; ++it) {
						ends.emplace_back(
								address_v4::from_string(*it), port_number);
			}
		}

		dispatcher* dis = new dispatcher;

		vector<basic_session*> sessions;


		for(vector<string>::iterator it = rdev.begin() ; it != rdev.end() ; ++it) {
			basic_session* session;
			if(vmap.count("test")) {
				session =	new serial_read_log_session(
						*service, logging_directory, dis, *it);
			}
			if(vmap.count("fp-comm")) {
				session = new serial_read_parse_session(
						*service, logging_directory, dis, *it);
			}
			sessions.push_back(session);
		}

		for(vector<string>::iterator it = wdev.begin() ; it != wdev.end() ; ++it) {
			basic_session* session;
			if(vmap.count("fp-comm")) {
				session = new serial_write_nonsense_session(
						*service,	logging_directory, dis, *it);
			}
			sessions.push_back(session);
		}

		if(vmap.count("test")) cout << "Accepting connections on:\n";
		for(vector<tcp::endpoint>::iterator it = ends.begin() ;	it != ends.end() ; ++it) {
			network_acceptor_session* session =
					new network_acceptor_session(*service, logging_directory, dis, *it);
			sessions.push_back(session);
		}

		/*
		 * Set signals to catch for graceful termination.
		 */

		boost::asio::signal_set signals(*service, SIGINT, SIGTERM);
		signals.async_wait(boost::bind(&graceful_exit,_1,_2, dis));

		/*
		 * io_service.run() will run the io_service until there are no jobs or han-
		 * dlers left to be invoked.  Because the handlers as written invoke new
		 * work, run() should never terminate.
		 *
		 */

		service->run();



	} catch (std::exception& e) {
		cerr << e.what() << endl;
	}
	return 0;
}

