# Determinismo entre sistemas (Fase 1, Task 2)

## Resultado

**Os 3 sistemas NÃO produzem a mesma partida para a mesma seed.** Cada sistema, sozinho, é determinístico (hash repetível), mas os hashes diferem entre macOS, Linux e Windows nas **6 de 6** seeds de ouro.

- Execução do CI: https://github.com/andre-zaguette/bygfoot-moderno/actions/runs/41443841078 (job `compare` vermelho; jobs de build e testes verdes nos 3 sistemas).
- Seeds: 0, 1, 431199, 2147483647, 20261102 e `431199` com `B. Tórshavn` (acento). Todas divergem, inclusive a sem acento, então o acento não é a causa.

## Evidência do diagnóstico (execução 41445189552, seed 0, `V. Gotu`, 2 semanas)

- A partida é a mesma: `fixture_id` 25, `V. Gotu` x `K. Klaksvík` nos três. O calendário não diverge.
- Os lances divergem desde a **linha 1**: macOS começa com posse do time 1, Ubuntu com posse do time 0; Windows e macOS coincidem na linha 1 mas divergem na linha 3. Placar final: macOS 1-0, Ubuntu 1-0, Windows 2-0.
- A ordem em que o sistema devolve os arquivos de `support_files/definitions` (o que `g_dir_read_name` usa, sem ordenar, em `upstream/src/file.c`) é **diferente nos três**: macOS e Ubuntu em ordens distintas e não ordenadas, Windows em ordem alfabética.

## O que isto prova e o que não prova

Prova: a divergência existe e vem do estado do sorteio antes do primeiro lance, não dos nomes nem do calendário.
**Não prova** a causa. A ordem de diretório é hipótese (ordem de carga muda a ordem em que o gerador de números aleatórios é consumido); ponto flutuante e a versão da GLib continuam candidatos. O teste que decide: aplicar um remendo que ordena as listagens e conferir se os 3 hashes passam a coincidir. Isso está no `ADR-002`.
