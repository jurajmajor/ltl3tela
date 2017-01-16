/*
    Copyright (c) 2016 Juraj Major

    This file is part of LTL3HOA.

    LTL3HOA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LTL3HOA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LTL3HOA.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nondeterministic.hpp"

// Returns the id for a set of SLAA states
// It creates a new state if not present
unsigned get_state_id_for_set(spot::twa_graph_ptr aut, std::set<unsigned> state_set) {
	auto sets = aut->get_named_prop<std::vector<std::set<unsigned>>>("state-sets");

	for (unsigned i = 0; i < sets->size(); ++i) {
		std::set<unsigned> candidate = (*sets)[i];
		if (candidate == state_set) {
			return i;
		}
	}

	unsigned i = aut->new_state();
	if (i != sets->size()) {
		throw "Unexpected index.";
	} else {
		sets->push_back(state_set);
		return i;
	}
}

// Returns a string representation of a set
std::string set_to_str(std::set<unsigned> set) {
	std::string name = "{";
	for (auto &state : set) {
		 name += std::to_string(state);
		 name += ",";
	}
	if (name[name.size() - 1] == ',') {
		name[name.size() - 1] = '}';
	} else {
		name += "}";
	}
	return name;
}

// Converts a given SLAA to NA
spot::twa_graph_ptr make_nondeterministic(SLAA* slaa) {
	unsigned last_inserted = 0;

	// create an empty automaton
	spot::bdd_dict_ptr dict = spot::make_bdd_dict();
	spot::twa_graph_ptr aut = make_twa_graph(dict);
	// copy the APs from SLAA
	aut->copy_ap_of(slaa->spot_aut);
	// set the name of automaton
	spot::tl_simplifier simp;
	aut->set_named_prop("automaton-name", new std::string(str_psl(spot::unabbreviate(simp.simplify(slaa->get_input_formula()), "WM"))));

	// create a map of names
	auto sets = new std::vector<std::set<unsigned>>;
	aut->set_named_prop<std::vector<std::set<unsigned>>>("state-sets", sets);

	// a map { mark => SLAA state } of Fin-marks removed from NA
	// filled only if -t flag is active
	std::map<spot::acc_cond::mark_t::value_t, unsigned> tgba_mark_owners;
	// acr is a representation of the final acceptance condition
	auto acr = slaa->mark_transformation(tgba_mark_owners);

	auto& ac = aut->acc();

	std::queue<unsigned> q;

	NA* nha = new NA(sets);
	// copy the Inf-marks from SLAA
	nha->remember_inf_mark(slaa->get_inf_marks());
	// put initial configurations into queue, create states
	// and link them to the corresponding set
	std::set<unsigned> na_init_states;

	for(auto& init_set : slaa->get_init_sets()) {
		auto index = get_state_id_for_set(aut, init_set);

		q.push(index);
		// ignore the return value, just make sure we create the state
		nha->get_state_id(index);
		na_init_states.insert(index);
		last_inserted = index;
	}

	// map { mark => set of owner SLAA states } of Fin-marks removed from NA
	std::map<spot::acc_cond::mark_t::value_t, std::set<unsigned>> removed_fin_marks;

	// map { mark => mark } of the siblings of removed Fin-marks
	std::map<spot::acc_cond::mark_t::value_t, spot::acc_cond::mark_t::value_t> sibling_of_removed_fin;
	for (auto& disj : acr) {
		auto disj_f = spot::acc_cond::acc_code::f();

		for (auto& conj : disj) {
			for (auto& pair : conj) {
				if (tgba_mark_owners.count(pair.first) > 0) {
					sibling_of_removed_fin.insert(pair);
				}
			}
		}
	}

	// while the queue is not empty, create a state using the subset construction
	while(!q.empty()) {
		auto source_id = q.front();
		q.pop();
		std::set<unsigned> source_sets = (*sets) [source_id];

		if (source_sets.size() == 0) {
			// if the state is ∅, add a true loop
			nha->add_edge(nha->get_state_id(source_id), bdd_true(), std::set<unsigned>({ nha->get_state_id(source_id) }));
		} else {
			// count the product
			std::set<std::set<unsigned>> edges_for_product;
			for (auto& state_id : source_sets) {
				edges_for_product.insert(slaa->get_state_edges(state_id));
			}

			std::set<unsigned> product_edges = slaa->product(edges_for_product, true);

			// check each successor and if needed, create a new state and add to queue
			for (auto& edge_id : product_edges) {
				auto label = slaa->get_edge(edge_id)->get_label();
				// do not add the false edges
				if (label == bddfalse) {
					continue;
				}

				std::set<unsigned> targets = slaa->get_edge(edge_id)->get_targets();

				// creates state if not existe for given set
				unsigned target_id = get_state_id_for_set(aut, targets);
				if (target_id > last_inserted) {
					last_inserted = target_id;
					q.push(target_id);
				}

				auto marks = slaa->get_edge(edge_id)->get_marks();
				nha->add_edge(nha->get_state_id(source_id), label, std::set<unsigned>({ nha->get_state_id(target_id) }), marks);
			}
		}
	}

	// do we have more than one init state?
	// if so, we'll merge them to one new state
	unsigned spot_init_state_id = 0;
	unsigned nha_init_state_id = 0;

	if (na_init_states.size() > 1) {
		spot_init_state_id = aut->new_state();
		nha_init_state_id = nha->get_state_id(spot_init_state_id);

		for (auto old_init_state : na_init_states) {
			// each transition of former initial state is copied
			for (auto edge_id : nha->get_state_edges(old_init_state)) {
				nha->add_edge(nha_init_state_id, edge_id);
			}
		}
	}

	aut->set_init_state(spot_init_state_id);
	nha->set_init_state(nha_init_state_id);

	// Convert state-sets to names of states
	auto state_sets = aut->get_named_prop<std::vector<std::set<unsigned>>>("state-sets");
	auto sn = new std::vector<std::string>(state_sets->size() + (spot_init_state_id > 0 ? 1 : 0));

	for (unsigned i = 0; i < state_sets->size(); ++i) {
		std::set<unsigned> ss = (*state_sets)[i];
		if (spot_init_state_id > 0 && i == spot_init_state_id) {
			(*sn)[i] = "init";
		} else {
			(*sn)[i] = set_to_str(ss);
		}
	}

	aut->set_named_prop<std::vector<std::string>>("state-names", sn);

	// merge edges with the same source and destination
	nha->merge_edges();

	// assign the marks as LTL2BA does
	for (unsigned st_id = 0, st_count = nha->states_count(); st_id < st_count; ++st_id) {
		auto source_id = nha->state_name(st_id);

		for (auto& edge_id : nha->get_state_edges(st_id)) {
			auto edge = nha->get_edge(edge_id);

			auto targets = edge->get_targets();
			auto target_id = nha->state_name(*(targets.begin()));
			auto label = edge->get_label();
			auto marks = edge->get_marks();
			auto target_set = (*sets)[target_id];

			for (auto& rec : tgba_mark_owners) {
				// is the transition marked by the appropriate mark?
				if (marks.count(rec.first) == 0) {
					// no; does this edge go somewhere else than the source state?
					if (target_set.count(rec.second) == 0) {
						// yes so add the sibling
						marks.insert(sibling_of_removed_fin[rec.first]);
					} else {
						// find some edge f from target state that satisfies:
						// 1) f goes to subset of target_set not containing the owner of mark
						// 2) f.label ⊆ current edge.label
						for (auto& f_edge_id : slaa->get_state_edges(rec.second)) {
							auto f_edge = slaa->get_edge(f_edge_id);
							auto f_targets = f_edge->get_targets();

							if (f_targets.count(rec.second) == 0
								&& std::includes(target_set.begin(), target_set.end(), f_targets.begin(), f_targets.end())
								&& ((label & bdd_not(f_edge->get_label())) == bdd_false())
							) {
								marks.insert(sibling_of_removed_fin[rec.first]);
								break;
							}
						}

					}
				} else {
					// yes, remove it
					marks.erase(rec.first);
				}
			}

			// remove old edge and add the updated one
			nha->remove_edge(source_id, edge_id);
			nha->add_edge(source_id, label, targets, marks);
		}
	}

	// we merge edges again
	nha->merge_edges();
	// some states may become unreachable
	nha->remove_unreachable_states();

	// merge the equivalent states
	if (o_eq_level > 0) {
		nha->merge_equivalent_states();
	}

	// again, some may become unreachable
	nha->remove_unreachable_states();

	// count all used marks to remove the unused ones
	std::set<spot::acc_cond::mark_t::value_t> used_marks;

	for (unsigned st_id = 0, st_count = nha->states_count(); st_id < st_count; ++st_id) {
		for (auto& edge_id : nha->get_state_edges(st_id)) {
			auto j = nha->get_edge(edge_id)->get_marks();
			used_marks.insert(j.begin(), j.end());
		}
	}

	// create a conversion table { old mark => new mark }
	std::map<spot::acc_cond::mark_t::value_t, spot::acc_cond::mark_t::value_t> mark_conversion;
	spot::acc_cond::mark_t::value_t mark_counter = 0;
	for (auto old_mark : used_marks) {
		mark_conversion[old_mark] = mark_counter;
		++mark_counter;
	}

	// reset spot's init state
	aut->set_init_state(nha->state_name(nha->get_init_state()));

	// build the acceptance condition
	for (auto& disj : acr) {
		auto disj_f = spot::acc_cond::acc_code::f();

		bool not_having_true = false;
		for (auto& conj : disj) {
			auto conj_f = spot::acc_cond::acc_code::t();

			for (auto& pair : conj) {
				if (tgba_mark_owners.count(pair.first) > 0) {
					if (used_marks.count(pair.second) > 0) {
						conj_f &= ac.inf(spot::acc_cond::mark_t({ mark_conversion[pair.second] }));
					} else {
						// Inf(unused mark) can be never satisfied
						conj_f &= spot::acc_cond::acc_code::f();
					}
					not_having_true = true;
				} else {
					bool fin_used = used_marks.count(pair.first) > 0;
					bool inf_used = used_marks.count(pair.second) > 0;

					if (fin_used && inf_used) {
						conj_f &= ac.fin(spot::acc_cond::mark_t({ mark_conversion[pair.first] })) | ac.inf(spot::acc_cond::mark_t({ mark_conversion[pair.second] }));
						not_having_true = true;
					} else if (fin_used) {
						// Inf cannot be satisfied, so we rely on Fin
						conj_f &= ac.fin(spot::acc_cond::mark_t({ mark_conversion[pair.first] }));
						not_having_true = true;
					}
				}
			}
			disj_f |= conj_f;
		}

		if (not_having_true) {
			ac.set_acceptance(ac.get_acceptance() & disj_f);
		}
		aut->set_acceptance(used_marks.size(), ac.get_acceptance());
	}

	// now we can finally create the Spot structure
	for (unsigned st_id = 0, st_count = nha->states_count(); st_id < st_count; ++st_id) {
		auto source_id = nha->state_name(st_id);

		for (auto& edge_id : nha->get_state_edges(st_id)) {
			auto edge = nha->get_edge(edge_id);

			auto target_id = nha->state_name(*(edge->get_targets().begin()));
			auto label = edge->get_label();
			auto marks = edge->get_marks();
			auto target_set = (*sets)[target_id];

			std::set<spot::acc_cond::mark_t::value_t> marks_relabelled;
			for (auto mark : marks) {
				marks_relabelled.insert(mark_conversion[mark]);
			}

			aut->new_edge(source_id, target_id, label, spot::acc_cond::mark_t(marks_relabelled.begin(), marks_relabelled.end()));
		}
	}

	//aut->merge_edges(); we do this for nha
	if (o_spot_scc_filter || o_spot_simulation) {
		aut = spot::scc_filter(aut);
	} else {
		// older versions of spot remove the state names after scc_filter calls
		// hence we add a possibility not to call scc_filter
		aut->purge_dead_states();
	}

	if (o_spot_simulation) {
		spot::postprocessor pp;
		pp.set_type(spot::postprocessor::Generic);
		aut = pp.run(aut);
		spot::cleanup_acceptance_here(aut);
	}

	return aut;
}
