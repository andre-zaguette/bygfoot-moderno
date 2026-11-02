"""Imprime o hash da saída do bf-headless para cada seed de ouro.

Linha: <seed>\t<time>\t<semanas>\t<sha256 do stdout em bytes>\t<número de linhas>
O hash cobre a saída completa de `--format timeline-v1` (lances, jogadores, nomes, comentários). O campo source.patches é
forçado a [] (BYGFOOT_PATCHES vazio) para o hash depender só do conteúdo da partida.
Saída em UTF-8 com \n em qualquer sistema. Código 1 se alguma execução falhar.
"""
import argparse
import hashlib
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def parse(path):
    rows = []
    with open(path, encoding="utf-8") as f:
        for n, raw in enumerate(f, 1):
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = line.split("|")
            if len(parts) != 3:
                sys.exit(f"{path}:{n}: esperado seed|time|semanas")
            rows.append((parts[0], parts[1], parts[2]))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", default=os.path.join(ROOT, "tests", "golden-seeds", "seeds.txt"))
    args = ap.parse_args()
    out = sys.stdout.buffer
    failed = False
    for seed, team, weeks in parse(args.seeds):
        r = subprocess.run(
            ["sh", os.path.join(ROOT, "spike", "headless", "bf-headless"),
             "--seed", seed, "--team", team, "--weeks", weeks, "--format", "timeline-v1"],
            capture_output=True, env={**os.environ, "BYGFOOT_PATCHES": ""})
        if r.returncode != 0:
            sys.stderr.write(f"seed {seed} ({team}, {weeks}): bf-headless saiu com {r.returncode}\n")
            sys.stderr.write(r.stderr.decode("utf-8", "replace")[:500] + "\n")
            failed = True
            continue
        digest = hashlib.sha256(r.stdout).hexdigest()
        lines = r.stdout.count(b"\n")
        out.write(f"{seed}\t{team}\t{weeks}\t{digest}\t{lines}\n".encode("utf-8"))
    out.flush()
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
