# -*- coding: utf-8 -*-
import subprocess
import sys
import os.path
import re
import math
import spot
from IPython.display import SVG
from datetime import datetime
import pandas as pd
from experiments_lib import hoa_to_spot, dot_to_svg, pretty_print

def bogus_to_lcr(form):
    """Converts a formula as it is printed in ``_bogus.ltl`` file
    (uses ``--relabel=abc``) to use ``pnn`` AP names.
    """
    args = ['-r0','--relabel=pnn','-f',form]
    return subprocess.check_output(["ltlfilt"] + args, universal_newlines=True).strip()

def parse_check_log(log_f):
    """Parses a given log file and locates cases where
    sanity checks found some error.

    Returns:
    bugs: a dict: ``form_id``->``list of error lines``
    bogus_forms: a dict: ``form_id``->``form``
    tools: a dict: ``tool_id``->``command``
    """
    log = open(log_f,'r')
    bugs = {}
    bogus_forms = {}

    formula = re.compile('.*ltl:(\d+): (.*)$')
    empty_line = re.compile('^\s$')
    problem = re.compile('error: .* nonempty')

    for line in log:
        m_form = formula.match(line)
        if m_form:
            form = m_form
            f_bugs = []
        m_empty = empty_line.match(line)
        if m_empty:
            if len(f_bugs) > 0:
                form_id = int(form.group(1))-1
                bugs[form_id] = f_bugs
                bogus_forms[form_id] = form.group(2)
        m_prob = problem.match(line)
        if m_prob:
            f_bugs.append(m_prob.group(0))
    log.close()
    tools = parse_log_tools(log_f)
    return bugs, bogus_forms, tools

def find_log_for(tool_code, form_id, log_f):
    """Returns an array of lines from log for
    given tool code (P1,N3,...) and form_id. The
    form_id is taken from runner - thus we search for
    formula number ``form_id+1``
    """
    log = open(log_f,'r')
    current_f = -1
    formula = re.compile('.*ltl:(\d+): (.*)$')
    tool = re.compile('.*\[([PN]\d+)\]: (.*)$')
    gather = re.compile('Performing sanity checks and gathering statistics')
    output = []
    for line in log:
        m_form = formula.match(line)
        if m_form:
            current_f = int(m_form.group(1))
            curr_tool = ''
        if current_f < form_id+1:
            continue
        if current_f > form_id+1:
            break
        m_tool = tool.match(line)
        if m_tool:
            curr_tool = m_tool.group(1)
        if gather.match(line):
            curr_tool = 'end'
        if curr_tool == tool_code:
            output.append(line.strip())
    log.close()
    return output

def hunt_error_types(log_f):
    log = open(log_f,'r')
    errors = {}
    err_forms = {}

    formula = re.compile('.*ltl:(\d+): (.*)$')
    empty_line = re.compile('^\s$')
    tool = re.compile('.*\[([PN]\d+)\]: (.*)$')
    problem = re.compile('error: .*')
    nonempty = re.compile('error: (.*) is nonempty')

    for line in log:
        m_form = formula.match(line)
        if m_form:
            form = m_form
            f_bugs = {}
        m_tool = tool.match(line)
        if m_tool:
            tid = m_tool.group(1)
        m_empty = empty_line.match(line)
        if m_empty:
            if len(f_bugs) > 0:
                form_id = int(form.group(1))-1
                errors[form_id] = f_bugs
                err_forms[form_id] = form.group(2)
        m_prob = problem.match(line)
        if m_prob:
            prob = m_prob.group(0)
            m_bug = nonempty.match(line)
            if m_bug:
                prob = 'nonempty'
                tid = m_bug.group(1)
            if prob not in f_bugs:
                f_bugs[prob] = []
            f_bugs[prob].append(tid)
    log.close()
    tools = parse_log_tools(log_f)
    return errors, err_forms, tools

