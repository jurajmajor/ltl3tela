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

#include "automaton.hpp"

template class Automaton<spot::formula>;
template class Automaton<unsigned>;

// Print handler for bdd_allsat
// Taken from LTL3BA source: https://sourceforge.net/projects/ltl3ba/
bool print_or = false;
void allsatPrintHandler(char* varset, int size) {
	if (print_or) {
		std::cout << " | ";
	}

	bool print_and = false;

	std::cout << "(";
	for (int v = 0; v < size; ++v) {
		if (varset[v] < 0) {
			continue;
		}
		if (print_and) {
			std::cout << " & ";
		}
		if (varset[v] == 0) {
			std::cout << "!";
		}
		std::cout << v;
		print_and = true;
	}
	if (!print_and) {
		// we still didn't print anything
		std::cout << "t";
	}
	std::cout << ")";
	print_or = true;
}

template<typename T> unsigned Automaton<T>::get_state_id(T f) {
	unsigned size = states.size();
	for (unsigned i = 0; i < size; ++i) {
		if (states[i] == f) {
			return i;
		}
	}

	states.push_back(f);
	state_edges.push_back(std::set<unsigned>());
	return size;
}

template<typename T> T Automaton<T>::state_name(unsigned state_id) {
	return states[state_id];
}

template<typename T> bool Automaton<T>::state_exists(T f) {
	return find(states.begin(), states.end(), f) != states.end();
}

template<typename T> unsigned Automaton<T>::states_count() {
	return states.size();
}

template<typename T> unsigned Automaton<T>::create_edge(bdd label) {
	auto e = new Edge(label);
	edges.push_back(e);

	return edges.size() - 1;
}

template<typename T> void Automaton<T>::add_edge(unsigned from, bdd label, std::set<unsigned> to, std::set<acc_mark> marks) {
	if (label == bddfalse) {
		return;
	}

	unsigned edge_id = create_edge(label);

	Edge* e_this = get_edge(edge_id);

	e_this->add_target(to);
	e_this->add_mark(marks);

	// the domination of transitions
	// we always do this for NA
	if (o_slaa_determ || spot_id_to_slaa_set != nullptr) {
		std::set<unsigned> edges_to_add;
		// we look at all other edges and check if the new edge dominates the other
		// NOP in the increment part as we sometimes increment the iterator with erase
		for (auto e_other_it = state_edges[from].begin(); e_other_it != state_edges[from].end(); /* NOP */) {
			auto e_other = get_edge(*e_other_it);

			int dom_level;
			if constexpr (!std::is_same<T, unsigned>::value) {
				dom_level = e_this->dominates(e_other, get_inf_marks());
			} else {
				dom_level = e_this->dominates(e_other,
					(*spot_id_to_slaa_set)[state_name(*(e_this->get_targets().begin()))],
					(*spot_id_to_slaa_set)[state_name(*(e_other->get_targets().begin()))],
					get_inf_marks()
				);
			}
			switch (dom_level) {
				case 1:
					// we are adding an edge that is equal in its targets and mark sets to e_other
					// remove e_other and add an edge labeled a1 | a2
					e_this->set_label(e_this->get_label() | e_other->get_label());
					state_edges[from].erase(e_other_it);
					add_edge(from, edge_id);

					add_edge(from, edges_to_add);
					return;
				case 2: {
					// we relabel each dominating edge
					// copy e_other and add it later
					auto f_id = create_edge(e_other->get_label() & bdd_not(e_this->get_label()));
					auto f = get_edge(f_id);
					f->add_target(e_other->get_targets());
					f->add_mark(e_other->get_marks());

					edges_to_add.insert(f_id);
				}
				// no break
				case 3:
					e_other_it = state_edges[from].erase(e_other_it);
				break;
				default:
					++e_other_it;
			}
		}

		// now we check if there exists some other edge that dominates our edge
		for (auto& e_other_id : state_edges[from]) {
			auto e_other = get_edge(e_other_id);
			int dom_level;
			if constexpr (!std::is_same<T, unsigned>::value) {
				dom_level = e_other->dominates(e_this, get_inf_marks());
			} else {
				dom_level = e_other->dominates(e_this,
					(*spot_id_to_slaa_set)[state_name(*(e_other->get_targets().begin()))],
					(*spot_id_to_slaa_set)[state_name(*(e_this->get_targets().begin()))],
					get_inf_marks()
				);
			}
			switch (dom_level) {
				case 3:
					// do not add
					// the new edge is killed by the existing edge
					add_edge(from, edges_to_add);
					return;
				case 2:
					// relabel the new edge
					e_this->set_label(e_this->get_label() & bdd_not(e_other->get_label()));
				// case 1 would happen in the previous for loop
			}
		}

		add_edge(from, edges_to_add);
	}

	state_edges[from].insert(edge_id);
}

template<typename T> void Automaton<T>::add_edge(unsigned from, unsigned edge_id) {
	auto orig = edges[edge_id];
	add_edge(from, orig->get_label(), orig->get_targets(), orig->get_marks());
}

