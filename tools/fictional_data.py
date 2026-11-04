#!/usr/bin/env python3
"""Gera uma cópia FICTÍCIA de upstream/support_files (opcional, ver ADR-005; o produto usa a base real).

O upstream (Bygfoot) traz clubes e competições reais nos arquivos de definição. Este gerador copia
support_files para uma pasta nova e troca, só nessa cópia:
  * <team_name>: cada clube real vira um nome inventado, escolhido por hash do nome original
    (determinístico: mesma entrada, mesma saída, em qualquer máquina e sistema operacional);
  * <name>/<short_name> de ligas e copas: nomes genéricos derivados do identificador (sid);
  * marcas (UEFA, FIFA, World Cup...) das competições internacionais: nomes neutros.
Ficam como estão: seleções nacionais (nomes de países), times genéricos do upstream ("England 374"),
bandeiras, listas de nomes de pessoas e toda a estrutura dos XML. O upstream nunca é alterado.

Uso:  python3 tools/fictional_data.py [--src upstream/support_files] [--out build/fictional/support_files]
"""
import argparse
import hashlib
import random
import re
import shutil
import sys
from pathlib import Path

VERSION = "v1"
NATIONAL_DIRS = {"eu", "euro08", "fifa", "wc06"}  # sob definitions/miscellaneous: seleções, não clubes
GENERIC_RE = re.compile(r"^([A-Za-z .'-]+?) \d+$")
GENERIC_MIN = 5  # um prefixo com ao menos 5 números ("England 1".."England 374") é time genérico do upstream

# --- marcas das competições internacionais -> nomes neutros (ordem importa: o mais longo primeiro) ---
BRAND_MAP = [
    ("UEFA Champions League", "Continental Champions Cup"),
    ("UEFA Europa League", "Continental Challenge Cup"),
    ("UEFA Super Cup", "Continental Super Cup"),
    ("UEFA SC", "Cont. Super Cup"),
    ("WC Qualification", "NC Qualification"),
    ("World Cup 2006", "World Nations 2006"),
    ("World Cup", "World Nations Cup"),
    ("W C2006", "WN2006"),
    ("Euro 2008", "European Nations 2008"),
    ("EuroCup", "European Nations Cup"),
    ("Confederations Cup", "Inter-Continental Cup"),
    ("Copa América", "South American Cup"),
    ("Gold Cup", "North American Cup"),
    ("CONCACAF", "North America"),
    ("CONMEBOL", "South America"),
    ("UEFA", "Europe"),
    ("Fifa", "World Football"),
    ("FIFA", "World Football"),
    ("AFC", "Asia"),
    ("CAF", "Africa"),
    ("OFC", "Oceania"),
    ("UCL", "CCC"),
    ("UEL", "CHC"),
    ("WCQ", "NCQ"),
    ("WC", "NC"),
]

# --- estilos de nomes inventados -------------------------------------------------------------
PHON = {
    "iberian": dict(on="b c d f g l m n p r s t v z br tr gr pl cr".split(), vo="a e i o u a o a".split(),
                    end=["a", "o", "as", "os", "ia", "ero", "ana", "ino", "al", "ez"]),
    "germanic": dict(on="b d f g h k l m n r s t w z br gr kl schw st".split(), vo="a e i o u au ei".split(),
                     end=["berg", "dorf", "heim", "burg", "stadt", "bach", "feld", "au", "en", "ingen"]),
    "nordic": dict(on="b d f g h k l m n r s t v sk sv st".split(), vo="a e i o u y ö å".split(),
                   end=["by", "vik", "näs", "sund", "holm", "fors", "berg", "lund", "stad", "mark"]),
    "slavic": dict(on="b d g k l m n p r s t v z br gr kr zl st".split(), vo="a e i o u y".split(),
                   end=["grad", "ov", "ino", "ica", "ovac", "sk", "ava", "lje", "ec", "nik"]),
    "latin": dict(on="b c d f g l m n p r s t v ch tr".split(), vo="a e i o u ou ai".split(),
                  end=["ville", "ano", "elli", "ese", "ac", "on", "ia", "etto", "ara", "ier"]),
    "anglo": dict(on="b c d f g h k l m n p r s t w br cl fr st".split(), vo="a e i o u oa ea".split(),
                  end=["ford", "ton", "ham", "bury", "field", "wick", "mouth", "ley", "worth", "by"]),
    "turkic": dict(on="b d g k m n s t y z ç".split(), vo="a e i o u ı ü".split(),
                   end=["ar", "ak", "an", "li", "kent", "ova", "abad", "lar", "yar", "el"]),
    "african": dict(on="b d k l m n s t z ng mb nd".split(), vo="a e i o u".split(),
                    end=["a", "i", "o", "ane", "ula", "ani", "ume", "oko", "iwe", "ba"]),
    "asian": dict(on="b d h j k m n s t y z ch sh".split(), vo="a i u e o".split(),
                  end=["a", "an", "ko", "ma", "ra", "jin", "to", "ri", "gawa", "ng"]),
}
PATTERNS = {
    "iberian": ["{P}", "{P}", "Atlético {P}", "Real {P}", "Sporting {P}", "{P} FC", "Deportivo {P}", "Clube {P}", "{P} CF"],
    "germanic": ["FC {P}", "SV {P}", "TSV {P}", "{P} United", "SC {P}", "VfB {P}", "{P} 09"],
    "nordic": ["{P} IF", "{P} BK", "IK {P}", "{P} FF", "FC {P}", "{P} IL"],
    "slavic": ["FK {P}", "{P}", "Dinamo {P}", "Slavia {P}", "{P} Sport", "NK {P}", "{P} United"],
    "latin": ["AS {P}", "US {P}", "{P} Calcio", "FC {P}", "{P} Olympique", "Stade {P}", "{P} SC"],
    "anglo": ["{P} Town", "{P} United", "{P} Rovers", "{P} Athletic", "{P} City", "{P} Wanderers", "{P} Albion"],
    "turkic": ["{P} SK", "{P}spor", "Al-{P}", "{P} FK", "{P} Belediye", "{P} United"],
    "african": ["{P} FC", "{P} United", "{P} Stars", "{P} Rangers", "{P} Wanderers", "{P} City", "{P} Eagles"],
    "asian": ["{P} FC", "{P} United", "{P} SC", "{P} City", "{P} Dragons", "{P} Stars"],
}
EUROPE_STYLE = {
    "anglo": "england scotland ireland n_ireland wales malta gibraltar".split(),
    "germanic": "germany austria switzerland liechtenstein luxembourg netherlands belgium".split(),
    "nordic": "denmark norway sweden finland iceland faroe_islands estonia latvia lithuania".split(),
    "latin": "france italy san_marino romania moldova monaco".split(),
    "iberian": "spain portugal andorra".split(),
    "turkic": "turkey azerbaijan kazakhstan".split(),
}
CONTINENT_STYLE = {"south_america": "iberian", "north_america": "iberian", "africa": "african",
                   "asia": "asian", "australia": "anglo", "europe": "slavic", "miscellaneous": "anglo"}
