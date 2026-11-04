# Revisão inicial de licenças e assets

Estado: **levantamento de fatos da Fase 0. Isto não é parecer jurídico** e não conclui que nada é permitido. As perguntas da seção 5 precisam de uma pessoa com conhecimento jurídico antes de qualquer distribuição.

Base: upstream fixado em `9b7b0062` (`docs/spikes/upstream-pin.md`). Comando: `sh tools/license-scan.sh upstream`.

## 1. O que o código diz

- `upstream/COPYING` é o texto da **GNU GPL versão 2** (junho de 1991).
- Dos 201 arquivos `.c`/`.h` de `upstream/src`, **178 têm cabeçalho** dizendo "versão 2 da licença **ou, a seu critério, qualquer versão posterior**" (classe `GPL-2.0-or-later`). Copyright citado: Gyözö Both, 2005.
- **23 arquivos não têm cabeçalho de licença reconhecido** (classe `SEM-CABECALHO`):
  - `json_compat.h`, `json_deserialize.c/.h`, `json_interface.c/.h`, `json_serialize.c/.h` (serialização do save);
  - `bygfoot.c`, `bygfoot_struct.h`, `bygfoot_typedefs.h`;
  - `interface.c/.h`, `misc2_interface.c/.h`, `misc3_interface.c/.h`, `misc_interface.h`, `options_interface.c/.h`, `training_interface.c/.h` (nomes que sugerem código gerado a partir de arquivos Glade; **hipótese**, não confirmei);
  - `league_table_callbacks.c/.h`.
- O histórico git do clone de estudo mostra que `json_serialize.c` e `sqlite_data_store.c` foram escritos recentemente por duas pessoas (8 e 1 commits), com datas de 2025 e 2026. Não verifiquei a licença sob a qual essas pessoas contribuíram.

## 2. Discrepância com o roadmap

O roadmap diz que o pacote Fedora da 3.0.0 identifica **GPL-2.0-only**. Os cabeçalhos dos fontes dizem **GPL-2.0-or-later** em 178 arquivos. Não consegui conciliar os dois apenas pela leitura:

- o pacote Fedora pode estar classificando o conjunto pela cláusula mais restritiva (COPYING + arquivos sem cabeçalho);
- ou os cabeçalhos mudaram depois; ou o pacote está impreciso.

Isso importa porque GPL-2.0-only e código GPLv3 não podem ser combinados num mesmo binário, e GPL-2.0-or-later pode. **Pergunta aberta 1.**

## 3. Assets em `upstream/support_files`

Fonte: linhas `ASSET` do `license-scan.sh`.

| Pasta | Arquivos | Tipos | Nota de licença encontrada |
|---|---:|---|---|
| `definitions` | 1.218 | xml 1.216, TXT 1, txt 1 | só um `README.TXT` de calendário (Ilhas Faroe), sem licença |
| `pixmaps` | 255 | png | nenhuma |
| `names` | 34 | xml | nenhuma |
| `lg_commentary` | 18 | xml 17, in 1 | nenhuma |
| `news` | 17 | xml 16, in 1 | nenhuma |
| `mmedia` | 15 | jpg | nenhuma |
| `hints` | 13 | sem extensão | nenhuma |
| `strategy` | 9 | xml | nenhuma |

Fatos a destacar:

- **Nomes de clubes reais:** `support_files/definitions` tem 9.223 entradas `<team_name>`. As que li incluem nomes brasileiros reais (`ABC`, `Americana`, `Criciuma`, `Goias`, `Duque de Caxias`). Parte das ligas (England) usa nomes genéricos como `England 1009`. Não classifiquei as 9.223.
- O `AUTHORS` do upstream credita ícones a uma pessoa (Mark Symonds) e a página a outra, sem dizer em qual licença cada contribuição foi entregue.
- Não encontrei escudos de clubes nem bandeiras entre os nomes de `pixmaps` (conferi só pelos nomes de arquivo, não abri as imagens).
- Nenhum dos 255 PNG nem dos 15 JPG tem nota de licença ao lado. As fotos de `mmedia` (`match*.jpg`) vêm de onde? **Pergunta aberta 3.**

## 4. Dependências planejadas

Fontes: conhecimento geral das licenças, **não verificado nesta sessão**. Cada item deve ser confirmado nos arquivos `LICENSE` das versões que serão usadas.

| Dependência | Licença (a confirmar) | Observação |
|---|---|---|
| GLib | LGPL-2.1-or-later | ligação dinâmica vs. estática muda as obrigações |
| SQLite | domínio público | |
| json-c | MIT | |
| Tauri v2 | MIT ou Apache-2.0 (dupla) | escolher MIT evita o conflito do item abaixo |
| Crates Rust | em geral MIT/Apache-2.0 | rodar `cargo deny`/`cargo about` antes do release |
| React, PixiJS e demais pacotes npm | em geral MIT | rodar um inventário de licenças antes do release |

Nota: a FSF considera Apache-2.0 incompatível com GPL-2.0-only e compatível com GPLv3. Como o Bygfoot é GPL-2.0-or-later na maioria dos arquivos e GPL-2.0-only em dúvida, o conjunto de dependências deve preferir MIT ou BSD.

## 5. Perguntas abertas (precisam de pessoa qualificada)

1. Qual é a licença efetiva do upstream: GPL-2.0-only, GPL-2.0-or-later, ou mista? O que fazer com os 23 arquivos sem cabeçalho?
2. Um aplicativo Tauri que chama o núcleo em C por biblioteca ou por processo separado configura uma obra derivada? O roadmap já alerta que IPC não garante independência.
3. Qual é a origem e a licença das imagens (`pixmaps`, `mmedia`) e dos dados em `definitions`? Nomes de clubes e de jogadores reais podem ser usados no lançamento?
4. Como atender à obrigação de oferecer o código-fonte nas builds para macOS, Windows e Linux?
5. As contribuições recentes de JSON/SQLite (2025–2026) foram entregues sob a mesma licença?

## 6. Decisão provisória para o projeto

Sem resposta jurídica, o projeto fica assim (já é a regra 7 do `CLAUDE.md`):

- o projeto usa a base de dados do Bygfoot (decisão do usuário em 2026-11-04); nenhum escudo, logo ou foto de atleta novo sem direito de uso comprovado;
- os dados do upstream não vão para o lançamento até a pergunta 3 ser respondida;
- novas dependências só com licença compatível (MIT/BSD preferidas);
- avisos de copyright do upstream preservados em `upstream/` sem edição.
