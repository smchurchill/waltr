/*
 * types.h
 *
 *  Created on: Nov 24, 2015
 *      Author: schurchill
 */

#ifndef TYPES_H_
#define TYPES_H_

namespace dew {

typedef uint8_t u8;
typedef uint16_t u16;
typedef std::deque<u8> pBuff;
typedef std::vector<u8> bBuff;

typedef std::map<std::string, std::function<std::string()> > cmd_map;


}; // namespace dew

#endif /* TYPES_H_ */