template<typename T> void Automaton<T>::add_edge(unsigned from, std::set<unsigned> edge_ids) {
	for (auto& edge_id : edge_ids) {
		add_edge(from, edge_id);
	}
}

template<typename T> void Automaton<T>::remove_edge(unsigned state_id, unsigned edge_id) {
	state_edges[state_id].erase(edge_id);
}

template<typename T> std::set<acc_mark> Automaton<T>::get_inf_marks() const {
	return inf_marks;
}

template<typename T> void Automaton<T>::remember_inf_mark(acc_mark mark) {
	inf_marks.insert(mark);
}

template<typename T> void Automaton<T>::remember_inf_mark(std::set<acc_mark> marks) {
	inf_marks.insert(marks.begin(), marks.end());
}

std::set<std::set<unsigned>> SLAA::get_init_sets() const {
	return init_sets;
}

void SLAA::add_init_set(std::set<unsigned> init_set) {
	init_sets.insert(init_set);
}

unsigned NA::get_init_state() const {
	return *(init_sets.begin()->begin());
}

void NA::set_init_state(unsigned s) {
	init_sets = { { s } };
}

template<typename T> Edge* Automaton<T>::get_edge(unsigned edge_id) const {
	return edges[edge_id];
}

template<typename T> std::set<unsigned> Automaton<T>::get_state_edges(unsigned state_id) const {
	assert(state_edges.size() > state_id);
	return state_edges[state_id];
}

template<typename T> unsigned Automaton<T>::edge_product(unsigned e1, unsigned e2, bool preserve_ixsets) {
	auto e = new Edge(edges[e1]->get_label() & edges[e2]->get_label());
	edges.push_back(e);
	e->add_target(edges[e1]->get_targets());
	e->add_target(edges[e2]->get_targets());

	if (preserve_ixsets) {
		e->add_mark(edges[e1]->get_marks());
		e->add_mark(edges[e2]->get_marks());
	}

	return edges.size() - 1;
}

// returns a set of edges in the product
template<typename T> std::set<unsigned> Automaton<T>::product(std::set<std::set<unsigned>> edges_sets, bool preserve_ixsets) {
	if (edges_sets.empty()) {
		// this is not a correct value for product of empty set
		// in NA, a ∅ state is true and should contain a loop
		// however, a source state is not an argument of product
		// so we have to handle this outside of this method
		return std::set<unsigned>();
	}

	auto s0 = *(edges_sets.begin());

	if (edges_sets.size() == 1) {
		return s0;
	}

	edges_sets.erase(edges_sets.begin());
	auto s1 = product(edges_sets, preserve_ixsets);

	std::set<unsigned> result;

	for (auto& e0 : s0) {
		for (auto& e1 : s1) {

			unsigned p = edge_product(e0, e1, preserve_ixsets);
			if (get_edge(p)->get_label() != bdd_false()) {
				result.insert(p);
			}
		}
	}

	return result;
}

void SLAA::build_acc() {
	// auto& ac = slaa->spot_aut->acc();
	for (auto& ac : acc) {
		// Fin(x)
		auto fin_acc = spot::acc_cond::acc_code::t();
		fin_acc &= fin_acc.fin(spot::acc_cond::mark_t({ ac.second.fin }));

		// Fin(y_1) | ... | Fin(y_n)
		if (!ac.second.fin_disj.empty()) {
			auto sub_fin_acc = spot::acc_cond::acc_code::f();
			for (auto& j : ac.second.fin_disj) {
				sub_fin_acc |= sub_fin_acc.fin(spot::acc_cond::mark_t({ j }));
			}

			fin_acc &= sub_fin_acc;
		}

		// do we have Inf(z)?
		if (ac.second.inf == -1U) {
			spot_aut->acc().set_acceptance(spot_aut->acc().get_acceptance() & fin_acc);
		} else {
			auto fin_inf_acc = fin_acc;
			fin_inf_acc |= fin_inf_acc.inf(spot::acc_cond::mark_t({ ac.second.inf }));
			spot_aut->acc().set_acceptance(spot_aut->acc().get_acceptance() & fin_inf_acc);
		}
	}
}

std::set<std::set<acc_mark>> SLAA::get_minimal_models_of_acc_cond() const {
	std::set<std::set<acc_mark>> models = {{}};
	for (auto& ac : acc) {
		// for each conjunct, either add Inf, or "large" Fin + one Fin from disjunction (if exists)
		std::set<std::set<acc_mark>> options;
		if (ac.second.fin_disj.empty()) {
			options.insert(std::set<acc_mark>({ ac.second.fin }));
		} else {
			for (auto fd : ac.second.fin_disj) {
				options.insert(std::set<acc_mark>({ ac.second.fin, fd }));
			}
		}
		if (ac.second.inf != -1U) {
			options.insert(std::set<acc_mark>({ ac.second.inf }));
		}

		// extend existing models with all of the options
		std::set<std::set<acc_mark>> new_models;
		for (auto& m : models) {
			for (auto o : options) {
				o.insert(std::begin(m), std::end(m));
				new_models.insert(o);
			}
		}
		models = new_models;
	}
	return models;
}

