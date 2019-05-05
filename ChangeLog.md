## [2.0.0] - 2019-05-04

* It is now possible to generate deterministic automata with `-D1` flag.
* The input formula is split to temporal formulae which are translated independently.
* LTL3TELA now employs Spotela reduction method to reduce the state space of TGBA.
* Spot is used to translate both formula and its negation.
* An extended check of SLAA edge dominance has been implemented.
* Minor optimization of translation of disjunction to SLAA has been added.
* More complex automata comparison (for choosing the best one) has been implemented.
* If >32 acceptance sets were used during the translation, we still use Spot to produce the result.
* With `-x1`, LTL3TELA prints some statistics about chosen translation tool(s) to STDERR.

## [1.2.1] - 2018-11-13

* Fixed segfault with -p1.
* Support for -m2 (detection of mergeable G for experimental purposes) added.

## [1.2.0] - 2018-07-07
* LTL3TELA now tries to translate the formula with Spot and outputs the smaller automaton.
* When both the original automaton and the complemented automaton for !f have the same number of states, the latter is preferred. This should produce deterministic automata in more cases.
* -v now outputs the used version of Spot.

## [1.1.2] - 2018-05-24
* Replace incorrect usage of spot::acc_cond::mark_t::value_t with an alias of unsigned.

## [1.1.1] - 2018-01-10
* FIX: segafault when a fresh initial state is created
* If more than 32 acceptance sets are used (limitation of Spot) exit with return code `32` instead of segfault

## [1.1.0] - 2017-10-20
* The tool is now named LTL3TELA
* LTL3TELA now also translates the negation of formula; if the resulting automaton is deterministic and smaller than the original automaton, its complement is used as an output
* Spot 2.4+ is now required to compile LTL3TELA

## [1.0.1] - 2017-01-16
* hide "Garbage collection" messages from BuDDy
* set the name of produced nondeterministic automaton
* remove unnecessary call to purge_unreachable_states
* catch exception thrown by parse_formula

## [1.0.0] - 2017-01-09
### Added
* LTL3HOA translates the LTL formulae to alternating or nondeterministic automata with generic acceptance condition.
