# Execução da otimização PS2 — 2026-10-25

Referência: `PLANO_UNIFICADO_OTIMIZACAO_PS2.md`. O objetivo completo continua em
execução. Nenhum ganho de FPS foi medido e nenhum gate visual foi concluído.

## Etapa 0 — linha de base

- HEAD inicial: `3c87a18`; apenas os dois planos estavam não rastreados.
- Nenhum AGENTS.md foi encontrado na árvore. Nenhuma alteração do usuário foi revertida.
- Os quatro testes host foram recompilados com `g++ -std=c++17 -Wall -Wextra`
  e passaram: clipping, frustum, interior e ciclo de vida do render.
- `python tools/audit_etip_inside.py`: PASS, 4.966 triângulos com quatro portas,
  dez texturas e 163.840 bytes RGBA.
- Executáveis da linha de base: diretório temporário do Windows, sufixo `-baseline.exe`.
- Docker acessível após autorização; compilador PS2 encontrado no container
  `tyra-compiler`. Build em cópia isolada `/tmp/droid-optimization-check`, preservando `/src`.
- Instrumentação opcional, baseline temporal e comparação histórica ainda pendentes.

## Etapa 1 — clipping homogêneo

`planes_clip_algorithm.cpp` calcula seis bits por vértice usando os mesmos
predicados inclusivos anteriores. Um plano externo comum rejeita o triângulo;
todos os vértices internos retornam diretamente. Os demais mantêm os seis planos
de recorte e sua interpolação. Projeção, frustum, guard bands, fullClipChecks,
controles, geometria e efeitos foram preservados.

`tests/host/clipper_reference.cpp` preserva o algoritmo anterior. O teste
`clipper_equivalence_test.cpp` passou em **400.792 comparações exatas**, incluindo
as oito combinações de atributos, os seis limites e seus vizinhos representáveis,
degenerados, w zero/negativo e 50.000 triângulos pseudoaleatórios com semente fixa.
O teste existente de paredes a 0,6 m, near/eye e giro completo também passou após
a mudança. Comandos em `tests/host/README.md`.

O teste comparativo também passou em build host com `-O3`; `git diff --check`
passou. O serviço nativo de Computer Use falhou em duas tentativas de listar
janelas (`Computer Use native pipe is unavailable`, erro 2). Não foi possível
inspecionar ou controlar o PCSX2 por essa ferramenta; nenhuma validação visual
foi realizada nesta execução.

Casos inteiramente internos/externos evitam seis passes. O custo adicional da
classificação nos casos mistos e o efeito no quadro PS2 não foram medidos.
Testes host não comprovam pixels, DMA, GS ou VRAM. Gate visual aberto.

## Achados para as próximas etapas

- Limitar a entrada a `maxVertCount / 3` não garante que a saída caiba no VU1.
  Um triângulo recortado pode produzir até nove vértices de polígono e sete
  triângulos. Implementar divisão da saída conforme a capacidade real.
- QBuffers precisam manter os dados referenciados vivos até a conclusão do DMA.
  Não substituir por um único armazenamento compartilhado.
- O clipper cria vetor por chamada; QBuffers desalocam e realocam a cada cópia.
- `StaticPipeline::render` aloca dados de iluminação incondicionalmente.
- Descartar a bounding box transformada da malha antes de preparar bags,
  mantendo a classificação precisa dos pacotes que continuarem visíveis.
- Névoa muda apenas o campo CPU. Usar a API existente de envio ZBUF exige
  ordenação PATH1/PATH3 e restauração do estado anterior, também para o vidro.
- Descartar vidro invisível antes das duas barreiras; preservá-las quando visível.
- HUD: preservar primeira ocorrência dos caracteres, fallback, tabulação,
  quebras de linha, larguras e ordem ao introduzir lookup e batching.

## Pendências

Etapas 1–5 implementadas com validação parcial. Restam instrumentação, benchmark histórico,
validação visual e temporal por etapa e PS2 físico se disponível. O objetivo de
30 FPS/p95 de 33,3 ms permanece sem evidência. Próxima ação: preparar a medição
temporal e validar o conjunto visualmente, incluindo clipping e DMA, quando o controle do
emulador estiver disponível.

## Build PS2 da etapa 1

Engine e jogo compilados com sucesso (código de saída 0), incluindo programas
VU1 e linkedição do ELF, em cópia temporária isolada. Comando de build dentro
do container: `cd /tmp/droid-optimization-check/game && make build-engine all`.