void SLAA::apply_extended_domination() {
	const auto& mm = get_minimal_models_of_acc_cond();
	for (unsigned state_id = 0, states_count = states.size(); state_id < states_count; ++state_id) {
		for (auto e1_id : state_edges[state_id]) {
			auto e1 = get_edge(e1_id);

			for (auto e2_it = std::begin(state_edges[state_id]); e2_it != std::end(state_edges[state_id]); /* NOP */) {
				if (e1_id == *e2_it) {
					++e2_it;
					continue; // do not check dominance of edge over itself
				}

				auto e2 = get_edge(*e2_it);

				auto o1 = e1->get_targets();
				auto o2 = e2->get_targets();

				auto j1 = e1->get_marks();
				auto j2 = e2->get_marks();

				// e1 dominates e2 iff
				// 1. e1.targets ⊆ e2.targets
				// 2. ∀ minimal model M of Φ:
				// 2a. M ∩ Fin(Φ) ∩ e2.marks = ∅ => M ∩ Fin(Φ) ∩ e1.marks = ∅
				// 2b. M ∩ Inf(Φ) ∩ e1.marks = ∅ => M ∩ Inf(Φ) ∩ e2.marks = ∅
				bool dominates = false;
				if (std::includes(std::begin(o2), std::end(o2), std::begin(o1), std::end(o1))) { // condition (1) holds
					const auto& inf_marks = get_inf_marks();

					dominates = true; // we may rewrite this again later

					for (const auto& model : mm) {
						std::set<acc_mark> m_e1;
						std::set<acc_mark> m_e2;
						std::set_intersection(std::begin(model), std::end(model), std::begin(j1), std::end(j1), std::inserter(m_e1, std::begin(m_e1)));
						std::set_intersection(std::begin(model), std::end(model), std::begin(j2), std::end(j2), std::inserter(m_e2, std::begin(m_e2)));

						bool m_fin_in_e1 = false;
						bool m_fin_in_e2 = false;
						bool m_inf_in_e1 = false;
						bool m_inf_in_e2 = false;

						for (auto i : m_e1) {
							if (std::find(std::begin(inf_marks), std::end(inf_marks), i) == std::end(inf_marks)) {
								m_fin_in_e1 = true;
							} else {
								m_inf_in_e1 = true;
							}
						}

						for (auto i : m_e2) {
							if (std::find(std::begin(inf_marks), std::end(inf_marks), i) == std::end(inf_marks)) {
								m_fin_in_e2 = true;
							} else {
								m_inf_in_e2 = true;
							}
						}

						if (!m_fin_in_e2 && m_fin_in_e1 || !m_inf_in_e1 && m_inf_in_e2) {
							dominates = false;
							break;
						}
					}
				}

				if (dominates) {
					// e1 dominates e2 so we restrict e2's label
					bdd e2_new_label = e2->get_label() & bdd_not(e1->get_label());

					if (e2_new_label == bddfalse) {
						e2_it = state_edges[state_id].erase(e2_it);
					} else {
						e2->set_label(e2_new_label);
						++e2_it;
					}
				} else {
					++e2_it;
				}
			}
		}
	}
}

spot::formula SLAA::get_input_formula() const {
	return phi;
}

template<typename T> void Automaton<T>::remove_unreachable_states() {
	std::map<unsigned, unsigned> conversion_table;
	std::queue<unsigned> bfs_queue;
	unsigned reachable_state_ct = 0;

	std::set<std::set<unsigned>> new_init_sets;

	for (auto& init_set : init_sets) {
		std::set<unsigned> new_init_set;
		for (auto& init_state : init_set) {
			if (conversion_table.count(init_state) == 0) {
				bfs_queue.push(init_state);

				conversion_table.insert(std::make_pair(init_state, reachable_state_ct));

				++reachable_state_ct;
			}
			new_init_set.insert(conversion_table[init_state]);
		}
		new_init_sets.insert(new_init_set);
	}

	init_sets = new_init_sets;

	while (!bfs_queue.empty()) {
		unsigned state_id = bfs_queue.front();
		bfs_queue.pop();

		for (auto& edge_id : get_state_edges(state_id)) {
			auto edge = get_edge(edge_id);
			auto targets = edge->get_targets();
			std::set<unsigned> new_target_set;
			for (auto& target_id : targets) {
				unsigned new_target_id;
				if (conversion_table.count(target_id) == 0) {
					// we didn't explore this state yet
					// we'll assign a new ID and add it to BFS queue
					new_target_id = reachable_state_ct++;
					conversion_table.insert(std::make_pair(target_id, new_target_id));
					bfs_queue.push(target_id);
				} else {
					new_target_id = conversion_table[target_id];
				}

				new_target_set.insert(new_target_id);
			}
			// we replace target IDs with converted ones
			edge->replace_target_set(new_target_set);
		}
	}

	unsigned conv_table_size = conversion_table.size();

	// replace state tables with a new one
	std::vector<T> new_state_table(conv_table_size);
	std::vector<std::set<unsigned>> new_state_edges_table(conv_table_size);

	for (auto& rec : conversion_table) {
		new_state_table[rec.second] = states[rec.first];
		new_state_edges_table[rec.second] = state_edges[rec.first];
	}

	states = new_state_table;
	state_edges = new_state_edges_table;
}

