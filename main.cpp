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

#include <iostream>
#include <spot/misc/version.hh>
#include <spot/twaalgos/dot.hh>
#include <spot/twa/twagraph.hh>
#include <string>
#include "utils.hpp"
#include "alternating.hpp"
#include "nondeterministic.hpp"
#include "automaton.hpp"
#include "spotela.hpp"

unsigned o_try_ltl2tgba_spotela;	// -b
bool o_single_init_state;	// -i
unsigned o_slaa_determ;		// -d
unsigned o_eq_level;		// -e
bool o_ltl_split;			// -l
unsigned o_mergeable_info;	// -m
bool o_try_negation;		// -n
bool o_simplify_formula;	// -s
bool o_ac_filter_fin;		// -t
unsigned o_debug;			// -x

bool o_deterministic;		// -D

unsigned o_u_merge_level;	// -F
unsigned o_g_merge_level;	// -G
bool o_disj_merging;		// -O
bool o_x_single_succ;		// -X

int main(int argc, char* argv[])
{
	std::string version("2.1.0");

	bdd_init(1000, 1000);
	// hide "garbage collection" messages from BuDDy
	bdd_gbc_hook(nullptr);

	std::map<std::string, std::string> args = parse_arguments(argc, argv);

	if (args.count("v") > 0) {
		std::cout << "LTL3TELA " << version << " (using Spot " << spot::version() << ")\n";
		return 0;
	}

	bool invalid_run = args.count("f") == 0;

	if (invalid_run || args.count("h") > 0) {
		std::cout << "LTL3TELA " << version << " (using Spot " << spot::version() << ")\n\n"
			<< "usage: " << argv[0] << " [-flags] -f formula\n"
			<< "available flags:\n"
			<< "\t-a[0|2|3]\tact like\n"
			<< "\t\t0\tdo not simulate anything (default)\n"
			<< "\t\t2\tltl2ba (like -d0 -n0 -e1)\n"
			<< "\t\t3\tltl3ba (like -n0 -i1 -X1)\n"
			<< "\t-b[0|1|2|3]\tproduce TGBA if smaller\n"
			<< "\t\t0\tno action\n"
			<< "\t\t1\ttry ltl2tgba\n"
			<< "\t\t2\ttry SPOTELA\n"
			<< "\t\t3\ttry ltl2tgba+SPOTELA (default)\n"
			<< "\t-d\tmore deterministic SLAA construction\n"
			<< "\t\t0\tno optimization\n"
			<< "\t\t1\tclassical transition dominance\n"
			<< "\t\t2\textended transition dominance (default)\n"
			<< "\t-D[0|1]\tproduce deterministic NA\n"
			<< "\t-e[0|1|2]\tequivalence check on NA\n"
			<< "\t\t0\tno check\n"
			<< "\t\t1\tltl2ba's simple check\n"
			<< "\t\t2\tltl3ba's improved check (default)\n"
			<< "\t-F[0|1|2|3]\toptimized treatment of mergeable U\n"
			<< "\t\t0\tno merge\n"
			<< "\t\t1\tmerge that minimizes NA\n"
			<< "\t\t2\tmerge that minimizes SLAA (default)\n"
			<< "\t\t3\tmerge states not containing looping alternating edge\n"
			<< "\t-G[0|1|2]\toptimized treatment of G\n"
			<< "\t\t0\tno merge\n"
			<< "\t\t1\tmerge Gf if f is temporal formula\n"
			<< "\t\t2\tmerge Gf is f is conjunction of temporal formulae (default)\n"
			<< "\t-h, -?\tprint this help\n"
			<< "\t-i[0|1]\tproduce SLAA with one initial state (default off)\n"
			<< "\t-m\t(for experiments only) check formula for containment of\n"
			<< "\t\t0\tnothing, translate formula as usual (default)\n"
			<< "\t\t1\tmergeable F\n"
			<< "\t\t2\tmergeable G\n"
			<< "\t-n[0|1]\ttry translating !f and complementing the automaton (default on)\n"
			<< "\t-o [hoa|dot]\ttype of output\n"
			<< "\t\thoa\tprint automaton in HOA format (default)\n"
			<< "\t\tdot\tprint dot format\n"
			<< "\t-O[0|1]\tdisjunction merging (default off)\n"
			<< "\t-p[1|2|3]\tphase of translation\n"
			<< "\t\t1\tprint SLAA\n"
			<< "\t\t2\tprint NA (default)\n"
			<< "\t\t3\tprint both\n"
			<< "\t-s[0|1]\tspot's formula simplifications (default on)\n"
			<< "\t-t[0|1]\timproved construction of acceptance condition (default on)\n"
			<< "\t-v\tprint version and exit\n"
			<< "\t-x\t(for experiments only) special experiments-related options\n"
			<< "\t\t0\toff (default)\n"
			<< "\t\t1\tstatistics to STDERR\n"
			<< "\t\t2\tuse only external translator, not LTL3TELA algorithm\n"
			<< "\t\t3\tboth -x1 and -x2\n"
			<< "\t-X[0|1]\ttranslate X phi as (X phi) --tt--> (phi) (default off)\n";

		return invalid_run;
	}

	spot::formula f;
	try {
		f = spot::parse_formula(args["f"]);
	} catch (spot::parse_error& e) {
		std::cerr << "The input formula is invalid.\n" << e.what();
		return 1;
	}

	o_try_ltl2tgba_spotela = std::stoi(args["b"]);
	o_single_init_state = std::stoi(args["i"]);
	o_slaa_determ = std::stoi(args["d"]);
	o_eq_level = std::stoi(args["e"]);
	o_ltl_split = std::stoi(args["l"]);
	o_mergeable_info = std::stoi(args["m"]);
	o_try_negation = std::stoi(args["n"]);
	o_simplify_formula = std::stoi(args["s"]);
	o_ac_filter_fin = std::stoi(args["t"]);
	o_debug = std::stoi(args["x"]);

	o_deterministic = std::stoi(args["D"]);

	o_u_merge_level = std::stoi(args["F"]);
	o_g_merge_level = std::stoi(args["G"]);
	o_disj_merging = std::stoi(args["O"]);
	o_x_single_succ = std::stoi(args["X"]);

	// -O1 implies -i1
	o_single_init_state = o_single_init_state || o_disj_merging;

	unsigned int print_phase = std::stoi(args["p"]);

	// -p1 implies -l0
	o_ltl_split = o_ltl_split && (print_phase & 2);

	// -x2 implies -b1 (mind the bitwise operations)
	if (o_debug & 2) {
		o_try_ltl2tgba_spotela = o_try_ltl2tgba_spotela | 1;
	}

	spot::twa_graph_ptr nwa = nullptr;
	SLAA* slaa = nullptr;
	std::string stats("");

	try {
		std::tie(nwa, slaa, stats) = build_best_nwa(f, nullptr, print_phase & 1, print_phase == 1);

		f = simplify_formula(f);

		if (o_ltl_split) {
			auto dict = nwa->get_dict();

			spot::twa_graph_ptr nwa_prod;
			std::string stats_prod;

			std::tie(nwa_prod, stats_prod) = build_product_nwa(f, dict);
			std::tie(nwa, stats) = compare_automata(nwa, nwa_prod, stats, stats_prod);
		}
	} catch (std::runtime_error& e) {
		std::string what(e.what());

		if (what.find("Too many acceptance sets used.") == 0) {
			std::cerr << "LTL3TELA is unable to set more than 32 acceptance marks.\n";
			return 32;
		} else {
			std::cerr << what << std::endl;
			return 3;
		}
	}

	if (slaa) {
		if (args["o"] == "dot") {
			slaa->print_dot();
		} else {
			slaa->print_hoaf();
		}

		delete slaa;
	}

	if (nwa) {
		if (args["o"] == "dot") {
			spot::print_dot(std::cout, nwa);
		} else {
			spot::print_hoa(std::cout, nwa);
			std::cout << '\n';
		}
	}

	if (o_debug & 1) {
		std::cerr << stats;
	}

	// do not call bdd_done(), we use libbddx

	return 0;
}
