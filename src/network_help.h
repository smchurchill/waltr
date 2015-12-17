/*
 * network_help.h
 *
 *  Created on: Dec 16, 2015
 *      Author: schurchill
 */

#ifndef NETWORK_HELP_H_
#define NETWORK_HELP_H_

#include <string>

#include "network_session.h"

namespace dew {

using ::std::string;

/*=============================================================================
 * December 16, 2015
 *
 * The following return help strings for network commands.
 */

void help(nsp in) {
	string to_write ("help called.\n");
	in->do_write(to_write);
}

void help_help(nsp in) {
	string to_write ("help_help called.\n");
	in->do_write(to_write);
}

void get_help(nsp in) {
	string to_write ("get_help called.\n");
	in->do_write(to_write);
}

void get_help_rx(nsp in) {
	string to_write ("get_help_rx(called.\n");
	in->do_write(to_write);
}

void get_help_tx(nsp in) {
	string to_write ("get_help_tx called.\n");
	in->do_write(to_write);
}

void get_help_messages_received_tot(nsp in) {
	string to_write ("get_help_messages_received_tot called.\n");
	in->do_write(to_write);
}

void get_help_messages_lost_tot(nsp in) {
	string to_write ("get_help_messages_lost_tot called.\n");
	in->do_write(to_write);
}

void get_help_ports_for_zabbix(nsp in) {
	string to_write ("get_help_ports_for_zabbix called.\n");
	in->do_write(to_write);
}

void help_get(nsp in) {
	get_help(in);
}

void subscribe_help(nsp in) {
	string to_write ("subscribe_help() called.\n");
	in->do_write(to_write);
}

void help_subscribe(nsp in) {
	subscribe_help(in);
}

void unsubscribe_help(nsp in) {
	string to_write ("unsubscribe_help called.\n");
	in->do_write(to_write);
}

void help_unsubscribe(nsp in) {
	unsubscribe_help(in);
}


} // dew namespace

#endif /* NETWORK_HELP_H_ */
