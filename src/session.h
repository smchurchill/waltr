/*
 * session.h
 *
 *  Created on: Nov 20, 2015
 *      Author: schurchill
 */

#ifndef SESSION_H_
#define SESSION_H_

class basic_session;

class dispatcher {
	friend class basic_session;

private:
	std::vector<basic_session*> friends;
	void hello(basic_session* new_friend);

public:
	dispatcher() {};
	void brag();
};

class basic_session{
public:
	basic_session(
				boost::asio::io_service& io_in,
				std::string log_in,
				dispatcher* ref_in);/* :
					io_service(&io_in),
					logdir_(log_in),
					ref(ref_in) {
			start_ = boost::chrono::steady_clock::now();
			ref->hello(this);
		}*/
	virtual std::string print() =0;
	virtual ~basic_session() {};

protected:
	boost::asio::io_service* io_service;
	std::string logdir_;
	dispatcher* ref;
  boost::chrono::time_point<boost::chrono::steady_clock> start_;

private:
};


#endif /* SESSION_H_ */
