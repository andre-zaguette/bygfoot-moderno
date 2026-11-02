"""Lê o dump JSON do Bygfoot e imprime os lances da última partida do usuário.

Uma linha por LiveGameUnit: area, event (tipo), minute, possession, result, team, time.
Última linha: {"final": [casa, fora], "fixture_id": N, "home": nome, "away": nome}.
Não grava hora nem caminhos: a saída só depende do estado do jogo.
"""
import argparse
import json
import sys


def player(p):
    return {"id": p.get("id"), "name": p.get("name")}


def timeline_v1(dump, meta):
    """Documento MatchTimeline v1 (schemas/match-timeline-v1.schema.json)."""
    lg = dump["users"][0]["live_game"]
    names = lg.get("team_names") or [None, None]
    units = []
    for u in lg["units"]:
        ev = u.get("event") or {}
        e = {"type": ev.get("type"), "team": ev.get("team")}
        for k in ("player", "player2"):
            if ev.get(k):
                e[k] = player(ev[k])
        if ev.get("commentary"):
            e["commentary"] = ev["commentary"]
        units.append({"minute": u["minute"], "time": u["time"], "possession": u["possession"],
                      "area": u["area"], "result": u["result"], "event": e})
    return {
        "schemaVersion": 1,
        "source": {"engine": "bygfoot", "upstreamCommit": meta.commit, "seed": meta.seed,
                   "country": meta.country, "team": meta.team, "weeks": meta.weeks},
        "match": {"fixtureId": lg.get("fix_id"), "home": names[0], "away": names[1],
                  "final": units[-1]["result"]},
        "units": units,
    }


def main(path, meta=None):
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
    if meta is not None and meta.format == "timeline-v1":
        print(json.dumps(timeline_v1(dump, meta), sort_keys=True, ensure_ascii=False, indent=1))
        return
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
    ap = argparse.ArgumentParser()
    ap.add_argument("path")
    ap.add_argument("--format", default="lines", choices=("lines", "timeline-v1"))
    ap.add_argument("--seed", type=int)
    ap.add_argument("--country")
    ap.add_argument("--team")
    ap.add_argument("--weeks", type=int)
    ap.add_argument("--commit")
    a = ap.parse_args()
    main(a.path, a)
