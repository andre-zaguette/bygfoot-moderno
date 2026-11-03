import { useState } from "react";
import * as api from "./api";
import type { Timeline, UiError } from "./types";
import LiveMatch from "./live/LiveMatch";

const MAX_SEED = 2147483647;

export default function App() {
  const [seed, setSeed] = useState("431199");
  const [country, setCountry] = useState("faroe_islands");
  const [team, setTeam] = useState("V. Gotu");
  const [weeks, setWeeks] = useState("2");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<UiError | null>(null);
  const [timeline, setTimeline] = useState<Timeline | null>(null);
  const [watching, setWatching] = useState(false);

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

  return (
    <main className="app">
      <h1>Primeira partida</h1>
      <form onSubmit={onSubmit} className="form">
        <label>Seed
          <input type="number" value={seed} onChange={(e) => setSeed(e.target.value)} />
        </label>
        <label>País
          <input value={country} onChange={(e) => setCountry(e.target.value)} />
        </label>
        <label>Time
          <input value={team} onChange={(e) => setTeam(e.target.value)} />
        </label>
        <label>Semanas
          <input type="number" value={weeks} onChange={(e) => setWeeks(e.target.value)} />
        </label>
        <button type="submit" disabled={loading}>{loading ? "Simulando…" : "Simular partida"}</button>
      </form>
      {error && <p role="alert" className="error">{error.message}</p>}
      {timeline ? <Result timeline={timeline} watching={watching} onWatch={setWatching} /> : !loading && <p className="empty">Nenhuma partida simulada ainda.</p>}
    </main>
  );
}

function Result({ timeline, watching, onWatch }: { timeline: Timeline; watching: boolean; onWatch: (w: boolean) => void }) {
  const { match, units } = timeline;
  if (watching) {
    return (
      <section aria-label="Partida">
        <LiveMatch timeline={timeline} />
        <p><button type="button" onClick={() => onWatch(false)}>Ver só os lances</button></p>
      </section>
    );
  }
  return (
    <section aria-label="Resultado">
      <h2 className="scoreboard">
        <span>{match.home}</span>
        <span data-testid="score">{match.final[0]} x {match.final[1]}</span>
        <span>{match.away}</span>
      </h2>
      <p><button type="button" onClick={() => onWatch(true)}>Assistir partida</button></p>
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
