# Interior ETIP

## Entregas e organização

Os modelos ficam em `../DroidCampos-models/`: `ETIP-inside.blend` compõe o cenário;
`ETIP-inside-{estrutura,portas,salas,terminal}.blend` contêm os módulos separados.
O exterior original não foi alterado. A imagem `ETIP-inside-preview.png` é uma
prévia do Blender; a intensidade final no jogo ainda precisa de avaliação no PCSX2.

O corredor mede 24 × 3 × 2,8 m. Quatro salas laterais de aproximadamente 4 × 4 m
ficam a 8 e 16 m da entrada. O corredor termina numa sala de 10 × 8 × 3,4 m,
entre Y=24 e Y=32 no Blender. O terminal é visual, na parede esquerda, voltado
para o centro. A sala permite circulação livre, exceto no volume do terminal.
As quatro portas começam fechadas e deslizam 1,55 m para dentro da parede.

`game/res/etip_inside/` contém três trechos de corredor, quatro salas, sala final, terminal e
um painel de porta em coordenadas locais. A mesma malha e as mesmas texturas são
reutilizadas nos quatro painéis. OBJ triangulado, UVs positivas, transformação
Blender → Tyra `(X,Z,-Y)`, PNG RGBA 8-bit, prefixos `PS2_*` preservados.

Orçamento auditado: **3.948 triângulos renderizados**, oito PNGs de 64 × 64,
128 KiB de pixels RGBA decodificados (sem metadados e buffers da engine).
Sombras de borda e detalhes luminosos estão nas texturas; o jogo também aplica
tonalidade escura aos materiais não luminosos. Não depende de luzes do Blender.

## Comportamento e ciclo de recursos

- Quadrado perto do leitor, com rádio fechado, dispara o evento existente.
  O LED verde aparece por 20 frames; movimento e novas ações ficam suspensos.
- `GameApp` apresenta o loading, sincroniza VIF/GIF/GS e libera cenário e névoa
  antes de carregar o destino. Uma parte é carregada por etapa entre frames.
  O parser OBJ continua síncrono: o indicador pode pausar durante uma etapa.
- UI e pistola permanecem carregadas. O interior possui um proprietário por PNG
  e múltiplos vínculos de material; descarregar libera cada textura uma vez.
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

O gerador aceita `-- --models CAMINHO --resources CAMINHO` e usa a matriz externa
como origem dos materiais de ferrugem e advertência. Regenerar sobrescreve apenas
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
