"""Mostra o primeiro lance (linha) que difere entre duas saídas de bf-headless. Sai 0 se iguais, 1 se não."""
import sys


def main(a, b):
    la = open(a, "rb").read().split(b"\n")
    lb = open(b, "rb").read().split(b"\n")
    for i in range(max(len(la), len(lb))):
        x = la[i] if i < len(la) else None
        y = lb[i] if i < len(lb) else None
        if x != y:
            show = lambda v: "(fim do arquivo)" if v is None else v.decode("utf-8", "replace")
            print(f"primeira diferença na linha {i + 1}")
            print(f"  {a}: {show(x)}")
            print(f"  {b}: {show(y)}")
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
