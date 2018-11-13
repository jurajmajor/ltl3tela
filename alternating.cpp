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

#include "utils.hpp"
#include "alternating.hpp"

bool is_mergeable(SLAA* slaa, spot::formula f) {
	if (!f.is(spot::op::U)) {
		throw "Argument of is_mergeable is not an U-formula";
	}

	// we are not interested in cases where we save nothing
	if (f[1].is_boolean()) {
		return false;
	}

	// left argument has to be a state formula
	if (!f[0].is_boolean()) {
		return false;
	}

	// bdd of the left argument
	auto alpha = spot::formula_to_bdd(f[0], slaa->spot_bdd_dict, slaa->spot_aut);
	bool at_least_one_loop = false;
	// for each conjunction in DNF of psi test whether loops are covered by alpha
	for (auto& clause : f_bar(f[1])) {
		// convert a set of formulae into their conjunction
		auto sf = spot::formula::And(std::vector<spot::formula>(clause.begin(), clause.end()));
		// create the state for the conjunction
		unsigned state_id = make_alternating_recursive(slaa, sf);
		// Check that any loop label impies alpha(f[0])
		for (auto& edge_id : slaa->get_state_edges(state_id)) {
			auto t = slaa->get_edge(edge_id);
			//check t is a loop
			auto tar_states = t->get_targets();
			std::set<spot::formula> targets;
			for (auto& tar_state : tar_states) {
				targets.insert(slaa->state_name(tar_state));
			}
			if (std::includes(targets.begin(), targets.end(), clause.begin(), clause.end())) {
				// If label does not satisfy alpha, return false
				at_least_one_loop = true;
				if ((t->get_label() & alpha) != t->get_label()) {
					return false;
				}
			}
		}
	}

	if (o_mergeable_info == 1 && at_least_one_loop) {
		// only for experiments purposes
		std::cout << true << std::endl;
		std::exit(0);
	}

	return true;
}

void register_ap_from_boolean_formula(SLAA* slaa, spot::formula f) {
	// recursively register APs from a state formula f
	if (f.is(spot::op::And) || f.is(spot::op::Or)) {
		for (unsigned i = 0, size = f.size(); i < size; ++i) {
			register_ap_from_boolean_formula(slaa, f[i]);
		}
	} else {
		slaa->spot_aut->register_ap((f.is(spot::op::Not) ? spot::formula::Not(f) : f).ap_name());
	}
}

