# ADR-005 — Escudos de clubes na pasta do usuário

Data: 2026-11-04 · Status: aceito

## Contexto
O Bygfoot não traz escudos (os `symbol` dos XML são só bandeiras de países). Escudos de clubes são marcas e têm direitos autorais; o projeto não tem como licenciá-los.

## Decisão
- O app não distribui nenhum escudo. O usuário coloca PNGs em `<dados do app>/escudos/<slug>.png`, em que `<slug>` é o nome do clube em minúsculas, sem acentos, com `-` no lugar de espaços e pontos (`K. Klaksvík` → `k-klaksvik.png`).
- Sem arquivo, a interface mostra um emblema gerado (iniciais e cor estável por nome), sem imitar clube algum.
- O **Guia de escudos** dentro do jogo explica isso passo a passo e mostra, para os clubes da partida, o nome exato do arquivo e a situação (Encontrado, Faltando, Inválido + motivo).
- Dois comandos novos, com permissão própria (`allow-crest-folder`, `allow-crest-status`): `crest_folder` e `crest_status`. A interface continua sem acesso geral a arquivos.
- Validação em Rust (`app/src-tauri/src/crests.rs`): só PNG (assinatura e cabeçalho IHDR), 16 a 1024 px por lado, até 512 KB, sem links simbólicos; o nome do arquivo vem só do slug, então não há como sair da pasta. No máximo 64 clubes por consulta.

## Consequências
- Nenhum risco de direitos para o projeto; a responsabilidade pelas imagens é de quem as adiciona (o guia diz isso).
- A pasta fica no diretório de dados do app; `BYGFOOT_ESCUDOS` a substitui (testes e uso avançado).
- Abrir a pasta pelo sistema (botão "Abrir pasta") não existe: exigiria um plugin com mais permissões; hoje o guia mostra o caminho e permite copiá-lo.
- Sair do ao vivo para abrir o guia desmonta a partida em reprodução; ao voltar, ela recomeça pelo resultado.
