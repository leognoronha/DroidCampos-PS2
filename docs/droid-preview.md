# DroidCampos animado na sala ETIP

O interior agora carrega uma demonstra√ß√£o autom√°tica adicional na sala do terminal.
O modelo da c√°psula continua sendo parte do cen√°rio. O personagem m√≥vel fica na
√°rea central, entre X=-2 e 0 e Z=-30 e -26, longe da c√°psula e do terminal.

Ele percorre quatro trechos de 2 m a 0,5 m/s. Em cada esquina para durante 3 s e
executa DC_Direto_Carregado_Horizontal (soco com amplitude, v07). O ciclo completo
dura 28 s. Caminhada: DC_Caminhada_Pesada, ciclo de 2 s. A reprodu√ß√£o √© interpolada,
com 12 poses de caminhada e 24 de soco. A escala de apresenta√ß√£o √© 1,0 (cerca de
3,27 m de altura), igual ao ocupante da c√°psula (sem a eleva√ß√£o da plataforma). √â uma demonstra√ß√£o visual, sem dano, alvo ou colis√£o de personagem.
Pausa, r√°dio, terminal e introdu√ß√£o congelam a reprodu√ß√£o. Sair da sala descarrega
malha, banco de anima√ß√µes e texturas; uma nova entrada reinicia o percurso.

Fonte: ../DroidCampos-models/DroidCampos/animacoes/DroidCampos-animacoes-v07.blend.
As imagens e v√≠deos em previas s√£o refer√™ncias; as a√ß√µes s√£o exportadas do rig.
O exportador n√£o sobrescreve o arquivo Blender de origem.

Regenerar recursos:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' -b -t 2 --python tools/export_droid_preview.py
```

Compilar e testar:

```powershell
./tests/host/run.ps1
./tools/build-etip.ps1
./tools/run-combat.ps1 -ShowWindow
```

Na sala final, observe o personagem caminhando e dando socos automaticamente.
O jogador mant√©m os controles de primeira pessoa habituais para circular e observar.

Recursos adicionais: 2.300 tri√¢ngulos, banco de posi√ß√µes de 2.980.800 bytes,
duas texturas (256x256 e 128x128, 320 KiB RGBA decodificados). Custos de malha,
carregamento, alinhamento GS e metadados n√£o est√£o inclu√≠dos nessa soma.
√â necess√°rio medir desempenho e mem√≥ria no PCSX2/PS2 com os inimigos ativos.

Valida√ß√£o nesta altera√ß√£o: exporta√ß√£o do Blender e auditoria do cen√°rio executadas;
compila√ß√£o nativa e ciclo de carga/descarga verificados pela su√≠te host.
Compila√ß√£o PS2 e inspe√ß√£o visual no PCSX2 pendentes: acesso ao Docker local negado
pelo ambiente desta conversa. O ELF anterior n√£o cont√©m esta demonstra√ß√£o.

## CorreÁ„o da renderizaÁ„o e proporÁ„o

O carregador OBJ triangulava novamente faces j· trianguladas com Earcut, que
reordenava seus cantos. A malha est·tica e seus UVs eram reordenados juntos, mas
as poses do banco mantinham a ordem original. Ao atualizar apenas as posiÁıes,
a textura passava a usar cantos diferentes (5.736 de 6.900 no teste real).

DroidPreview ativa ObjLoaderOptions::preserveTriangleOrder. Isso preserva a
ordem de importaÁ„o e a correspondÍncia posiÁ„o/UV em todas as poses. O padr„o
do carregador continua igual para os demais modelos.

A escala 0,65 foi removida. A malha animada usa as mesmas dimensıes locais da
vers„o da c·psula: aproximadamente 3,265 m dos pÈs ‡ cabeÁa, com pequenas
variaÁıes de altura durante a caminhada. O modelo da c·psula est· elevado
1,225 m pela plataforma, o que n„o faz parte da altura corporal.

Regressıes: obj_animation_order_test compila o carregador OBJ real e verifica
os 6.900 pares posiÁ„o/UV; droid_preview_test verifica a escala integral do banco,
caminhada, soco parado, pausa e recarga. Os testes usam dependÍncias de plataforma
simuladas; n„o substituem a inspeÁ„o visual no PCSX2.

ApÛs esta correÁ„o, reconstruir **engine e jogo** com tools/build-etip.ps1.
Os recursos de animaÁ„o existentes continuam v·lidos. O ELF antigo n„o aplica
a correÁ„o do carregador nem a nova escala.
