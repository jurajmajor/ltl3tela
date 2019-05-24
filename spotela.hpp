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

#ifndef SPOTELA_H
#define SPOTELA_H
#include <algorithm>
#include <numeric>
#include <tuple>
#include <spot/graph/graph.hh>
#include <spot/twa/acc.hh>
#include <spot/twaalgos/cleanacc.hh>
#include <spot/twaalgos/contains.hh>
#include <spot/twaalgos/dualize.hh>
#include <spot/twaalgos/postproc.hh>
#include <spot/twaalgos/sccinfo.hh>
#include "utils.hpp"

typedef spot::twa_graph::graph_t::edge_storage_t edge_t;

typedef struct {
	std::vector<edge_t> inE;
	std::vector<edge_t> outE;
	unsigned state;
	std::vector<edge_t> loops;
	bdd c_cond;
	edge_t b;
} state_info;

std::vector<edge_t> get_loops(spot::twa_graph_ptr aut, unsigned state);
std::vector<edge_t> out_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2);
std::vector<edge_t> in_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2);
maybe<std::vector<edge_t>> check_out_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2);
maybe<std::vector<edge_t>> check_in_edges(spot::twa_graph_ptr aut, unsigned state1, unsigned state2);
maybe<state_info> check_simplifiability(spot::twa_graph_ptr aut, unsigned base_state, unsigned state2);
bool is_one_state_scc(spot::scc_info si, unsigned scc);
bool has_successors(spot::scc_info si, unsigned scc);
spot::twa_graph_ptr build_simplified_automaton(spot::twa_graph_ptr aut, unsigned base_state, std::vector<state_info> state_infos);
maybe<bdd> get_connecting_edge_condition(spot::twa_graph_ptr aut, unsigned state1, unsigned state2, bdd b_loop_cond);
spot::twa_graph_ptr create_aut_from_state(spot::twa_graph_ptr aut, unsigned state);
spot::twa_graph_ptr simplify_one_scc(spot::twa_graph_ptr aut);
spot::twa_graph_ptr spotela_simplify(spot::twa_graph_ptr aut);
bool implies_language(spot::twa_graph_ptr aut, unsigned state1, unsigned state2);
maybe<edge_t> check_snd_pattern(spot::twa_graph_ptr aut, edge_t edge, bdd c_edge_cond, unsigned state1, unsigned state2);
std::tuple<spot::twa_graph_ptr, unsigned, std::vector<unsigned>> copy_aut(spot::twa_graph_ptr aut, std::vector<unsigned> states);
void add_edges(spot::twa_graph_ptr aut, state_info& si, unsigned new_state, std::vector<unsigned> states, std::vector<unsigned> acc);
std::vector<unsigned> set_fin_cond(spot::twa_graph_ptr aut, unsigned n);
#endif