void SLAA::add_edge(unsigned from, bdd label, std::set<unsigned> to, std::set<acc_mark> marks) {
	std::set<unsigned> to_(to);
	for (auto& kv : dom_states) {
		if (to_.find(kv.first) != std::end(to_)) {
			for (auto& s : kv.second) {
				to_.erase(s);
			}
		}
	}
	Automaton<spot::formula>::add_edge(from, label, to_, marks);
}

void SLAA::add_edge(unsigned from, unsigned edge_id) {
	auto orig = edges[edge_id];
	add_edge(from, orig->get_label(), orig->get_targets(), orig->get_marks());
}

void SLAA::add_edge(unsigned from, std::set<unsigned> edge_ids) {
	for (auto& edge_id : edge_ids) {
		add_edge(from, edge_id);
	}
}

// removes all marks on non-loops
void SLAA::remove_unnecessary_marks() {
	for (unsigned state_id = 0, states_count = states.size(); state_id < states_count; ++state_id) {
		for (auto& edge_id : state_edges[state_id]) {
			// check if this is not a loop
			if (get_edge(edge_id)->get_targets().count(state_id) == 0) {
				get_edge(edge_id)->clear_marks();
			}
		}
	}
}

SLAA::ac_representation SLAA::mark_transformation(std::map<acc_mark, unsigned>& tgba_mark_owners) {
	// get a set of all Inf marks; also remember marks having escaping Inf
	std::map<acc_mark, bool> inf_marks;
	std::map<acc_mark, acc_mark> orig_sibling_of;
	for (auto& ac : acc) {
		if (ac.second.inf != -1U) {
			inf_marks[ac.second.inf] = true;

			orig_sibling_of[ac.second.fin] = ac.second.inf;
			for (auto& f : ac.second.fin_disj) {
				orig_sibling_of[f] = ac.second.inf;
			}
		}
	}

	// map of escaping marks
	std::map<acc_mark, acc_mark> sibling_of;

	// this maps each old mark to map { state => 'new mark for this state' }
	std::map<acc_mark, std::map<unsigned, acc_mark>> mark_owners;

	for (unsigned state_id = 0, states_count = states.size(); state_id < states_count; ++state_id) {
		std::map<acc_mark, acc_mark> marks_to_escape;

		for (auto& edge_id : state_edges[state_id]) {
			auto edge = get_edge(edge_id);

			std::set<acc_mark> new_edge_marks;
			for (auto& mark : edge->get_marks()) {

				// is this the first time we see this mark?
				if (mark_owners.count(mark) == 0) {
					// yes, we can assign this mark to our state
					mark_owners[mark].insert(std::make_pair(state_id, mark));
				} else {
					// no; check if we have already seen the mark on current state
					if (mark_owners[mark].count(state_id) == 0) {
						// no so create a new mark
						mark_owners[mark][state_id] = spot_aut->acc().add_set();
					}
				}

				if (inf_marks.count(mark) == 0) {
					// this is not an Inf mark so we need to escape it
					marks_to_escape.insert(std::make_pair(mark_owners[mark][state_id], mark));
				}

				new_edge_marks.insert(mark_owners[mark][state_id]);
			}

			// clear all existing marks and add the new ones
			edge->clear_marks();
			edge->add_mark(new_edge_marks);
		}

		for (auto rec : marks_to_escape) {
			auto mark = rec.first;

			// create j' for j; if there was an escaping Inf because of G, use it
			if (orig_sibling_of.count(rec.second) > 0) {
				// now mark_owners[orig_sibling_of[rec.second]][state_id] should be our escaping Inf
				// just to be sure: if we haven't seen this Inf on our state,
				// do the same procedure as for Fin marks
				if (mark_owners.count(orig_sibling_of[rec.second]) == 0) {
					mark_owners[orig_sibling_of[rec.second]].insert(std::make_pair(state_id, orig_sibling_of[rec.second]));
				} else {
					if (mark_owners[orig_sibling_of[rec.second]].count(state_id) == 0) {
						mark_owners[orig_sibling_of[rec.second]][state_id] = spot_aut->acc().add_set();
					}
				}

				sibling_of[mark] = mark_owners[orig_sibling_of[rec.second]][state_id];
			} else {
				// create a new escaping Inf
				sibling_of[mark] = spot_aut->acc().add_set();
			}
		}

		// now let's go once again through edges
		// mark each non-looping transition with marks_to_escape
		// sibling_of remembers the escaping mark
		// also check if all loops share the same and only mark
		unsigned shared_set_mark = 0;
		// mark_found_level:
		// 0 = not a single mark has been found
		// 1 = exactly one mark has been found
		// 2 = give up, one edge has 0 or more than 1 mark
		unsigned mark_found_level = 0;
		for (auto& edge_id : state_edges[state_id]) {
			auto edge = get_edge(edge_id);

			if (edge->get_targets().count(state_id) == 0) {
				// this is not a loop

				for (auto rec : marks_to_escape) {
					// now we can escape j with j'
					edge->add_mark(sibling_of[rec.first]);
				}
			} else if (o_ac_filter_fin) {
				if (mark_found_level == 0) {
					if (edge->get_marks().size() == 1) {
						mark_found_level = 1;
						shared_set_mark = *(edge->get_marks().begin());
					} else {
						mark_found_level = 2;
					}
				} else if (mark_found_level == 1 && (edge->get_marks().size() != 1 || shared_set_mark != *(edge->get_marks().begin()))) {
					mark_found_level = 2;
				}
			}
		}

		if (mark_found_level == 1) {
			tgba_mark_owners[shared_set_mark] = state_id;
		}
	}

	// now represent the acceptance condition
	ac_representation acr;

	// iterate through original acc_phi structures
	for (auto& ac : acc) {
		// these are 2 conjuncts for AC
		std::set<std::set<std::pair<acc_mark, acc_mark>>> fin_xs_disj;
		std::set<std::set<std::pair<acc_mark, acc_mark>>> fin_ys_disj;

		// Fin(x) -> (Fin(x_s1) & ... & Fin(x_sn)) plus their escaping Infs
		std::set<std::pair<acc_mark, acc_mark>> fin_xs;

		for (auto& state_mark : mark_owners[ac.second.fin]) {
			fin_xs.insert(std::make_pair(state_mark.second, sibling_of[state_mark.second]));
			remember_inf_mark(sibling_of[state_mark.second]);
		}
		fin_xs_disj.insert(fin_xs);


		// the same for each Fin(y_i)

		for (auto& y : ac.second.fin_disj) {
			// Fin(y_i1) & ... & Fin(y_ij)
			std::set<std::pair<acc_mark, acc_mark>> fin_ys;

			for (auto& state_mark : mark_owners[y]) {
				fin_ys.insert(std::make_pair(state_mark.second, sibling_of[state_mark.second]));
				remember_inf_mark(sibling_of[state_mark.second]);
			}

			fin_ys_disj.insert(fin_ys);
		}

		acr.insert(fin_xs_disj);
		acr.insert(fin_ys_disj);
	}

	return acr;
}

