ATVA19 evaluation
=================

Please see the [Jupyter notebook](Experiments/Evaluation_ATVA19.ipynb) with all results,
namely the commands to call the compared tools, procedure to generate the random formulae,
cummulative numbers of states, edges and acceptance marks for each dataset and group of
tools, cross comparisons, formulae excluded from the evaluation with the partial results,
and formulae for which the difference between best and worst tool is larger than the
specified threshold.

If the link doesn't work, please open the notebook in
[NBViewer](https://nbviewer.jupyter.org/github/jurajmajor/ltl3tela/blob/master/Experiments/Evaluation_ATVA19.ipynb).

The following input data were used to evaluate the tools:

* [Set of 1000 random formulae](Experiments/formulae/atva19/rand.ltl)
* [Formulae from literature and scalable patterns](Experiments/formulae/atva19/patterns.ltl)

The random formulae have been generated using the function `generate` in the notebook
(however, it will produce different formulae on other computers). The following command
generated the formulae from literature and patterns:
```
genltl --and-f=1..5 --and-fg=1..5 --and-gf=1..5 --ccj-alpha=1..5 --ccj-beta=1..5 --ccj-beta-prime=1..5 --dac-patterns --eh-patterns --fxg-or=1..5 --gf-equiv=1..5 --gf-equiv-xn=1..5 --gf-implies=1..5 --gf-implies-xn=1..5 --gh-q=1..5 --gh-r=1..5 --go-theta=1..5 --gxf-and=1..5 --hkrss-patterns --kr-n=1..5 --kr-nlogn=1..5 --kv-psi=1..5 --ms-example=1..5 --ms-phi-h=1..5 --ms-phi-r=1..5 --ms-phi-s=1..5 --or-fg=1..5 --or-g=1..5 --or-gf=1..5 --p-patterns --r-left=1..5 --r-right=1..5 --rv-counter=1..5 --rv-counter-carry=1..5 --rv-counter-carry-linear=1..5 --rv-counter-linear=1..5 --sb-patterns --sejk-f=1..5 --sejk-j=1..5 --sejk-k=1..5 --sejk-patterns=1..3 --tv-f1=1..5 --tv-f2=1..5 --tv-g1=1..5 --tv-g2=1..5 --tv-uu=1..5 --u-left=1..5 --u-right=1..5 | ltlfilt -u
```

CSV files with the measured data are available:

* [Deterministic automata, random formulae](Experiments/formulae/atva19/det.rand.csv)
* [Deterministic automata, patterns](Experiments/formulae/atva19/det.patterns.csv)
* [Nondeterministic automata, random formulae](Experiments/formulae/atva19/nondet.rand.csv)
* [Nondeterministic automata, patterns](Experiments/formulae/atva19/nondet.patterns.csv)

### Additional data

Moreover, we measured the performance of translators to deterministic automata (over the same
sets of input data) in a way that the automata produced by Delag and Rabinizer 4 have been
simplified using the Spot optimization procedures (using the `autfilt` tool). The measured data can
be found in [Evaluation_ATVA19_Autfilt](Experiments/Evaluation_ATVA19_Autfilt.ipynb) notebook.
