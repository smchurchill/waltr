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

#include "structs.h"
#include "types.h"
#include "utils.h"

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

using ::boost::system::error_code;

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

/* December 16, 2015 :: constructors */

serial_session::serial_session(context_struct context_in) :
	serial_session(context_in, "/dev/null")
{
}

serial_session::serial_session(
		context_struct context_in,
		string device_in,
		milliseconds timeout_in
) :
		context_(context_in),
		port_(*context_.service, device_in),
		fd_(port_.native_handle()),
		name_(device_in),
		timeout_(timeout_in),
		timer_(*context_.service),
		read_type_is_timeout_(true)
{
}

serial_session::serial_session(
		context_struct context_in,
		string device_in
) :
		serial_session(context_in, device_in, {0,0,0,0})
{
}

serial_session::serial_session(
		context_struct context_in,
		string device_in,
		write_test_struct wts_in
) :
		context_(context_in),
		port_(*context_.service, device_in),
		fd_(port_.native_handle()),
		name_(device_in),
		timeout_(milliseconds(0)),
		timer_(*context_.service),
		read_type_is_timeout_(false),
		wts_(wts_in)
{
}

shared_ptr<serial_session> ss::get_ss() {
	return shared_from_this();
}

void ss::start_write() {
	write_type_is_test = true;
	srand(time(0));
	do_write();
}

void ss::start_read() {
	do_read();
	if(read_type_is_timeout_)
		set_read_timer();
}

void ss::do_write() {
	auto message = generate_message();
	do_write(message);
}

void ss::do_write(bBuffp message) {
	auto self (shared_from_this());
	auto Message = boost::asio::buffer(*message);
	auto handler = bind(&ss::handle_write, self, _1, _2, message);
	boost::asio::async_write(port_, Message, handler);
}

void ss::do_read() {
	auto self (shared_from_this());
	auto buffer = make_shared<bBuff> (BUFFER_LENGTH);
	auto Buffer = boost::asio::buffer(*buffer);
	auto handler = bind(&ss::handle_read, self, _1, _2, buffer);
	if(read_type_is_timeout_)
		boost::asio::async_read(port_, Buffer, handler);
	else
		port_.async_read_some(Buffer, handler);
}

void ss::handle_write(const error_code& ec, size_t len, bBuffp message) {
	if(write_type_is_test)
		do_write();
	return;
}

void ss::handle_read(const error_code& ec, size_t len, bBuffp buffer) {
	auto self (shared_from_this());
	counts.bytes_received += len;

	if(!buffer->empty()) {
		if(to_parse.empty())
			front_last = steady_clock::now();
		copy(buffer->begin(),buffer->begin()+len, back_inserter(to_parse));
	}

	set_a_check();
	do_read();
}

void ss::scope(bBuffp buff) {
	dprint(buff.use_count());
}

void ss::set_a_check() {
	auto self (shared_from_this());
	context_.service->post(boost::bind(&ss::check_the_deque,self));
}

void ss::check_the_deque() {
	auto self (shared_from_this());
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
	 * The payload is a google protobuf message of unknown length, so we stay
	 * at the conservative 18 character minimum.
	 */
	if(to_parse.size() < 18)
		return;
	assert(to_parse.size()>=18);

	if(to_parse[0]!=0xff || to_parse[1]!=0xfe) {
		counts.bad_prefix += scrub(to_parse.begin()+2);
		set_a_check();
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
		counts.bad_crc += scrub(to_parse.begin()+12);
		set_a_check();
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
			set_a_check();
			return;
		} else if (steady_clock::now() - front_last > milliseconds(500)) {
			counts.frame_too_old += scrub(to_parse.begin()+1);
			set_a_check();
			return;
		}
	} else { /* We have a message */
		++counts.messages_received;
		auto to_send = make_shared<string>();
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
			++counts.messages_lost_tot;
		}
		counts.last_msg=counts.curr_msg;
		context_.dispatch->delivery(to_send);

		/* Loop checks until to_parse has no more messages waiting for us. */
		set_a_check();
	}
}

int ss::scrub(pBuff::iterator iter) {
	pBuff::iterator oter = find(iter,to_parse.end(),0xff);
	int bytes = oter - to_parse.begin();
	to_parse.erase(to_parse.begin(), oter);
	front_last = steady_clock::now();
	return bytes;
}

int ss::pop_counters() {
	memset(&ioctl_counters, 0, sizeof(serial_icounter_struct));
	return ioctl(fd_, TIOCGICOUNT, &ioctl_counters);
}

