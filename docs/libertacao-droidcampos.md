# Libertação do DroidCampos

Depois das três ondas, abra a nova entrada em OCORRENCIAS e feche o terminal.
O mastro vermelho emerge em quatro segundos no centro da sala (X=0, Z=-26).
Aproxime-se a até 1,5 m, mire no botão e pressione Triângulo. A interação só
funciona depois da subida e consome os demais comandos desse quadro.

A cápsula abre durante três segundos; o vidro desce para dentro da base e as
travas sobem. Nos três segundos seguintes, o ocupante sai sobre a base elevada
e desce ao piso. Iluminação e câmera oscilam durante esses seis segundos.
Movimento e mira permanecem livres. Rádio, terminal e pausa congelam os tempos.
A mira e os disparos usam a mesma câmera com tremor; a posição física não oscila.

O personagem percorre a instalação a 2 m/s, abre portas e dá socos de três
segundos quando alcança o jogador. O impacto causa 20 de dano uma vez por golpe,
na janela dos quadros 46–49, respeitando distância, orientação, altura e cenário.
O jogador pode esquivar. O laser produz impactos, mas não reduz a vida do
DroidCampos. Não há derrota do chefe nem morte automática por tempo.

O soco usa a variante curvada para alcançar a altura do jogador. Corredores
usam caminhada curvada; portas usam uma passagem lateral, sem reduzir a escala.
A pose de passagem tem 2,132 m de altura e 1,340 m de largura transversal,
compatíveis com as aberturas de 2,2 x 1,4 m. Os ossos não são escalados.
O personagem afasta fisicamente o jogador que bloquear a passagem da porta,
com deslocamento verificado contra o cenário, sem sobreposição intencional.

A morte nesse encontro exibe GAME OVER / DROIDCAMPOS E INEVITAVEL e somente
NEW GAME e LOAD GAME. Mortes nas ondas preservam o retry original.

## Recursos e reprodução

A cápsula original permanece como fonte auditada. tools/split_droid_capsule.py
separa casca, ocupante e travas, mantendo as texturas e o total de triângulos.
O gerador do interior chama essa separação ao terminar a exportação.

```powershell
python tools/split_droid_capsule.py
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' -b -t 2 --python tools/export_release_button.py
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' -b -t 2 --python tools/export_droid_preview.py
./tests/host/run.ps1
./tools/build-etip.ps1
```

O exportador do botão lê apenas as seis peças sob o objeto-raiz do mastro;
não importa o estúdio. O banco do DroidCampos mantém 6.900 vértices por pose,
84 poses: caminhada e soco originais, suas variantes curvadas e a passagem.
Posições ocupam 6.955.200 bytes, mais cabeçalho de 16 bytes. O renderizador
preserva a ordem dos triângulos e não avança a simulação durante a renderização.
O antigo percurso automático de demonstração foi removido.

## Validação

A suíte host passou, incluindo terminal, combate, ondas, armas, cena, renderização,
clipping e recursos. Os testes novos cobrem gatilho do registro, tempos, pausa,
janela de dano, bloqueio por paredes, esquiva, invulnerabilidade, perseguição
nos corredores e quatro portas, reset, interação e rotas de game over.
Testes de renderização cobrem ocupante único, botão e três ciclos de descarga.

O build PS2 de engine e jogo passou. A execução automatizada isolada no PCSX2,
com ExtraMemory=false (32 MiB), passou por subida, libertação, perseguição,
cinco golpes e game over. As primeiras amostras foram 59 FPS, com cerca de
15,6 MiB de RAM livre. Isso não mede pico de RAM, VRAM, p95/p99 nem hardware real.
Logs e ELF instrumentado ficam em artifacts/droid-release/ (ignorados pelo Git).
O ELF de produção não inclui a instrumentação que pula diretamente para a cena.

A revisão visual interativa e o balanceamento com controle humano permanecem
pendentes. A execução de teste foi oculta e não expôs janela capturável pela
ferramenta de controle do computador.
