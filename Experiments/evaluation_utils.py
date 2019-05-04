import pandas as pd

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

def split_cols(res,symbol='/',axis=1,names=None,repl_empty=True):
    tmp = res.copy()
    if axis == 1:
        source = tmp.columns
        dest = tmp.columns
    elif axis == 0:
        source = tmp.index
        dest = tmp.index
    ## To handle victories in cross-comparisons correctly
    source = ['{}//'.format(t) if t == 'V' else t for
                      t in source]
    tuples = to_tuples(source,symbol)
    if repl_empty:
        tuples = [tuple(t[i] if t[i] != '' else '---' for
                        i in range(len(t))) for t in tuples]
    new = pd.MultiIndex.from_tuples(tuples,sortorder=None,names=names)
    if axis == 1:
        tmp.columns = new
    elif axis == 0:
        tmp.index = new
    return tmp #tmp.sort_index(axis=axis,level=[0],sort_remaining=False)

def high_min(data):
    return ['\high ${:0.0f}$'.format(m) if 
            m == data.min() else m for m in data]

def high_max(data):
    return ['\high ${:0.0f}$'.format(m) if 
            m == data.max() else m for m in data]

def fix_tool(t):
    t = t.replace('-D',' -D')
    t = t.replace('-G',' -G')
    t = t.replace('delag','Delag')
    t = t.replace('rabinizer4','Rabinizer 4\\tgdra')
    t = t.replace('ltl3ba','ltl3ba -H2\\tgba')
    if t == 'ltl2tgba':
        t = 'ltl2tgba\\tgba'
    if t.startswith('ltl'):
        t = f'\\spottool{{{t}}}'
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