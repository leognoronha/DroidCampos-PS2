# Uso e organização de artifacts

Revisão em 31/10/2026, antes do commit da cápsula ocupada.

O build do jogo usa game/res, game/src, game/inc e engine. A pasta artifacts não é uma fonte de assets de produção. Entretanto, os launchers de validação ainda dependem de arquivos nela; não deve ser movida integralmente sem atualizar essas referências.

| Pasta | Conteúdo local | Ação recomendada |
| --- | --- | --- |
| optimization (~33,5 MiB, 93 arquivos) | Builds históricos, logs, manifesto de hashes, perfis PCSX2 e uma cópia antiga dos recursos em run-integrated | Arquivar os resultados históricos fora do checkout; preservar o resumo e hashes em docs. Remover do índice Git as cópias de assets após atualizar o launcher. |
| combat (~19,4 MiB, 13 arquivos) | ELFs de validação, logs e perfil local do PCSX2 | Manter como saída local ignorada; já coberta pelo .gitignore. Pode ser arquivada externamente. |
| terminal-preview (~0,42 MiB, 7 arquivos) | preview.cpp, render.py, dumps de desenho, PNGs e executável | Mover os dois fontes para tools/terminal-preview ou helpers de tests/host e parametrizar a saída. Manter dumps, PNGs e executável como saídas locais ignoradas. |
| enemies | Pasta vazia | Não há conteúdo necessário; diretórios vazios não são versionados. |

## Dependências que precisam ser ajustadas antes da limpeza

- tools/run-optimized.ps1 usa hud-step5.elf, run-integrated e pcsx2-validation dentro de artifacts/optimization. É uma execução histórica e não deve ser confundida com o jogo atual.
- tools/run-combat.ps1 executa game/bin, mas copia seu modelo de configuração de artifacts/optimization/pcsx2-validation/PCSX2/inis/PCSX2.ini.
- docs/validacao-pcsx2.md e docs/otimizacao-ps2-execucao.md apontam para resultados históricos dessa pasta.
- preview.cpp e render.py usam caminhos fixos de artifacts/terminal-preview para dumps e imagens.

## Organização proposta

- game/res: recursos de produção únicos, incluindo a cápsula ocupada e texturas do DroidCampos.
- game/bin: saída do build usada pelo emulador.
- tools/pcsx2 ou tests/pcsx2: somente configuração mínima de teste reproduzível, sem caminhos pessoais, cache, memory cards ou credenciais.
- tools/terminal-preview: fontes da ferramenta de prévia, caso ela seja mantida.
- docs: conclusões de validação e manifesto histórico relevante.
- artifacts: saídas locais regeneráveis, ignoradas pelo Git; arquivos históricos volumosos podem ir para uma pasta externa de arquivo.

A limpeza deve preservar os arquivos locais antes de removê-los do índice e migrar as dependências dos launchers. Não remover dados pessoais de emulador ou descartar evidências de validação. Mover arquivos para fora de um commit futuro não elimina o conteúdo já presente no histórico Git; reescrever o histórico não faz parte desta proposta.

Esta revisão apenas documenta a recomendação. O conteúdo e os caminhos de artifacts não foram reorganizados no commit da cápsula ocupada. As prévias de terminal que já estavam fora do Git ficam fora desse commit.
