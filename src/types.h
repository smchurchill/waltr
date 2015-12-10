/*
 * types.h
 *
 *  Created on: Nov 24, 2015
 *      Author: schurchill
 */

#ifndef TYPES_H_
#define TYPES_H_

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

typedef std::map<std::string, std::function<std::string()> > cmd_map;

typedef std::shared_ptr<srs> srsp;
typedef std::shared_ptr<swps> swpsp;
typedef std::shared_ptr<nss> nssp;
typedef std::shared_ptr<nas> nasp;

}; // namespace dew

#endif /* TYPES_H_ */
