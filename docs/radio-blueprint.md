# Radio blueprint

Quando chega uma mensagem, o radio entra em Waiting. Durante gameplay, o sistema
inicia radio_alert.adpcm e muda para Alert: somente o icone de radio 32 x 34 no
canto superior esquerdo e exibido. Apos o fim do alerta, toca
radio_confirmation.adpcm e muda para Message, mostrando o painel e seu texto.
A interface usa preto, ciano, molduras chanfradas, frequencia e monitor de sinal.
X fecha a mensagem; nao pode dispensar o alerta antes de o painel abrir.
L2/R2 continua chamando o radio quando nao existe transmissao ativa.

Os sons originais estao em game/assets/audio; conversoes ADPCM mono 22050 Hz
sem loop estao em game/res/audio. Regenerar com tools/build_menu_audio.py.
O alerta original dura aproximadamente 2,537 s; confirmacao, 1,356 s.
A duracao de espera vem do tamanho dos blocos APCM (incluindo padding) e do pitch
SPU2, medida com Tyra Timer (15625 ticks/s), sem depender da taxa de quadros.
A confirmacao e o painel comecam juntos; o texto tem o efeito de digitacao.

O mesmo sistema de audio da interface carrega os cinco efeitos uma unica vez e
usa o canal 22 para o radio, sem esperar por canais livres. Volume segue SFX.
Mudo ou falha de arquivo nunca impede acesso a mensagem. Fora do gameplay, nao
abre o painel nem inicia confirmacao. Um alerta em andamento pode terminar ao
abrir o menu; a mensagem aguarda o retorno ao gameplay. Nenhum reset do banco
ocorre nas transicoes: a limpeza continua somente no encerramento do jogo.
Mensagens iniciais do exterior e do interior passam pela mesma sequencia.

Teste tests/host/radio_codec_test.cpp compila os sistemas reais de radio, audio,
input, drawer e fonte contra backend simulado. Verifica o limite de tempo,
ordem dos sons, exclusao mutua de icone/painel, nao repeticao, fechamento,
pausa, mudo, arquivos ausentes, interior e limites dos sprites em 512 x 448.
A previa visual e recomposta a partir das submissoes reais dos sprites C++;
nao e captura do PCSX2. Build PS2 e audicao continuam pendentes porque o
endpoint Docker nao esta acessivel nesta sessao.