unsigned make_alternating_recursive(SLAA* slaa, spot::formula f) {
	if (slaa->state_exists(f)) {
		// we already have a state for f
		return slaa->get_state_id(f);
	} else {
		// create a new state
		unsigned state_id = slaa->get_state_id(f);

		if (f.is_tt()) {
			slaa->add_edge(state_id, bdd_true(), std::set<unsigned>());
		} else if (f.is_ff()) {
			// NOP
		} else if (f.is_boolean()) {
			// register APs in f
			register_ap_from_boolean_formula(slaa, f);

			// add the only edge to nowhere
			slaa->add_edge(state_id, spot::formula_to_bdd(f, slaa->spot_bdd_dict, slaa->spot_aut), std::set<unsigned>());
		} else if (f.is(spot::op::And)) {
			std::set<std::set<unsigned>> conj_edges;
			// create a state for each conjunct
			for (unsigned i = 0, size = f.size(); i < size; ++i) {
				conj_edges.insert(slaa->get_state_edges(make_alternating_recursive(slaa, f[i])));
			}
			// and add the product edges
			auto product = slaa->product(conj_edges, true);
			for (auto& edge : product) {
				slaa->add_edge(state_id, edge);
			}
		} else if (f.is(spot::op::Or)) {
			// create a state for each disjunct
			for (unsigned i = 0, size = f.size(); i < size; ++i) {
				auto fi_state_edges = slaa->get_state_edges(make_alternating_recursive(slaa, f[i]));
				// and add all its edges
				for (auto& edge : fi_state_edges) {
					slaa->add_edge(state_id, edge);
				}
			}
		} else if (f.is(spot::op::X)) {
			if (o_x_single_succ) {
				// translate X φ as (X φ) --tt--> (φ)
				std::set<unsigned> target_set = { make_alternating_recursive(slaa, f[0]) };
				slaa->add_edge(state_id, bdd_true(), target_set);
			} else {
				// we add an universal edge to all states in each disjunct
				auto f_dnf = f_bar(f[0]);

				for (auto& g_set : f_dnf) {
					std::set<unsigned> target_set;
					for (auto& g : g_set) {
						target_set.insert(make_alternating_recursive(slaa, g));
					}
					slaa->add_edge(state_id, bdd_true(), target_set);
				}
			}

		} else if (f.is(spot::op::R)) {
			// we build automaton for f[0] even if we don't need it for G
			// however, it doesn't cost much if f[0] == ff
			// the advantage is that we don't break order of APs
			unsigned left = make_alternating_recursive(slaa, f[0]);
			unsigned right = make_alternating_recursive(slaa, f[1]);

			auto f1_dnf = f_bar(f[1]);
			if (o_g_merge_level > 0 && f[0].is_ff() && f1_dnf.size() == 1 && (o_g_merge_level == 2 || f1_dnf.begin()->size() == 1)) {
				// we have G(φ_1 & ... & φ_n) for temporal formulae φ_i
				if (o_mergeable_info == 2) {
					// only for experiments purposes
					std::cout << true << std::endl;
					std::exit(0);
				}

				auto f1_conjuncts = *(f1_dnf.begin());

				std::set<std::set<unsigned>> edges_for_product;

				for (auto& phi : f1_conjuncts) {
					unsigned phi_state = make_alternating_recursive(slaa, phi);
					std::set<unsigned> phi_edges;

					if (phi.is(spot::op::U)) {
						// φ is U subformula
						// copy each edge for φ
						// each edge that is not a loop gets an Inf mark
						acc_mark inf = slaa->acc[phi].inf;

						for (auto& edge_id : slaa->get_state_edges(phi_state)) {
							auto edge = slaa->get_edge(edge_id);
							auto targets = edge->get_targets();
							auto marks = edge->get_marks();

							if (targets.count(phi_state) > 0) {
								// this is a loop
								targets.erase(phi_state);
							} else {
								// this is not a loop, add the Inf mark
								if (inf == -1U) {
									// we don't have an mark for Inf, create one
									auto& ac = slaa->spot_aut->acc();
									inf = ac.add_set();
									slaa->acc[phi].inf = inf;

									slaa->remember_inf_mark(inf);
								}

								marks = { inf };
							}

							// each copied edge should loop
							targets.insert(state_id);

							auto new_edge_id = slaa->create_edge(edge->get_label());
							auto new_edge = slaa->get_edge(new_edge_id);
							new_edge->add_target(targets);
							new_edge->add_mark(marks);

							phi_edges.insert(new_edge_id);
						}
					} else {
						// each edge goes to Gφ; transition to φ (if any) is removed
						for (auto& edge_id : slaa->get_state_edges(phi_state)) {
							auto edge = slaa->get_edge(edge_id);
							auto targets = edge->get_targets();

							targets.erase(phi_state);
							targets.insert(state_id);

							auto new_edge_id = slaa->create_edge(edge->get_label());
							auto new_edge = slaa->get_edge(new_edge_id);
							new_edge->add_target(targets);
							new_edge->add_mark(edge->get_marks());

							phi_edges.insert(new_edge_id);
						}
					}

					edges_for_product.insert(phi_edges);
				}

				// create a product of all new edges
				auto phi_product = slaa->product(edges_for_product, true);
				for (auto edge_id : phi_product) {
					slaa->add_edge(state_id, edge_id);
				}
			} else {
				// use traditional construction
				std::set<unsigned> left_edges = slaa->get_state_edges(left);
				std::set<unsigned> right_edges = slaa->get_state_edges(right);

				unsigned loop_id = slaa->create_edge(bdd_true());
				slaa->get_edge(loop_id)->add_target(state_id);

				// remember the mark-discarding product should be used
				for (auto& right_edge : right_edges) {
					for (auto& left_edge : left_edges) {
						slaa->add_edge(state_id, slaa->edge_product(right_edge, left_edge, false));
					}
					slaa->add_edge(state_id, slaa->edge_product(right_edge, loop_id, false));
				}
			}
		} else if (f.is(spot::op::U)) {
			auto& ac = slaa->spot_aut->acc();
			slaa->acc[f].fin = ac.add_set(); // create a new mark
			slaa->acc[f].inf = -1U; // default value for Inf-mark, meaning the mark does not have a value

			unsigned left = make_alternating_recursive(slaa, f[0]);
			unsigned right = make_alternating_recursive(slaa, f[1]);

			if (o_u_merge_level > 0 && is_mergeable(slaa, f)) {
				// we always have a loop with the Fin-mark
				slaa->add_edge(
					state_id,
					spot::formula_to_bdd(f[0], slaa->spot_bdd_dict, slaa->spot_aut),
					std::set<unsigned>({ state_id }),
					std::set<unsigned>({ slaa->acc[f].fin })
				);

				// if f is a disjunction of at least two subformulae, create marks for each of these
				auto f_dnf = f_bar(f[1]);
				unsigned mark = -1U;
				unsigned f_dnf_size = f_dnf.size();
				if (f_dnf_size > 1) {
					// we add fin_disj marks only if at least two automata for the disjuncts contain a loop
					unsigned states_with_loop = 0;
					for (auto& m : f_dnf) {
						auto product_state = make_alternating_recursive(
							slaa,
							spot::formula::And(std::vector<spot::formula>(m.begin(), m.end()))
						);
						auto product_edges = slaa->get_state_edges(product_state);
						std::set<unsigned> m_state_ids;
						for (auto& m_formula : m) {
							m_state_ids.insert(make_alternating_recursive(slaa, m_formula));
						}

						bool merge = true;
						if (o_u_merge_level == 3) {
							// we won't merge if there is a looping alternating transition
							for (auto& edge_id : product_edges) {
								auto edge_targets = slaa->get_edge(edge_id)->get_targets();

								if (edge_targets.count(product_state) > 0 && edge_targets.size() >= 2) {
									merge = false;
									break;
								}
							}
						}

						for (auto& edge_id : product_edges) {
							auto edge = slaa->get_edge(edge_id);
							auto p = edge->get_targets();
							if (o_u_merge_level == 1 && p == m_state_ids || o_u_merge_level >= 2 && merge && std::includes(p.begin(), p.end(), m_state_ids.begin(), m_state_ids.end())) {
								++states_with_loop;
								break;
							}
						}
					}

					if (states_with_loop > 1) {
						mark = ac.add_sets(f_dnf_size);
						// now we have marks in range mark .. mark + f_dnf_size - 1
						// each set of edges gets all but its number
						for (acc_mark i = mark; i < mark + f_dnf_size; ++i) {
							slaa->acc[f].fin_disj.insert(i);
						}
					}
				}

				// for each M ∈ DNF of f
				unsigned m_mark = mark;
				for (auto& m : f_dnf) {
					// set of state IDs of M
					std::set<unsigned> m_state_ids;
					for (auto& m_formula : m) {
						m_state_ids.insert(make_alternating_recursive(slaa, m_formula));
					}

					// build a state for product of M (equal to conjunction of M)
					auto product_state = make_alternating_recursive(
						slaa,
						spot::formula::And(std::vector<spot::formula>(m.begin(), m.end()))
					);
					auto product_edges = slaa->get_state_edges(product_state);

					bool merge = true;
					if (o_u_merge_level == 3) {
						// we won't merge if there is a looping alternating transition
						for (auto& edge_id : product_edges) {
							auto edge_targets = slaa->get_edge(edge_id)->get_targets();
								if (edge_targets.count(product_state) > 0 && edge_targets.size() >= 2) {
								merge = false;
								break;
							}
						}
					}

					// for each edge of the product
					for (auto& edge_id : product_edges) {
						auto edge = slaa->get_edge(edge_id);

						auto p = edge->get_targets();
						// does M ⊆ P hold?
						if ((o_u_merge_level == 1 && p == m_state_ids) || (o_u_merge_level >= 2 && merge && std::includes(p.begin(), p.end(), m_state_ids.begin(), m_state_ids.end()))) {
							// yes so new target set is P ∖ M plus loop to our state
							std::set<unsigned> new_edge_targets;
							auto net_it = std::set_difference(
								p.begin(), p.end(),
								m_state_ids.begin(), m_state_ids.end(),
								std::inserter(new_edge_targets, new_edge_targets.begin())
							);
							new_edge_targets.insert(state_id);

							auto new_edge_marks = edge->get_marks();
							if (mark != -1U) {
								for (unsigned i = mark; i < mark + f_dnf_size; ++i) {
									if (i != m_mark) {
										new_edge_marks.insert(i);
									}
								}
							}

							slaa->add_edge(state_id, slaa->get_edge(edge_id)->get_label(), new_edge_targets, new_edge_marks);
						} else {
							// M ⊆ P does not hold
							slaa->add_edge(state_id, edge->get_label(), edge->get_targets());
						}
					}

					++m_mark;
				}
			} else {
				// the classical construction for U
				std::set<unsigned> left_edges = slaa->get_state_edges(left);
				std::set<unsigned> right_edges = slaa->get_state_edges(right);

				unsigned loop_id = slaa->create_edge(bdd_true());
				slaa->get_edge(loop_id)->add_target(state_id);

				slaa->add_edge(state_id, right_edges);

				for (auto& left_edge : left_edges) {
					auto p = slaa->edge_product(left_edge, loop_id, true);
					// the only mark is the new Fin
					slaa->get_edge(p)->clear_marks();
					slaa->get_edge(p)->add_mark(slaa->acc[f].fin);
					slaa->add_edge(state_id, p);
				}
			}
		}

		return state_id;
	}
}

SLAA* make_alternating(spot::formula f) {
	SLAA* slaa = new SLAA(f);

	if (o_single_init_state) {
		std::set<unsigned> init_set = { make_alternating_recursive(slaa, f) };
		slaa->add_init_set(init_set);
	} else {
		std::set<std::set<spot::formula>> f_dnf = f_bar(f);

		for (auto& g_set : f_dnf) {
			std::set<unsigned> init_set;
			for (auto& g : g_set) {
				unsigned init_state_id = make_alternating_recursive(slaa, g);
				init_set.insert(init_state_id);
			}
			slaa->add_init_set(init_set);
		}
	}

	slaa->build_acc();

	return slaa;
}
