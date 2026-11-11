# Interior ETIP

## Entregas e organização

Os modelos ficam em `../DroidCampos-models/`: `ETIP-inside.blend` compõe o cenário;
`ETIP-inside-{estrutura,portas,salas,terminal,capsula}.blend` contêm os módulos separados.
O exterior original não foi alterado. A imagem `ETIP-inside-preview.png` é uma
prévia do Blender; a intensidade final no jogo ainda precisa de avaliação no PCSX2.

O corredor mede 24 × 3 × 2,8 m. Quatro salas laterais de aproximadamente 4 × 4 m
ficam a 8 e 16 m da entrada. O corredor termina numa sala de 10 × 8 × 6,25 m,
entre Y=24 e Y=32 no Blender. O terminal é visual, na parede esquerda, voltado
para o centro. A cápsula de 5,63 m fica junto à parede direita, centrada em
`(3.58,29,0.012)` no Blender, com a janela voltada ao centro. Há 8 cm de folga
da parede, 60,8 cm do teto e 12 mm do piso; nada atravessa a estrutura da sala.
A circulação central permanece livre, com colisão no terminal e na cápsula.
As quatro portas começam fechadas e deslizam 1,55 m para dentro da parede.

`game/res/etip_inside/` contém três trechos de corredor, quatro salas, sala final, terminal, cápsula opaca, vidro e
um painel de porta em coordenadas locais. A mesma malha e as mesmas texturas são
reutilizadas nos quatro painéis. OBJ triangulado, UVs positivas, transformação
Blender → Tyra `(X,Z,-Y)`, PNG RGBA 8-bit, prefixos `PS2_*` preservados.

Orçamento auditado do interior: **7.266 triângulos**, com 12 PNGs: dez de 64 × 64, corpo de 256 × 256 e cabeça de 128 × 128. São **480 KiB** de pixels RGBA decodificados, sem metadados, alinhamento GS, buffers ou inimigos.
O Blender mantém a cápsula detalhada original de 5.073 triângulos; a composição
visível tem 11.517 triângulos, incluindo o DroidCampos estático de 2.300 triângulos. A coleção oculta `Inside_Capsula_Runtime_LOD`
contém a versão ocupada de exportação de 3.122 triângulos (774 da estrutura, 2.300 do personagem e 48 do vidro).
Ela não é desenhada junto com o modelo detalhado na prévia. O original
`ETIP-capsule.blend` não é sobrescrito pelo gerador.
Sombras de borda e detalhes luminosos estão nas texturas; o jogo também aplica
tonalidade escura aos materiais não luminosos. Não depende de luzes do Blender.

## Comportamento e ciclo de recursos

- Quadrado perto do leitor, com rádio fechado, arma e cartão coletados,
  dispara a abertura. Ambos os itens ficam no chão junto à guarita.
  O LED verde aparece por 20 frames; movimento e novas ações ficam suspensos.
- A vida e o inventário seguem para o interior. Há um kit médico no fundo
  do pátio e outro na primeira sala lateral esquerda. Quadrado recolhe o kit.
  R2 restaura até 40 pontos e o jogador pode carregar dois kits.
- `GameApp` apresenta o loading, sincroniza VIF/GIF/GS e libera cenário e névoa
  antes de carregar o destino. Uma parte é carregada por etapa entre frames.
  O parser OBJ continua síncrono: o indicador pode pausar durante uma etapa.
- UI e pistola permanecem carregadas. O interior possui um proprietário por PNG
  e múltiplos vínculos de material; descarregar libera cada textura uma vez.
- A cápsula é carregada em duas etapas, usando o mesmo loader de cenas:
  `capsula.obj` (estrutura + DroidCampos estático) e `capsula_vidro.obj`. O vidro é desenhado após os sólidos, com
  alpha e teste de profundidade, sem gravar Z. A engine agora envia a alteração
  de máscara ao registro GS via `setZBufferWriteEnabled`, restaurando-a antes
  de desenhar a pistola. As cores Kd dos materiais da cápsula são preservadas.
- Spawn interno `(0,1.7,-1.8)`, olhando para `-Z`. Não há transição de retorno.
  L3/R3 reinicia na cena atual; nova partida pede o exterior novamente.
- Quadrado alterna a porta próxima. Colisão acompanha o painel; fechar reverte
  para abrir se o jogador ocupa a passagem. Rádio e loading impedem interação.
- Antes da descarga são verificados OBJ, MTL e PNGs referenciados. Ausência gera
  tela de erro e X volta ao menu. Corrupção interna do arquivo ainda pode atingir
  as assertions/traps do carregador Tyra existente; ele não foi substituído.

As definições de portas e nomes de partes estão em `game/inc/core/interior_layout.hpp`.
As colisões são AABBs em `CollisionSystem`, reaproveitando o resolvedor existente.

## Regeneração e auditoria

Execute a partir da raiz do repositório:

```text
blender --background --threads 2 --python tools/build_etip_inside.py
python tools/audit_etip_inside.py
```