OVERRIDE = {"usa": "anglo", "canada": "anglo", "israel": "turkic", "turkey": "turkic",
            "northern_mariana": "anglo"}


def style_for(rel):
    parts = rel.parts  # definitions/<continente>/<país>/arquivo
    cont = parts[1] if len(parts) > 2 else "miscellaneous"
    country = parts[2] if len(parts) > 3 else ""
    if country in OVERRIDE:
        return OVERRIDE[country]
    if cont == "europe":
        for st, cs in EUROPE_STYLE.items():
            if country in cs:
                return st
    return CONTINENT_STYLE.get(cont, "anglo")


def is_national_file(rel):
    p = Path(rel).parts
    return len(p) >= 3 and p[0] == "definitions" and p[1] == "miscellaneous" and p[2] in NATIONAL_DIRS


# --- leitura/escrita preservando a codificação ------------------------------------------------
def read_text(p):
    raw = Path(p).read_bytes()
    try:
        return raw.decode("utf-8"), "utf-8"
    except UnicodeDecodeError:
        return raw.decode("latin-1"), "latin-1"


def write_text(p, text, enc):
    Path(p).parent.mkdir(parents=True, exist_ok=True)
    Path(p).write_bytes(text.encode(enc))


# --- clubes -----------------------------------------------------------------------------------
def _rng(key, attempt):
    d = hashlib.sha256(f"bygfoot-moderno/fictional/{VERSION}|{key}|{attempt}".encode("utf-8")).digest()
    return random.Random(int.from_bytes(d[:8], "big"))


def _place(rng, style):
    """Topônimo inventado com 5 a 9 letras (curto demais parece sigla; longo demais, ruído)."""
    ph = PHON[style]
    for _ in range(50):
        n = rng.choice([1, 2, 2, 3])
        s = "".join(rng.choice(ph["on"]) + rng.choice(ph["vo"]) for _ in range(n))
        s += rng.choice(ph["end"])
        if 5 <= len(s) <= 9:
            return s[0].upper() + s[1:]
    return s[0].upper() + s[1:]


def make_name(key, style, attempt):
    rng = _rng(key, attempt)
    return rng.choice(PATTERNS[style]).format(P=_place(rng, style))


def generic_prefixes(texts):
    count = {}
    for t in texts:
        m = GENERIC_RE.match(t)
        if m:
            count[m.group(1)] = count.get(m.group(1), 0) + 1
    return {k for k, v in count.items() if v >= GENERIC_MIN}


def is_generic(name, prefixes):
    m = GENERIC_RE.match(name)
    return bool(m) and m.group(1) in prefixes


TEAM_RE = re.compile(r"<team_name>(.*?)</team_name>")
FIRST_RE = re.compile(r"<first_team>(.*?)</first_team>")


