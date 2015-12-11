/*
 * serial_sessions.hpp
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef SERIAL_SESSION_HPP_
#define SERIAL_SESSION_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <utility>
#include <deque>
#include <cstdio>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <iterator>

#include <termios.h>
#include <linux/ioctl.h>
#include <linux/serial.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/chrono/duration.hpp>

#include <boost/range/iterator_range.hpp>

#include "utils.h"

#include "session.h"
#include "serial_session.h"

namespace dew {

using ::boost::asio::io_service;

using ::boost::chrono::steady_clock;
using ::boost::asio::basic_waitable_timer;
using ::boost::chrono::time_point;
using ::boost::chrono::milliseconds;

using ::boost::asio::serial_port;
using ::boost::asio::mutable_buffers_1;

using ::boost::make_iterator_range;

using ::boost::bind;

using ::std::to_string;
using ::std::string;
using ::std::vector;
using ::std::deque;

using ::std::srand;
using ::std::rand;

using ::std::search;
using ::std::find;
using ::std::copy;
using ::std::reverse_copy;
using ::std::floor;
using ::std::exp;

/*-----------------------------------------------------------------------------
 * November 20, 2015 :: base methods
 */

string serial_session::get_tx() {
	pop_counters();
	long int tx = (unsigned)counters.tx;
	return to_string(tx);
}

string serial_session::get_rx() {
	pop_counters();
	long int rx = (unsigned)counters.rx;
	return to_string(rx);
}



/*=============================================================================
 * December 9, 2015 :: _read_ methods
 *
 * _read_ class cannibalized _read_parse_ class as of December 9, 2015.
 */
void srs::set_timer() {
	srsp self (shared_from_this());
	time_point<steady_clock> now_ =	steady_clock::now();

	while(dead_< now_)
		dead_ = dead_ + timeout_;

	timer_.expires_at(dead_);
	timer_.async_wait(
  	boost::bind(&srs::handle_timeout, self, _1));
}

void srs::handle_timeout(boost::system::error_code ec) {
	if(!ec)
		port_.cancel();
	set_timer();
}

void srs::start() {
	start_read();
	dead_ = steady_clock::now() + timeout_;
	set_timer();
}
void srs::start_read() {
	bBuff* buffer_ = new bBuff (BUFFER_LENGTH);
	mutable_buffers_1 Buffer_ = boost::asio::buffer(*buffer_);
	boost::asio::async_read(port_,Buffer_,boost::bind(
			&srs::handle_read, this, _1, _2, buffer_));
}
void srs::handle_read(boost::system::error_code ec,
		size_t length, bBuff* buffer_) {
	srsp self (shared_from_this());
	counts.bytes_received += length;

	if(!buffer_->empty()) {
		if(to_parse.empty())
			front_last = steady_clock::now();
		copy(buffer_->begin(),buffer_->begin()+length, back_inserter(to_parse));
	}
	delete buffer_;

	io_ref->post(
			bind(&srs::check_the_deque,self));
	start_read();
}

