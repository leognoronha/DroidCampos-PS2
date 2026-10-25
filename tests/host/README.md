# Testes host do interior

O header local `tyra` é um double mínimo. Os testes compilam os sistemas reais
do jogo sem PS2SDK; não substituem build PS2 ou testes no emulador.

```sh
g++ -std=c++17 -Wall -Wextra -Itests/host -Igame/inc tests/host/interior_test.cpp game/src/systems/input/input_system.cpp game/src/systems/player/player_controller_system.cpp game/src/systems/physics/collision_system.cpp game/src/hardware/ps2_scratchpad.cpp game/src/events/event_bus.cpp -o /tmp/etip-input-test
/tmp/etip-input-test
g++ -std=c++17 -Wall -Wextra -Itests/host -Igame/inc tests/host/render_lifecycle_test.cpp game/src/systems/render3d/world_render_system.cpp game/src/events/event_bus.cpp game/src/hardware/ps2_scratchpad.cpp -o /tmp/etip-render-test
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