bBuffp ss::generate_message() {
	auto message = make_shared<bBuff>();

	while(message->size() < 1024) {
		++counts.messages_sent;
		bBuff nonce1 = {0xff, 0xfe}, nonce2;
		u8 byte;

		for(int i=4;i;--i) {
			byte = (u8)(mod(rand(),256));
			nonce1.emplace_back(byte);
			byte = (u8)(mod(rand(),256));
			nonce2.emplace_back(byte);
		}
		/* nonce 1 add */
		copy(nonce1.begin(), nonce1.end(), back_inserter(*message));

		/* nonce2 add */
		copy(nonce2.begin(), nonce2.end(), back_inserter(*message));

		/* msg number sequence */
		message->push_back(mod(counts.messages_sent,256));

		assert(message->size()>=11);
		u8 crc = crc8(make_iterator_range(message->end()-11,message->end()));

		/* crc byte */
		message->push_back(crc);

		int num = mod(rand(),9);
		string name (to_string(num) + "of09");


		flopointpb::FloPointMessage fpwf;
		fpwf.set_name(name);


		/*=========================================================================
		 * Waveform generated is simple sigmoid
		 *  65000 / ( 1 + e^ (c * (i-32)))
		 * where c is a constant between .16 and .4 and i is evaluated on integers
		 * 0 to 63.
		 */

		/* Once allocated, this memory is freed when *fpwf is deleted */
		auto wf = new flopointpb::FloPointMessage_Waveform;

		double to_add = 0.0;
		if(wts_.sample_size > 0)
			to_add = (wts_.max_c-wts_.min_c)*mod(counts.messages_sent,wts_.sample_size)/wts_.sample_size;
		else
			to_add = (wts_.max_c-wts_.min_c)*(rand()/RAND_MAX);

		double c = wts_.min_c + to_add;

		for(int i = 0  ; i<64 ; ++i) {
			double value = wts_.peak / (1 + exp(c*(32-i)));
			u32 int_value = static_cast<u32>(value);
			wf->add_wheight(int_value);
		}

		fpwf.set_allocated_waveform(wf);

		string fpwf_str;
		if(!(fpwf.SerializeToString(&fpwf_str))) {
			string filename;
			filename += context_.dispatch->get_logdir();
			filename +=	name_.substr(name_.find_last_of("/\\")+1);
			filename +=".message_generation";
			FILE * logfile = fopen(filename.c_str(),"a");
			string s;
			s += to_string(steady_clock::now());
			s += ": Could not serialize message to string.\n";
			std::fwrite(s.c_str(), sizeof(u8), s.length(), logfile);
			fclose(logfile);
		}

		copy(fpwf_str.begin(), fpwf_str.end(), back_inserter(*message));
		reverse_copy(nonce1.begin(),nonce1.end(), back_inserter(*message));
	}

	return message;
}

void ss::set_read_timer() {
	auto self (shared_from_this());
	time_point<steady_clock> now =	steady_clock::now();
	while(dead < now)
		dead = dead + timeout_;

	timer_.expires_at(dead);
	timer_.async_wait(bind(&ss::handle_read_timeout, self, _1));
}

void ss::handle_read_timeout(const error_code& ec) {
	if(!ec)
		port_.cancel();
	set_read_timer();
}

string ss::get_tx() {
	pop_counters();
	long int tx = (unsigned)ioctl_counters.tx;
	return to_string(tx);
}

string ss::get_rx() {
	pop_counters();
	long int rx = (unsigned)ioctl_counters.rx;
	return to_string(rx);
}

void ss::get_tx(nsp in) {
	in->do_write(make_shared<string>(get_tx()));
}
void ss::get_rx(nsp in) {
	in->do_write(make_shared<string>(get_rx()));
}

string ss::get_messages_received_tot() {
	return to_string(counts.messages_received);
}
string ss::get_messages_lost_tot() {
	return to_string(counts.messages_lost_tot);
}

void ss::get_messages_received_tot(nsp in) {
	in->do_write(make_shared<string>(get_messages_received_tot()));
}
void ss::get_messages_lost_tot(nsp in) {
	in->do_write(make_shared<string>(get_messages_lost_tot()));
}

string ss::get_messages_sent_tot() {
	return to_string(counts.messages_sent);
}

string ss::get_frame_too_old() {
	return to_string(counts.frame_too_old);
}

string ss::get_frame_too_long() {
	return to_string(counts.frame_too_long);
}

string ss::get_bad_prefix() {
	return to_string(counts.bad_prefix);
}

string ss::get_bad_crc() {
	return to_string(counts.bad_crc);
}

string ss::get_bytes_received() {
	return to_string(counts.bytes_received);
}

string ss::get_msg_bytes_tot() {
	return to_string(counts.msg_bytes_tot);
}

string ss::get_wrapper_bytes_tot() {
	return to_string(counts.wrapper_bytes_tot);
}

string ss::get_garbage() {
	return to_string(counts.garbage);
}

string ss::get_name() {
	return name_;
}

string ss::get_type() {
	return string("serial");
}


} // dew namespace

#endif /* SERIAL_SESSION_HPP_ */
