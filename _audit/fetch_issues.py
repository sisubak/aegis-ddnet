import json, urllib.request

def get(url):
    req = urllib.request.Request(url, headers={"User-Agent":"x","Accept":"application/vnd.github+json"})
    return json.load(urllib.request.urlopen(req))

items=[]
for page in (1,2):
    d=get("https://api.github.com/search/issues?q=repo:ddnet/ddnet+is:issue+is:open+label:server+label:bug&per_page=100&page=%d"%page)
    items+=d["items"]
    if len(d["items"])<100: break

print("TOTAL", len(items))
out=[]
for it in items:
    labels=[l["name"] for l in it["labels"]]
    out.append({"n":it["number"],"title":it["title"],"labels":labels,"body":(it.get("body") or "")[:1500],"comments":it["comments"]})

with open("issues.json","w",encoding="utf-8") as f:
    json.dump(out,f,ensure_ascii=False,indent=1)

for o in out:
    print(o["n"], "|", o["title"], "|", ",".join(o["labels"]))
