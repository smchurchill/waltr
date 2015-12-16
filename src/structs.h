/*
 * structs.h
 *
 *  Created on: Dec 14, 2015
 *      Author: schurchill
 */

#ifndef STRUCTS_H_
#define STRUCTS_H_

#include <memory>
#include <vector>
#include "session.hpp"

#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>


using ::std::shared_ptr;

using ::boost::asio::io_service;

using ::boost::chrono::time_point;
using ::boost::chrono::steady_clock;

using ::std::vector;

namespace dew {

struct context_struct {
	context_struct(const context_struct& in) {
		start = steady_clock::now();
		service = in.service;
		dispatch = in.dispatch;
	}


	time_point<steady_clock> start;
	const shared_ptr<io_service> service;
	const shared_ptr<dispatcher> dispatch;

	void set_start() { start = steady_clock::now(); }

};

/*
struct serial_icounter_struct {
  int cts, dsr, rng, dcd;
  int rx, tx;
  int frame, overrun, parity, brk;
  int buf_overrun;
  int reserved[9];
};
*/

struct message_counter_struct {
	int messages_received;
	int messages_sent;
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


/*=============================================================================
 * December 14, 2015
 *
 * Dissatisfied with inelegant command parsing.  Hoping that this makes things
 * a bit more structured.
 */

struct command_struct {

};


} //namespace dew

#endif /* STRUCTS_H_ */
