import pandas as pd
import re

def sort_by_tools(df, tool_order, axis=0):
    rows = []
    for tool in tool_order:
        rows.append(df.loc(axis=axis)[[tool]])
    return pd.concat(rows,axis=axis)

def to_tuples(cols,symbol='/'):
    res = []
    for i in cols:
        res.append(tuple(i.split(symbol)))
    return res

def split_cols(res,delimiter='/',axis=1,names=None,repl_empty=True):
    tmp = res.copy()
    if axis == 1:
        source = tmp.columns
        dest = tmp.columns
    elif axis == 0:
        source = tmp.index
        dest = tmp.index
    tuples = to_tuples(source,delimiter)
    if repl_empty:
        tuples = [tuple(t[i] if t[i] != '' else '---' for
                        i in range(len(t))) for t in tuples]
    new = pd.MultiIndex.from_tuples(tuples,sortorder=None,names=names)
    if axis == 1:
        tmp.columns = new
    elif axis == 0:
        tmp.index = new
    return tmp #tmp.sort_index(axis=axis,level=[0],sort_remaining=False)

def high_min(data, skip_zero=True):
    return ['\high ${:0.0f}$'.format(m) if 
            m == data.min() and not (skip_zero and m == 0)
            else m for m in data]

def high_max(data):
    return ['\high ${:0.0f}$'.format(m) if 
            m == data.max() else m for m in data]

def highlight_by_level(df, high_fcn):
    '''Highlights df with multiindex by high_fcn, each part separately
    '''
    parts = []
    names = []
    idx = df.index
    lab = 0
    while lab < len(df):
        # How many rows do we have for the part
        label_index = idx.codes[0][lab]
        level_length = len(df.loc[idx.levels[0][label_index]])
        
        label = idx.levels[0][label_index]
        parts.append(df.loc(axis=0)[label].astype(float).apply(high_fcn))
        names.append(label)
        
        lab += level_length 
    return pd.concat(parts, keys=names)

def fix_lines(filename, end=9, vertical=False):
    end = str(end)
    with open(filename) as f:
        lines = f.read()
    lines = lines.replace('cline','cmidrule')
    lines = lines.replace('$nan$','---')
    with open(filename,"w") as f1:
        f1.write(lines)

def fix_tool(t):
    t = t.replace('-D',' -D')
    t = t.replace('-G',' -G')
    t = t.replace('delag','Delag')
    t = t.replace('rabinizer4','Rabinizer 4\\tgdra')
    t = t.replace('ltl3ba','ltl3ba -H2\\tgba')
    if t == 'ltl2tgba':
        t = 'ltl2tgba\\tgba'
    if t.startswith('ltl'):
        t = '\\spottool{{{}}}'.format(t)
    return t

def fix_type(t,vertical=False,color=None):
    t = t.replace('deterministic','det.')
    if color:
        source, t = t.split('_')
        t = '\\parbox[c]{{12.9ex}}{{\centering {}}}'.format(t)
        if source == color:
            t = high_frag(t)
    if vertical:
        t = '\rotatebox[origin=c]{90}{\\footnotesize ' + t + '}'
    return t
        
def fix_header_colors(lines):
    '''Make each but second occurence of multicols {c} argument a {b}.
    '''
    lines = lines.replace('{c}','{b}',1)
    i = lines.find('{c}')
    return lines[:i+1] + lines[i+1:].replace('{c}','{b}',1)

def color_table(filename, extrarowheight='.75ex'):
    setup = '''\\newcolumntype{{a}}{{>{{\\columncolor{{blue!10}}}}r}}
\\newcolumntype{{b}}{{>{{\\columncolor{{blue!10}}}}c}}
\\setlength{{\\aboverulesep}}{{0pt}}
\\setlength{{\\belowrulesep}}{{0pt}}
\\setlength{{\\extrarowheight}}{{{}}}
\\setlength{{\\heavyrulewidth}}{{.8pt}}
\\def\\high{{\\cellcolor{{green!80!black!30}}}}
\\setlength\\tabcolsep{{.6em}}
'''.format(extrarowheight)
    with open(filename) as f:
        lines = f.read()
    lines = fix_header_colors(lines)
    with open(filename,"w") as f1:
        f1.write(setup + lines)
        
def get_tabular_cols(df, color=True, first_col_centered=True):
    col_f = 'c' if first_col_centered else 'r'
    col_f += 'r' * (len(df.index.levels)-1)
    color_type = 'a' if color else 'r'
    cols = df.columns
    # Iterate over outermost-columns
    # The multiindex is a tuple:
    #  * levels [list of lists of column names]
    #  * labels [list of lists of pointers to levels] (the inner
    #                           lists have size = number of columns)
    # Each column col on level lev is labeled in the table by the
    # name stored in levels[lev][labels[lev][col]]
    lab = 0
    i = 0
    while lab < len(cols.codes[0]):
        # How many columns do we have for the current outermost-level column
        label_index = cols.codes[0][lab]
        level_length = len(df[cols.levels[0][label_index]].columns)
        # color if even
        if i % 2 == 0:
            col_f += color_type*level_length
        else:
            col_f += 'r'*level_length
        # i for colors, lab for iterating over outermost-level columns
        i += 1
        lab += level_length 
    return col_f

def prepare_for_latex(df):
    ci = [(fix_type(t[0],True,None),fix_tool(t[1])) for t in df.index.values]
    df.index=pd.MultiIndex.from_tuples(ci)
    return df
        
def cummulative_to_latex(res,filename,transpose=False,color=True):
    if transpose:
        res = res.T
    
    res = prepare_for_latex(res)
    col_f = get_tabular_cols(res, color)
    
    res.to_latex(buf=open(filename,'w'), multirow=True,
         escape=False, na_rep='---',
         float_format=lambda x: '$' + '%0.0f' % x + '$',
         column_format=col_f, multicolumn_format='c')
    if color:
        color_table(filename,extrarowheight='.2ex')
        

def fix_heading(filename):
    with open(filename) as f:
        lines = f.read()
    # Move the tools column label one line up
    lines = re.sub(r"\s+&\s+&\s+states", r"& tool & states", lines)
    lines = re.sub(r"\{\}\s+&\s+tool\s+&\s+&.*\\\\", r"%", lines)
    with open(filename,"w") as f1:
        f1.write(lines)
        
def fix_spacing(filename):
    '''Remove the spacing at the start of lines generated by 
    df.to_latex()'''
    with open(filename) as f:
        lines = f.read()
    # Remove spacing from the beginings of lines
    lines = re.sub(r"^\s+&", r" &", lines, flags=re.M)
    with open(filename,"w") as f1:
        f1.write(lines)
        
def get_table_width(df):
    id_l = len(df.index.levels)
    col_l = len(df.columns)
    return id_l + col_l
        
def fix_latex(df, filename):
    fix_heading(filename)
    fix_lines(filename, get_table_width(df))
    fix_spacing(filename)
