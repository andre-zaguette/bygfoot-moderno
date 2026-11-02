"""Amostra legível do dump: jogadores da base do usuário e dos 2 primeiros times da liga. Para comparar valores crus entre sistemas."""
import json
import sys

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", newline="\n")
d = json.load(open(sys.argv[1], encoding="utf-8"))
print(json.dumps(d["users"][0]["youth_academy"]["players"][:3], sort_keys=True, indent=1, ensure_ascii=False))
for t in d["country"]["leagues"][0]["teams"][:2]:
    print(t.get("name"), json.dumps(t["players"][:3], sort_keys=True, indent=1, ensure_ascii=False))
print("random_seed", d["random_seed"])
