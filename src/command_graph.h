/*
 * command_tree.h
 *
 *  Created on: Dec 16, 2015
 *      Author: schurchill
 */

#ifndef COMMAND_GRAPH_H_
#define COMMAND_GRAPH_H_

namespace dew {

class node : public enable_shared_from_this<node> {
public:
	node(){}
	node(map<string,nodep> children_in) : children(children_in) {}
	node(std::function<void(nsp)> fn_in) : fn(fn_in) {}
	node(map<string,nodep> children_in, std::function<void(nsp)> fn_in) :
		children(children_in), fn(fn_in) {}

	void spawn(pair<string,nodep>);
	void spawn(string, nodep);
	void spawn(map<string,nodep>);
	void purge();

	void set_fn(std::function<void(nsp)> fn_in) { fn = fn_in; }
	void operator()(nsp) const;
	const shared_ptr<node>& child(const string & key) const { return children.at(key); }

	string descendants(const int);

	void own() { owned = true; }
	bool is_owned() { return owned; }
	bool is_leaf() { return children.empty(); }

private:
	map<string,nodep> children;
	std::function<void(nsp)> fn;
	bool owned = false;
};

} //dew namespace

#endif /* COMMAND_GRAPH_H_ */
