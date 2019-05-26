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

std::set<std::set<spot::formula>> f_bar(spot::formula f) {
	std::set<std::set<spot::formula> > r;
	if (f.is(spot::op::And)) {
		std::set<std::set<spot::formula> > r1 = f_bar(f[0]);
		// f can be of a format f_0 & f_1 & ... & f_n and we don't know the precise number of operands
		// the solution is to build an equivalent formula f_0 & (f_1 & ... & f_n)
		// building f_0 & (f_1) is safe as Spot doesn't consider (f_1) to be an And formula
		std::vector<spot::formula> other_operands;
		for (unsigned int i = 1, s = f.size(); i < s; ++i) {
			other_operands.push_back(f[i]);
		}
		std::set<std::set<spot::formula> > r2 = f_bar(spot::formula::And(other_operands));
		for (std::set<std::set<spot::formula> >::iterator i1 = r1.begin(); i1 != r1.end(); ++i1) {
			for (std::set<std::set<spot::formula> >::iterator i2 = r2.begin(); i2 != r2.end(); ++i2) {
				// add union of *i1 and *i2 into r
				std::set<spot::formula> s;
				s.insert(i1->begin(), i1->end());
				s.insert(i2->begin(), i2->end());
				r.insert(s);
			}
		}
	} else if (f.is(spot::op::Or)) {
		std::set<std::set<spot::formula> > r1 = f_bar(f[0]);
		std::vector<spot::formula> other_operands;
		for (unsigned int i = 1, s = f.size(); i < s; ++i) {
			other_operands.push_back(f[i]);
		}
		std::set<std::set<spot::formula> > r2 = f_bar(spot::formula::Or(other_operands));
		r.insert(r1.begin(), r1.end());
		r.insert(r2.begin(), r2.end());
	} else {
		std::set<spot::formula> s;
		s.insert(f);
		r.insert(s);
	}
	return r;
}

std::map<std::string, std::string> parse_arguments(int argc, char * argv[]) {
	std::string last_arg_name;
	std::map<std::string, std::string> result;

	// the first value in a vector is the default one
	std::map<std::string, std::vector<std::string>> allowed_values = {
		{"a", { "0", "2", "3" }},
		{"b", { "3", "2", "1", "0" }},
		{"d", { "2", "0", "1" }},
		{"D", { "0", "1" }},
		{"e", { "2", "0", "1" }},
		{"F", { "2", "0", "1", "3" }},
		{"G", { "2", "0", "1" }},
		{"i", { "0", "1" }},
		{"l", { "1", "0" }},
		{"m", { "0", "1", "2" }},
		{"n", { "1", "0" }},
		{"o", { "hoa", "dot" }},
		{"O", { "0", "1" }},
		{"p", { "2", "1", "3" }},
		{"s", { "1", "0" }},
		{"t", { "1", "0" }},
		{"u", { "1", "0" }},
		{"x", { "0", "1", "2", "3" }},
		{"X", { "0", "1" }},
		{"z", { "1", "0" }},
	};

	for (int i = 1; i < argc; ++i) {
		if (last_arg_name.empty()) {
			if (argv[i][0] == '-') {
				last_arg_name.assign(argv[i]);
				last_arg_name.erase(0, 1); // remove -

				// -? equals to -h
				if (last_arg_name[0] == '?') {
					last_arg_name[0] = 'h';
				}

				// pad flags with no value
				if (last_arg_name == "h" || last_arg_name == "v") {
					last_arg_name += "1";
				}

				// break -x123 into "x" => "123"
				if (last_arg_name.size() >= 2) {
					result[last_arg_name.substr(0, 1)] = last_arg_name.substr(1);
					last_arg_name.clear();
				}
			} else {
				result.clear();
				break;
			}
		} else {
			result[last_arg_name].assign(argv[i]);
			last_arg_name.clear();
		}
	}

	if (!last_arg_name.empty()) {
		result.clear();
	}

	// simulation of LTL2BA means default values -d0 -u0 -X0 -n0
	// simulating LTL3BA means -u0 -i1 -X1 -n1
	if (result.count("a") > 0) {
		std::set<std::string> params_default_null;
		std::set<std::string> params_default_true;

		if (result["a"] == "2") {
			params_default_null = { "d", "u", "n" };
			allowed_values["e"] = { "1", "0", "2" };
		} else if (result["a"] == "3") {
			params_default_null = { "u", "n" };
			params_default_true = { "i", "X" };
		}

		for (auto param : params_default_null) {
			allowed_values[param] = { "0", "1" };
		}

		for (auto param : params_default_true) {
			allowed_values[param] = { "1", "0" };
		}
	}

	// if we cleared the result for some reason,
	// return it, the input was wrong
	if (result.empty()) {
		return result;
	}

	for (auto& val : allowed_values) {
		if (result.count(val.first) == 0) {
			result[val.first] = val.second[0];
		} else if (std::find(val.second.begin(), val.second.end(), result[val.first]) == val.second.end()) {
			result.clear();
			break;
		}
	}

	return result;
}

