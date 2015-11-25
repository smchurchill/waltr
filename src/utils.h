/*
 * utils.h
 *
 *  Created on: Nov 24, 2015
 *      Author: schurchill
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <iterator>
/*-----------------------------------------------------------------------------
 * November 24, 2015
 *
 * We will use the generating polynomial x^8 + x^2 + x^1 + 1.  This
 * can also be written as 10000111 or 135 or 0x87, etc.
 *
 * This implementation taken from chromium.googlesource.com.
 * first google result for "crc8 c".
 */

u8 crc8(const u8* vptr, int len) {
	const u8 * data = vptr;
	unsigned crc = 0;
	int i, j;

	for(j=len ; j ; --j , ++data) {
		crc ^= (*data << 8);
		for(i=8 ; i ; --i) {
			if(crc & 0x8000)
				crc ^= (0x1070 << 3);
			crc <<= 1;
		}
	}

	return (u8)(crc >> 8);
}


#endif /* UTILS_H_ */
