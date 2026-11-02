# Build do Bygfoot original em macOS

**Resultado: compilou e linkou** (`build/upstream/bygfoot`, Mach-O arm64, 1,1 MB). Não executei o programa nesta tarefa, então "compila" é tudo que este documento afirma.

## Ambiente (2026-11-01)

- macOS no Apple Silicon (arm64), `Apple clang 21.0.0`, `cmake 4.4.5`
- Dependências via Homebrew: `gtk+3 3.24.52`, `glib 2.90.1`, `json-c 0.19`, `sqlite 3.54.0`, `pkgconf 3.0.9`, `gettext 1.0`
- Instaladas para esta tarefa, com autorização: `cmake` e `gtk+3`

## Comando

```
sh tools/build-upstream.sh
```

Equivale a `cmake -S upstream -B build/upstream && cmake --build build/upstream -j 4`, com `PKG_CONFIG_PATH` apontando para `/opt/homebrew`. A pasta `build/` não vai para o git. Log completo: `docs/spikes/build-macos.log` (438 linhas).

## Evidência

- Saída termina em `[100%] Built target bygfoot` e `== exit=0`.
- `sh tools/verify-upstream-pristine.sh 9b7b0062` → `OK` depois do build (o build não alterou `upstream/`).
- O binário liga com GTK 3, GDK, Pango, Cairo e GLib/GIO (`otool -L`).

## Avisos do compilador (33 linhas com `warning:`)

Nada impediu o build, mas alguns merecem atenção na Fase 1 porque estão no caminho do save:

| Arquivo | Aviso | Por que importa |
|---|---|---|
| `json_serialize.c` | função não-void sem `return` | comportamento indefinido se o valor for usado |
| `sqlite_data_store.c` | função não-void sem `return` em todos os caminhos | idem |
| `json_deserialize.c` | passa `guint32 *` onde se espera `gint *` | diferença de sinal; pode corromper valores grandes |
| `sqlite_data_store.c` | `const unsigned char *` usado como `const char *` | só estilo, em geral inofensivo |
| `callbacks.c` | `switch` sem tratar vários `STATUS_*` | esperado, estados de interface |
| `gui.c` | `const OptionList *` passado como `OptionList *` | descarta `const` |

Esses avisos são do upstream. Nenhum foi corrigido (regra: não editar `upstream/` antes do diagnóstico).

## Não foi feito

- Rodar o programa (precisa de display; está fora do escopo desta tarefa).
- Windows e Linux: ficam para o CI da Fase 1.
- Medir tempo de build.