- Executável: `artifacts/optimization/clipping-step1.elf`.
- Log completo: `artifacts/optimization/clipping-step1-build.log`.
- Recursos usados pela build: cópia de `game/res/` no diretório `game/bin/` da
  cópia temporária. O ELF exportado precisa dos recursos do jogo para execução.
- A compilação não demonstra correção visual, segurança DMA em execução ou FPS.

## Etapa 2 — memória, capacidade e ciclo de vida

- O clipper mantém scratch reutilizável. A saída é copiada em lotes completos
  de triângulos, limitados à capacidade VU1, sem referenciar o scratch pelo DMA.
- Cada QBuffer mantém seus próprios vetores de posições e atributos, com
  capacidade retida ao alternar pacotes por ponteiro, tamanho e atributos.
- A metade enviada ao VIF1 é registrada; antes de entregar um buffer dessa
  mesma metade para escrita, o renderer espera a conclusão da transferência.
  A preparação da outra metade continua permitida. O descarregamento também
  espera VIF1 antes de destruir os buffers.
- Bags de opções, informação, cor, textura e iluminação usam descritores locais.
  Dados direcionais só são construídos quando há iluminação. O estado enviado
  por DMA permanece nos arrays dos meshes, QBuffers ou pacotes próprios.
- Pacotes principais e subdivididos usam vetores separados, preservando os pais.
  Os índices auxiliares e o descritor de textura também deixam de alocar por chamada.
- A API alocadora do empacotador continua disponível para outros chamadores.

O novo teste reproduziu uma entrada de 18 vértices, permitida pelo limite antigo
de um QBuffer de 54 vértices, cuja saída exige dois lotes. Ambos preservam
exatamente posições, UVs, cores e normais da referência. Foram testados saída
vazia, crescimento, atributos opcionais, alinhamento de 16 bytes no host,
independência dos pacotes pai/filho e ownership da API alocadora.

Medição host: **zero alocações em 1.000 ciclos aquecidos de reutilização do
QBuffer**, verificado por contador de `operator new`. Não representa contagem
de alocações de todo o quadro nem ganho de FPS do PS2.

`tests/host/run.ps1 -Compiler C:/msys64/ucrt64/bin/g++.exe` recompilou e executou
os sete testes host com `-O3 -Wall -Wextra`: todos passaram. A auditoria dos
assets também passou. Build PS2 com buffers, pacotes e bags locais passou.
Gate 2 permanece parcial: ainda falta verificar DMA e aparência em execução.

Artefatos da etapa 2: `artifacts/optimization/buffers-step2.elf` e
`artifacts/optimization/buffers-step2-build.log`. A build final terminou com
código 0. Os avisos de redefinição da receita `libtyra.a` já pertencem aos
Makefiles existentes. As convenções LF/CRLF originais de cada arquivo foram
preservadas; a verificação de whitespace passou com `cr-at-eol` para reconhecer
os arquivos CRLF. Todas as mudanças continuam não commitadas.

## Etapa 3 — descarte e localidade espacial

`StaticPipeline::render` testa agora a caixa da malha transformada antes de
preparar materiais, iluminação ou consultar texturas, também no modo preciso.
Objetos parcialmente visíveis mantêm o clipping por pacote. O modo sem culling
da arma continua sem esse teste. `isVisible` reutiliza a mesma política para os
passes que precisarem consultar visibilidade antes de uma barreira.

Durante o carregamento do exterior e das peças opacas do interior, uma função
interna compara Morton e seis ordens por eixos, por material. Só muda a ordem
se diminuir a soma das áreas das caixas sem piorar separadamente pacotes e
subpacotes, usando a capacidade real consultada à pipeline. A primeira tentativa
com Morton sozinho piorou a métrica nos assets reais e foi substituída antes da
entrega. Não houve alteração dos OBJ ou de seus atributos.

Posições, UVs, normais e cores seguem a mesma permutação de triângulos completos.
A ordem só pode mudar com alfa de material e vértices exatamente 128 e textura
RGB ou RGBA totalmente opaca no formato GS. Formatos desconhecidos, paletizados
ou com transparência mantêm a ordem. Névoa e vidro não passam pelo preparo.
As caixas globais permanecem válidas porque o conjunto de vértices não muda;
a ordenação acontece antes de criar caches de pacotes. A limpeza de caches na
troca de cena já existente foi preservada.

