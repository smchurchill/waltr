/*
 * network_command.h
 *
 *  Created on: Dec 14, 2015
 *      Author: schurchill
 */

#ifndef NETWORK_COMMAND_H_
#define NETWORK_COMMAND_H_

namespace dew {

class network_command {
public:
	network_command();
	network_command(sentence to_read) {
		read(to_read);
	}

	void read(sentence to_read);

private:
	command_struct detail;

};


} // dew namespace

#endif /* NETWORK_COMMAND_H_ */
