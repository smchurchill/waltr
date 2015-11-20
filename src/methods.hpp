/*
 * methods.hpp
 *
 *  Created on: Nov 19, 2015
 *      Author: schurchill
 */

#ifndef METHODS_HPP_
#define METHODS_HPP_


template<typename session_type, typename unique_identifier >
std::vector<io_session<session_type, unique_identifier>*> make_asio_work(
		boost::asio::io_service& io_in, std::vector<unique_identifier> i, std::string log_in ){

	std::vector<io_session<session_type, unique_identifier>*>* v = new std::vector<io_session<session_type, unique_identifier>*>;

	for(typename std::vector<unique_identifier>::iterator it = i.begin() ; it != i.end() ; ++it ) {
		io_session* h = new io_session<session_type, unique_identifier>(io_in, *it, log_in);
		v->push_back(h);
	}

	for(typename std::vector<io_session*>::iterator it = v->begin() ;	it != v->end() ; ++it ) {
		(**it).start();
	}

	return *v;
}






#endif /* METHODS_HPP_ */
