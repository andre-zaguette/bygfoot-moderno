"""Lê o dump JSON do Bygfoot e imprime os lances da última partida do usuário.

Uma linha por LiveGameUnit: area, event (tipo), minute, possession, result, team, time.
Última linha: {"final": [casa, fora], "fixture_id": N, "home": nome, "away": nome}.
Não grava hora nem caminhos: a saída só depende do estado do jogo.
"""
import json
import sys


def main(path):
    # Saída sempre em UTF-8 com \n, também no Windows (onde o stdout usa a página de código local e CRLF).
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", newline="\n")
    with open(path, encoding="utf-8") as f:
        dump = json.load(f)
    users = dump.get("users") or []
    if not users:
        sys.exit("dump sem usuário")
    lg = users[0].get("live_game") or {}
    units = lg.get("units") or []
    if not units:
        sys.exit("o time do usuário não jogou nas semanas simuladas (aumente --weeks)")
    for u in units:
        ev = u.get("event") or {}
        row = {
            "area": u["area"],
            "event": ev.get("type"),
            "minute": u["minute"],
            "possession": u["possession"],
            "result": u["result"],
            "team": ev.get("team"),
            "time": u["time"],
        }
        print(json.dumps(row, sort_keys=True, ensure_ascii=False))
    names = lg.get("team_names") or [None, None]
    last = units[-1]["result"]
    print(json.dumps({"final": last, "fixture_id": lg.get("fix_id"),
                      "home": names[0], "away": names[1]},
                     sort_keys=True, ensure_ascii=False))


if __name__ == "__main__":
    main(sys.argv[1])