// the comparison now works as follows:
// 1. return the smaller automaton (wrt. number of states)
// 2. choose deterministic automaton
// 3. choose semideterministic automaton
// 4. choose automaton with smaller number of acc. sets
// 5. return aut1
std::pair<spot::twa_graph_ptr, std::string> compare_automata(spot::twa_graph_ptr aut1, spot::twa_graph_ptr aut2, std::string stats_id1, std::string stats_id2) {
	auto ns1 = aut1->num_states();
	auto ns2 = aut2->num_states();

	auto p1 = std::make_pair(aut1, stats_id1);
	auto p2 = std::make_pair(aut2, stats_id2);

	auto det1 = spot::is_universal(aut1);
	auto det2 = spot::is_universal(aut2);

	if (o_deterministic) {
		if (det1 && !det2) {
			return p1;
		}

		if (det2 && !det1) {
			return p2;
		}
	}

	if (ns1 < ns2) {
		return p1;
	}

	if (ns2 < ns1) {
		return p2;
	}

	if (!o_deterministic) { // otherwise we have already tested this
		if (det1 && !det2) {
			return p1;
		}

		if (det2 && !det1) {
			return p2;
		}
	}

	auto as1 = aut1->acc().num_sets();
	auto as2 = aut2->acc().num_sets();

	if (as1 < as2) {
		return p1;
	}

	if (as2 < as1) {
		return p2;
	}

	auto ne1 = aut1->num_edges();
	auto ne2 = aut2->num_edges();

	if (ne1 < ne2) {
		return p1;
	}

	if (ne2 < ne1) {
		return p2;
	}

	auto sdet1 = spot::is_semi_deterministic(aut1);
	auto sdet2 = spot::is_semi_deterministic(aut2);

	if (sdet1 && !sdet2) {
		return p1;
	}

	if (sdet2 && !sdet1) {
		return p2;
	}

	return p1;
}

spot::formula simplify_formula(spot::formula f) {
	f = spot::negative_normal_form(spot::unabbreviate(f));

	if (o_simplify_formula) {
		spot::tl_simplifier tl_simplif;
		f = tl_simplif.simplify(f);
	}

	f = spot::unabbreviate(f);

	return f;
}

bool is_suspendable(spot::formula f) {
	if (f.is(spot::op::G)) {
		return f[0].is_eventual() || is_suspendable(f[0]);
	}

	if (f.is(spot::op::R)) {
		return (f[0].is_ff() && f[1].is_eventual()) || is_suspendable(f[1]);
	}

	if (f.is(spot::op::F)) {
		return f[0].is_universal() || is_suspendable(f[1]);
	}

	if (f.is(spot::op::U)) {
		return (f[0].is_tt() && f[1].is_universal()) || is_suspendable(f[1]);
	}

	if (f.is(spot::op::X)) {
		return is_suspendable(f[0]);
	}

	if (f.is(spot::op::And, spot::op::Or)) {
		for (auto g : f) {
			if (!is_suspendable(g)) {
				return false;
			}
		}
		return true;
	}

	return false;
}
