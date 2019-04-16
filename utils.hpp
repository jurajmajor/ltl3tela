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

#ifndef INTERFACES_H
#define INTERFACES_H
#include <cassert>
#include <climits>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <queue>
#include <bddx.h>
#include <spot/tl/formula.hh>
#include <spot/tl/parse.hh>
#include <spot/tl/print.hh>
#include <spot/tl/unabbrev.hh>
#include <spot/tl/nenoform.hh>
#include <spot/tl/simplify.hh>
#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/isdet.hh>

extern unsigned o_try_ltl2tgba_spotela;	// -b
extern bool o_single_init_state;	// -i
extern bool o_slaa_determ;			// -d
extern unsigned o_eq_level;			// -e
extern bool o_ltl_split;			// -l
extern unsigned o_mergeable_info;	// -m
extern bool o_try_negation;			// -n
extern bool o_simplify_formula;		// -s
extern bool o_ac_filter_fin;		// -t
extern bool o_stats;				// -x
extern bool o_spot_simulation;		// -u
extern bool o_spot_scc_filter;		// -z

extern unsigned o_u_merge_level;	// -F
extern unsigned o_g_merge_level;	// -G
extern bool o_disj_merging;			// -O
extern bool o_x_single_succ;		// -X

// returns the DNF representation of LTL formula f
std::set<std::set<spot::formula>> f_bar(spot::formula f);

// parses arguments from argv
std::map<std::string, std::string> parse_arguments(int argc, char* argv[]);

// return the better (smaller, more deterministic) of the two automata
std::pair<spot::twa_graph_ptr, std::string> compare_automata(spot::twa_graph_ptr aut1, spot::twa_graph_ptr aut2, std::string stats_id1 = "", std::string stats_id2 = "");

// simplifies the formula in a way corresponding to used flags
spot::formula simplify_formula(spot::formula f);

// checks whether formula is suspendable = alternating formula according to
// Babiak et al - LTL to Büchi Automata Translation: Fast and More Deterministic
bool is_suspendable(spot::formula f);

template<typename T> class maybe {
    T val;
    bool hasVal;
public:
    maybe() {
        hasVal = false;
    }

    maybe(T v) {
        val = v;
        hasVal = true;
    }

    T fromJust() {
        if (!hasVal) {
            throw "Can't call fromJust on Nothing.";
        }
        return val;
    }

    bool isJust() {
        return hasVal;
    }

    bool isNothing() {
        return !hasVal;
    }

    static maybe<T> nothing() {
        maybe<T> r;
        return r;
    }

    static maybe<T> just(T v) {
        maybe<T> r(v);
        return r;
    }
};

#endif