def build_team_map(files):
    """files: lista ordenada de (rel, texto). Devolve {nome real: nome fictício}, 1:1 e sem colisões."""
    first_style, originals = {}, []
    for rel, text in files:
        if is_national_file(rel):
            continue
        for n in TEAM_RE.findall(text):
            if n not in first_style:
                first_style[n] = style_for(rel)
                originals.append(n)
    prefixes = generic_prefixes(originals)
    keep = {n for n in originals if is_generic(n, prefixes)}
    taken = {n.lower() for n in originals}  # nunca devolve um nome igual a um clube real do upstream
    for rel, text in files:
        if is_national_file(rel):
            taken.update(n.lower() for n in TEAM_RE.findall(text))
    mapping = {}
    for n in sorted(originals):
        if n in keep:
            continue
        for attempt in range(1000):
            cand = make_name(n, first_style[n], attempt)
            if cand.lower() not in taken and re.fullmatch(r"[^<>&\"\\\x00-\x1f]{3,40}", cand):
                taken.add(cand.lower())
                mapping[n] = cand
                break
        else:  # pragma: no cover
            raise RuntimeError(f"sem nome livre para o clube #{len(mapping)}")
    return mapping, keep


# --- competições ------------------------------------------------------------------------------
def pretty(s):
    return " ".join(w.capitalize() for w in s.replace("-", "_").split("_") if w)


def brand_clean(text):
    for a, b in BRAND_MAP:
        text = text.replace(a, b)
    return text


def competition_names(rel, text, country_name, used):
    """Devolve (name, short_name) genéricos para liga/copa de país, únicos entre si."""
    sid_m = re.search(r"<sid>(.*?)</sid>", text)
    sid = sid_m.group(1) if sid_m else Path(rel).stem
    country_dir = Path(rel).parts[2] if len(Path(rel).parts) > 3 else ""
    rest = re.sub(r"^(league|cup)_", "", sid)
    if country_dir and rest.startswith(country_dir):
        rest = rest[len(country_dir):]
    rest = rest.strip("_").replace("_", " ")
    kind = "League" if Path(rel).name.startswith("league_") else "Cup"
    name = f"{country_name} {kind} {rest}".strip()
    if name in used:
        name = f"{name} [{sid}]"
    used.add(name)
    cc = (country_dir[:3] or "INT").upper()
    short = f"{cc} {'L' if kind == 'League' else 'C'}{rest}".strip()
    return name, short


NAME_RE = re.compile(r"<name>.*?</name>")
SHORT_RE = re.compile(r"<short_name>.*?</short_name>")


def transform(rel, text, mapping, used_names, country_names):
    rel = Path(rel)
    if not is_national_file(rel):
        text = TEAM_RE.sub(lambda m: f"<team_name>{mapping.get(m.group(1), m.group(1))}</team_name>", text)
        # <first_team> aponta para um clube pelo nome (time reserva -> time principal): mesmo mapa
        text = FIRST_RE.sub(lambda m: f"<first_team>{mapping.get(m.group(1), m.group(1))}</first_team>", text)
    if is_national_file(rel) or "miscellaneous" in rel.parts:
        text = SHORT_RE.sub(lambda m: brand_clean(m.group(0)), text)
        return NAME_RE.sub(lambda m: brand_clean(m.group(0)), text)
    if rel.name.startswith(("league_", "cup_")):
        cdir = rel.parts[2] if len(rel.parts) > 3 else ""
        name, short = competition_names(rel, text, country_names.get(cdir, pretty(cdir)), used_names)
        text = NAME_RE.sub(f"<name>{name}</name>", text, count=1)
        text = SHORT_RE.sub(f"<short_name>{short}</short_name>", text, count=1)
    return text


def build(src, out):
    src, out = Path(src).resolve(), Path(out).resolve()
    if out == src or src in out.parents or out in src.parents:
        raise SystemExit("saída não pode ficar dentro (ou acima) do support_files de origem")
    defs = src / "definitions"
    files = []
    for p in sorted(defs.rglob("*.xml")):
        text, enc = read_text(p)
        files.append((p.relative_to(src), text, enc))
    mapping, keep = build_team_map([(r, t) for r, t, _ in files])
    country_names = {}
    for rel, text, _ in files:
        if rel.name.startswith("country_") and len(rel.parts) > 3:
            m = NAME_RE.search(text)
            if m:
                raw = re.sub(r"</?name>", "", m.group(0))
                country_names[rel.parts[2]] = " ".join(w.capitalize() for w in raw.split())
    if out.exists():
        shutil.rmtree(out)
    shutil.copytree(src, out, ignore=shutil.ignore_patterns("definitions"))
    used = set()
    for rel, text, enc in files:
        write_text(out / rel, transform(rel, text, mapping, used, country_names), enc)
    return {"team_map": mapping, "generic_kept": len(keep), "files": len(files)}


def main(argv=None):
    root = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--src", default=str(root / "upstream" / "support_files"))
    ap.add_argument("--out", default=str(root / "build" / "fictional" / "support_files"))
    a = ap.parse_args(argv)
    r = build(a.src, a.out)
    print(f"ok: {r['files']} arquivos de definição, {len(r['team_map'])} clubes reais trocados, "
          f"{r['generic_kept']} times genéricos mantidos -> {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
