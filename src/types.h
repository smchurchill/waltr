/*
 * types.h
 *
 *  Created on: Nov 24, 2015
 *      Author: schurchill
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <boost/chrono.hpp>
#include <boost/chrono/time_point.hpp>

#include <boost/bimap.hpp>
#include <boost/bimap/unordered_multiset_of.hpp>


namespace dew {

class serial_read_session;
class serial_write_pb_session;
class network_socket_session;
class network_acceptor_session;

typedef serial_session ss;
typedef network_session ns;
typedef std::shared_ptr<ss> ssp;
typedef std::shared_ptr<ns> nsp;

typedef uint8_t u8;
typedef uint16_t u16;
typedef std::deque<u8> pBuff;
typedef std::vector<u8> bBuff;
typedef std::shared_ptr<bBuff> bBuffp;
typedef std::shared_ptr<pBuff> pBuffp;

typedef std::list<std::string> sentence;

}; // namespace dew

#endif /* TYPES_H_ */