int srs::scrub(pBuff::iterator iter) {
	pBuff::iterator oter = find(iter,to_parse.end(),0xff);
	int bytes = oter - to_parse.begin();
	to_parse.erase(to_parse.begin(), oter);
	front_last = steady_clock::now();
	return bytes;
}
void srs::check_the_deque() {
	srsp self (shared_from_this());
	/* The deque to check is d. From least restrictive to most restrictive, we
	 * check that:
	 * 1. size(d) > 0
	 * 2. size(d) >= min.size(message)
	 * 3. d[0] == FF
	 * 4. d[1] == FE
	 * 5. CRC matches
	 *
	 * If d fails 1. or 2., then there's nothing we can do.  We just need to wait
	 * for more characters.  We exit.
	 *
	 * If d fails on 3., 4., or 5., then we need to scrub.  As the test fails, we
	 * call scrub to tell the appropriate failure counter how many bytes it ate.
	 *
	 * If d passes all 5, then we have a valid prefix and search for a message.
	 * If no message is found, we check whether d is too large to contain a frame
	 * matching our prefix or whether the prefix is deemed too old to ever be
	 * completed.  In either case, we scrub to the next available FF.
	 */


	/* the prefix delimiter of a frame takes up
	 * FF + FE + nonce1(4) + nonce2(4) + seq + crc = 12 characters.
	 * The back delimiter of a frame takes up
	 * nonce1(4) + FE + FF = 6 characters.  If we have fewer than 18 characters
	 * in to_parse then we cannot succeed.
	 *
	 * With exactly 18 characters there is an edge case of an empty payload.  We
	 * would want to find and remove that.
	 */
	if(to_parse.size() < 18)
		return;

	assert(to_parse.size()>=18);


	if(to_parse[0]!=0xff || to_parse[1]!=0xfe) {

		counts.bad_prefix += scrub(to_parse.begin()+2);

		io_ref->post(boost::bind(&srs::check_the_deque,this));
		return;
	}

	assert(to_parse[0]==0xff);
	assert(to_parse[1]==0xfe);

	/* GATP that to_parse has at least 18 characters and the first two characters
	 * are 'FF' then 'FE'.
	 *
	 * The last check of whether we have a valid prefix is to compute the crc.
	 * The crc is located at byte location 11, counting from 0, of to_parse.
	 */

	u8 crc_comp = crc8(make_iterator_range(to_parse.begin(),to_parse.begin()+11));
	if(crc_comp!=to_parse[11]) {
		if(0)
		cout << "Bad CRC:\n" <<
				"\tMessage:" << debug_str(make_iterator_range(to_parse.begin(),to_parse.begin()+13)) <<
				'\n' << "\t Computed CRC: " << filter_unprintable(crc_comp) << '\n';

		counts.bad_crc += scrub(to_parse.begin()+12);

		io_ref->post(boost::bind(&srs::check_the_deque,this));
		return;
	}

	/* We now have a valid prefix, so we gather the rest of the frame delimiter,
	 * which is of the form
	 * 	'FF' + 'FE' + <nonce>
	 * where nonce is 4 character identifier occuring in positions 2 through 5 of
	 * to_parse.
	 *
	 * the matching delimiter we search for is:
	 * 	<ecnon> + 'FE' + 'FF'
	 *
	 * We search to_parse for a matching delimiter and, if found, cut it away
	 * from the rest of to_parse and ...
	 *  - currently printing to console
	 *  - later, will send to dispatcher
	 */



	pBuff delim (6);
	reverse_copy(to_parse.begin(),to_parse.begin()+6,delim.begin());

	/* matching nonce1 to nonce1 would be very bad, so we starting searching
	 * after the first 12 characters.  We've already asserted that to_parse has
	 * size at least 18, so we'll be fine.
	 */
	pBuff::iterator match_point =
			search(to_parse.begin()+12, to_parse.end(), delim.begin(), delim.end());

	/* if we've reached the end of the deque without finding a match, then we
	 * need to check if the prefix is too old or if the deque is too large to
	 * contain a valid message.  In either case we would need to scrub, and in
	 * either case all we know is that a valid prefix failed to find a message.
	 * Corner cases require us to discard only the first element, as there may be
	 * the beginning of a valid prefix lurking within our prefix to discard.
	 */
	if(match_point == to_parse.end()) {
		if(to_parse.size()>MAX_FRAME_LENGTH) {
			counts.frame_too_long += scrub(to_parse.begin()+1);

			io_ref->post(bind(&srs::check_the_deque,this));
			return;
		} else if (steady_clock::now() - front_last > milliseconds(500)) {
			counts.frame_too_old += scrub(to_parse.begin()+1);

			io_ref->post(boost::bind(&srs::check_the_deque,this));
			return;
		}
	} else if(match_point!=to_parse.end()) { /* We have a message */
		++counts.msg_tot;

		/* dispatcher::forward() function promises to release this memory once the
		 * message has been processed.
		 */
		string* to_send = new string;
		pBuff xfix;

		/* We have a match and match_point points to the beginning of the endframe
		 * delimiter.  endframe delimiter is 6 characters long, so match_point + 6
		 * is less than to_parse.end().
		 */
		assert(match_point+6 <= to_parse.end());
		copy(to_parse.begin(),to_parse.begin()+12,back_inserter(xfix));
		copy(to_parse.begin()+12, match_point, back_inserter(*to_send));
		copy(match_point, match_point+6, back_inserter(xfix));

		counts.wrapper_bytes_tot += xfix.size();
		counts.msg_bytes_tot += to_send->size();
		counts.garbage += scrub(match_point+6);
		counts.garbage -= to_send->size();

		assert(xfix.size()>=11);
		counts.curr_msg = (int)(xfix.at(10));
		if(counts.last_msg > counts.curr_msg )
			counts.last_msg -= 256;
		while(counts.last_msg < counts.curr_msg - 1){
			++counts.last_msg;
			++counts.lost_msg_count;
		}
		counts.last_msg=counts.curr_msg;

		/* debug logging */
		if(0) {
		FILE * log = fopen((
				dis_ref->get_logdir() + name_.substr(name_.find_last_of("/\\")+1) + ".received.prefix").c_str(),"a");
		stringstream ss;
		debug(make_iterator_range(to_send->begin(),to_send->begin()+18),&ss);
		std::fwrite(ss.str().c_str(), sizeof(u8), ss.str().length(), log);
		fclose(log);
		}

		io_ref->post(bind(&srs::forward,this,to_send));

		/* Loop checks until to_parse has no more messages waiting for us. */
		io_ref->post(bind(&srs::check_the_deque,this));
	}
}

