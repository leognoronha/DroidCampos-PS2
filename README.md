# DroidCampos-PS2 (v0.0.1)

## Interior ETIP

O leitor externo agora solicita a transição com loading para o corredor interno,
com quatro salas e portas deslizantes, terminal à esquerda e cápsula de contenção
à direita da sala final de pé-direito elevado. Modelos, exportação, ciclo de memória e
verificações estão documentados em [docs/etip-inside.md](docs/etip-inside.md).
Build PS2 e validação no PCSX2 desta implementação ainda estão pendentes.

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

## Controles de gameplay

- X: pular (um pulo por toque, apenas no chao).
- Quadrado: pegar itens e interagir com portas e leitor de acesso.
- L2: abrir ou fechar a revisao do radio durante a gameplay. O fecha as transmissoes automaticas.
- Triangulo: abrir/fechar a consulta ao terminal quando estiver proximo.
- L1 segurado: correr; solte para voltar a andar.
- Analogico direito: camera. Analogico esquerdo/direcional: movimento.

O terminal apresenta uma consulta local de conexao; ainda nao executa objetivos
ou comandos de missao. O pulo preserva as colisoes horizontais existentes.

- Start: abrir/fechar pausa (Continuar ou Menu principal); O tambem retoma.
- L2: consultar silenciosamente a mensagem do cenario atual.
- R2: usar um kit medico. Cada kit restaura ate 40 pontos de vida.
- O: fechar a mensagem com o toque de confirmacao do radio.

Portas e leitor de acesso: a mensagem e o Quadrado exigem proximidade e mira
sobre o alvo. Olhar para o corredor, chao ou teto nao seleciona portas laterais.
Com a porta aberta, mire na abertura para fecha-la.
Pegue a pistola e o cartao junto a guarita antes de usar o leitor.
Ha um kit medico no fundo do patio e outro na primeira sala lateral esquerda.

## Tiro a laser (teste)

- R1: um disparo por toque; intervalo minimo de 0,25 s.
- O em gameplay: recarga manual de 0,514 s, quando o pente nao esta cheio.
- Pente de 8 tiros, reserva infinita; sem recarga automatica no vazio.
- Transmissoes automaticas suspendem tiro, recarga e uso de kit ate fechar com O. Na revisao por L2, a gameplay continua e O recarrega.
- Pausa, menus, introducao e loading bloqueiam a arma; os temporizadores congelam.

Feixe ciano de 50 m no maximo, impacto na geometria real, portas moveis e vidro.
A pistola recolhida perto da guarita vem com o pente cheio.
Os WAV originais estao em `game/assets/audio`; converter com
`python tools/build_weapon_audio.py --ffmpeg <ffmpeg> --adpenc <adpenc-do-PS2SDK>`.
Sao 39.296 bytes adicionais de ADPCM mono 22.050 Hz. Canais 10�20: tiro;
21: recarga; 22: radio; 0�9: interface. Falha de audio nao bloqueia disparos.

Validacao: `tests/host/run.ps1`, incluindo arma, geometria e HUD. Os testes host
nao substituem a compilacao PS2 e a verificacao de pixels/desempenho no PCSX2.
