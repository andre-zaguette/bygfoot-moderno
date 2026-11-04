# ADR-006 — Catálogo e seletor da base de dados do Bygfoot

Data: 2026-11-04 · Status: aceito

## Decisão
- O comando `catalog` (Rust, `app/src-tauri/src/catalog.rs`) lê `upstream/support_files/definitions` (ou `BYGFOOT_SUPPORT`) e devolve continentes → países → ligas (com clubes) e copas. A interface mostra isso num seletor País → Liga → Clube, com filtro por nome sem acentos.
- A leitura é tolerante (extrator de tags, sem parser estrito): o upstream tem XML quebrados (`league_mexico3-1.xml`, `cup_apertura.xml`, `league_wc06_ncamerica_teams.xml`...). Clubes repetidos na mesma liga (`austria2`) aparecem uma vez; nomes com aspas, barra invertida ou controles são omitidos porque o `bf-headless` não os aceita.
- O `sid` do país é o nome do arquivo `country_<sid>.xml` (o `mexico` não tem `<sid>` dentro do XML).
- As semanas a simular vêm do `first_week` da liga (`max(2, first_week + 1)`: a primeira rodada pode ser folga do clube; `sweden5j` falhou com 5 semanas e passou com 6); sem `first_week` usamos 12 (`peru3` não rodou com 4 nem 8 semanas, rodou com 12).
- Se o catálogo não carregar, o formulário volta a campos de texto para país e time, com o erro visível.

## Limites conhecidos (medidos)
- Os modos de seleções `fifa`, `wc06` e `euro08` encerram o motor original com SIGSEGV (exit 139) ao simular (`bf-headless --country <modo>`); ficam fora do seletor (`ENGINE_CRASHES`). A causa não foi investigada.
- Times "genéricos" do upstream (p.ex. `England 374`) aparecem na lista como qualquer outro clube.
- O motor devolve só a partida já disputada nas semanas pedidas; por isso o seletor sugere as semanas.
