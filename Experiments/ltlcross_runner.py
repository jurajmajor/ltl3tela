# -*- coding: utf-8 -*-
import subprocess
import sys
import os.path
import spot
from IPython.display import SVG
import pandas as pd
from experiments_lib import hoa_to_spot, dot_to_svg, pretty_print


def create_ltl3hoa_cmd(ltlcross_cmd):
    '''Given a command inltlcross'es format to produce a NA by LTL3HOA, it
    induce the LTL3HOA's options and produce appropriete VWAA.

    Returns
    =======
    An SVG Jupyter-object.
    '''
    if "ltl3ba" in ltlcross_cmd:
        return "ltl3hoa -F0 -G0 -X1 -i1 -d1"
    if "ltl2ba" in ltlcross_cmd:
        return "ltl3hoa -F0 -G0 -X0 -i0 -d0"

    s = str(ltlcross_cmd)
    for token in ["-f", "%f", "%O", ">", "-p 2", "-p2", "-o", "dot",
                  " | autfilt --high", "-z0", "-z1", "-z 0", "-z 1"]:
        s = s.replace(token, "")
    # print(' '.join(s.split()))
    return ' '.join(s.split())


class LtlcrossRunner(object):
    """A class for running Spot's `ltlcross` and storing and manipulating
    its results. For LTL3HOA it can also draw very weak alternating automata
    (VWAA).

    Parameters
    ----------
    tools : a dict (String -> String)
        The records in the dict of the form ``name : ltlcross_cmd``
    >>> tools = {"LTL3HOA"    : "ltl3hoa -d -x -i -p 2 -f %f > %O",
    >>>          "SPOT":   : "ltl2tgba"
    >>>         }

    formula_files : a list of strings
        paths to files with formulas to be fed to `ltlcross`
    res_filename : String
        filename to store the ltlcross`s results
    cols : list of Strings, default ``['states','edges','transitions']``
        names of ltlcross's statistics columns to be recorded
    """
    def __init__(self, tools,
                 formula_files=['formulae/classic.ltl'],
                 res_filename='na_comp.csv',
                 cols=['states', 'edges', 'transitions'],
                ):
        self.tools = tools
        self.mins = []
        self.f_files = formula_files
        self.cols = cols
        self.automata = None
        self.values = None
        self.form = None
        if res_filename == '' or res_filename is None:
            self.res_file = '_'.join(tools.keys()) + '.csv'
        else:
            self.res_file = res_filename

    def run_ltlcross(self, automata=True, check=False, timeout='300'):
        """Removes any older version of ``self.res_file`` and runs `ltlcross`
        on all tools.
        """
        ### Prepare ltlcross command ###
        subprocess.call(["rm", "-f", self.res_file])

        tools_strs = ["{"+name+"}" + cmd for (name, cmd) in self.tools.items()]

        ## Run ltlcross ##
        args = tools_strs +  ' '.join(['-F '+F for F in self.f_files]).split()
        if timeout:
            args.append('--timeout='+timeout)
        if automata:
            args.append('--automata')
        if not check:
            args.append('--no-checks')
        args.append('--products=0')
        args.append('--csv='+self.res_file)
        subprocess.call(["ltlcross"] + args)

    def parse_results(self):
        """Parses the ``self.res_file`` and sets the values, automata, and
        form. If there are no results yet, it runs ltlcross before.
        """
        if not os.path.isfile(self.res_file):
            self.run_ltlcross()
        res = pd.read_csv(self.res_file)
        # Removes unnecessary parenthesis from formulas
        res.formula = res['formula'].map(pretty_print)

        form = pd.DataFrame(res.formula.drop_duplicates())
        form['form_id'] = range(len(form))
        form.index = form.form_id
        #form['interesting'] = form['formula'].map(is_interesting)

        res = form.merge(res)
        # Shape the table
        table = res.set_index(['form_id', 'formula', 'tool'])
        table = table.unstack(2)
        table.axes[1].set_names(['column','tool'],inplace=True)

        # Create separate tables for automata
        automata = table[['automaton']]

        # Removes formula column from the index
        automata.index = automata.index.levels[0]

        # Removes `automata` from column names -- flatten the index
        automata.columns = automata.columns.levels[1]
        form = form.set_index(['form_id', 'formula'])

        # stores the followed columns only
        values = table[self.cols]
        self.form = form
        self.values = values.sort_index(axis=1,level=['column','tool'])
        # self.compute_best("Minimum")
        self.automata = automata

    def compute_best(self, tools=None, colname="Minimum"):
        """Computes minimum values over tools in ``tools`` for all
        formulas and stores them in column ``colname``.
        
        Parameters
        ----------
        tools : list of Strings
            column names that are used to compute the min over
        colname : String
            name of column used to store the computed values
        """
        if tools is None:
            tools = list(self.tools.keys())
        self.mins.append(colname)
        for col in self.cols:
            self.values[col, colname] = self.values[col][tools].min(axis=1)
        self.values.sort_index(axis=1, level=0, inplace=True)

    def aut_for_id(self, form_id, tool, sets=False):
        """For given formula id and tool it returns the corresponding
        non-deterministic automaton as a Spot's object.

        Parameters
        ----------
        form_id : int
            id of formula to use
        tool : String
            name of the tool to use to produce the automaton
        sets : Bool
            if ``True``, disable Spot's simplifications and thus
            keep the original state names. **MAY BE DIFFERENT FROM
            THE ORIGINAL AUTOMATON**
        """
        if self.automata is None:
            raise AssertionError("No results parsed yet")
        if tool not in self.tools.keys():
            raise ValueError(tool)
        if not sets:
            return hoa_to_spot(self.automata.loc[form_id, tool])

        cmd = create_ltl3hoa_cmd(self.tools[tool])
        cmd += ' -p2 -o hoa -z0 -u0'
        f = self.form_of_id(form_id, False)
        ltl3hoa = subprocess.Popen(cmd.split() + ["-f", f],
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = ltl3hoa.communicate()
        if stderr:
            print("Calling the translator produced the message:\n" +
                  stderr.decode('utf-8'), file=sys.stderr)
        ret = ltl3hoa.wait()
        if ret:
            raise subprocess.CalledProcessError(ret, 'translator')
        hoa = stdout.decode('utf-8')
        return hoa_to_spot(hoa)

    def cummulative(self, col="states"):
        """Returns table with cummulative numbers of given ``col``.

        Parameters
        ---------
        col : String
            One of the followed columns (``states`` default)
        """
        return self.values[col].dropna().sum()

    def smaller_than(self, tool1, tool2,
                     restrict=True,
                     col='states', restrict_cols=True):
        """Returns a dataframe with results where ``col`` for ``tool1``
        has strictly smaller value than ``col`` for ``tool2``.

        Parameters
        ----------
        tool1 : String
            name of tool for comparison (the better one)
            must be among tools
        tool2 : String
            name of tool for comparison (the worse one)
            must be among tools
        restrict : Boolean, default ``True``
            if ``True``, the returned DataFrame contains only the compared
            tools
        col : String, default ``'states'``
            name of column use for comparison.
        restrict_cols : Boolean, default ``True``
            if ``True``, show only the compared column
        """
        if tool1 not in list(self.tools.keys())+self.mins:
            raise ValueError(tool1)
        if tool2 not in list(self.tools.keys())+self.mins:
            raise ValueError(tool2)
        if col not in self.cols:
            raise ValueError(col)
        v = self.values
        res = v[v[col][tool1] < v[col][tool2]]
        if restrict:
            res = res.loc(axis=1)[:, [tool1, tool2]]
        if restrict_cols:
            res = res[col]
        return res

    def vwaa_for_id(self, form_id, tool, unreachable=False):
        """For given formula id and tool it returns the corresponding
        non-deterministic automaton as a Spot's object

        Parameters
        ----------
        form_id : int
            id of formula to use
        tool : String
            Name of the tool (and options) to use to produce the automaton.
            It uses the ``tools`` dict to gather the correct options.
        unreachable : Bool
            Prints also unreachable states if ``True``. Default ``False``
        """
        if self.automata is None:
            raise AssertionError("No results parsed yet")
        if tool not in self.tools.keys():
            raise ValueError(tool)
        cmd = create_ltl3hoa_cmd(self.tools[tool])
        cmd += ' -p1 -o dot'
        if unreachable:
            cmd += ' -z0'
        f = self.form_of_id(form_id, False)
        ltl3hoa = subprocess.Popen(cmd.split() + ["-f", f],
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = ltl3hoa.communicate()
        if stderr:
            print("Calling the translator produced the message:\n" +
                  stderr.decode('utf-8'), file=sys.stderr)
        ret = ltl3hoa.wait()
        if ret:
            raise subprocess.CalledProcessError(ret, 'translator')
        dot = stdout.decode('utf-8')
        return SVG(dot_to_svg(dot))

    def form_of_id(self, form_id, spot_obj=True):
        """For given form_id returns the formula

        Parameters
        ----------
        form_id : int
            id of formula to return
        spot_obj : Bool
            If ``True``, returns Spot formula object (uses Latex to
            print the formula in Jupyter notebooks)
        """
        f = self.values.index[form_id][1]
        if spot_obj:
            return spot.formula(f)
        return f