O gerador aceita `-- --models CAMINHO --resources CAMINHO` e `--skip-previews`, e usa a matriz externa
como origem dos materiais de ferrugem e advertência e `ETIP-capsule.blend` como
origem da estrutura. O ocupante vem de `DroidCampos/DroidCampos-v02.blend`, coleção `DroidCampos_PS2_v02`, preservando os ajustes manuais das mãos e pelve. A estrutura leve usa geometria paramétrica controlada, com cilindros de 12 lados e suportes retos. A auditoria limita a estrutura do interior a 5.000 triângulos, o ocupante a 2.000–2.500, o conjunto a 7.500 e os pixels das texturas do interior a 512 KiB.
Sua posição e envelope são auditados para preservar as folgas; uma redução por
colapso global foi descartada após a inspeção visual detectar deformações.
Regenerar sobrescreve apenas
as entregas do interior; preserve edições manuais antes de executar novamente.

## Verificação realizada e pendências

### Correção de desaparecimento e flicker — 25/10/2026

O guard band fixo de 10–25 unidades em `RenderBBox::clipFrustumCheck` podia
classificar polígonos que cruzam a câmera como totalmente internos em um cenário
modelado em metros. O programa VU1 de cull elimina esses triângulos inteiros.
O recortador CPU também comparava Z pós-MVP a uma distância de mundo de cerca
de 10 m, descartando geometria próxima quando esse caminho era selecionado.

Agora os pacotes parciais continuam no clipping CPU; near/far são recortados
em coordenadas homogêneas (`z<=w`, `z>=-w`), como exige a projeção da Tyra.
O cache de bounding boxes é limpo ao descarregar a cena para impedir colisões
de chaves quando o alocador reutiliza endereços de vértices do cenário anterior.

O gerador subdivide faces em painéis com lados de até 2 m, preserva UVs e evita
paredes coplanares duplicadas entre corredor e salas. Soleiras ficam 12 mm acima
do piso, eliminando a sobreposição visível que pode provocar z-fighting.

Passaram testes do recortador real com paredes a 0,6 m, triângulos cruzando near,
vértices atrás da câmera e 360 ângulos, além da política de frustum, circulação
na sala final, colisão do terminal e ciclo de recursos. Ainda requer validação
visual e de desempenho no PCSX2 com a engine recompilada.

Passaram a auditoria de OBJ/MTL/PNG, a conferência visual da prévia Blender e os
testes C++ host com o código real de input/colisão e ciclo de recursos. Os doubles
em `tests/host/tyra` simulam controle, render e repositório de texturas; não simulam
DMA, VU1, GS, PCSX2 nem RAM real do PS2.

O usuário compilou e testou a versão anterior no PCSX2 e reportou o problema
visual. Esta revisão ainda não foi compilada para PS2: o acesso automatizado ao
pipe Docker continua negado pelo sandbox.

Para compilar: executar `tools/build-etip.ps1`, que usa `make build-engine all`,
e copiar `game/bin` atualizado para o host. Testar leitor → loading → interior;
visitar quatro salas, testar fechamento ocupado, rádio, reset e nova partida.
Confirmar a intensidade das texturas no render real e repetir novas partidas.

Os logs `[ETIP MEMORY]` registram RAM disponível e quantidade de texturas antes e
depois da descarga e após cada parte. Eles são amostras, não uma medição do pico
temporário dentro do parser/decoder. O orçamento total de 32 MB e o uso efetivo
de VRAM ainda precisam ser confirmados em execução, idealmente no console.


### Cápsula ocupada — 31/10/2026

A exportação substitui a cápsula vazia no mesmo `capsula.obj`, sem alterar as etapas de carregamento, colisão externa ou posição. O personagem recebe a mesma transformação da cápsula, com a frente voltada ao centro da sala. Corpo e cabeça usam dois materiais próprios dentro da malha opaca, reutilizando a política de tonalidade dos materiais de cápsula. A geometria local do personagem é conferida contra raio de 1 m e altura entre Z=1,224 e 4,69 m antes da colocação. O vidro permanece separado e não foi alterado.

Os arquivos atualizados também foram copiados para `game/bin/etip_inside`, com hashes iguais aos de `game/res/etip_inside`. O ELF existente pode carregar os novos assets porque não houve mudança no código de runtime. A tentativa de recompilação em 31/10 não concluiu: o pipe `dockerDesktopLinuxEngine` estava indisponível. A contagem adicional de geometria/texturas requer avaliação de desempenho e memória no PCSX2/PS2; os limites de exportação não são garantia de taxa de quadros.

Validação desta exportação: suíte completa de 20 casos host e auditoria de assets aprovadas; três ciclos de carga/descarga preservam as 12 texturas de cena, dois atlas de inimigos e a pistola persistente. A prévia Blender da malha runtime com o vidro foi inspecionada. Não foi feita validação visual desta revisão no PCSX2.
