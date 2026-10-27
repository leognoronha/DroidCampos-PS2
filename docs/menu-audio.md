# Efeitos do menu

X / Start: sci-fi-interface-positive.wav para confirmacoes aceitas.
O / Select: sci-fi-interface-backing.wav para voltar das subtelas.
Slot vazio e comandos durante gameplay nao tocam esses efeitos.
O ou X tambem retornam de falhas de loading com o som de retorno.

WAVs originais em game/assets/audio; ADPCMs prontos em game/res/audio.
Convertidos com FFmpeg (mono 22050 Hz / 16 bits) e adpenc oficial do PS2SDK,
sem loop e sem corte de duracao. Duracoes: 4 s e 0,825 s. Cerca de 60 KiB.

MenuAudioSystem roda apos InputSystem no estagio 0. Volume segue SFX;
0 silencia. tryPlay nao bloqueia a entrada. Buffers em heap alinhados em 64 bytes.
Carregamento unico. Este PS2SDK nao oferece audsrv_free_adpcm. No encerramento
final do jogo, audio.adpcm.reset() para as vozes e libera o banco ADPCM antes
de liberar os buffers EE. O menu e o unico dono de amostras ADPCM no jogo atual;
se outros sistemas passarem a carregar efeitos, centralizar esse encerramento.
O banco nunca e resetado durante navegacao ou transicoes de tela.
Arquivos ausentes ou invalidos deixam o menu funcional e geram log.

Teste host menu_audio_test.cpp usa InputSystem real e backend simulado.
A suite tests/host/run.ps1 passou (13 testes e auditoria dos assets).
Build PS2 e audicao no PCSX2 pendentes: Docker inacessivel nesta sessao.

Navegacao: interface-click.wav, convertido para audio/menu_navigate.adpcm.
Toca uma vez por mudanca real de opcao ou ajuste com D-pad/analogico,
respeitando repeticao do analogico e o volume SFX. Nenhum clique para direcoes
ignoradas ou volumes ja no limite. X/O simultaneos conservam seu som proprio.