void srs::forward(string* to_send) {
	dis_ref->forward(to_send);
}


/*-----------------------------------------------------------------------------
 * November 20, 2015 :: _write_ methods
 */

/*=============================================================================
 * December 7, 2015 :: _write_pb_ methods
 */

void swps::start() {
	srand(time(0));
	start_write();
}

void swps::start_write() {
	swpsp self (shared_from_this());
	bBuff* message = generate_message();
	mutable_buffers_1 bMessage = boost::asio::buffer(*message);
	boost::asio::async_write(port_, bMessage, bind(
			&swps::handle_write, self, _1, _2, message));
}

void swps::handle_write(
			const boost::system::error_code& error, size_t bytes_transferred,
			bBuff* message) {
	swpsp self (shared_from_this());
	io_ref->post(
			boost::bind(&swps::start_write,self));
	delete message;
}

bBuff* swps::generate_message() {
	/* Caller accepts the responsibility of freeing this memory */
	bBuff* msg = new bBuff;

	while(msg->size() < 1024) {
		++internal_counter;
		bBuff nonce1 = {0xff, 0xfe}, nonce2;
		u8 byte;

		for(int i=4;i;--i) {
			byte = (u8)(mod(rand(),256));
			nonce1.emplace_back(byte);
			byte = (u8)(mod(rand(),256));
			nonce2.emplace_back(byte);
		}
		/* nonce 1 add */
		copy(nonce1.begin(), nonce1.end(), back_inserter(*msg));

		/* nonce2 add */
		copy(nonce2.begin(), nonce2.end(), back_inserter(*msg));

		/* msg number sequence */
		msg->push_back(mod(internal_counter,256));

		assert(msg->size()>=11);
		u8 crc = crc8(make_iterator_range(msg->end()-11,msg->end()));

		/* crc byte */
		msg->push_back(crc);

		int num = mod(rand(),9);
		string name = to_string(num) + "of09";

		flopointpb::FloPointWaveform fpwf;
		fpwf.set_name(name);


		/*=========================================================================
		 * Waveform generated is Gompertz function samples at x = 0 .. 63
		 * with parameters a=2^32-1, b=rand(1), c=rand(2)
		 */
		flopointpb::FloPointWaveform_Waveform* wf = new flopointpb::FloPointWaveform_Waveform;

		double gomp_b = 1 - (static_cast<double>(rand())/RAND_MAX);
		double gomp_c = 1 - (static_cast<double>(rand())/RAND_MAX);


		for(int i = 0  ; i<64 ; ++i) {
			wf->add_wheight(
					static_cast<int>(65000*exp((-1)*gomp_b*exp((-1)*gomp_c)))*i);
		}

		fpwf.set_allocated_waveform(wf);

		string fpwf_str;
		if(!(fpwf.SerializeToString(&fpwf_str))) {
			FILE * log = fopen((
					dis_ref->get_logdir() + name_.substr(name_.find_last_of("/\\")+1) + ".message_generation").c_str(),"a");
			string s (to_string(steady_clock::now()) + ": Could not serialize message to string.\n");
			std::fwrite(s.c_str(), sizeof(u8), s.length(), log);
			fclose(log);
		}
		copy(fpwf_str.begin(), fpwf_str.end(), back_inserter(*msg));

		reverse_copy(nonce1.begin(),nonce1.end(),back_inserter(*msg));
	}

	//debug(make_iterator_range(msg->begin(),msg->end()));

	return msg;
}

} // dew namespace

#endif /* SERIAL_SESSION_HPP_ */