void SLAA::register_dom_states(unsigned strong, unsigned weak, unsigned option_level) {
	if (o_slaa_trans_red & option_level) {
		dom_states[strong].insert(weak);
	}
}

void SLAA::print_hoaf() {
	bool sink_state_needed = false;
	bool true_state_exists = false;
	unsigned sink_state_id;

	unsigned state_counter = 0;
	for (auto& edges_list : state_edges) {
		if (states[state_counter].is_tt()) {
			// we have a state for true: use this as a sink state
			sink_state_id = state_counter;
			true_state_exists = true;
		}

		for (auto& edge_id : edges_list) {
			if (get_edge(edge_id)->get_targets().size() == 0) {
				// this edge has empty set of targets
				sink_state_needed = true;
			}
		}

		++state_counter;
	}

	sink_state_needed = sink_state_needed && !true_state_exists;

	if (sink_state_needed) {
		// we need a new virtual sink state
		sink_state_id = state_counter;
	}

	spot::tl_simplifier simp;

	std::cout << "HOA: v1\n";
	std::cout << "tool: \"LTL3TELA\"\n";
	std::cout << "name: \"SLAA for " << spot::unabbreviate(simp.simplify(phi), "WM") << "\"\n";
	std::cout << "States: " << (sink_state_needed ? state_counter + 1 : state_counter) << '\n'; // + 1 is for sink state

	auto bdd_dict = spot_aut->ap();
	unsigned bdd_dict_size = bdd_dict.size();
	std::cout << "AP: " << bdd_dict_size;
	for (unsigned i = 0; i < bdd_dict_size; ++i) {
		std::cout << " \"" << bdd_dict[i] << '"';
	}
	std::cout << '\n';

	// initial states
	for (auto& init_set : init_sets) {
		std::cout << "Start: ";
		bool target_printed = false;
		for (auto& target_id : init_set) {
			if (target_printed) {
				std::cout << "&";
			}
			std::cout << target_id;
			target_printed = true;
		}
		std::cout << std::endl;
	}

	// acceptance condition
	std::cout << "Acceptance: " << spot_aut->acc().num_sets() << ' ';
	spot_aut->acc().get_acceptance().to_text(std::cout);

	std::cout << "\n--BODY--\n";
	for (unsigned state_id = 0, state_count = states.size(); state_id < state_count; ++state_id) {
		std::cout << "State: " << state_id << " \"" << spot::unabbreviate(simp.simplify(states[state_id]), "WM") << "\"\n";
		// for every edge of this state
		for (auto& edge_id : state_edges[state_id]) {
			Edge* edge = edges[edge_id];

			std::cout << "  [";
			print_or = false;
			bdd_allsat(edge->get_label(), allsatPrintHandler);
			std::cout << "] ";

			bool target_printed = false;
			auto targets = edge->get_targets();

			for (auto& target_id : targets) {
				if (target_printed) {
					std::cout << "&";
				}
				std::cout << target_id;
				target_printed = true;
			}

			// this edge leads to empty set
			if (!target_printed) {
				std::cout << sink_state_id;
			}

			auto marks = edge->get_marks();
			if (!marks.empty()) {
				std::cout << " {";
				bool mark_printed = false;

				for (auto& mark : marks) {
					if (mark_printed) {
						std::cout << ' ';
					}
					std::cout << mark;
					mark_printed = true;
				}

				std::cout << '}';
			}

			std::cout << "\n";
		}
	}

	if (sink_state_needed) {
		// we print the sink state with the true edge
		std::cout << "State: " << state_counter << " \"t\"\n  [t] " << state_counter << "\n";
	}

	std::cout << "--END--\n";
}

