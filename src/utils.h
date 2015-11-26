/*
 * utils.h
 *
 *  Created on: Nov 24, 2015
 *      Author: schurchill
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <iterator>

#include <iostream>
#include <vector>
#include <deque>
#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <string>
#include <sstream>

#include <cctype>
#include <ios>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

namespace dew {

/*-----------------------------------------------------------------------------
 * November 25, 2015
 *
 * wanted a nonnegative modulus operator
 */

inline int mod(int n, int m) {
	return (n%m < 0 ? n%m + m : n%m);
}


/*-----------------------------------------------------------------------------
 * November 25, 2015
 *
 * specialized fn from AJS
 */

using ::std::stringstream;
using ::std::string;

using ::std::iterator_traits;
using ::std::enable_if;
using ::std::is_same;
using ::std::copy;
using ::std::ostream_iterator;
using ::std::cout;
using ::std::isgraph;


template <typename T> using ivt = typename iterator_traits<T>::value_type;

template<typename Input>
typename enable_if<!is_same<ivt<Input>,u8>::value>::type
debug (Input first, Input last) {
    copy (first, last, ostream_iterator<ivt<Input>> (cout, " "));
    cout << '\n';
}

template<typename Input>
typename enable_if<is_same<ivt<Input>,u8>::value>::type
debug (Input first, Input last) {
    copy (first, last, ostream_iterator<ivt<Input>> (cout));
    cout << '\n';
}


void test_specialization (void)
{
	cout << "==Test specialization==\n";

	string sc = "hello world";

    std::vector<int> vi = { 3, 1, 2 };
    debug (vi.begin(), vi.end());

    std::vector<u8> vc (sc.begin(), sc.end());
    debug (vc.begin(), vc.end());

    std::deque<u8> dc (sc.begin(), sc.end());
    debug (dc.begin(), dc.end());
}


/*-----------------------------------------------------------------------------
 * November 25, 2015
 *
 * Range adapter from AJS
 */




/* could just as well transform !is_print(c) to <ff><fe> notation*/
string filter_unprintable (u8 c) {
	stringstream ss;
	ss << "<" << std::hex << c << ">";
  return "nothing"; // string ( isgraph(c) ? c : ss.str() );
}

template<typename Range>
void debug (Range rng) {
	// typedef typename Range::first_type::value_type T;
	boost::transform (rng | boost::adaptors::reversed, ostream_iterator<string> (cout), filter_unprintable);
	cout << '\n';
}

 /*
void test_range (void)
{
	cout << "==Test range==\n";
	string sc = "hello world";
    std::deque<u8> dc (sc.begin(), sc.end());
    debug (std::make_pair (dc.begin(), dc.end()));
}
*/



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

}; // dew namespace

#endif /* UTILS_H_ */
