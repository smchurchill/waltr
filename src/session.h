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
using ::boost::chrono::steady_clock;
using ::boost::chrono::time_point;

using ::std::string;
using ::std::vector;
using ::std::deque;

class basic_session;

class dispatcher {
	friend class basic_session;

private:
	vector<basic_session*> friends;
	void hello(basic_session* new_friend);

public:
	dispatcher() {};
	void brag();

	template<class container_type>
	void forward(basic_session* msg_from, container_type* msg) {delete msg;}
};

class basic_session{
public:
	basic_session( io_service& io_in, string log_in, dispatcher* ref_in) :
			io_ref(&io_in), logdir_(log_in), dis_ref(ref_in)
	{
		dis_ref->hello(this);
	};
	virtual string print() =0;
	virtual ~basic_session() {};

protected:
  time_point<steady_clock> start_ = steady_clock::now();
	io_service* io_ref;
	string logdir_;
	dispatcher* dis_ref;
private:
};

} // dew namespace

#endif /* SESSION_H_ */
