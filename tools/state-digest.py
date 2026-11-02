"""Lê o dump JSON (arquivo) e imprime 'caminho<TAB>hash16' até a profundidade dada. Para localizar divergência de estado entre sistemas."""
import hashlib
import json
import sys


def main(path, depth, prefix=None):
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", newline="\n")
    d = json.load(open(path, encoding="utf-8"))
    out = []

    def h(v):
        return hashlib.sha256(json.dumps(v, sort_keys=True, ensure_ascii=False).encode("utf-8")).hexdigest()[:16]

    def walk(p, v, dep):
        out.append(f"{p or '.'}\t{h(v)}")
        if dep == 0:
            return
        if isinstance(v, dict):
            for k in sorted(v):
                walk(f"{p}.{k}", v[k], dep - 1)
        elif isinstance(v, list):
            for i, x in enumerate(v):
                walk(f"{p}[{i}]", x, dep - 1)

    walk("", d, depth)
    if prefix:
        import re
        out = [l for l in out if re.match(prefix, l)]
    print("\n".join(out))


if __name__ == "__main__":
    main(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 6, sys.argv[3] if len(sys.argv) > 3 else None)
