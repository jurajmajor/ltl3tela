/*
    Copyright (c) 2016 Juraj Major

    This file is part of LTL3TELA.

    LTL3TELA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LTL3TELA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LTL3TELA.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AUTOMATON_H
#define AUTOMATON_H
#include <algorithm>
#include <map>
#include <stack>
#include <string>
#include <sstream>
#include <spot/tl/print.hh>
#include <queue>
#include <vector>
#include <spot/tl/unabbrev.hh>
#include <spot/misc/escape.hh>
#include <spot/twa/acc.hh>
#include <spot/twa/bddprint.hh>
#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/cleanacc.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/postproc.hh>
#include <spot/twaalgos/simulation.hh>
#include <spot/twaalgos/sccfilter.hh>
#include "utils.hpp"

class Edge {
protected:
	// target set of the edge
	std::set<unsigned> targets;

	// the acceptance label
	std::set<spot::acc_cond::mark_t::value_t> marks;

	// the transition labels in BDD
	bdd label;
public:
	Edge(bdd l);

	// adds a state or a set of states to the target set
	void add_target(unsigned state_id);
	void add_target(std::set<unsigned> state_ids);

	// removes a state from the target set
	void remove_target(unsigned state_id);

	// replaces the target set with given set
	void replace_target_set(std::set<unsigned> state_ids);

	// adds an acceptance mark or a set of them
	void add_mark(unsigned ix);
	void add_mark(std::set<unsigned> ixs);

	// removes an acceptance mark
	void remove_mark(unsigned ix);

	// removes all acceptance marks
	void clear_marks();

	// returns the target set
	std::set<unsigned> get_targets() const;

	// returns the acceptance label
	std::set<unsigned> get_marks() const;

	// returns the transition label
	bdd get_label() const;

	// sets the transition label
	void set_label(bdd l);

	int dominates(Edge* other, std::set<unsigned> o1, std::set<unsigned> o2, std::set<spot::acc_cond::mark_t::value_t> inf_marks) const;
	int dominates(Edge* other, std::set<spot::acc_cond::mark_t::value_t> inf_marks) const;
};

template<typename T> class Automaton {
protected:
	// vector of names of states
	std::vector<T> states;

	// vector of edges
	std::vector<Edge*> edges;

	// state_edges maps a set of edges to each state
	std::vector<std::set<unsigned>> state_edges;

	std::vector<std::set<unsigned>>* spot_id_to_slaa_set = nullptr; // this has to be nullptr for SLAA

	// a set of Inf-marks used in the automaton
	std::set<spot::acc_cond::mark_t::value_t> inf_marks;

	// the set of initial configurations
	std::set<std::set<unsigned>> init_sets;

public:
	// returns a state ID by its name, possibly creating a new one
	unsigned get_state_id(T f);

	// returns a name of the state with the given ID
	T state_name(unsigned state_id);

	// checks whether a state with given name already exists
	bool state_exists(T f);

	// returns the number of states
	unsigned states_count();

	// creates an edge and returns its ID (index in the `edges' set)
	unsigned create_edge(bdd label);

	// creates an edge with given source state, labels and target set
	void add_edge(unsigned from, bdd label, std::set<unsigned> to, std::set<spot::acc_cond::mark_t::value_t> marks = std::set<spot::acc_cond::mark_t::value_t>());

	// copies the given edge to the source `from'
	void add_edge(unsigned from, unsigned edge_id);

	// copies the given edges to the source `from'
	void add_edge(unsigned from, std::set<unsigned> edge_ids);

	// removes the given edge from the source
	void remove_edge(unsigned state_id, unsigned edge_id);

	// returns a pointer to the edge specified by ID
	Edge* get_edge(unsigned edge_id) const;

	// returns state_edges[state_id]
	std::set<unsigned> get_state_edges(unsigned state_id) const;

	// returns the registered Inf-marks
	std::set<spot::acc_cond::mark_t::value_t> get_inf_marks() const;

	// registers the marks in the set `inf_marks'
	void remember_inf_mark(spot::acc_cond::mark_t::value_t mark);
	void remember_inf_mark(std::set<spot::acc_cond::mark_t::value_t> marks);

	// removes states unreachable from the initial states
	void remove_unreachable_states();

	// returns an edge ID that is a (mark-preserving or mark-discarding) product of given edges
	unsigned edge_product(unsigned e1, unsigned e2, bool preserve_mark_sets);

	// for the family of sets { M_1, ..., M_n } of edges,
	// returns set of products of each n edges from distinct M_i
	std::set<unsigned> product(std::set<std::set<unsigned>> edges_sets, bool preserve_mark_sets);

	~Automaton();
};

class SLAA : public Automaton<spot::formula> {
protected:
	spot::formula phi;

public:
	// each U-subformula has its own acceptance condition
	// (Fin(x) & (Fin(y_1) | ... | Fin(y_i))
	// x and y_i are stored in fin and fin_disj members
	// if such formula appears as a direct subformula of G,
	// the condition changes to (...) | Inf(z)
	// this z is stored in inf
	typedef struct {
		spot::acc_cond::mark_t::value_t fin;
		spot::acc_cond::mark_t::value_t inf;
		std::set<spot::acc_cond::mark_t::value_t> fin_disj;
	} acc_phi;

	// the set representation of resultant acceptance condition
	typedef std::set<std::set<std::set<std::pair<spot::acc_cond::mark_t::value_t, spot::acc_cond::mark_t::value_t>>>> ac_representation;

	// Spot structures
	spot::twa_graph_ptr spot_aut;
	spot::bdd_dict_ptr spot_bdd_dict;

	// getter for the input formula
	spot::formula get_input_formula() const;

	// acceptance formula of each Until state
	std::map<spot::formula, acc_phi> acc;

	// sets the Spot acceptance condition from acc
	void build_acc();

	// removes marks from non-looping transitions
	void remove_unnecessary_marks();

	// returns a set of initial configurations
	std::set<std::set<unsigned>> get_init_sets() const;

	// adds an initial configuration
	void add_init_set(std::set<unsigned> init_set);

	// converts the automaton to single-owner
	// the output argument tgba_mark_owners contains pairs of mark j and its owner q
	// such that all loops over q contain j as the only mark
	ac_representation mark_transformation(std::map<spot::acc_cond::mark_t::value_t, unsigned>& tgba_mark_owners);

	// prints the automaton in HOA format
	void print_hoaf();

	// prints the automaton in DOT format
	void print_dot();

	SLAA(spot::formula f);
};

class NA : public Automaton<unsigned> {
public:
	// merges edges with the same source and target
	void merge_edges();

	// merges states with the same outgoing transitions
	void merge_equivalent_states();

	// returns true if two given states are equivalent,
	// with the equivalence test from LTL2BA or LTL3BA
	bool states_equivalent(unsigned s1, unsigned s2, unsigned eq_level);

	// setter and getter of the init states are wrappers
	// over the structure of initial configurations
	// sets the init state
	void set_init_state(unsigned s);

	// returns the init state
	unsigned get_init_state() const;

	NA(std::vector<std::set<unsigned>>* sets);
};

#endif