def parse_log_tools(log_f):
    log = open(log_f,'r')
    tools = {}
    tool = re.compile('.*\[(P\d+)\]: (.*)$')
    empty_line = re.compile('^\s$')
    for line in log:
        m_tool = tool.match(line)
        m_empty = empty_line.match(line)
        if m_empty:
            break
        if m_tool:
            tid = m_tool.group(1)
            tcmd = m_tool.group(2)
            tools[tid] = tcmd
    log.close()
    return tools

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
                 log_file=None,
                ):
        self.tools = tools
        self.mins = []
        self.f_files = formula_files
        self.cols = cols.copy()
        self.automata = None
        self.values = None
        self.form = None
        if res_filename == '' or res_filename is None:
            self.res_file = '_'.join(tools.keys()) + '.csv'
        else:
            self.res_file = res_filename
        if log_file is None:
            self.log_file = self.res_file[:-3] + 'log'
        else:
            self.log_file = log_file

    def create_args(self, automata=True, check=False, timeout='300',
                     log_file=None, res_file=None,
                     save_bogus=True, tool_subset=None,
                     forms = True, escape_tools=False):
        """Creates args that are passed to run_ltlcross
        """
        if log_file is None:
            log_file = self.log_file
        if res_file is None:
            res_file = self.res_file
        if tool_subset is None:
            tool_subset=self.tools.keys()

        ### Prepare ltlcross command ###
        tools_strs = ["{"+name+"}" + cmd for (name, cmd) in self.tools.items() if name in tool_subset]
        if escape_tools:
            tools_strs = ["'{}'".format(t_str) for t_str in tools_strs]
        args = tools_strs
        if forms:
            args +=  ' '.join(['-F '+F for F in self.f_files]).split()
        if timeout:
            args.append('--timeout='+timeout)
        if automata:
            args.append('--automata')
        if save_bogus:
            args.append('--save-bogus={}_bogus.ltl'.format(res_file[:-4]))
        if not check:
            args.append('--no-checks')
        #else:
        #    args.append('--reference={ref_Spot}ltl2tgba -H %f')
        args.append('--products=0')
        args.append('--csv='+res_file)
        return args

    def ltlcross_cmd(self, args=None, automata=True,
                     check=False, timeout='300',
                     log_file=None, res_file=None,
                     save_bogus=True, tool_subset=None,
                     forms=True, lcr='ltlcross'):
        """Returns ltlcross command for the parameters.
        """
        if log_file is None:
            log_file = self.log_file
        if res_file is None:
            res_file = self.res_file
        if tool_subset is None:
            tool_subset=self.tools.keys()
        if args is None:
            args = self.create_args(automata, check, timeout,
                                    log_file, res_file,
                                    save_bogus, tool_subset, forms,
                                    escape_tools=True)
        return ' '.join([lcr] + args)

    def run_ltlcross(self, args=None, automata=True,
                     check=False, timeout='300',
                     log_file=None, res_file=None,
                     save_bogus=True, tool_subset=None,
                     lcr='ltlcross'):
        """Removes any older version of ``self.res_file`` and runs `ltlcross`
        on all tools.

        Parameters
        ----------
        args : a list of ltlcross arguments that can be used for subprocess
        tool_subset : a list of names from self.tools
        """
        if log_file is None:
            log_file = self.log_file
        if res_file is None:
            res_file = self.res_file
        if tool_subset is None:
            tool_subset=self.tools.keys()
        if args is None:
            args = self.create_args(automata, check, timeout,
                                    log_file, res_file,
                                    save_bogus, tool_subset)

        # Delete ltlcross result and lof files
        subprocess.call(["rm", "-f", res_file, log_file])

        ## Run ltlcross ##
        log = open(log_file,'w')
        cmd = self.ltlcross_cmd(args,lcr=lcr)
        print(cmd, file=log)
        print(datetime.now().strftime('[%d.%m.%Y %T]'), file=log)
        print('=====================', file=log,flush=True)
        self.returncode = subprocess.call([lcr] + args, stderr=subprocess.STDOUT, stdout=log)
        log.writelines([str(self.returncode)+'\n'])
        log.close()

    def parse_results(self, res_file=None):
        """Parses the ``self.res_file`` and sets the values, automata, and
        form. If there are no results yet, it runs ltlcross before.
        """
        if res_file is None:
            res_file = self.res_file
        if not os.path.isfile(res_file):
            raise FileNotFoundError(res_file)
        res = pd.read_csv(res_file)
        # Add incorrect columns to track flawed automata
        if not 'incorrect' in res.columns:
            res['incorrect'] = False
        # Removes unnecessary parenthesis from formulas
        res.formula = res['formula'].map(pretty_print)

        form = pd.DataFrame(res.formula.drop_duplicates())
        form['form_id'] = range(len(form))
        form.index = form.form_id

        res = form.merge(res)
        # Shape the table
        table = res.set_index(['form_id', 'formula', 'tool'])
        table = table.unstack(2)
        table.axes[1].set_names(['column','tool'],inplace=True)

        # Create separate tables for automata
        automata = None
        if 'automaton' in table.columns.levels[0]:
            automata = table[['automaton']]

            # Removes formula column from the index
            automata.index = automata.index.levels[0]

            # Removes `automata` from column names -- flatten the index
            automata.columns = automata.columns.levels[1]
        form = form.set_index(['form_id', 'formula'])

        # Store incorrect and exit_status information separately
        self.incorrect = table[['incorrect']]
        self.incorrect.columns = self.incorrect.columns.droplevel()
        self.exit_status = table[['exit_status']]
        self.exit_status.columns = self.exit_status.columns.droplevel()

        # stores the followed columns only
        values = table[self.cols]
        self.form = form
        self.values = values.sort_index(axis=1,level=['column','tool'])
        # self.compute_best("Minimum")
        if automata is not None:
            self.automata = automata

    def compute_sbacc(self,col='states'):
        def get_sbacc(aut):
            if isinstance(aut, float) and math.isnan(aut):
                return None
            a = spot.automata(aut+'\n')
            aut = next(a)
            aut = spot.sbacc(aut)
            if col == 'states':
                return aut.num_states()
            if col == 'acc':
                return aut.num_sets()

        df = self.automata.copy()

        # Recreate the same index as for other cols
        n_i = [(l, self.form_of_id(l,False)) for l in df.index]
        df.index = pd.MultiIndex.from_tuples(n_i)
        df.index.names=['form_id','formula']
        # Recreate the same columns hierarchy
        df = df.T
        df['column'] = 'sb_{}'.format(col)
        self.cols.append('sb_{}'.format(col))
        df = df.set_index(['column'],append=True)
        df = df.T.swaplevel(axis=1)

        # Compute the requested values and add them to others
        df = df.applymap(get_sbacc)
        self.values = self.values.join(df)

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
        else:
            tools = [t for t in tools if t in self.tools.keys()
                    or t in self.mins]
        self.mins.append(colname)
        for col in self.cols:
            self.values[col, colname] = self.values[col][tools].min(axis=1)
        self.values.sort_index(axis=1, level=0, inplace=True)

    def aut_for_id(self, form_id, tool):
        """For given formula id and tool it returns the corresponding
        non-deterministic automaton as a Spot's object.

        Parameters
        ----------
        form_id : int
            id of formula to use
        tool : String
            name of the tool to use to produce the automaton
        """
        if self.automata is None:
            raise AssertionError("No results parsed yet")
        if tool not in self.tools.keys():
            raise ValueError(tool)
        return hoa_to_spot(self.automata.loc[form_id, tool])

    def cummulative(self, col="states"):
        """Returns table with cummulative numbers of given ``col``.

        Parameters
        ---------
        col : String
            One of the followed columns (``states`` default)
        """
        return self.values[col].dropna().sum()

    def smaller_than(self, t1, t2, reverse=False,
                     restrict=True,
                     col='states', restrict_cols=True):
        """Returns a dataframe with results where ``col`` for ``tool1``
        has strictly smaller value than ``col`` for ``tool2``.

        Parameters
        ----------
        t1 : String
            name of tool for comparison (the better one)
            must be among tools
        t2 : String
            name of tool for comparison (the worse one)
            must be among tools
        reverse : Boolean, default ``False``
            if ``True``, it switches ``tool1`` and ``tool2``
        restrict : Boolean, default ``True``
            if ``True``, the returned DataFrame contains only the compared
            tools
        col : String, default ``'states'``
            name of column use for comparison.
        restrict_cols : Boolean, default ``True``
            if ``True``, show only the compared column
        """
        return self.better_than(t1,t2,reverse=reverse,
                    props=[col],include_fails=False,
                    restrict_cols=restrict_cols,
                    restrict_tools=restrict)

    def better_than(self, t1, t2, props=['states','acc'],
                    reverse=False, include_fails=True,
                    restrict_cols=True,restrict_tools=True
                    ):
        """Compares ``t1`` against ``t2`` lexicographicaly
        on cols from ``props`` and returns DataFrame with
        results where ``t1`` is better than ``t2``.

        Parameters
        ----------
        t1 : String
            name of tool for comparison (the better one)
            must be among tools
        t2 : String
            name of tool for comparison (the worse one)
            must be among tools
        props : list of Strings, default (['states','acc'])
            list of columns on which we want the comparison (in order)
        reverse : Boolean, default ``False``
            if ``True``, it switches ``t1`` and ``t2``
        include_fails : Boolean, default ``True``
            if ``True``, include formulae where t2 fails and t1 does not
            fail
        restrict_cols : Boolean, default ``True``
            if ``True``, the returned DataFrame contains only the compared
            property columns
        restrict_tools : Boolean, default ``True``
            if ``True``, the returned DataFrame contains only the compared
            tools
        """
        if t1 not in list(self.tools.keys())+self.mins:
            raise ValueError(t1)
        if t2 not in list(self.tools.keys())+self.mins:
            raise ValueError(t2)
        if reverse:
            t1, t2 = t2, t1
        v = self.values
        t1_ok = self.exit_status[t1] == 'ok'
        if include_fails:
            t2_ok = self.exit_status[t2] == 'ok'
            # non-fail beats fail
            c = v[t1_ok & ~t2_ok]
            # We work on non-failures only from now on
            eq = t1_ok & t2_ok
        else:
            c = pd.DataFrame()
            eq = t1_ok
        for prop in props:
            # For each prop we add t1 < t2
            better = v[prop][t1] < v[prop][t2]
            # but only from those which were equivalent so far
            equiv_and_better = v.loc[better & eq]
            c = c.append(equiv_and_better)
            # And now choose those equivalent also on prop to eq
            eq = eq & (v[prop][t1] == v[prop][t2])

        # format the output
        idx = pd.IndexSlice
        tools = [t1,t2] if restrict_tools else slice(None)
        props = props if restrict_cols else slice(None)
        return c.loc[:,idx[props,tools]]

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

    def id_of_form(self, f, convert=False):
        """Returns id of a given formula. If ``convert`` is ``True``
        it also calls ``bogus_to_lcr`` first.
        """
        if convert:
            f = bogus_to_lcr(f)
        ni = self.values.index.droplevel(0)
        return ni.get_loc(f)

    def mark_incorrect(self, form_id, tool,output_file=None,input_file=None):
        """Marks automaton given by the formula id and tool as flawed
        and writes it into the .csv file
        """
        if tool not in self.tools.keys():
            raise ValueError(tool)
        # Put changes into the .csv file
        if output_file is None:
            output_file = self.res_file
        if input_file is None:
            input_file = self.res_file
        csv = pd.read_csv(input_file)
        if not 'incorrect' in csv.columns:
            csv['incorrect'] = False
        cond = (csv['formula'].map(pretty_print) ==
                pretty_print(self.form_of_id(form_id,False))) &\
                (csv.tool == tool)
        csv.loc[cond,'incorrect'] = True
        csv.to_csv(output_file,index=False)

        # Mark the information into self.incorrect
        self.incorrect.loc[self.index_for(form_id)][tool] = True

    def na_incorrect(self):
        """Marks values for flawed automata as N/A. This causes
        that the touched formulae will be removed from cummulative
        etc. if computed again. To reverse this information you
        have to parse the results again.

        It also sets ``exit_status`` to ``incorrect``
        """
        self.values = self.values[~self.incorrect]
        self.exit_status[self.incorrect] = 'incorrect'

    def index_for(self, form_id):
        return (form_id,self.form_of_id(form_id,False))

    def get_error_count(self,err_type='timeout',drop_zeros=True):
        """Returns a Series with total number of er_type errors for
        each tool.

        Parameters
        ----------
        err_type : String one of `timeout`, `parse error`,
                                 `incorrect`, `crash`, or
                                 'no output'
                  Type of error we seek
        drop_zeros: Boolean (default True)
                    If true, rows with zeros are removed
        """
        if err_type not in ['timeout', 'parse error',
                            'incorrect', 'crash',
                            'no output']:
            raise ValueError(err_type)

        if err_type == 'crash':
            c1 = self.exit_status == 'exit code'
            c2 = self.exit_status == 'signal'
            res = (c1 | c2).sum()
        else:
            res = (self.exit_status == err_type).sum()
        if drop_zeros:
            return res.iloc[res.nonzero()]
        return res

    def cross_compare(self,tools=None,props=['states','acc'],
                      include_fails=True, total=True,
                      include_other=True):
        def count_better(tool1,tool2):
            if tool1 == tool2:
                return float('nan')
            try:
                return len(self.better_than(tool1,tool2,props,
                               include_fails=include_fails))
            except ValueError as e:
                if include_other:
                    return float('nan')
                else:
                    raise e
        if tools is None:
            tools = self.tools.keys()
        c = pd.DataFrame(index=tools, columns=tools).fillna(0)
        for tool in tools:
            c[tool] = pd.DataFrame(c[tool]).apply(lambda x:
                      count_better(x.name,tool), 1)
        if total:
            c['V'] = c.sum(axis=1)
        return c

    def min_counts(self, tools=None, restrict_tools=False, unique_only=False, col='states',min_name='min(count)'):
        if tools is None:
            tools = list(self.tools.keys())
        else:
            tools = [t for t in tools if
                     t in self.tools.keys() or
                     t in self.mins]
        min_tools = tools if restrict_tools else list(self.tools.keys())
        self.compute_best(tools=min_tools, colname=min_name)
        s = self.values.loc(axis=1)[col]
        df = s.loc(axis=1)[tools+[min_name]]
        is_min = lambda x: x[x == x[min_name]]
        best_t_count = df.apply(is_min, axis=1).count(axis=1)
        choose = (df[best_t_count == 2]) if unique_only else df
        choose = choose.index
        min_counts = df.loc[choose].apply(is_min,axis=1).count()
        return pd.DataFrame(min_counts[min_counts.index != min_name])

def param_runner(name, tools, data_dir='data_param'):
    cols=["states","transitions","acc","time","nondet_states"]
    r = LtlcrossRunner(tools,\
        res_filename='{}/{}.csv'.format(data_dir,name),\
        formula_files=['formulae/{}.ltl'.format(name)],\
        cols=cols)
    return r