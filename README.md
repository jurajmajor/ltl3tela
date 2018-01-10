Overview
========

LTL3TELA is a tool that translates LTL formulae to nondeterministic automata.

Requirements
============

The Spot library <https://spot.lrde.epita.fr/> has to be installed. Version
2.4 or higher is required for LTL3TELA to compile and work properly.

Installation
============
`make` should be enough to compile LTL3TELA.

Usage
=====
Use `./ltl3tela -f 'formula to translate'`.
See `./ltl3tela -h` for more information.

Known bugs
==========

LTL3TELA is unable to set more than 32 acceptance marks, therefore some larger
formulae cannot be translated. This is a limitation of Spot.
