Overview
========

LTL3TELA is a tool that translates LTL formulae to deterministic or nondeterministic
automata. The translation follows in the following steps.
1. The formula is translated into an equivalent Self-loop Alternating Automaton (SLAA)
2. The SLAA is dealternated into nondeterministic automaton.
3. If needed, the nondeterministic automaton is determinized.

Requirements
============

The Spot library <https://spot.lrde.epita.fr/> has to be installed. Version
2.6 or higher is required for LTL3TELA to compile and work properly.

Installation
============
`make` should be enough to compile LTL3TELA.

Usage
=====
Use `./ltl3tela -f 'formula to translate'`.
See `./ltl3tela -h` for more information.

Experimental evaluation
=======================

The translation of LTL to deterministic and nondeterministic automata was submitted
for presentation at the [ATVA 2019](http://atva2019.iis.sinica.edu.tw/) conference.
Jupyter notebook [Evaluation_ATVA19](Experiments/Evaluation_ATVA19.ipynb) contains
scripts and other data to evaluate the performance compared to the state-of-the-art
tools. The impact of the merging was performed on the set of formulae traditionally
used to benchmark LTL translators and on 1000 randomly generated formulae.

### Requirements

If you would like to run the notebook by yourself, you need to have the
following tools installed in `PATH` on your system.

* [SPOT](https://spot.lrde.epita.fr/) v. 2.7.4 with Python bindings
* [Pandas](http://pandas.pydata.org/) Python library v. 20.3+
* [Jupyter](http://jupyter.org/) notebook v. 5.0+
* [LTL3TELA](https://github.com/jurajmajor/ltl3tela) v. 2.0.0
* [LTL3BA](https://sourceforge.net/projects/ltl3ba/) v. 1.1.3
* [Owl](https://owl.model.in.tum.de/) v. 18.06

Known bugs
==========

With the standard configuration of Spot, LTL3TELA is unable to set more than 32 acceptance
marks, therefore some larger formulae are only translated with Spot and not with standard
LTL3TELA translation (even if it would, in theory, produce smaller automaton). To specify
larger maximum number of acceptance marks, `./configure` Spot with `--enable-max-accsets=N`.
