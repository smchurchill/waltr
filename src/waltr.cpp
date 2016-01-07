/* The waltr subscriber client
 *
 * waltr is a companion of the dewd.  waltr subscribes to the dewd's channels,
 * logs pb messages in a raw format, and cleans up after itself.
 */

#define AJS_HACK
#define BOOST_CHRONO_DONT_PROVIDES_DEPRECATED_IO_SINCE_V2_0_0

#include <ctime>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <cstdio>
#include <functional>
#include <memory>

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "structs.h"
#include "types.h"
#include "utils.h"

#include "network_session.hpp"
#include "session.hpp"



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

using ::boost::chrono::steady_clock;
using ::boost::chrono::time_point;
using ::boost::chrono::nanoseconds;

using ::std::string;
using ::std::vector;

using ::std::cout;
using ::std::cerr;
using ::std::to_string;
using ::std::endl;

using ::std::ifstream;
using ::std::shared_ptr;


int main(int argc, char** argv) {
	static_assert(BOOST_VERSION >= 104900,
			"asio waitable timer only supported in boost versions at least 1.49");

	try {
		string dewd;
		vector<string> chan;

		string logg;
		string conf;


		po::options_description ifaces("Interface options");
		ifaces.add_options()
			("dewd",
				po::value< string >(&dewd)->default_value("192.168.16.0:2023"),
				"Specify remote ip4_address:port location of the dewd." )
			("subscribe-to, s",
				po::value< vector<string> >(&chan)->multitoken(),
				"Specify channels to subscribe to.  Communications over a channel"
				" are unidirectional from the dewd, and logged in"
				" ($LOGDIR)/channel/y/m/d/time")
			;
		po::options_description general("General options");
		general.add_options()
			("help,h",
				"Print help messages.")
			("logging,l",
				po::value<string>(&logg)->default_value(
				"/usr/local/etc/waltr/logs/"),
				"Specify the path to logging directory, where input string must have"
				" trailing '/'. Directory must exist and be writable by waltr.")
			("config,c",
				po::value<string>(&conf)->default_value(
				"/usr/local/etc/waltr/waltr.conf"),
				"Specify a configuration file.")
			;

		po::options_description cmdline_options;
		cmdline_options.add(ifaces).add(general);

		po::variables_map vmap;
		try {
			po::store(po::parse_command_line(argc,argv,cmdline_options), vmap);
			ifstream ifs {vmap["config"].as<string>().c_str()};
			po::store(po::parse_config_file(ifs, cmdline_options),vmap);

			if(vmap.count("help")) {
				cout << "dewd companion walter.\n" << cmdline_options << '\n';

				return SUCCESS;
			}

			po::notify(vmap);
		}
		catch(po::error& poe) {

			cerr << "Exception: " << poe.what() << ".\nExiting.\n";
			return ERROR_UNHANDLED_EXCEPTION;
		}


		auto service = make_shared<io_service>();
		auto ep (stoe(dewd));
		auto mat = make_shared<maintainer>(service, logg, ep);

		if(dewd.size()) {
			mat->make_subs(chan);
		}


		/*
		 * Set signals to catch for graceful termination.
		 */

		boost::asio::signal_set signals(*service, SIGINT, SIGTERM);
		signals.async_wait(boost::bind(&graceful_exit,_1,_2));

		/*
		 * io_service::run() will run the io_service until there are no jobs or
		 * handlers left to be invoked.  Because the handlers as written invoke new
		 * work, run() should never terminate.
		 *
		 */

		service->run();


	} catch (std::exception& e) {
		cerr << e.what() << endl;
	}
	return 0;
}

