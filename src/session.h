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
class dispatcher;

class basic_session{
public:
	basic_session( io_service& io_in, string log_in, dispatcher* ref_in);
	virtual string print() =0;
	virtual ~basic_session() {};

protected:
  time_point<steady_clock> start_ = steady_clock::now();
	io_service* io_ref;
	string logdir_;
	dispatcher* dis_ref;
private:
};



class dispatcher {
	friend class basic_session;

private:
	vector<basic_session*> comrades;
	void hello(basic_session* new_comrade);

public:
	dispatcher() {};
	~dispatcher();
	void brag();

	template<class container_type>
	void forward(basic_session* msg_from, container_type* msg) {delete msg;}
};


} // dew namespace

#endif /* SESSION_H_ */
