# ADR-003: Adaptador do motor como processo separado e casca Tauri

Status: aceita (Fase 2, 2026-11-03)

## Decisão
- O app desktop é **Tauri 2 + React + TypeScript (Vite)**. A interface nunca calcula resultado: só mostra a `MatchTimeline v1` devolvida pelo motor.
- O motor C do Bygfoot roda como **processo separado**, chamado pelo crate Rust `engine-adapter` (via `spike/headless/bf-headless`). Sem FFI, sem ligar o C ao processo do app.
- O adaptador valida a saída de forma estrita: campos desconhecidos são recusados e o último `result` precisa ser igual a `match.final`. Erros têm tipo estável (`invalid_params`, `engine_unavailable`, `engine_failed`, `bad_output`, `invalid_timeline`).
- A interface só pode chamar comandos listados em `app/src-tauri/build.rs` (ACL do Tauri, privilégio mínimo). Sem rede e sem acesso a arquivos.

## Por quê
- Processo separado mantém o `upstream/` e os remendos intactos, evita acoplamento de ABI e falha do motor não derruba o app.
- Validar na fronteira transforma mudança do motor (ex.: sincronização com o upstream) em erro claro, não em tela quebrada.

## Evidência
- `engine-adapter/tests` (11 testes) e `app/src-tauri/tests/ipc.rs`: o comando, chamado pelo IPC do Tauri com o motor real, devolve JSON **igual** à amostra `tests/contract/timeline-v1.sample.json`.
- Workflow `app.yml` roda tudo em macOS, Linux e Windows.

## Limites conhecidos (dívida assumida)
- **Modo de desenvolvimento.** O wrapper é `sh` + `python3`, e o binário do motor vem de `BYGFOOT_BIN` ou de `build/patched/`. Um instalador para usuário final precisa empacotar o motor e portar o wrapper para Rust (Fase 6). Hoje o app não roda fora de um checkout com o motor compilado.
- Cada partida leva alguns segundos (inicia o motor, sorteia o mundo e simula as semanas pedidas); a interface mostra "Simulando…".
- A simulação ainda é por semanas a partir de um mundo novo, não "a partida X". Partida avulsa por ids e lances ao vivo continuam pendentes (Fase 3).
- Windows: os testes de integração do Tauri precisam do manifesto Common Controls v6 (`windows-test.manifest`).
