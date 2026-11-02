"""Compara arquivos de golden-hash.py gerados em sistemas diferentes.

Saída 0: todos iguais byte a byte (por seed). 1: divergência (lista as seeds e os hashes por arquivo).
2: arquivo ausente ou ilegível.
"""
import sys


def load(path):
    try:
        with open(path, "rb") as f:
            data = f.read()
    except OSError as e:
        sys.stderr.write(f"não consegui ler {path}: {e}\n")
        sys.exit(2)
    rows = {}
    for raw in data.split(b"\n"):
        if raw:
            key = tuple(raw.split(b"\t")[:3])
            rows[key] = raw  # a linha inteira (com \r, se houver) é o valor comparado
    return rows


def main(paths):
    if len(paths) < 2:
        sys.stderr.write("uso: compare-golden.py ARQ1 ARQ2 [ARQ3...]\n")
        sys.exit(2)
    tables = [load(p) for p in paths]
    keys = sorted(set().union(*[t.keys() for t in tables]))
    bad = False
    for k in keys:
        vals = [t.get(k) for t in tables]
        if len(set(vals)) != 1:
            bad = True
            seed, team, weeks = (x.decode("utf-8", "replace") for x in k)
            print(f"DIVERGE seed={seed} time={team} semanas={weeks}")
            for p, v in zip(paths, vals):
                shown = "(ausente)" if v is None else repr(v.decode("utf-8", "replace").split("\t")[3:])
                print(f"  {p}: {shown}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main(sys.argv[1:])