void SLAA::print_dot() {
	std::cout << "digraph G {\n\trankdir=LR\n";

	std::string init_state_style("[label=\"\", style=invis, width=0]");
	std::string split_alt_edge_style("[label=\"\", peripheries=0, width=0.02, height=0.02, style=filled, fillcolor=black]");

	std::vector<std::string> colors = {
		"red", "blue", "green", "yellow"
	};

	std::vector<std::string> bullets = {
		"⓿", "❶", "❷", "❸", "❹", "❺", "❻",
		"❼", "❽", "❾", "❿", "⓫", "⓬", "⓭",
		"⓮", "⓯", "⓰", "⓱", "⓲", "⓳", "⓴"
	};

	// initial states
	unsigned int init_i = 0;
	for (auto& init_set : init_sets) {
		std::cout << "\tI" << init_i << " " << init_state_style << '\n';

		bool alternating_edge = init_set.size() >= 2;
		if (alternating_edge) {
			// create invisible node that splits edge - marked InX
			std::cout << "\tI" << init_i << "X " << split_alt_edge_style << '\n';
			std::cout << "\tI" << init_i << " -> I" << init_i << "X [dir=none]\n";
		}

		for (auto& init_state_id : init_set) {
			std::cout << "\tI" << init_state_id << (alternating_edge ? "X" : "") << " -> " << init_state_id << '\n';
		}

		++init_i;
	}

	spot::tl_simplifier simp;

	// through every state
	unsigned int empty_targets_ct = 0;
	for (unsigned state_id = 0, state_count = states.size(); state_id < state_count; ++state_id) {
		// print state
		std::cout << "\t" << state_id << " [label=\"" << state_id << " | " << spot::unabbreviate(simp.simplify(states[state_id]), "WM") << "\", peripheries=1]\n";

		// and its edges
		auto edges = get_state_edges(state_id);
		unsigned alt_edges_ct = 0;
		for (auto& edge_id : edges) {
			auto edge = get_edge(edge_id);

			auto targets = edge->get_targets();
			auto marks = edge->get_marks();

			unsigned targets_count = targets.size();

			std::stringstream ac;
			if (!marks.empty()) {
				ac << "<br />{";
				bool ixset_out = false;
				for (auto& mark : marks) {
					if (ixset_out) {
						ac << ", ";
					}
					ixset_out = true;
					ac << "<font color=\"" << colors[mark % colors.size()] << "\">" << (mark > 20 ? std::to_string(mark) : bullets[mark]) << "</font>";
				}
				ac << "}";
			}

			if (targets_count >= 2) {
				// alternating edge
				std::cout << "\tX" << state_id << alt_edges_ct << " " << split_alt_edge_style << '\n';
				std::cout << "\t" << state_id << " -> X" << state_id << alt_edges_ct << " [label=<";
				auto label = spot::bdd_format_formula(spot_bdd_dict, edge->get_label());
				spot::escape_html(std::cout, label);
				std::cout << ac.str() << ">, dir=none]\n";

				for (auto& target : targets) {
					std::cout << "\tX" << state_id << alt_edges_ct << (state_id == target ? ":s" : "") << " -> " << target << " [label=\"\"]\n";
				}

				++alt_edges_ct;
			} else if (targets_count == 1) {
				std::cout << "\t" << state_id << " -> " << *(targets.begin()) << " [label=<";
				auto label = spot::bdd_format_formula(spot_bdd_dict, edge->get_label());
				spot::escape_html(std::cout, label);
								std::cout << ac.str() << ">]\n";
			} else {
				// this edge has no targets - create invisible state
				std::cout << "\tV" << empty_targets_ct << " " << init_state_style << '\n';
				std::cout << "\t" << state_id << " -> V" << empty_targets_ct << " [label=<";
				auto label = spot::bdd_format_formula(spot_bdd_dict, edge->get_label());
				spot::escape_html(std::cout, label);
				std::cout << ac.str() << ">]\n";

				++empty_targets_ct;
			}
		}
	}

	std::cout << "\tlabel=\"";
	spot_aut->acc().get_acceptance().to_text(std::cout);
	std::cout << "\"\n\tlabelloc=bottom\n";
	std::cout << "\tlabeljust=right\n";
	std::cout << "}\n";
}

