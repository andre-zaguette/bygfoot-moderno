# ADR-003: Adaptador do motor como processo separado e casca Tauri

Status: aceita (Fase 2, 2026-11-03)

## Decisão
- O app desktop é **Tauri 2 + React + TypeScript (Vite)**. A interface nunca calcula resultado: só mostra a `MatchTimeline v1` devolvida pelo motor.
- O motor C do Bygfoot roda como **processo separado**, chamado pelo crate Rust `engine-adapter` (via `spike/headless/bf-headless`). Sem FFI, sem ligar o C ao processo do app.
- O adaptador valida a saída: campos desconhecidos são recusados; `schemaVersion`, `engine` e `upstreamCommit` são conferidos; `area`, `time`, `possession`, `minute`, `event.type` e `event.team` precisam estar nos intervalos do schema; placares não são negativos; o placar só muda em gol (+1 fora dos pênaltis); o primeiro lance é START_MATCH e o último END_MATCH; o último `result` é igual a `match.final`; e `source` (seed, país, time, semanas) tem de bater com o pedido. `commentary` é opcional, como no schema. Erros têm tipo estável (`invalid_params`, `engine_unavailable`, `engine_failed`, `timeout`, `bad_output`, `invalid_timeline`).
- O motor roda com **prazo** (180 s por padrão). Ao estourar, o `sh` e o motor (grupo de processos no Unix, `taskkill /T` no Windows) são encerrados e a interface recebe `timeout`.
- A interface só pode chamar `simulate_match`: a capability da janela principal tem apenas `allow-simulate-match` (sem `core:default`). Um teste confirma que `plugin:path|resolve_directory`, `plugin:app|version` e `plugin:window|title` são negados. Sem rede e sem acesso a arquivos.

## Por quê
- Processo separado mantém o `upstream/` e os remendos intactos, evita acoplamento de ABI e falha do motor não derruba o app.
- Validar na fronteira transforma mudança do motor (ex.: sincronização com o upstream) em erro claro, não em tela quebrada.

## Evidência
- `engine-adapter/tests` (11 testes) e `app/src-tauri/tests/ipc.rs`: o comando, chamado pelo IPC do Tauri com o motor real, devolve JSON **igual** à amostra `tests/contract/timeline-v1.sample.json`.
- Workflow `app.yml` roda tudo em macOS, Linux e Windows.

## Limites conhecidos (dívida assumida)
- **`source.patches` e `upstreamCommit` são declarativos.** Vêm de `patches/` e de `tools/upstream-pin.txt`, não de uma medição do binário. Se `BYGFOOT_BIN` apontar para um motor sem os remendos, a timeline ainda os declara. Um manifesto gravado pelo build ao lado do binário resolveria; fica para a Fase 6.
- **Windows (CI):** `sh`, `python3` e as DLLs do motor dependem de o MSYS2 estar no fim do PATH. O workflow falha se o MSYS2 com os pacotes não for encontrado, mas o desenho ainda é frágil a mudanças na imagem do runner.
- **Timeout no Windows:** o `taskkill /T` não alcança processos do MSYS2 como filhos, então o motor pode sobreviver órfão após o prazo (a interface recebe `timeout` na hora). No macOS e no Linux o grupo de processos inteiro é encerrado.
- **Concorrência:** nada impede várias chamadas simultâneas ao comando (cada uma inicia um motor).
- Saída do motor sem teto de tamanho (risco baixo com `weeks ≤ 100`).
- O binário embute o caminho do checkout em que foi compilado (`CARGO_MANIFEST_DIR`): nunca distribuir assim.
- **Modo de desenvolvimento.** O wrapper é `sh` + `python3`, e o binário do motor vem de `BYGFOOT_BIN` ou de `build/patched/`. Um instalador para usuário final precisa empacotar o motor e portar o wrapper para Rust (Fase 6). Hoje o app não roda fora de um checkout com o motor compilado.
- Cada partida leva alguns segundos (inicia o motor, sorteia o mundo e simula as semanas pedidas); a interface mostra "Simulando…".
- A simulação ainda é por semanas a partir de um mundo novo, não "a partida X". Partida avulsa por ids e lances ao vivo continuam pendentes (Fase 3).
- Windows: os testes de integração do Tauri precisam do manifesto Common Controls v6 (`windows-test.manifest`).
