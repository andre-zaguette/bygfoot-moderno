import { useEffect, useRef, useState } from "react";
import * as api from "./api";
import type { Catalog, CrestInfo, Timeline, UiError } from "./types";
import TeamPicker, { suggestedWeeks, type PickedSelection } from "./picker/TeamPicker";
import LiveMatch from "./live/LiveMatch";
import Crest from "./crests/Crest";
import CrestGuide from "./crests/CrestGuide";

const MAX_SEED = 2147483647;

export default function App() {
  const [seed, setSeed] = useState("431199");
  const [country, setCountry] = useState("faroe_islands");
  const [team, setTeam] = useState("V. Gotu");
  const [league, setLeague] = useState("faroe_islands1");
  const [weeks, setWeeks] = useState("2");
  const [catalog, setCatalog] = useState<Catalog | null>(null);
  const [catalogErr, setCatalogErr] = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<UiError | null>(null);
  const [timeline, setTimeline] = useState<Timeline | null>(null);
  const [watching, setWatching] = useState(false);
  const [guide, setGuide] = useState(false);
  const [crests, setCrests] = useState<Record<string, CrestInfo>>({});
  const guideBtn = useRef<HTMLButtonElement>(null);
  const wasGuide = useRef(false);
  const teams = timeline ? [timeline.match.home, timeline.match.away] : [];
  const teamsKey = teams.join("\u0000");

  // Base de dados do Bygfoot (países, ligas, clubes). Sem ela, o formulário cai para campos de texto.
  useEffect(() => {
    let alive = true;
    api.catalog().then(
      (c) => {
        if (!alive) return;
        setCatalog(c);
        const mine = c.continents.flatMap((k) => k.countries).find((p) => p.sid === country);
        const l = mine?.leagues.find((x) => x.teams.includes(team));
        if (l) { setLeague(l.sid); setWeeks(String(suggestedWeeks(l.firstWeek))); return; }
        // padrão indisponível nesta base: começa pelo primeiro país que tenha clubes
        const first = c.continents.flatMap((k) => k.countries).find((p) => p.leagues.some((x) => x.teams.length > 0));
        const fl = first?.leagues.find((x) => x.teams.length > 0);
        if (first && fl) { setCountry(first.sid); setLeague(fl.sid); setTeam(fl.teams[0]); setWeeks(String(suggestedWeeks(fl.firstWeek))); }
      },
      (e) => alive && setCatalogErr(api.toUiError(e).message),
    );
    return () => { alive = false; };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);
  const onPick = (p: PickedSelection) => {
    const changedLeague = p.league !== league || p.country !== country;
    setCountry(p.country); setLeague(p.league); setTeam(p.team);
    if (changedLeague) setWeeks(String(suggestedWeeks(p.firstWeek)));
  };

  // Escudos são opcionais: se a consulta falhar, a partida segue com os emblemas gerados.
  async function refreshCrests() {
    if (teams.length === 0) return;
    try {
      const list = await api.crestStatus(teams);
      setCrests(Object.fromEntries(list.map((c) => [c.team, c])));
    } catch {
      setCrests({});
    }
  }
  // eslint-disable-next-line react-hooks/exhaustive-deps
  useEffect(() => { void refreshCrests(); }, [teamsKey]);
  // Ao fechar o guia, devolve o foco ao botão que o abriu e relê a pasta (o usuário pode ter adicionado arquivos).
  useEffect(() => {
    if (!guide && wasGuide.current) { guideBtn.current?.focus(); void refreshCrests(); }
    wasGuide.current = guide;
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [guide]);

  async function onSubmit(e: React.FormEvent) {
    e.preventDefault();
    const s = Number(seed);
    const w = Number(weeks);
    if (!/^\d+$/.test(seed) || s > MAX_SEED) {
      setError({ kind: "invalid_params", message: `A seed deve ser um inteiro entre 0 e ${MAX_SEED}.` });
      return;
    }
    if (!/^\d+$/.test(weeks) || w < 1 || w > 100) {
      setError({ kind: "invalid_params", message: "As semanas devem ser um inteiro entre 1 e 100." });
      return;
    }
    setError(null);
    setWatching(false);
    setLoading(true);
    try {
      setTimeline(await api.simulateMatch({ seed: s, country, team, weeks: w }));
    } catch (err) {
      setError(api.toUiError(err));
    } finally {
      setLoading(false);
    }
  }

  if (guide) {
    return <main className="app"><CrestGuide teams={teams} onClose={() => setGuide(false)} /></main>;
  }

  return (
    <main className="app">
      <h1>Primeira partida</h1>
      <p><button ref={guideBtn} type="button" onClick={() => setGuide(true)}>Guia de escudos</button></p>
      <form onSubmit={onSubmit} className="form">
        <label>Seed
          <input type="number" value={seed} onChange={(e) => setSeed(e.target.value)} />
        </label>
        {catalog ? (
          <TeamPicker catalog={catalog} value={{ country, league, team }} onSelect={onPick} />
        ) : catalogErr ? (
          <>
            <p role="alert" className="error">{catalogErr} Digite o país e o time à mão.</p>
            <label>País
              <input value={country} onChange={(e) => setCountry(e.target.value)} />
            </label>
            <label>Time
              <input value={team} onChange={(e) => setTeam(e.target.value)} />
            </label>
          </>
        ) : (
          <p role="status">Carregando a base de dados do Bygfoot…</p>
        )}
        <label>Semanas
          <input type="number" value={weeks} onChange={(e) => setWeeks(e.target.value)} />
        </label>
        <button type="submit" disabled={loading}>{loading ? "Simulando…" : "Simular partida"}</button>
      </form>
      {error && <p role="alert" className="error">{error.message}</p>}
      {timeline ? <Result timeline={timeline} watching={watching} onWatch={setWatching} crests={crests} /> : !loading && <p className="empty">Nenhuma partida simulada ainda.</p>}
    </main>
  );
}

function Result({ timeline, watching, onWatch, crests }: { timeline: Timeline; watching: boolean; onWatch: (w: boolean) => void; crests: Record<string, CrestInfo> }) {
  const { match, units } = timeline;
  const assistBtn = useRef<HTMLButtonElement>(null);
  const wasWatching = useRef(false);
  // Ao voltar de "assistir", devolve o foco ao botão que abriu o modo (senão cai no <body>).
  useEffect(() => {
    if (!watching && wasWatching.current) assistBtn.current?.focus();
    wasWatching.current = watching;
  }, [watching]);
  if (watching) {
    return (
      <section aria-label="Partida">
        <LiveMatch timeline={timeline} crests={{ home: crests[match.home], away: crests[match.away] }} />
        <p><button type="button" onClick={() => onWatch(false)}>Ver só os lances</button></p>
      </section>
    );
  }
  return (
    <section aria-label="Resultado">
      <h2 className="scoreboard">
        <span><Crest team={match.home} info={crests[match.home]} /> {match.home}</span>
        <span data-testid="score">{match.final[0]} x {match.final[1]}</span>
        <span>{match.away} <Crest team={match.away} info={crests[match.away]} /></span>
      </h2>
      <p><button ref={assistBtn} type="button" onClick={() => onWatch(true)}>Assistir partida</button></p>
      <ol className="units">
        {units.map((u, i) => (
          <li key={i}>
            <span className="minute">{u.minute >= 0 ? `${u.minute}'` : "–"}</span> {u.event.commentary}
          </li>
        ))}
      </ol>
    </section>
  );
}
