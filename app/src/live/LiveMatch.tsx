import { useEffect, useMemo, useState } from "react";
import type { Timeline } from "../types";
import { buildChoreography, type EffectKind, type Frame } from "./choreography";
import { createPlayback, type PlaybackState, type Speed } from "./playback";
import PitchView from "./PitchView";

/** Lances que merecem ser anunciados a leitores de tela (os comuns ficam só na faixa visual). */
const ANNOUNCE: ReadonlySet<EffectKind> = new Set(["start", "goal", "yellow", "red", "penalty", "halftime", "end"]);

function usePrefersReducedMotion(): boolean {
  const q = typeof window !== "undefined" && window.matchMedia ? window.matchMedia("(prefers-reduced-motion: reduce)") : null;
  const [reduced, setReduced] = useState(q?.matches ?? false);
  useEffect(() => {
    if (!q) return;
    const on = () => setReduced(q.matches);
    q.addEventListener("change", on);
    return () => q.removeEventListener("change", on);
  }, [q]);
  return reduced;
}

export default function LiveMatch({ timeline }: { timeline: Timeline }) {
  const frames = useMemo(() => buildChoreography(timeline), [timeline]);
  const playback = useMemo(() => {
    const p = createPlayback(timeline.units);
    p.play();
    return p;
  }, [timeline]);
  const reducedMotion = usePrefersReducedMotion();
  const [st, setSt] = useState<PlaybackState>(playback.state());
  const [frame, setFrame] = useState<Frame>(frames[0]);
  const [announce, setAnnounce] = useState("");

  const refresh = () => {
    const s = playback.state();
    setSt(s);
    setFrame(frames[s.index]);
  };
  const onState = (s: PlaybackState, f: Frame) => {
    setSt(s);
    setFrame(f);
    if (f.effect && ANNOUNCE.has(f.effect.kind)) setAnnounce(f.caption);
  };

  const setSpeed = (v: Speed) => { playback.setSpeed(v); refresh(); };
  const togglePlay = () => { if (playback.state().playing) playback.pause(); else playback.play(); refresh(); };
  const skip = () => { playback.seekToEnd(); refresh(); };
  const replay = () => { playback.restart(); playback.play(); refresh(); };

  const onKeyDown = (e: React.KeyboardEvent) => {
    const onButton = (e.target as HTMLElement).closest("button") !== null;
    if (e.key === " " && !onButton) { e.preventDefault(); if (!st.finished) togglePlay(); }
    else if (e.key === "1") setSpeed(1);
    else if (e.key === "2") setSpeed(2);
    else if (e.key === "4") setSpeed(4);
    else if (e.key === "End") skip();
  };

  const { match } = timeline;
  const shown = st.finished ? match.final : frame.score;
  const minute = st.minute;

  return (
    <section className="live" role="group" aria-label="Partida ao vivo" tabIndex={0} onKeyDown={onKeyDown}>
      <div className="stage">
        <PitchView frames={frames} playback={playback} reducedMotion={reducedMotion} onState={onState} />
        <div className="hud">
          <div className="scorebug">
            <span>{match.home}</span>
            <span className="score-num" data-testid="score">{shown[0]} x {shown[1]}</span>
            <span>{match.away}</span>
          </div>
          <span className="clock">{minute}' {st.finished ? "· fim" : ""}</span>
        </div>
        <p className="caption" data-testid="caption" aria-hidden="true">{frame.caption}</p>
      </div>
      <div className="controls">
        {st.finished ? (
          <button type="button" onClick={replay}>Rever</button>
        ) : (
          <button type="button" onClick={togglePlay}>{st.playing ? "Pausar" : "Retomar"}</button>
        )}
        {([1, 2, 4] as const).map((v) => (
          <button key={v} type="button" aria-pressed={st.speed === v} onClick={() => setSpeed(v)}>{v}x</button>
        ))}
        <button type="button" onClick={skip} disabled={st.finished}>Pular para o fim</button>
        <span className="approx">Aproximação visual: o campo representa os lances do motor; ele não decide o resultado.</span>
      </div>
      <div data-testid="announcer" aria-live="polite" className="sr-only">{announce}</div>
    </section>
  );
}