SLAA::SLAA(spot::formula f, spot::bdd_dict_ptr dict) {
	spot_bdd_dict = dict ? dict : spot::make_bdd_dict();
	spot_aut = spot::make_twa_graph(spot_bdd_dict);

	phi = f;
}

NA::NA(std::vector<std::set<unsigned>>* sets) {
	spot_id_to_slaa_set = sets;
}

// inspired by spot's twa_graph::merge_edges
// merges edges with same source and destination
// mark sets J1 and J2 must satisfy
// 1) either J1 = J2
// 2) or each mark in J1 and J2 are Inf marks and labels are equal
// the resulting label is disjunction of labels
// and mark set is union of mark sets
void NA::merge_edges() {
	// set of edges IDs that are removed and we should ignore them
	std::set<unsigned> removed_edges;

	for (unsigned state_id = 0, states_no = states_count(); state_id < states_no; ++state_id) {
		auto state_edges = get_state_edges(state_id);
		std::vector<unsigned> edges_list(state_edges.begin(), state_edges.end());

		for (unsigned i = 0; i < edges_list.size(); ++i) {
			auto e1_id = edges_list[i];

			if (removed_edges.count(e1_id) > 0) {
				// this is not valid anymore
				continue;
			}

			auto e1 = get_edge(e1_id);

			for (unsigned j = i + 1; j < edges_list.size(); ++j) {
				auto e2_id = edges_list[j];

				if (removed_edges.count(e2_id) > 0) {
					// this is not valid anymore
					continue;
				}

				auto e2 = get_edge(e2_id);

				auto j1 = e1->get_marks();
				auto j2 = e2->get_marks();

				auto l1 = e1->get_label();
				auto l2 = e2->get_label();

				if (e1->get_targets() == e2->get_targets()) {
					// target sets are equal
					if (j1 == j2) {
						// acceptance labels too; join these edges
						e1->set_label(e1->get_label() | e2->get_label());
						removed_edges.insert(e2_id);
						remove_edge(state_id, e2_id);
					} else if (l1 == l2) {
						// are all marks from J1 and J2 Inf marks?
						bool merge_fail = false;

						for (auto mark : j1) {
							if (inf_marks.count(mark) == 0) {
								merge_fail = true;
								break;
							}
						}
						if (!merge_fail) {
							for (auto mark : j2) {
								if (inf_marks.count(mark) == 0) {
									merge_fail = true;
									break;
								}
							}
						}

						if (!merge_fail) {
							// join these edges
							e1->add_mark(j2);
							e1->set_label(l1 | l2);
							removed_edges.insert(e2_id);
							remove_edge(state_id, e2_id);
						}
					}
				}
			}
		}
	}
}

void NA::merge_equivalent_states() {
	unsigned states_size = states_count();

	for (unsigned s1 = 0; s1 < states_size; ++s1) {
		for (unsigned s2 = s1 + 1; s2 < states_size; ++s2) {
			// first try the basic check δ(q1) = δ(q2)
			bool st_equiv = states_equivalent(s1, s2, 1);

			// if it failed and we can test δ(q1)[q1/r] = δ(q2)[q2/r], do it
			if (!st_equiv && o_eq_level == 2) {
				st_equiv = states_equivalent(s1, s2, 2);
			}

			if (st_equiv) {
				// retarget each s2-transition to s1
				for (unsigned state_id = 0; state_id < states_size; ++state_id) {
					for (auto& edge_id : get_state_edges(state_id)) {
						auto edge = get_edge(edge_id);

						if (*(edge->get_targets().begin()) == s2) {
							edge->replace_target_set(std::set<unsigned>({ s1 }));
						}
					}
				}

				// if s2 was an initial state, s1 is the new one
				if (get_init_state() == s2) {
					set_init_state(s1);
				}
			}
		}
	}
}

