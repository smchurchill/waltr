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

typedef serial_read_session srs;
typedef serial_write_pb_session swps;
typedef network_socket_session nss;
typedef network_acceptor_session nas;


typedef uint8_t u8;
typedef uint16_t u16;
typedef std::deque<u8> pBuff;
typedef std::vector<u8> bBuff;
typedef std::list<std::string> sentence;

typedef std::map<std::string, std::function<std::string()> > cmd_map;

typedef std::shared_ptr<srs> srsp;
typedef std::shared_ptr<swps> swpsp;
typedef std::shared_ptr<nss> nssp;
typedef std::shared_ptr<nas> nasp;

typedef std::weak_ptr<nss> nssw;

typedef boost::chrono::time_point<boost::chrono::steady_clock> stime;
typedef boost::chrono::steady_clock sc;

/*=============================================================================
 * December 11, 2015
 *
 * Needed operator== to compare std::weak_ptr in boost::bimap creation.
 *
 */

template< typename PointsTo >
bool wk_comp (std::weak_ptr<PointsTo> l, std::weak_ptr<PointsTo> r) {
	if(l.use_count() && r.use_count())
		return l.lock() == r.lock();

	return false;
}


typedef boost::bimaps::bimap<
		boost::bimaps::unordered_multiset_of<std::string>,
		boost::bimaps::unordered_multiset_of<
			nssp > > channel_subscribers;

}; // namespace dew

#endif /* TYPES_H_ */
