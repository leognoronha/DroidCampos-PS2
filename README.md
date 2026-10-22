# DroidCampos-PS2 (v0.0.1)

Jogo low-poly industrial desenvolvido para **PlayStation 2** utilizando a engine C++ **Tyra (PS2SDK)**.

## Estrutura do Repositório

- `engine/` — Código-fonte da engine Tyra para PlayStation 2 (EE / GS / VU1 / IOP).
- `game/` — Código-fonte, headers e recursos de **DroidCampos-PS2** (`droid_campos.elf`):
  - `game/src/` — Implementação do jogo (`main.cpp`, `droid_campos_game.cpp`, `font_sprite.cpp`).
  - `game/inc/` — Cabeçalhos (`droid_campos_game.hpp`, `font_sprite.hpp`).
  - `game/res/` — Texturas, fontes e assets do jogo.
- `Makefile.base` — Regras base de compilação com `mips64r5900el-ps2-elf-g++`.
- `windows-pcsx2.ps1` — Script de execução automática do binário `.elf` no emulador PCSX2.

## Como Compilar e Executar

1. Inicie o container de compilação PS2SDK/Tyra:
   ```bash
   docker compose up -d
   ```
2. No VS Code, execute a task padrão de build (`Ctrl+Shift+B` -> **Build & run**), que compila `game/` gerando `game/bin/droid_campos.elf` e inicia o PCSX2 automaticamente.

## Versão Atual

- **v0.0.1** — Configuração inicial limpa do projeto e estrutura base do jogo.
