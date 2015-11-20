/*
 * methods.hpp
 *
 *  Created on: Nov 19, 2015
 *      Author: schurchill
 */

#ifndef METHODS_HPP_
#define METHODS_HPP_


template<typename session_type, typename unique_identifier_type >
std::vector<io_session*> make_asio_work(
		boost::asio::io_service& io_in, std::vector<unique_identifier_type> i, std::string log_in ){

	std::vector<io_session*>* v = new std::vector<io_session*>;

	for(typename std::vector<unique_identifier_type>::iterator it = i.begin() ; it != i.end() ; ++it ) {
		io_session* h = new io_session<session_type, unique_identifier_type>(io_in, *it, log_in);
		v->push_back(h);
	}

	for(typename std::vector<io_session*>::iterator it = v->begin() ;	it != v->end() ; ++it ) {
		(**it).start();
	}

	return *v;
}






#endif /* METHODS_HPP_ */
