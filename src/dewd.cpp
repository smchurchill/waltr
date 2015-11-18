/* The dewd server
 *
 *
 */

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <boost/version.hpp>
#include <boost/asio.hpp>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#define PORT_NUMBER 2023

namespace
{
	const size_t SUCCESS = 0;
	const size_t ERROR_IN_COMMAND_LINE = 1;
	const size_t ERROR_UNHANDLED_EXCEPTION = 2;
} //namespace

class serial_read_session;
class serial_write_session;
class fp_em_session;

/* November 17, 2015
 *
 *
 *
 */

template<typename handler_wrapper, typename initializer_list >
std::vector<handler_wrapper> make_asio_work(boost::asio::io_service io_service,
		initializer_list i ){

	std::vector<handler_wrapper> v = new std::vector<handler_wrapper>;

	for(initializer_list::iterator it = i.begin() ; it != i.end() ; ++it )
		v->emplace_back(handler_wrapper(io_service, *it));

	return v;
}


int main(int argc, char** argv) {
	static_assert(BOOST_VERSION >= 104900,
			"asio waitable timer only supported in boost versions at least 1.49");

	try {
		po::options_description desc("Options");
		desc.add_options()
				("help,h", "Print help messages.")
				("test,t", "Start in testing mode.  No other arguments considered when"
						" starting in test mode.  Test mode configuration is pang-specific"
						" and current as of November 17, 2015.")
				("logging,l", po::value<std::string>()->default_value("/tmp/dewd/"),
						"Specify the path to logging folder.  Default is /tmp/dewd/."
						" Permissions are not checked before logging begins -- it is"
						" assumed that dewd can write to the given directory.")
				("read-from,r", po::value<std::vector<std::string>>(),
						"Specify serial ports to read from. "
						"Allows multiple entries. Ex. /dev/ttyS0 /dev/ttyS1")
				("write-to,w", po::value<std::vector<std::string>>(),
						"Specify serial ports to write to. "
						"Allows multiple entries. Ex. /dev/ttyS0 /dev/ttyS1")
				("network,n",po::value<std::vector<std::string>>(),
						"Enable networking and specify local ip4 addresses to bind to." )
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

		po::variables_map vmap;
		try {
			po::store(po::parse_command_line(argc,argv,desc), vmap);

			if(vmap.count("help")) {
				std::cout << "The dewd DewDrop daemon.\n" << desc << '\n';

				return SUCCESS;
			}

			po::notify(vmap);

		}
		catch(po::error& poe) {

			std::cerr << "Exception:" << poe.what() << ".\nExiting.\n";
			return ERROR_UNHANDLED_EXCEPTION;
		}

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

		std::cout << "Logging to " << vmap["logging"].as<std::string>() << '\n';


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

		std::vector<std::string> rdev;
		std::vector<std::string> wdev;
		std::vector<std::string> ip;

		if(vmap.count("test")||(!vmap.count("read-from")&&!vmap.count("write-to")
				&&!vmap.count("network")))
		{
			std::cout << "Starting in test mode.\n";
			for(int i = 0; i < 13 ; ++i) {
				if(i < 9)
					ip.push_back("192.168.16." + std::to_string(i));
				if(i > 4)
					rdev.push_back("/dev/ttyS" + std::to_string(i));
			}
		}
		else {
			rdev = vmap["read-from"].as<std::vector<std::string> >();
			wdev = vmap["write-to"].as<std::vector<std::string> >();
			ip = vmap["network"].as<std::vector<std::string> >();
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
		for(std::vector<std::string>::iterator it = ip.begin() ;
				it != ip.end() ; ++it)
			std::cout << (*it) << '\n';

		if(vmap.count("fp-em")){
			make_asio_work<fp_em_session>(*io_service,fp_em_list);
		}
		else {
			make_asio_work<serial_read_session>
			make_asio_work<network_acceptor>
		}





		/*
		 * Instantiate the serial port sessions
		 *
		if(0){
		std::vector<serial_read_session*> vec_srs;

		for (int i = 2; i < argc; ++i) {
			serial_read_session* s = new serial_read_session(io_service, argv[i], argv[1]);
			vec_srs.push_back(s);
		}

		/* November 13, 2015
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