O teste novo verificou 4.858 triângulos dos OBJ opacos do interior, preservação
exata de atributos/winding, estabilidade, guardas de opacidade e redução ou
manutenção das caixas em cada material. Soma das áreas AABB dos subpacotes:

| Vértices por subpacote | Antes | Depois | Redução |
| --- | --- | --- | --- |
| 18 | 11.242,2 | 10.729,9 | 4,6% |
| 27 | 12.665,0 | 11.343,1 | 10,4% |
| 36 | 10.480,9 | 8.898,68 | 15,1% |

Essa métrica geométrica não mede FPS, tempo de quadro ou submissões do jogo.
Auditoria PNG somente leitura: nove texturas do interior tinham alfa 255 no
arquivo e o vidro tinha alfa 51. A checagem em runtime usa os pixels decodificados,
sem depender desses nomes ou assumir que futuras versões continuarão opacas.

O teste da pipeline real, com doubles de renderer/malha, passou: descarte antes
dos materiais/luzes, preservação de objetos parciais, transformação atual de
objetos móveis e arma sem culling. Engine e jogo também compilaram para PS2.
Gate 3 permanece parcial: visibilidade, carga adicional no loading e benefício
temporal ainda precisam ser avaliados em execução no PCSX2/PS2.

Regressão final da etapa: nove testes host e auditoria passaram; build PS2
terminou com código 0. Artefatos: `artifacts/optimization/spatial-step3.elf`
(2.487.360 bytes) e `spatial-step3-build.log`. A ordenação usa comparação com
índice original como desempate, preservando a ordem de chaves iguais sem exigir
o buffer temporário de `stable_sort`, que expôs uma limitação de linkedição do
toolchain PS2 durante a primeira tentativa. Não foram alteradas flags globais.

## Etapa 4 — névoa, vidro e estado GS

Os dois passes consultam visibilidade antes das barreiras. Quando visíveis,
usam a sequência `align3D → ZBUF sem escrita → align2D → desenho → align3D →
ZBUF anterior → align2D`. Isso drena PATH1 antes de alterar o estado por PATH3
e espera a conclusão do PATH3 antes da geometria seguinte. A névoa deixa de
modificar apenas o campo CPU; ambos os passes restauram o estado anterior,
inclusive se a escrita de profundidade já estava desabilitada.

O setter GS espera o DMA GIF antes de reutilizar o pacote compartilhado com
`enableZTests`. A checagem de visibilidade preserva as barreiras dos passes
visíveis e evita todas elas para passes invisíveis.

O teste de ciclo de vida passou com a ordem exata das chamadas para névoa e
vidro, masks iniciais 0 e 1, descarte invisível e três ciclos de cenas. A build
PS2 passou. Artefatos: `artifacts/optimization/transparency-step4.elf` e
`transparency-step4-build.log`. Os doubles não comprovam registradores GS,
pixels ou duração das esperas. A correção adiciona sincronização necessária aos
passes visíveis; seu custo deve ser medido separadamente antes de atribuir FPS.

A revisão final removeu o FINISH do pacote ZBUF: a cerca é emitida exclusivamente
por `align2D`, evitando duas conclusões para a mesma transição que poderiam
interferir em uma espera posterior. O contrato do setter foi documentado no
header: drenar PATH1 antes e PATH3 depois. O ELF final da etapa 5 inclui a revisão.

## Etapa 5 — HUD e texto

O lookup de caracteres agora usa tabela direta de 256 entradas com índice
unsigned, mantendo a primeira ocorrência e fallback. A fonte mantém scratch de
até 32 sprites e envia glifos consecutivos em lotes, preservando posições, UVs,
cor, escala, espaçamento, tabulações, quebras de linha e ordem de composição.

`Renderer2D::renderBatch` resolve e agrupa apenas texturas consecutivas iguais,
inclusive IDs de sprites que apontem para a mesma textura. Uma sequência A/A/B/A
permanece nessa ordem. As APIs existentes de desenho unitário foram preservadas.
`RendererCore2D` codifica até 32 retângulos por pacote, reaproveitando as duas
áreas de pacotes, mantendo limites e esperando DMA antes da destruição.

