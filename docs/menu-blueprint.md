# Menu blueprint

Interface em 512 x 448 com fundo preto, identidade ETIP Corporation., navegação
NEW GAME / LOAD GAME / SETTINGS e viewport reservado para um modelo 3D futuro.
O modelo 3D e sua rotação ainda não fazem parte desta mudança.

## Assets

- `game/res/ui_blueprint.png`: atlas 512 x 512, PNG indexado de 8 bits com alpha.
- `game/res/ui_menu_buttons.png`: atlas 512 x 256; três linhas, duas colunas
  (normal à esquerda, selecionado à direita), com margem entre recortes.
- `tools/build_blueprint_ui.py`: regeneração determinística com Pillow e fontes
  Agency/Consolas instaladas no Windows. As fontes não são distribuídas e não
  são necessárias para compilar ou executar o jogo; os PNGs já estão prontos.

A arte estática é recortada em sprites MODE_REPEAT. O fundo do viewport é
transparente; o passe do menu limpa a tela em preto. O retrato antigo e o
carregamento de bg_cyber.png foram retirados do drawer. As texturas novas são
liberadas no shutdown. drawPanel e a fonte do HUD continuam disponíveis.

## Progresso

NEURAL SYNC faz uma animação decorativa de inicialização de 0 a 100% usando o
relógio de animação existente (200 atualizações). O valor é limitado a 0..100.
Na tela Loading, usa loadStep / totalSteps reais; denominador zero indica 0%.
A barra segmentada acompanha a porcentagem, e todos os segmentos acendem a 100%.

## Verificação

Os arquivos reais menu_system.cpp, ui_context_drawer.cpp e font_sprite.cpp foram
compilados com g++ C++17, -Wall -Wextra, contra um renderer host simulado.
O smoke test local passou para as três seleções, telas de missão, slots,
configuração, loading e erro, limites 512 x 448, clamp do progresso,
exclusão do menu durante gameplay, init idempotente e liberação das texturas.
Prévia visual recomposta a partir das submissões dos sprites C++ em oito estados.
Essa verificação não valida pixels, DMA, VRAM ou temporização no hardware.

Build PS2 e teste PCSX2 pendentes: o endpoint Docker Desktop Linux Engine estava
indisponível. Com Docker iniciado, usar a task existente Build & run no VS Code.