bool NA::states_equivalent(unsigned s1, unsigned s2, unsigned eq_level) {
	auto s1_edges = get_state_edges(s1);
	auto s2_edges = get_state_edges(s2);

	// do the edges sets have equal size?
	if (s1_edges.size() != s2_edges.size()) {
		return false;
	}

	// for each edge of s1 find the corresponding edge of s2
	std::set<unsigned> used_edge_ids;

	for (auto& e1_id : s1_edges) {
		auto e1 = get_edge(e1_id);

		bool corresponding_edge_found = false;
		for (auto& e2_id : s2_edges) {
			if (used_edge_ids.count(e2_id) > 0) {
				continue;
			}

			auto e2 = get_edge(e2_id);

			// do the edges have equal transition label and acceptance label?
			if (e1->get_label() != e2->get_label()) {
				continue;
			}

			if (e1->get_marks() != e2->get_marks()) {
				continue;
			}

			// t1 and t2 are the only targets of our edges
			// we can simply test δ(s1) = δ(s2), or, if we opted for it,
			// check if δ(s1)[s1/r] = δ(s2)[s2/r] for a fresh state r
			auto t1 = *(e1->get_targets().begin());
			auto t2 = *(e2->get_targets().begin());

			// that is, for this test to fail, either targets are not equal,
			// or at least one of {t1, t2} is not a loop
			if (t1 != t2 && (eq_level != 2 || s1 != t1 || s2 != t2)) {
				continue;
			}

			// if we got here, e1 and e2 are equivalent
			used_edge_ids.insert(e2_id);
			corresponding_edge_found = true;
			break;
		}

		if (!corresponding_edge_found) {
			return false;
		}
	}

	// now the states are equivalent
	return true;
}

template<typename T> Automaton<T>::~Automaton() {
	for (auto e : edges) {
		delete e;
	}
	edges.clear();
}

Edge::Edge(bdd l) {
	set_label(l);
}

bdd Edge::get_label() const {
	return label;
}

void Edge::set_label(bdd l) {
	label = l;
}

void Edge::add_target(unsigned state_id) {
	targets.insert(state_id);
}

void Edge::add_target(std::set<unsigned> state_ids) {
	targets.insert(state_ids.begin(), state_ids.end());
}

void Edge::remove_target(unsigned state_id) {
	auto target_it = targets.find(state_id);
	if (target_it != targets.end()) {
		targets.erase(target_it);
	}
}

void Edge::replace_target_set(std::set<unsigned> state_ids) {
	targets.clear();
	add_target(state_ids);
}

void Edge::add_mark(unsigned ix) {
	marks.insert(ix);
}

void Edge::add_mark(std::set<unsigned> ixs) {
	marks.insert(ixs.begin(), ixs.end());
}

void Edge::remove_mark(unsigned ix) {
	auto mark_it = marks.find(ix);
	if (mark_it != marks.end()) {
		marks.erase(mark_it);
	}
}

void Edge::clear_marks() {
	marks.clear();
}

std::set<unsigned> Edge::get_targets() const {
	return targets;
}

std::set<unsigned> Edge::get_marks() const {
	return marks;
}

// returns domination level
// if O1 = O2 & J1 = J2 then returns 1
// else if O1 ⊆ O2 & J1 ⊆ J2 but at least one inclusion is proper then
//   returns 3 if a2 ⊆ a1
//   else returns 2
// else returns 0
// where edge 1 is this edge and edge 2 is the other edge
int Edge::dominates(Edge* other, std::set<acc_mark> inf_marks) const {
	auto o1 = get_targets();
	auto j1 = get_marks();

	auto o2 = other->get_targets();
	auto j2 = other->get_marks();

	// now check if O1 ⊆ O2 & J1 ⊆ J2
	if (!std::includes(o2.begin(), o2.end(), o1.begin(), o1.end())) {
		return 0;
	}

		// each not-Inf mark in J1 has to be in J2
		for (auto mark : j1) {
			if (inf_marks.count(mark) == 0 && j2.count(mark) == 0) {
				return 0;
			}
		}

		// and each Inf mark in J2 has to be in J1
		for (auto mark : j2) {
			if (inf_marks.count(mark) > 0 && j1.count(mark) == 0) {
				return 0;
			}
		}

	// is some inclusion proper?
	if (o1.size() == o2.size() && j1 == j2) {
		// nope
		return 1;
	} else {
		return ((other->get_label() & bdd_not(get_label())) == bdd_false()) ? 3 : 2;
	}
}

int Edge::dominates(Edge* other, std::set<unsigned> o1, std::set<unsigned> o2, std::set<acc_mark> inf_marks) const {
	// t1 kills t2 if O1 ⊆ O2 & a2 => a1
	auto j1 = get_marks();
	auto j2 = other->get_marks();

	if (std::includes(o2.begin(), o2.end(), o1.begin(), o1.end())
		&& (other->get_label() & bdd_not(get_label())) == bdd_false()
	) {
		// each not-Inf mark in J1 has to be in J2
		for (auto mark : j1) {
			if (inf_marks.count(mark) == 0 && j2.count(mark) == 0) {
				return 0;
			}
		}

		// and each Inf mark in J2 has to be in J1
		for (auto mark : j2) {
			if (inf_marks.count(mark) > 0 && j1.count(mark) == 0) {
				return 0;
			}
		}

		return 3;
	} else {
		return 0;
	}
}
