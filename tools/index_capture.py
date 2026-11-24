import argparse, hashlib, json, collections
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('capture',type=Path)
p.add_argument('manifest',type=Path)
p.add_argument('output',type=Path)
a=p.parse_args()
m=json.loads(a.manifest.read_text())
counts=collections.Counter()
transitions=collections.Counter()
prev=None
total=unknown=0
with a.capture.open('rb') as f:
    for line in f:
        total+=1
        key=hashlib.sha256(line.strip()).hexdigest()[:32]
        if key not in m:
            unknown+=1
            prev=None
            continue
        counts[key]+=1
        if prev is not None and prev!=key:
            transitions[prev,key]+=1
        prev=key
result={'capture_records':total,'unmatched_records':unknown,'matched_unique':len(counts),'sprites':{k:{**m[k],'occurrences':v} for k,v in counts.items()},'adjacent_draw_operations':[{'from':x,'to':y,'count':n} for (x,y),n in transitions.most_common()],'warning':'Adjacent draw operations are not animation frames.'}
a.output.write_text(json.dumps(result))
print(total,unknown,len(counts),len(transitions))
