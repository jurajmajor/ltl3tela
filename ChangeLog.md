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
