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

#ifndef ALTERATING_H
#define ALTERNATING_H
#include "utils.hpp"
#include "automaton.hpp"
#include <spot/tl/parse.hh>
#include <spot/twa/formula2bdd.hh>
#include <spot/twa/twagraph.hh>
#include <iostream>
#include <map>
#include <set>

// registers atomic proposition from a state formula
void register_ap_from_boolean_formula(SLAA* slaa, spot::formula f);

// checks whether U-formula f is mergeable
bool is_mergeable(SLAA* slaa, spot::formula f);

// converts an LTL formula to self-loop alternating automaton
SLAA* make_alternating(spot::formula f);

// helper function for LTL to automata translation
unsigned make_alternating_recursive(SLAA* slaa, spot::formula f);

#endif
