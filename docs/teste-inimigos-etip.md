# Teste inicial de inimigos no interior ETIP

Dois modelos da versão `animations-v3-fit` aparecem na sala final, perto da parede
do fundo, em faixas livres da cápsula e do terminal. Cada um repete, com defasagem,
postura de ataque, corrida de dois metros para a entrada, mira, animação de
disparo e corrida de retorno. O disparo é apenas visual: ainda não há IA, projétil,
dano nem colisão com o jogador. A pausa, o rádio, o loading e a introdução param
o relógio da animação; L3/R3 reinicia a sequência.

Entre pelo leitor, percorra o corredor até a sala final e observe os dois modelos
de frente e de perto. Confira as três ações, proporções, texturas, clipping quando
próximo, pausa/rádio, reinício e novas entradas; registre FPS e memória disponíveis.
O ELF compilado está em `game/bin/droid_campos.elf`, com recursos em
`game/bin/enemies/`. `tools/build-etip.ps1` recompila pelo contêiner Docker.

Os recursos em `game/res/enemies/` são produzidos por `tools/pack_enemy_test.py`
usando o Blender/OBJ aprovado em `../DroidCampos-models/enemies/cerberus/animations-v3-fit`.
Os 18 quadros de cada variante compartilham UVs e textura; os bancos de posições
ocupam aproximadamente 2,2 MiB juntos, excluindo malhas, texturas, parser e demais
alocações. A pipeline estática própria dos inimigos sincroniza o DMA antes de
atualizar os vértices e invalida apenas seu cache de geometria; a caixa de seleção
cobre todos os quadros. Não há medição real de pico de RAM, VRAM ou desempenho.

O carregamento verifica arquivos e cabeçalhos antes da troca; dados inválidos
detectados durante a leitura abortam a carga e retornam à tela de erro. O modelo
original, sua atribuição e a licença declarada pelo upload estão em
`game/res/enemies/SOURCE-LICENSE.md` e junto dos modelos de autoria.
