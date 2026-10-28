# Testes host do interior

O header local `tyra` é um double mínimo. Os testes compilam os sistemas reais
do jogo sem PS2SDK; não substituem build PS2 ou testes no emulador.

```sh
g++ -std=c++17 -Wall -Wextra -Itests/host -Igame/inc tests/host/interior_test.cpp game/src/systems/input/input_system.cpp game/src/systems/player/player_controller_system.cpp game/src/systems/physics/collision_system.cpp game/src/hardware/ps2_scratchpad.cpp game/src/events/event_bus.cpp -o /tmp/etip-input-test
/tmp/etip-input-test
g++ -std=c++17 -Wall -Wextra -Itests/host -Igame/inc tests/host/render_lifecycle_test.cpp game/src/systems/render3d/world_render_system.cpp game/src/systems/weapon/weapon_geometry.cpp game/src/events/event_bus.cpp game/src/hardware/ps2_scratchpad.cpp -o /tmp/etip-render-test
/tmp/etip-render-test "$PWD/game/res/"
```

No Windows, usar caminhos de saída `.exe` e o diretório `bin` do compilador
no PATH. Os testes só leem os assets. O teste de render verifica ownership,
vínculos e contagem de texturas com um repositório simulado, não pixels ou VRAM.

Regressões de clipping (compilam os arquivos reais da engine com doubles apenas
dos tipos matemáticos, settings e classificação de frustum):

```sh
g++ -std=c++17 -Wall -Wextra -Itests/host/engine_stubs -Iengine/inc tests/host/clipper_test.cpp engine/src/renderer/core/3d/clipper/planes_clip_algorithm.cpp -o /tmp/etip-clip-test
/tmp/etip-clip-test
g++ -std=c++17 -Wall -Wextra -Itests/host/engine_stubs tests/host/frustum_policy_test.cpp engine/src/renderer/core/3d/bbox/render_bbox.cpp -o /tmp/etip-frustum-test
/tmp/etip-frustum-test
```

## Equivalence of optimized clipping

The frozen implementation in `clipper_reference.cpp` is compared against the
optimized algorithm in 400,792 cases, including all attribute flags, six plane
boundaries, degenerates and zero/negative w. This does not validate FPS or DMA/GS.

```sh
g++ -std=c++17 -Wall -Wextra -Itests/host/engine_stubs -Iengine/inc tests/host/clipper_equivalence_test.cpp tests/host/clipper_reference.cpp engine/src/renderer/core/3d/clipper/planes_clip_algorithm.cpp -o /tmp/clip-equivalence-test
/tmp/clip-equivalence-test
```

## Buffers and package reuse

`clipper_buffer_test.cpp` compiles the real clipper and QBuffer implementation.
It reproduces output larger than one VU1 batch, compares all attributes with the
frozen reference, checks alignment and measures zero allocations in 1,000 warmed
reuse cycles. `packager_reuse_test.cpp` checks independent parent/child storage,
growth, disabled attributes and ownership of the legacy allocating API.
These tests use math and bag doubles; they do not prove real DMA synchronization.

Run all twelve host tests and the asset audit on Windows:

```powershell
./tests/host/run.ps1 -Compiler C:/msys64/ucrt64/bin/g++.exe
```

The spatial test compares attribute permutations and package AABB surface areas
on real interior OBJ assets for three capacities. Candidates are accepted only
when neither package nor subpackage area increases. It does not measure FPS.
The culling test compiles the actual `static_pipeline.cpp` against controlled
renderer/mesh doubles: it checks rejection before texture lookup/light setup,
partial visibility, current model transforms, and disabled weapon culling.

Font batching is compared against a frozen reference in 151 layout cases,
including all byte values and fallback. The real high-level 2D renderer is
tested for A/A-alias/B/A texture grouping. The real core 2D renderer is compared
against its frozen rectangle calculations, including 31/32/33-sprite boundaries.
Packet/DMA doubles simulate deferred reads and ownership; they do not validate
PS2SDK GIF encoding, pixels, real DMA timing or GS registers.

## Arma e HUD

`weapon_test.cpp` testa os sistemas reais de input, arma, radio e audio e as
intersecoes com os OBJ reais de ambos os cenarios. O double de ObjLoader agora
le vertices/faces e mantem sua propriedade compartilhada durante a vida dos meshes.
`weapon_hud_test.cpp` verifica limites, oito indicadores e ausencia do painel com
radio/terminal/introducao; um segundo argumento opcional exporta as chamadas de
sprites para uma pasta existente, para verificacao visual.
Os dois casos estao incluidos em `run.ps1`.
