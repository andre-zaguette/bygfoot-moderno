# Bygfoot Moderno

Fork modernizado do [Bygfoot](https://gitlab.com/bygfoot/bygfoot): gerenciador de futebol offline, com interface nova e partida acompanhada em 2D, para macOS, Windows e Linux.

**Estado:** Fase 2 (app desktop Tauri + primeira partida) pronta na branch `fase-2-app`, aguardando revisão e merge; Fase 1 (núcleo headless nos 3 sistemas) já está em `main`. O app roda só em modo de desenvolvimento (precisa do motor compilado, ver ADR-003). Nada aqui é um jogo jogável ainda.

- Plano: `ROADMAP_BYGFOOT_MODERNO.md`
- Regras do projeto: `CLAUDE.md`
- Plano da Fase 1: `docs/superpowers/plans/2026-11-02-fase-1-nucleo-3-sistemas.md`
- Decisões: `docs/adr/` (ADR-001 origem do código; ADR-002 determinismo entre sistemas)
- Remendos sobre o upstream: `patches/MANIFEST.md` (aplicados a uma cópia por `tools/apply-patches.sh`)

## Licença

- O código novo deste repositório (fora de `upstream/`) é distribuído sob a **GNU GPL, versão 2 ou, a seu critério, qualquer versão posterior** (GPL-2.0-or-later). O texto da GPL v2 está em `LICENSE`.
- `upstream/` é uma cópia sem edição do [Bygfoot](https://gitlab.com/bygfoot/bygfoot) (commit `9b7b0062`) e mantém os avisos e a licença originais, em `upstream/COPYING` e nos cabeçalhos dos arquivos. Cerca de 23 arquivos dele não têm cabeçalho de licença, e a licença efetiva ainda precisa de revisão jurídica: veja `docs/LEGAL_REVIEW.md`.
- Isto não é aconselhamento jurídico.
