Overview
========

LTL3TELA is a tool that translates LTL formulae to nondeterministic automata. The translation follows in two steps.
1. The formula is translated into a an equivalent Self-loop Alternating Automaton (SLAA)
2. The SLAA is deealternated into nondeterministic automaton.

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

Experimental evaluation
=======================

The first step of the translation used in LTL3TELA (LTL to SLAA) was submitted for presentation at the
[LPAR-22](https://easychair.org/smart-program/LPAR-22/) conference.
Jupyter notebook [Evaluation_LPAR18](Experiments/Evaluation_LPAR18.ipynb)
contains scripts and other data used to evaluate performance of F-merging and F,G-merging introduced in the paper. Their performance is also compared to translation implemented in the tool [LTL3BA](https://sourceforge.net/projects/ltl3ba/). The impact of the merging was performed on the set of formulae traditionally used to benchmark LTL translators and on 1000 randomly generated formulae that contain only operators F and G.

### Requirements

If you would like to run the notebook by yourself, you need to have the
folowing tools installed in `PATH` on your system.

* [SPOT](https://spot.lrde.epita.fr/) v. 2.6.1 with Python bindings
* [Pandas](http://pandas.pydata.org/) Python library v. 20.3+
* [Jupyter](http://jupyter.org/) notebook v 5.0+
* [LTL3TELA](https://github.com/jurajmajor/ltl3tela) v 1.2.0
* [LTL3BA](https://sourceforge.net/projects/ltl3ba/) v. 1.1.3

Known bugs
==========

LTL3TELA is unable to set more than 32 acceptance marks, therefore some larger
formulae cannot be translated. This is a limitation of Spot.
