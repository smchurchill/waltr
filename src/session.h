/*
 * session.h
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef SESSION_H_
#define SESSION_H_

namespace dew {

using ::boost::asio::io_service;
using ::boost::asio::ip::tcp;
using ::boost::asio::serial_port;
using ::boost::chrono::steady_clock;
using ::boost::chrono::time_point;
using ::boost::chrono::milliseconds;
using ::boost::bind;
using ::boost::system::error_code;

using ::std::string;
using ::std::to_string;
using ::std::vector;
using ::std::deque;
using ::std::list;
using ::std::set;
using ::std::map;
using ::std::pair;
using ::std::make_pair;

using ::std::move;

using ::std::unique_ptr;
using ::std::shared_ptr;
using ::std::weak_ptr;
using ::std::enable_shared_from_this;


class maintainer : public enable_shared_from_this<maintainer> {
public:
	maintainer(shared_ptr<io_service> const&);
	maintainer(shared_ptr<io_service> const&, string);
	maintainer(shared_ptr<io_service> const&, string, tcp::endpoint&);
	~maintainer() {}

private:
	tcp::endpoint dewd_;
	context_struct_lite context_;
	string logdir_;

	list<nsp> network;

/* Method type: creation and destruction of sessions */
public:
	void make_subs(vector<string>&);
	void remove_ns (nsp);

/* Method type: network communications */
public:
	void delivery(stringp);

private:
	string logfile(string);

	void forward(stringp);
	void forward_handler(const error_code&,size_t, bBuffp, nsp);

/* Method type: basic information */
public:
	string get_logdir() { return logdir_; }
};


} // dew namespace

#endif /* SESSION_H_ */
