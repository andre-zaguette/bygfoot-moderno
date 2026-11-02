# Teto de `--weeks` no `bf-headless`

Medido em macOS (build local do pin `9b7b0062`), time padrão `V. Gotu`, país `faroe_islands`.
Comando: `spike/headless/bf-headless --seed S --weeks W` (exit 0 = ok, 3 = motor caiu).

| Seed | Semanas testadas | Resultado |
|---|---|---|
| 0, 1, 431199, 2147483647 | 20, 30, 40, 50, 60 | exit 0 em todas |
| 1 | 80, 100, 110, 120, 130 | exit 0 |
| 1 | 135, 140, 150 | **exit 3** (motor saiu com 11) |
| 0, 431199, 2147483647 | 100 | exit 0 |
| 1 | 300 | exit 3 |

Causa vista no stderr de `--weeks 140`: `game_substitute_player_send_off: no suitable substitutes found (all injured/banned?)`, seguido da queda. Ou seja, o limite **não é fixo**: depende do estado da temporada (lesões e suspensões acumuladas), então outra seed pode cair antes de 130.

Decisão: `MAX_WEEKS=100` no wrapper (margem de 30 sobre o primeiro ponto de queda observado, 4 seeds testadas em 100). Acima disso o wrapper sai com 2. Valores abaixo de 100 não são garantidos para toda seed: o exit 3 do wrapper continua sendo a proteção.
