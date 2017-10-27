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

Experimental evaluation
=======================

The translation used in LTL3TELA was submitted for presentation of the
[TACAS'18](http://www.etaps.org/index.php/2018/tacas) conference. 
Jupyter notebook [Evaluation_TACAS18](Experiments/Evaluation_TACAS18.ipynb)
contains scripts and other data used for evaluation of the translations
presented there. In particular:
1. evaluation of impact of $\mathsf{F}$- and $\mathsf{F,G}$-merging on the size
  of produced automata, and
2. comparison of LTL3TELA to LTL3BA and SPOT,

on a benchmark of 500 LTL($\mathsf{F}$,$\mathsf{G}$) formulae.

The original evaluation contained a typo in LTL3TELA settings - the Spot's 
reductions were disabled by accident. To see the evaluation where these are
enabled (as claimed in the paper), see 
[Evaluation_TACAS18_corrected](Experiments/Evaluation_TACAS18_corrected.ipynb). The
results are even better for LTL3TELA.

### Requirements

If you would like to run the notebook by yourself, you need to have the 
folowing tools installed in `PATH` on your system.

* [SPOT](https://spot.lrde.epita.fr/) v. 2.4+ with Python bindings
* [Pandas](http://pandas.pydata.org/) Python library v. 20.3+
* [Jupyter](http://jupyter.org/) notebook v 5.0+
* [LTL3TELA](https://github.com/jurajmajor/ltl3tela) v 1.1
* [LTL3BA](https://sourceforge.net/projects/ltl3ba/) v. 1.1.3+