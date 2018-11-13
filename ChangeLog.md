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
