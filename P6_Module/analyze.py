"""Summaries, uncertainty, paired speedups and plots. Requires matplotlib only."""
import argparse, csv, collections, pathlib, random, statistics
p=argparse.ArgumentParser();p.add_argument('directory');a=p.parse_args();root=pathlib.Path(a.directory)
rows=[]
for f in sorted(root.glob('*_raw.csv')):
    with f.open() as stream: rows.extend(csv.DictReader(stream))
if not rows: raise SystemExit('No raw results found')
groups=collections.defaultdict(list)
for r in rows: groups[tuple(r[k] for k in ('backend','n','rate','distribution'))].append(r)
rng=random.Random(314159)
def interval(v):
    if len(v)<5:return ('','')
    means=sorted(statistics.mean(rng.choices(v,k=len(v))) for _ in range(2000))
    return means[49],means[1949]
summary=[]
for key,rs in sorted(groups.items()):
    v=[float(r['operations_per_second']) for r in rs];lo,hi=interval(v)
    summary.append(dict(zip(('backend','n','rate','distribution'),key),trials=len(v),mean_ops_s=statistics.mean(v),median_ops_s=statistics.median(v),sd_ops_s=statistics.stdev(v) if len(v)>1 else '',ci95_low=lo,ci95_high=hi,min_seconds=min(float(r['seconds']) for r in rs)))
def save(name,data,fields=None):
    with (root/name).open('w',newline='') as f:
        w=csv.DictWriter(f,fieldnames=fields or list(data[0]));w.writeheader();w.writerows(data)
save('summary.csv',summary)
# One ratio per shared workload seed: speedup >1 means heap faster.
paired=collections.defaultdict(dict)
for r in rows:
    if r['backend'] in ('heap','avl'):
        key=tuple(r[k] for k in ('n','rate','distribution','seed','repeat'))
        if r['backend'] in paired[key]: raise SystemExit('Duplicate paired trial')
        paired[key][r['backend']]=float(r['seconds'])
speed=collections.defaultdict(list)
for key,values in paired.items():
    if set(values)=={'heap','avl'}:speed[key[:3]].append(values['avl']/values['heap'])
speeds=[]
for key,values in sorted(speed.items()):
    lo,hi=interval(values)
    speeds.append(dict(zip(('n','rate','distribution'),key),pairs=len(values),mean_heap_speedup=statistics.mean(values),ci95_low=lo,ci95_high=hi))
save('paired_speedup.csv',speeds,['n','rate','distribution','pairs','mean_heap_speedup','ci95_low','ci95_high'])
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
# Both useful views, keeping distribution and size/rate controlled.
for axis,fixed in [('n','rate'),('rate','n')]:
    for dist,fix in sorted({(s['distribution'],s[fixed]) for s in summary}):
        fig,ax=plt.subplots(figsize=(7,4))
        for name in sorted({s['backend'] for s in summary}):
            ss=sorted([s for s in summary if s['distribution']==dist and s[fixed]==fix and s['backend']==name],key=lambda s:float(s[axis]))
            if not ss:continue
            ax.plot([float(s[axis]) for s in ss],[s['mean_ops_s'] for s in ss],'-o',label=name)
            if all(s['ci95_low']!='' for s in ss):ax.fill_between([float(s[axis]) for s in ss],[s['ci95_low'] for s in ss],[s['ci95_high'] for s in ss],alpha=.15)
        if axis=='n':ax.set_xscale('log')
        ax.set(xlabel='Live queue size N' if axis=='n' else 'Cancellation fraction of removals',ylabel='Event-set API calls / second',title=f'{dist}; {fixed}={fix}')
        ax.legend();ax.grid(alpha=.2);fig.tight_layout();fig.savefig(root/f'throughput_{axis}_{dist}_{fix}.png',dpi=150);plt.close(fig)
for f in root.glob('*_diagnostics.csv'):
    with f.open() as stream: ds=list(csv.DictReader(stream))
    if not ds:continue
    first_seed=ds[0]['seed']
    fig,ax=plt.subplots(figsize=(7,4));drawn=False
    for name in sorted({r['backend'] for r in ds}):
        ss=[r for r in ds if r['backend']==name and r['seed']==first_seed and int(r['physical'])>=0 and int(r['live'])>0]
        if ss:ax.plot([int(r['step']) for r in ss],[int(r['physical'])/int(r['live']) for r in ss],label=name);drawn=True
    if drawn:
        ax.set(xlabel='Measured cycles',ylabel='Physical entries / live events',title='Sampled occupancy; first seed only');ax.legend();fig.tight_layout();fig.savefig(root/(f.stem+'_occupancy.png'),dpi=150)
    plt.close(fig)
print(f'Analyzed {len(rows)} trials. CI omitted with fewer than five trials.')
print('Trials under 0.2 s need more steps for report-quality measurements.')
# Diagnostic aggregates and graphs from separate, untimed replays.
diagnostic_groups=collections.defaultdict(list)
for f in sorted(root.glob('*_diagnostics.csv')):
    with f.open() as stream:
        for r in csv.DictReader(stream):
            diagnostic_groups[tuple(r[k] for k in ('backend','n','rate','distribution','seed'))].append(r)
records=[]
for key,rs in sorted(diagnostic_groups.items()):
    rs.sort(key=lambda r:int(r['step']));first,last=rs[0],rs[-1]
    def delta(field):
        return int(last[field])-int(first[field]) if int(first['step'])==0 and int(first[field])>=0 and int(last[field])>=0 else ''
    records.append(dict(zip(('backend','n','rate','distribution','seed'),key),
        accounted_bytes_final=int(last['bytes']) if int(last['bytes'])>=0 else '',
        sampled_peak_bytes=max(int(r['bytes']) for r in rs) if int(last['bytes'])>=0 else '',
        sampled_peak_ratio=max(int(r['physical'])/int(r['live']) for r in rs if int(r['live'])>0) if int(last['physical'])>=0 else '',
        measured_purges=delta('purges'),measured_purge_input=delta('purged_entries'),
        measured_rotations=delta('rotations'),final_height=int(last['height']) if int(last['height'])>=0 else ''))
if records:
    save('diagnostic_summary.csv',records)
    for dist,n in sorted({(r['distribution'],r['n']) for r in records}):
        fig,axs=plt.subplots(2,2,figsize=(10,7))
        for ax,(field,title) in zip(axs.flat,[('accounted_bytes_final','Accounted bytes at end'),('measured_purges','Purges in measured interval'),('measured_rotations','Primitive rotations in measured interval'),('final_height','AVL height at end')]):
            drawn=False
            for name in sorted({r['backend'] for r in records}):
                values=collections.defaultdict(list)
                for r in records:
                    if r['distribution']==dist and r['n']==n and r['backend']==name and r[field]!='':values[float(r['rate'])].append(r[field])
                if values:
                    xs=sorted(values);ax.plot(xs,[statistics.mean(values[x]) for x in xs],'-o',label=name);drawn=True
            ax.set(xlabel='Cancellation fraction',ylabel=title);ax.grid(alpha=.2)
            if drawn:ax.legend()
        fig.suptitle(f'Untimed diagnostics: {dist}, N={n}; mean across seeds')
        fig.tight_layout();fig.savefig(root/f'diagnostics_{dist}_{n}.png',dpi=130);plt.close(fig)
