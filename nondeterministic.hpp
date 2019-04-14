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

#ifndef NONDETERMINISTIC_H
#define NONDETERMINISTIC_H
#include <utility>
#include <spot/tl/print.hh>
#include <spot/twaalgos/cleanacc.hh>
#include <spot/twaalgos/hoa.hh>
#include <spot/twaalgos/postproc.hh>
#include <spot/twaalgos/simulation.hh>
#include <spot/twaalgos/sccfilter.hh>
#include <spot/twaalgos/dualize.hh>
#include <spot/twaalgos/isdet.hh>
#include <spot/twaalgos/translate.hh>
#include <spot/twaalgos/product.hh>
#include "automaton.hpp"
#include "alternating.hpp"
#include "spotela.hpp"
#include "utils.hpp"

// turns the given SLAA into an equivalent nondeterministic
// automaton in the Spot's structure
spot::twa_graph_ptr make_nondeterministic(SLAA* slaa);

// chooses the best nondeterministic automaton for a given formula
// returns nullptr in the first element of the pair if only alternating automaton is to be produced
// returns nullptr in the second element if alternating automaton is not to be printed
std::pair<spot::twa_graph_ptr, SLAA*> build_best_nwa(spot::formula f, spot::bdd_dict_ptr dict = nullptr, bool print_alternating = false, bool exit_after_alternating = false);

spot::twa_graph_ptr build_product_nwa(spot::formula f, spot::bdd_dict_ptr dict);

spot::twa_graph_ptr try_postprocessing(spot::twa_graph_ptr aut);

#endif
