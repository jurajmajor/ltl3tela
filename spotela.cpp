/*
    Copyright (c) 2018 Tatiana Zboncakova
    Copyright (c) 2019 Juraj Major

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

#include "spotela.hpp"

std::vector<edge_t> get_loops(spot::twa_graph_ptr aut, unsigned state) {
	std::vector<edge_t> loops;
	for (auto& edge : aut->out(state)) {
		if (edge.dst == state) {
			loops.push_back(edge);
		}
	}
	return loops;
}

std::vector<edge_t> out_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2) {
	std::vector<edge_t> edges;
	for (auto& edge : aut->out(state1)) {
		if (edge.dst != state1 && edge.dst != state2) {
			edges.push_back(edge);
		}
	}
	return edges;
}

std::vector<edge_t> in_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2) {
	std::vector<edge_t> edges;
	for (unsigned s = 0, ns = aut->num_states(); s < ns; ++s) {
		for (auto& edge : aut->out(s)) {
			if (edge.src != state1 && edge.src != state2 && edge.dst == state1) {
				edges.push_back(edge);
			}
		}
	}
	return edges;
}

std::optional<std::vector<edge_t>> check_out_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2) {
	auto s1_edges = out_edges(aut, state1, state2);
	auto s2_edges = out_edges(aut, state2, state1);

	for (auto& edge2 : s2_edges) {
		bool any = false;
		for (auto& edge1 : s1_edges) {
			if (edge1.dst == edge2.dst && bdd_implies(edge2.cond, edge1.cond)) {
				any = true;
				break;
			}
		}

		if (!any) {
			return std::nullopt;
		}
	}

	return { s1_edges };
}

std::optional<std::vector<edge_t>> check_in_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2) {
	auto s1_edges = in_edges(aut, state1, state2);
	auto s2_edges = in_edges(aut, state2, state1);

	for (auto& edge2 : s2_edges) {
		bool any = false;
		for (auto& edge1 : s1_edges) {
			if (edge1.src == edge2.src && bdd_implies(edge2.cond, edge1.cond)) {
				any = true;
				break;
			}
		}

		if (!any) {
			return std::nullopt;
		}
	}

	return { s1_edges };
}

spot::twa_graph_ptr create_aut_from_state(spot::twa_graph_ptr aut, unsigned state) {
	auto aut2 = spot::make_twa_graph(aut->get_dict());
	aut2->copy_ap_of(aut);
	aut2->copy_acceptance_of(aut);

	aut2->new_states(aut->num_states());
	for (auto& edge : aut->edges()) {
		aut2->new_edge(edge.src, edge.dst, edge.cond, edge.acc);
	}

	aut2->set_init_state(state);

	spot::postprocessor pp;
	pp.set_type(spot::postprocessor::Generic);
	aut2 = pp.run(aut2);
	spot::cleanup_acceptance_here(aut2);

	return aut2;
}

bool is_one_state_scc(spot::scc_info si, unsigned scc) {
	return si.states_of(scc).size() == 1 && !si.is_trivial(scc);
}

bool has_successors(spot::scc_info si, unsigned scc) {
	return si.succ(scc).size() > 0;
}

std::optional<bdd> get_connecting_edge_condition(spot::twa_graph_ptr aut, unsigned state1, unsigned state2, bdd b_loop_cond) {
	bdd cond = bddfalse;
	for (auto& edge : aut->out(state1)) {
		if (edge.dst == state2) {
			if (!bdd_implies(edge.cond, b_loop_cond)) {
				return std::nullopt;
			}
			cond = bdd_or(cond, edge.cond);
		}
	}
	return { cond };
}

spot::twa_graph_ptr build_simplified_automaton(spot::twa_graph_ptr aut, unsigned base_state, std::vector<state_info> state_infos) {
	std::vector<unsigned> states;
	for (auto& si : state_infos) {
		states.push_back(si.state);
	}
	states.push_back(base_state);

	spot::twa_graph_ptr new_aut;
	unsigned new_state;
	std::vector<unsigned> state_v;
	std::tie(new_aut, new_state, state_v) = copy_aut(aut, states);

	auto si_size = state_infos.size();
	auto fin_marks = set_fin_cond(new_aut, si_size);

	for (unsigned i = 0; i < si_size; ++i) {
		std::vector<unsigned> fin_marks_minus_ith;
		std::copy_if(std::begin(fin_marks), std::end(fin_marks), std::back_inserter(fin_marks_minus_ith),
			[&fin_marks, i](unsigned j) { return fin_marks[i] != j; });
		add_edges(new_aut, state_infos[i], new_state, state_v, fin_marks_minus_ith);
	}

	new_aut->new_edge(new_state, new_state, state_infos[0].b.cond,
		spot::acc_cond::mark_t(std::begin(fin_marks), std::end(fin_marks)));
	return new_aut;
}

spot::twa_graph_ptr simplify_one_scc(spot::twa_graph_ptr aut) {
	spot::scc_info si(aut);
	for (int scc = si.scc_count() - 1; scc >= 0; --scc) {
		std::vector<state_info> states;
		auto state_of_scc = si.one_state_of(scc);
		if (is_one_state_scc(si, scc) && has_successors(si, scc) && si.is_rejecting_scc(scc)) {
			for (auto& succ : si.succ(scc)) {
				if (is_one_state_scc(si, succ) && si.is_accepting_scc(succ)) {
					auto simpl_state = check_simplifiability(aut, state_of_scc, si.states_of(succ)[0]);
					if (simpl_state != std::nullopt) {
						states.push_back(*simpl_state);
					}
				}
			}
		}

		if (!states.empty()) {
			return build_simplified_automaton(aut, state_of_scc, states);
		}
	}

	return aut;
}

spot::twa_graph_ptr spotela_simplify(spot::twa_graph_ptr aut) {
	if (!aut->acc().is_generalized_buchi()) {
		// the algorithm only works for (T)GBA
		return aut;
	}

	auto aut2 = simplify_one_scc(aut);
	while (aut2->num_states() < aut->num_states()) {
		aut = aut2;
		aut2 = simplify_one_scc(aut);
	}

	return aut2;
}

bool implies_language(spot::twa_graph_ptr aut, unsigned state1, unsigned state2) {
	// check L(A1) ⊆ L(A2): this holds iff L(A1) ∩ co-L(A2) = ∅
	auto aut1 = create_aut_from_state(aut, state1);
	auto coaut2 = spot::dualize(create_aut_from_state(aut, state2));

	return !aut1->intersects(coaut2);
}

std::optional<edge_t> check_snd_pattern(spot::twa_graph_ptr aut, edge_t edge, bdd c_edge_cond, unsigned state1, unsigned state2) {
	auto oe = out_edges(aut, state1, state1);
	auto condition = bdd_and(edge.cond, bdd_not(c_edge_cond));

	for (auto& e : oe) {
		if (bdd_implies(condition, e.cond) && implies_language(aut, state2, e.dst)) {
			return { e };
		}
	}

	return std::nullopt;
}

std::tuple<spot::twa_graph_ptr, unsigned, std::vector<unsigned>> copy_aut(spot::twa_graph_ptr aut, std::vector<unsigned> states) {
	if (states.empty()) {
		throw "copy_aut: states vector is not expected to be empty.";
	}

	unsigned new_state = *(std::min_element(std::begin(states), std::end(states)));
	unsigned init_state = aut->get_init_state_number();

	auto aut2 = spot::make_twa_graph(aut->get_dict());
	aut2->copy_ap_of(aut);
	aut2->copy_acceptance_of(aut);
	aut2->new_states(aut->num_states() - states.size() + 1);

	std::vector<unsigned> states_v;
	unsigned j = 0;
	for (unsigned i = 0, ns = aut->num_states(); i < ns; ++i) {
		if (std::find(std::begin(states), std::end(states), i) != std::end(states)) {
			states_v.push_back(new_state);
			if (i == new_state) {
				++j;
			}
		} else {
			states_v.push_back(j);
			++j;
		}
	}

	if (std::find(std::begin(states), std::end(states), init_state) != std::end(states)) {
		aut2->set_init_state(new_state);
	} else {
		aut2->set_init_state(states_v[init_state]);
	}

	for (auto& edge : aut->edges()) {
		if (std::find(std::begin(states), std::end(states), edge.src) == std::end(states)
			&& std::find(std::begin(states), std::end(states), edge.dst) == std::end(states)) {
			aut2->new_edge(states_v[edge.src], states_v[edge.dst], edge.cond, edge.acc);
		}
	}

	return { aut2, new_state, states_v };
}

void add_edges(spot::twa_graph_ptr aut, state_info& si, unsigned new_state, std::vector<unsigned> states, std::vector<unsigned> acc) {
	for (auto& edge : si.inE) {
		aut->new_edge(states[edge.src], new_state, edge.cond);
		edge.dst = edge.src;
	}

	for (auto& edge : si.outE) {
		if (states[edge.dst] != new_state) {
			aut->new_edge(new_state, states[edge.dst], edge.cond);
		}
	}

	for (auto& edge : si.loops) {
		auto cond = bdd_and(edge.cond, si.c_cond);
		if (cond != bddfalse) {
			aut->new_edge(new_state, new_state, cond, edge.acc | spot::acc_cond::mark_t(std::begin(acc), std::end(acc)));
		}
	}
}

std::optional<state_info> check_simplifiability(spot::twa_graph_ptr aut, unsigned base_state, unsigned state2) {
	auto state2_loops = get_loops(aut, state2);
	auto loops = get_loops(aut, base_state);
	if (loops.size() != 1) {
		return std::nullopt;
	}
	auto& b_loop_edge = loops[0];

	auto outEdges = check_out_edges(aut, base_state, state2);
	if (!outEdges) {
		return std::nullopt;
	}

	auto inEdges = check_in_edges(aut, base_state, state2);
	if (!inEdges) {
		return std::nullopt;
	}

	auto c_edge_cond = get_connecting_edge_condition(aut, base_state, state2, b_loop_edge.cond);
	if (!c_edge_cond) {
		return std::nullopt;
	}

	for (auto& loop : state2_loops) {
		if (loop.acc.count() != 0) {
			if (!bdd_implies(loop.cond, *c_edge_cond) && !check_snd_pattern(aut, loop, *c_edge_cond, base_state, state2)) {
				return std::nullopt;
			}
		} else {
			if (!bdd_implies(loop.cond, b_loop_edge.cond) && !check_snd_pattern(aut, loop, b_loop_edge.cond, base_state, state2)) {
				return std::nullopt;
			}
		}
	}

	state_info rv;
	rv.inE = *inEdges;
	rv.outE = *outEdges;
	rv.state = state2;
	rv.loops = state2_loops;
	rv.c_cond = *c_edge_cond;
	rv.b = loops[0];
	return rv;
}

std::vector<unsigned> set_fin_cond(spot::twa_graph_ptr aut, unsigned n) {
	auto& acceptance = aut->acc();
	auto num_acc_sets = acceptance.num_sets();

	std::vector<unsigned> fin_marks(n);
	std::iota(std::begin(fin_marks), std::end(fin_marks), num_acc_sets);

	auto cond = spot::acc_cond::acc_code::f();
	for (auto mark : fin_marks) {
		cond = cond | spot::acc_cond::acc_code::fin({ mark });
	}

	aut->set_acceptance(num_acc_sets + n, acceptance.get_acceptance() & cond);

	return fin_marks;
}
