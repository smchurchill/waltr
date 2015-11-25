/*
 * debug.h
 *
 *  Created on: Nov 24, 2015
 *      Author: schurchill
 */

#ifndef DEBUG_H_
#define DEBUG_H_

namespace dew {

template<typename container_type>
void printc(container_type* container) {
	for(auto i : container_type(container->begin(),container->end()))
		std::cout << i;
	std::cout << '\n';
}

template<typename container_type>
void printi(container_type* container) {
	for(auto i : container_type(container->begin(),container->end()))
		std::cout << (int) i << ' ';
	std::cout << '\n';
}

}; //namespace dew

#endif /* DEBUG_H_ */