Foram preservadas referências anteriores de fonte e cálculo de retângulos.
Resultados host: 151 comparações exatas de layout, todos os 256 bytes, casos de
fallback, texto longo, espaço, tabulação, escala e cor; 96 glifos passaram de
96 chamadas para três lotes. O teste do renderer verificou mudanças de textura,
entrada vazia e APIs unitárias. O teste do core verificou retângulos exatos para
0/1/31/32/33/64/97 sprites, flips, UVs, cor e ordem, com leitura de pacotes adiada
para simular uso assíncrono. Não é uma validação do encoding GIF do PS2SDK.

Tradeoff de memória: os dois pacotes 2D passam de 16 para 528 quadwords cada,
um acréscimo de 16 KiB, além da tabela de 512 bytes e scratch de 32 sprites por
fonte. Isso reduz envios e configuração de estado repetida, sem afirmar ganho
de quadro. A medição de FPS, a aparência do HUD e DMA/GS real seguem pendentes.

Regressão integrada: `tests/host/run.ps1 -Compiler C:/msys64/ucrt64/bin/g++.exe`
recompilou os 12 testes com `-O3 -Wall -Wextra` e executou a auditoria de assets;
todos passaram. A engine e o jogo PS2 compilaram com código 0 após a revisão de
FINISH. ELF integrado: `artifacts/optimization/hud-step5.elf` (2.492.320 bytes);
log: `hud-step5-build.log`. Os artefatos precisam dos recursos de `game/res/`
para execução. Nenhuma captura visual/temporal foi realizada nesta sessão.

O artefato isolado `transparency-step4.elf` também foi regenerado com a revisão
de FINISH e com os seis arquivos de HUD originais de HEAD, em cópia separada
`/tmp/droid-step4-check`. Isso mantém a comparação da etapa 4 independente do
batching do HUD. `artifacts/optimization/elf-manifest.json` registra tamanho e
SHA-256 dos cinco ELFs exportados. Não houve mudança de controles, scheduler,
resolução ou remoção de cenário/efeitos. Nenhum gate visual está concluído.

## Inicialização no PCSX2 e bloqueio restante

Sem nova build, o ELF integrado foi copiado para `artifacts/optimization/run-integrated/`
junto aos recursos: 70 arquivos, 5.074.347 bytes. O SHA-256 da cópia coincide com
o ELF integrado: `beea1a45fbd91f477e5d312223f7b4664ca6d795c70047205b89e77546d14fe4`.

A ajuda do executável instalado identificou PCSX2 **v2.9.81**, apesar de a pasta
ter nome antigo. O teste usou perfil isolado no projeto, BIOS já instalada,
resolução nativa, ciclos EE padrão, velocidade nominal 1 e cheats/patches
desabilitados. A primeira execução identificou que `-datapath` acrescenta uma
subpasta `PCSX2`; após corrigir o local do INI isolado, a inicialização passou.

O log `artifacts/optimization/pcsx2-smoke-run2.log` confirmou carregamento do ELF,
inicialização da engine, áudio, controle, fonte, interface e arma, chegando a
`DroidCampos-PS2 GameApp initialized` em 18,6641 segundos do log. Esse valor é
tempo de inicialização no log do host, não tempo de quadro ou benchmark. O
`Frame rate: 59.94` do início também é alvo do emulador, não FPS medido do jogo.
Não foi comprovado sequer o primeiro quadro visual do menu.

A instância oculta de teste foi encerrada após a verificação. A configuração
pessoal e os memory cards do usuário não foram usados para escrita. A sintaxe
do launcher `tools/run-optimized.ps1` foi validada pelo parser do PowerShell.
`./tools/run-optimized.ps1 -ShowWindow` abre o mesmo ELF com recursos, perfil
isolado e log próprio, sem Docker ou compilação.

O controle de janelas voltou a falhar com `Computer Use native pipe is unavailable`
(erro 2), condição já repetida nas sessões anteriores. A parte restante exige
controle interativo ou validação pelo usuário: pixels, rotas exteriores/interiores,
loading, transparências, DMA/GS em execução e medições comparativas/históricas.
O objetivo completo permanece não concluído e está bloqueado nessa validação.
Próxima ação concreta: seguir [validacao-pcsx2.md](validacao-pcsx2.md) com o serviço
de controle restaurado ou com execução interativa pelo usuário. Nenhuma nova
build é necessária para iniciar esses testes; a instrumentação opcional não foi implementada.
