# ADR-005 — Dataset real do Bygfoot por padrão; gerador fictício opcional

Data: 2026-11-04 · Status: aceito

## Decisão
O produto usa a base de dados do próprio Bygfoot (`upstream/support_files`, 1216 arquivos de definição: países, ligas, copas e clubes), por pedido do usuário. Nada nos testes, no `bf-headless` ou no app foi trocado.

`tools/fictional_data.py` continua no repositório como **ferramenta opcional**. Ela gera, em `build/fictional/support_files`, uma cópia com clubes e competições inventados (determinística, estrutura dos XML intacta, `<first_team>` incluído). O motor simulou partidas com essa cópia em 5 países. Para usá-la: `BYGFOOT_SUPPORT=build/fictional/support_files`.

## Por que ainda importa
O usuário removeu a preferência por dados fictícios do CLAUDE.md (regra 7 reescrita em 2026-11-04). A base do Bygfoot é distribuída com o código sob GPL, mas os nomes de clubes e competições reais podem ser marcas de terceiros. Para uso pessoal e desenvolvimento isso não bloqueia nada. **Antes de distribuir o aplicativo** (binários, loja, divulgação), a decisão deve ser revista com a revisão legal pendente.

## Consequências
- Os nomes de jogadores são sorteados de listas genéricas de nomes e sobrenomes; não há elencos reais na base.
- 9 arquivos XML do próprio upstream são malformados (ex.: `cup_apertura.xml` vazio); a ferramenta os repassa sem alterar a estrutura.
- A liga `austria2` tem um clube repetido no upstream; a ferramenta não piora isso.
