import { useId, useState } from "react";
import type { Catalog, CatalogCountry } from "../types";

export interface Selection { country: string; league: string; team: string }
export interface PickedSelection extends Selection { firstWeek: number | null }

const CONTINENT_LABEL: Record<string, string> = {
  africa: "África", asia: "Ásia", australia: "Oceania", europe: "Europa",
  miscellaneous: "Seleções e competições", north_america: "América do Norte", south_america: "América do Sul",
};

/** Minúsculas e sem acentos, para o filtro achar "K. Klaksvík" digitando "klaksvik". */
export function normalize(s: string): string {
  return s.normalize("NFD").replace(/\p{M}/gu, "").toLowerCase();
}

/** Semanas a simular para que a liga já tenha jogado a 1ª rodada (o motor só devolve partidas já disputadas). */
export function suggestedWeeks(firstWeek: number | null): number {
  // `first_week` + 1: o clube pode folgar na 1ª rodada (sweden5j falhou com 5 semanas e passou com 6).
  // Sem `first_week` no XML (p.ex. peru3, que falhou com 8) usamos 12, o valor testado que funcionou.
  return firstWeek === null ? 12 : Math.max(2, firstWeek + 1);
}

function findCountry(catalog: Catalog, sid: string): CatalogCountry | undefined {
  for (const c of catalog.continents) {
    const f = c.countries.find((x) => x.sid === sid);
    if (f) return f;
  }
  return undefined;
}

/** Primeira liga com clubes do país e o primeiro clube dela; null se o país não existe. */
export function firstSelection(catalog: Catalog, countrySid: string): PickedSelection | null {
  const c = findCountry(catalog, countrySid);
  if (!c) return null;
  const l = c.leagues.find((x) => x.teams.length > 0);
  if (!l) return { country: c.sid, league: "", team: "", firstWeek: null };
  return { country: c.sid, league: l.sid, team: l.teams[0], firstWeek: l.firstWeek };
}

const plural = (n: number) => `${n} ${n === 1 ? "clube" : "clubes"}`;

export default function TeamPicker({ catalog, value, onSelect }: { catalog: Catalog; value: Selection; onSelect: (s: PickedSelection) => void }) {
  const id = useId();
  const [filter, setFilter] = useState("");
  const country = findCountry(catalog, value.country);
  const leagues = (country?.leagues ?? []).filter((l) => l.teams.length > 0);
  const league = leagues.find((l) => l.sid === value.league);
  const f = normalize(filter.trim());
  const teams = (league?.teams ?? []).filter((t) => !f || normalize(t).includes(f));

  const pickCountry = (sid: string) => { setFilter(""); const s = firstSelection(catalog, sid); if (s) onSelect(s); };
  const pickLeague = (sid: string) => {
    setFilter("");
    const l = leagues.find((x) => x.sid === sid);
    if (l) onSelect({ country: value.country, league: l.sid, team: l.teams[0], firstWeek: l.firstWeek });
  };
  const pickTeam = (team: string) => onSelect({ country: value.country, league: value.league, team, firstWeek: league?.firstWeek ?? null });

  return (
    <fieldset className="picker">
      <legend>Escolha o clube</legend>
      <div className="picker-row">
        <div>
          <label htmlFor={`${id}-c`}>País</label>
          <select id={`${id}-c`} value={value.country} onChange={(e) => pickCountry(e.target.value)}>
            {catalog.continents.map((c) => (
              <optgroup key={c.id} label={CONTINENT_LABEL[c.id] ?? c.id}>
                {c.countries.map((p) => <option key={p.sid} value={p.sid}>{p.name}</option>)}
              </optgroup>
            ))}
          </select>
        </div>
        {leagues.length > 0 && (
          <div>
            <label htmlFor={`${id}-l`}>Liga</label>
            <select id={`${id}-l`} value={value.league} onChange={(e) => pickLeague(e.target.value)}>
              {leagues.map((l) => <option key={l.sid} value={l.sid}>{`${l.name} (${plural(l.teams.length)})`}</option>)}
            </select>
          </div>
        )}
      </div>
      {leagues.length === 0 ? (
        <p>{country?.name ?? "Este item"} não tem ligas com clubes para escolher.</p>
      ) : (
        <div className="picker-teams">
          <label htmlFor={`${id}-f`}>Filtrar clubes</label>
          <input id={`${id}-f`} type="search" value={filter} onChange={(e) => setFilter(e.target.value)} placeholder="Digite parte do nome" />
          <label htmlFor={`${id}-t`}>Clube</label>
          <select id={`${id}-t`} size={8} value={value.team} onChange={(e) => pickTeam(e.target.value)}>
            {teams.map((t) => <option key={t} value={t}>{t}</option>)}
          </select>
          {teams.length === 0 && <p role="status">Nenhum clube encontrado com esse filtro.</p>}
        </div>
      )}
      {country && country.cups.length > 0 && (
        <p className="picker-cups">Campeonatos do país: {country.cups.map((c) => c.name).join(" · ")}</p>
      )}
    </fieldset>
  );
}
