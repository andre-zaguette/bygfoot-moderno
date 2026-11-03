import { useEffect, useRef } from "react";
import { Application, Graphics, Text } from "pixi.js";
import { tokens } from "../design/tokens";
import { FIELD_H, FIELD_W, type Frame } from "./choreography";
import { stepFrame } from "./step";
import type { Playback, PlaybackState } from "./playback";

const hex = (c: string) => parseInt(c.replace("#", ""), 16);

export interface PitchViewProps {
  frames: Frame[];
  playback: Playback;
  reducedMotion: boolean;
  /** Chamado só quando o lance, o estado de reprodução ou a velocidade mudam (não a cada quadro). */
  onState?: (s: PlaybackState, f: Frame) => void;
  /** heat[time][área] do overlay; null = camada desligada. */
  heat?: number[][] | null;
}

export default function PitchView({ frames, playback, reducedMotion, onState, heat = null }: PitchViewProps) {
  const host = useRef<HTMLDivElement>(null);
  const live = useRef({ frames, playback, reducedMotion, onState, heat });
  live.current = { frames, playback, reducedMotion, onState, heat };

  useEffect(() => {
    const el = host.current;
    if (!el) return;
    let cancelled = false;
    let app: Application | null = null;
    let cleanup = () => {};

    (async () => {
      const a = new Application();
      await a.init({ resizeTo: el, antialias: true, autoDensity: true, resolution: window.devicePixelRatio || 1, background: hex(tokens.pitch.grassA) });
      if (cancelled) { a.destroy(true, { children: true }); return; }
      app = a;
      el.appendChild(a.canvas);

      const bg = new Graphics();
      const heatLayer = new Graphics();
      const shadows = new Graphics();
      const players = [...Array(22)].map(() => new Graphics());
      const labels = [...Array(22)].map(() => new Text({ text: "", style: { fontFamily: "IBM Plex Mono, monospace", fontSize: 12, fontWeight: "500", fill: 0xffffff } }));
      const ball = new Graphics();
      const ballShadow = new Graphics();
      a.stage.addChild(bg, heatLayer, shadows, ballShadow, ...players, ...labels, ball);
      labels.forEach((l) => l.anchor.set(0.5));

      let s = 0; // pixels por metro
      const layout = () => {
        const W = a.screen.width;
        s = W / FIELD_W;
        const { pitch, kits } = tokens;
        bg.clear();
        for (let i = 0; i < 10; i++) bg.rect((i * W) / 10, 0, W / 10 + 1, a.screen.height).fill(hex(i % 2 ? pitch.grassA : pitch.grassB));
        const line = { width: Math.max(1.5, 0.4 * s), color: hex(pitch.line), alpha: 0.9 };
        bg.rect(s, s, W - 2 * s, (FIELD_H - 2) * s).stroke(line);
        bg.moveTo((FIELD_W / 2) * s, s).lineTo((FIELD_W / 2) * s, (FIELD_H - 1) * s).stroke(line);
        bg.circle((FIELD_W / 2) * s, (FIELD_H / 2) * s, 9.15 * s).stroke(line);
        bg.circle((FIELD_W / 2) * s, (FIELD_H / 2) * s, 0.5 * s).fill(hex(pitch.line));
        bg.rect(s, (FIELD_H / 2 - 20.15) * s, 16.5 * s, 40.3 * s).stroke(line);
        bg.rect((FIELD_W - 17.5) * s, (FIELD_H / 2 - 20.15) * s, 16.5 * s, 40.3 * s).stroke(line);
        bg.rect(s, (FIELD_H / 2 - 9.16) * s, 5.5 * s, 18.32 * s).stroke(line);
        bg.rect((FIELD_W - 6.5) * s, (FIELD_H / 2 - 9.16) * s, 5.5 * s, 18.32 * s).stroke(line);
        players.forEach((g, i) => {
          const k = i < 11 ? kits.home : kits.away;
          g.clear().circle(0, 0, 2.2 * s).fill(hex(k.fill)).stroke({ width: 0.45 * s, color: hex(k.border) });
        });
        labels.forEach((l, i) => {
          const k = i < 11 ? kits.home : kits.away;
          l.text = String((i % 11) + 1);
          l.style.fontSize = Math.max(8, 2.5 * s);
          l.style.fill = hex(k.border);
        });
        ball.clear().circle(0, 0, 1.1 * s).fill(0xffffff).stroke({ width: 0.2 * s, color: 0x222222 });
        ballShadow.clear().ellipse(0, 0, 1.1 * s, 0.5 * s).fill({ color: 0x000000, alpha: 0.3 });
      };
      layout();

      // Calor por zona física (esquerda/centro/direita): soma o que cada time fez ali.
      // Aproximação visual: o motor só informa a área do lance, não coordenadas.
      let heatKey = "";
      const drawHeat = (h: number[][] | null) => {
        const key = JSON.stringify(h);
        if (key === heatKey) return;
        heatKey = key;
        heatLayer.clear();
        if (!h) return;
        const zones = [0, 1, 2].map((z) => h[0][z] + h[1][2 - z]);
        const max = Math.max(1, ...zones);
        const w = a.screen.width / 3;
        zones.forEach((v, z) => heatLayer.rect(z * w, 0, w, a.screen.height).fill({ color: hex(tokens.overlay.heat), alpha: 0.06 + 0.4 * (v / max) }));
      };

      let lastW = a.screen.width;
      let lastKey = "";
      const onTick = () => {
        const { frames: fr, playback: pb, reducedMotion: rm, onState: cb } = live.current;
        if (a.screen.width !== lastW) { lastW = a.screen.width; layout(); heatKey = ""; }
        drawHeat(live.current.heat);
        const { st, cur, frame: f, key, changed } = stepFrame(fr, pb, rm, a.ticker.deltaMS, lastKey);
        shadows.clear();
        [...f.players.home, ...f.players.away].forEach((p, i) => {
          players[i].position.set(p.x * s, p.y * s);
          labels[i].position.set(p.x * s, p.y * s);
          shadows.ellipse(p.x * s + 0.5 * s, p.y * s + 1.6 * s, 2.2 * s, 0.9 * s).fill({ color: 0x000000, alpha: 0.22 });
        });
        ball.position.set(f.ball.x * s, f.ball.y * s);
        ballShadow.position.set(f.ball.x * s + 0.4 * s, f.ball.y * s + 1.2 * s);
        if (changed) { lastKey = key; cb?.(st, cur); }
      };
      a.ticker.add(onTick);
      // O ResizePlugin do Pixi só escuta a janela; o contêiner também muda sozinho (barra de rolagem,
      // painel de combinações). Observa o contêiner e deixa o Pixi redimensionar.
      const ro = new ResizeObserver(() => a.resize());
      ro.observe(el);
      cleanup = () => { ro.disconnect(); a.ticker.remove(onTick); };
    })();

    return () => {
      cancelled = true;
      cleanup();
      if (app) { app.destroy(true, { children: true }); app = null; }
    };
  }, []);

  return (
    <div
      ref={host}
      role="img"
      aria-label="Campo de futebol visto de cima com os 22 jogadores (aproximação visual dos lances)"
      style={{ width: "100%", aspectRatio: `${FIELD_W} / ${FIELD_H}`, borderRadius: 8, overflow: "hidden", lineHeight: 0 }}
    />
  );
}
