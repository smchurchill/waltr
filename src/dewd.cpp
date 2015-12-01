/* The dewd server
 *
 *
 */

#define AJS_HACK

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <cstdio>
#include <functional>

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
				("sp-comm,S", "Start the dewd in serial port communication mode.  Need to"
						" specify serial ports to read and write to.")
				("net-comm,N", "Start the dewd in network communication mode.  Need to"
						" specify ip-addresses to bind to.")
				("fp-em,M", "Start dewd as a flo-point emulator rather than a"
						" communications server.  Not currently supported.")
				;
		po::options_description ifaces("Interface options");
		ifaces.add_options()
				("read-from,R", po::value<vector<string> >(&rdev)->multitoken(),
					"Specify serial ports to read from, where the input string must"
					" have no trailing '/'. Allows multiple entries. Ex. /dev/ttyS0")
				("write-to,W", po::value<vector<string> >(&wdev)->multitoken(),
					"Specify serial ports to write to, where the input string must"
					" have no trailing '/'. Allows multiple entries. Ex. /dev/ttyS0")
				("network,I",po::value<vector<string> >(&addr)->multitoken(),
					"Specify local ip4 addresses to bind to. It is assumed that these ip"
					" addresses can be bound to by dewd." )
				("port-number,P",po::value<int>()->default_value(2023),
					"Give a port number to bind to.  Used with --network flag.  It is"
					" assumed that dewd can bind to the given port.")
				;

		po::options_description testing("Test options");
		testing.add_options()
				("test,t", "Starting all test interfaces.  These interfaces are"
						" pang-specific and current as of November 27, 2015.  We use"
						" serial ports /dev/ttyS{5..12} in RW mode and tcp endpoints of"
						" 192.168.16.{0..8}:2023.  No other test options or interface"
						" options should be specified with --test.  --test is equivalent"
						" to -rwn.")
				("read-test,r", "Start serial ports as specified in --test in read"
						" mode.")
				("write-test,w", "Start serial ports as specified in --test in write"
						" mode.")
				("network-test,i", "Bind tcp endpoints specified in --test.")
				;

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
		cmdline_options.add(modes).add(ifaces).add(testing).add(general);


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

			if(vmap.count("help") ||
				 !(vmap.count("test")||
					 vmap.count("sp-comm")||
					 vmap.count("fp-em")||
					 vmap.count("net-comm"))) {
				cout << "The dewd DewDrop daemon.\n" << cmdline_options << '\n';

				return SUCCESS;
			}

			po::notify(vmap);
		}
		catch(po::error& poe) {

			cerr << "Exception:" << poe.what() << ".\nExiting.\n";
			return ERROR_UNHANDLED_EXCEPTION;
		}

		assert(vmap.count("test")||vmap.count("sp-comm")||vmap.count("fp-em")||vmap.count("net-comm"));


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

		dispatcher* dis = new dispatcher;
		vector<tcp::endpoint> ends;

		if(vmap.count("test")||vmap.count("read-test"))
			set_default_ports(&rdev);

		if(vmap.count("test")||vmap.count("write-test"))
			set_default_ports(&wdev);

		if(vmap.count("test")||vmap.count("network-test"))
			set_default_endpoints(&ends, port_number);
		else if (vmap.count("network"))
			set_endpoints(&ends,&addr,port_number);


		if(vmap.count("sp-comm")){
			for(auto it : rdev)
				new serial_read_parse_session(*service, logging_directory, dis, it);
			for(auto it : wdev)
				new serial_write_nonsense_session(*service,	logging_directory, dis, it);
		}

		if(vmap.count("net-comm")) {
			dis->init_net_cmd();
			for(auto it : ends)
				new network_acceptor_session(*service, logging_directory, dis, it);
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

