# Entrada de cenario

Novo fluxo: loading real concluido -> primeiro frame do cenario totalmente preto
-> fade-in de 1,5 s -> localizacao digitada -> texto sai em 0,5 s -> 0,75 s de
cenario sem sobreposicoes -> chamada de radio. A apresentacao externa dura cerca
de 4,6 s no total, com partes do fade e da digitacao ocorrendo simultaneamente.

Textos exibidos, com a capitalizacao solicitada:

Ano 2066.
Distrito Industrial UTINGA.
Matriz ETIP Corporation.

A linha do dossie inicial tambem foi alinhada para 2066 / UTINGA.
No interior ha somente fade de 1,5 s e pausa de 0,75 s, sem repetir a localizacao.

SceneIntroSystem usa Tyra Timer, nao contagem de frames. O relogio da abertura
so avanca depois que scene.resident e scene.presented estao verdadeiros.
GameApp marca presented somente depois de renderer.endFrame() do gameplay;
o tempo de IO anterior e descartado. O overlay e registrado por ultimo para
cobrir a imagem inteira. HUD, radio e comandos de gameplay aguardam a abertura.

A regressao da chamada prematura foi corrigida em dois pontos: GameApp converte
pedidos para Loading antes do estagio de simulacao/audio; updateRadio verifica
resident, presented, requested, accessTicks e intro.active antes de iniciar a
chamada. Assim o estado GameplayFps temporario escolhido pelo input nao pode
iniciar o som enquanto o cenario ainda esta sendo preparado.

Teste host scene_intro_test.cpp cobre esse intervalo, primeiro frame preto,
progressao do fade, textos, pausa em menus, bloqueio de controles/HUD, atraso da
chamada e fade interno. O teste radio_codec_test.cpp continua verificando a
sequencia alerta/icone -> confirmacao/mensagem apos a abertura.
Build PS2 e verificacao visual/audivel no PCSX2 pendentes: o Docker continua
inacessivel nesta sessao. As verificacoes host nao simulam pixels ou DMA reais.
