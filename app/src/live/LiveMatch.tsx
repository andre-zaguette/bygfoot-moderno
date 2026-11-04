import { useEffect, useMemo, useRef, useState } from "react";
import type { Timeline } from "../types";
import { buildChoreography, EFFECT_LABEL, type EffectKind, type Frame } from "./choreography";
import { createPlayback, type PlaybackState, type Speed } from "./playback";
import PitchView from "./PitchView";
import { buildOverlay } from "./overlay";
import Crest from "../crests/Crest";
import type { CrestInfo } from "../types";

/** Lances que merecem ser anunciados a leitores de tela (os comuns ficam só na faixa visual). */
const ANNOUNCE: ReadonlySet<EffectKind> = new Set(["start", "goal", "yellow", "red", "penalty", "halftime", "end"]);

function usePrefersReducedMotion(): boolean {
  const [reduced, setReduced] = useState(false);
  useEffect(() => {
    if (typeof window === "undefined" || !window.matchMedia) return;
    const q = window.matchMedia("(prefers-reduced-motion: reduce)");
    setReduced(q.matches);
    const on = () => setReduced(q.matches);
    q.addEventListener("change", on);
    return () => q.removeEventListener("change", on);
  }, []);
  return reduced;
}

export default function LiveMatch({ timeline, crests }: { timeline: Timeline; crests?: { home?: CrestInfo; away?: CrestInfo } }) {
  const frames = useMemo(() => buildChoreography(timeline), [timeline]);
  const playback = useMemo(() => {
    const p = createPlayback(timeline.units);
    p.play();
    return p;
  }, [timeline]);
  const reducedMotion = usePrefersReducedMotion();
  const [st, setSt] = useState<PlaybackState>(playback.state());
  const [frame, setFrame] = useState<Frame>(frames[0]);
  const [announce, setAnnounce] = useState<{ id: number; label: string; score: [number, number]; text: string; lang: "en" | "pt-BR" } | null>(null);
  const announceId = useRef(0);
  const section = useRef<HTMLElement>(null);
  const replayBtn = useRef<HTMLButtonElement>(null);
  const [layers, setLayers] = useState({ heat: false, passes: false, names: false });
  const anyLayer = layers.heat || layers.passes || layers.names;
  const overlay = useMemo(() => (anyLayer ? buildOverlay(timeline, st.index) : null), [anyLayer, timeline, st.index]);
  const flip = (k: keyof typeof layers) => setLayers((l) => ({ ...l, [k]: !l[k] }));

  // Ao abrir, o foco vai para a partida: os atalhos (Espaço, 1, 2, 4, End) funcionam sem clicar.
  useEffect(() => { section.current?.focus(); }, []);
  // Ao terminar (pulando ou chegando ao fim), se o foco estava na partida ou se perdeu, vai para "Rever".
  useEffect(() => {
    if (!st.finished) return;
    const a = document.activeElement;
    if (a === document.body || a === null || section.current?.contains(a)) replayBtn.current?.focus();
  }, [st.finished]);

  const refresh = () => {
    const s = playback.state();
    setSt(s);
    setFrame(frames[s.index]);
  };
  const onState = (s: PlaybackState, f: Frame) => {
    setSt(s);
    setFrame(f);
    if (f.effect && ANNOUNCE.has(f.effect.kind)) {
      setAnnounce({ id: ++announceId.current, label: EFFECT_LABEL[f.effect.kind], score: f.score, text: f.caption, lang: f.captionLang });
    }
  };

  const setSpeed = (v: Speed) => { playback.setSpeed(v); refresh(); };
  const togglePlay = () => { if (playback.state().playing) playback.pause(); else playback.play(); refresh(); };
  const skip = () => { playback.seekToEnd(); refresh(); };
  const replay = () => { playback.restart(); playback.play(); refresh(); };

  const onKeyDown = (e: React.KeyboardEvent) => {
    const onButton = (e.target as HTMLElement).closest("button") !== null;
    if (e.key === " " && !onButton) { e.preventDefault(); if (!st.finished) togglePlay(); }
    else if (e.ctrlKey || e.metaKey || e.altKey) return;
    else if (e.key === "1") setSpeed(1);
    else if (e.key === "2") setSpeed(2);
    else if (e.key === "4") setSpeed(4);
    else if (e.key === "End") { e.preventDefault(); skip(); }
  };

  const { match } = timeline;
  const shown = st.finished ? match.final : frame.score;
  const minute = st.minute;

  return (
    <section ref={section} className="live" role="group" aria-label="Partida ao vivo" tabIndex={0} onKeyDown={onKeyDown}>
      <div className="stage">
        <PitchView frames={frames} playback={playback} reducedMotion={reducedMotion} onState={onState} heat={layers.heat && overlay ? overlay.heat : null} />
        <div className="hud">
          <div className="scorebug">
            <span className="side"><Crest team={match.home} info={crests?.home} size={24} /> {match.home}</span>
            <span className="score-num" data-testid="score">{shown[0]} x {shown[1]}</span>
            <span className="side">{match.away} <Crest team={match.away} info={crests?.away} size={24} /></span>
          </div>
          {layers.names && (
            <p className="names" data-testid="names">
              {[timeline.units[st.index].event.player, timeline.units[st.index].event.player2].filter(Boolean).map((p) => p!.name).join(" e ") || "Sem jogador citado neste lance"}
            </p>
          )}
          <span className="clock">{minute}' {st.finished ? "· fim" : ""}</span>
        </div>
      </div>
      <p className="caption" data-testid="caption" lang={frame.captionLang}>{frame.caption}</p>
      <div className="controls">
        {st.finished ? (
          <button ref={replayBtn} type="button" onClick={replay}>Rever</button>
        ) : (
          <button type="button" onClick={togglePlay}>{st.playing ? "Pausar" : "Retomar"}</button>
        )}
        {([1, 2, 4] as const).map((v) => (
          <button key={v} type="button" aria-pressed={st.speed === v} onClick={() => setSpeed(v)}>{v}x</button>
        ))}
        <button type="button" onClick={skip} disabled={st.finished}>Pular para o fim</button>
        <span role="group" aria-label="Camadas de análise (aproximação visual)" className="layers">
          {(["heat", "passes", "names"] as const).map((k) => (
            <button key={k} type="button" aria-pressed={layers[k]} onClick={() => flip(k)}>
              {k === "heat" ? "Calor" : k === "passes" ? "Combinações" : "Nomes"}
            </button>
          ))}
        </span>
        {overlay && (
          <span className="possession" data-testid="possession">Posse {overlay.possession[0]}% · {overlay.possession[1]}%</span>
        )}
        <span className="approx">Aproximação visual: o campo representa os lances do motor; ele não decide o resultado.</span>
      </div>
      {layers.passes && overlay && (
        <section role="region" aria-label="Combinações mais citadas" className="passes">
          <h3>Combinações mais citadas</h3>
          {overlay.links.length === 0 ? (
            <p>Nenhuma combinação registrada ainda.</p>
          ) : (
            <ol>
              {overlay.links.slice(0, 5).map((l) => (
                <li key={`${l.team}-${l.a.id}-${l.b.id}`}>
                  <span className={l.team === 0 ? "dot-home" : "dot-away"} aria-hidden="true" />
                  {l.a.name} ↔ {l.b.name} <b>×{l.weight}</b>
                </li>
              ))}
            </ol>
          )}
          <p className="approx">Jogadores citados juntos em lances de jogo corrido (em geral, passes). O motor não informa a posição de cada um.</p>
        </section>
      )}
      <div data-testid="announcer" aria-live="polite" className="sr-only">
        {announce && (
          <span key={announce.id}>
            {announce.label} Placar {announce.score[0]} x {announce.score[1]}. <span lang={announce.lang}>{announce.text}</span>
          </span>
        )}
      </div>
    </section>
  );
}
