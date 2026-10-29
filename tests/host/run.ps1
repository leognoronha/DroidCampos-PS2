param([string]$Compiler = 'g++')
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
Push-Location $repo
try {
  $compilerPath = (Get-Command $Compiler).Source
  $env:PATH = (Split-Path $compilerPath) + ';' + $env:PATH
  $engine = @('-Itests/host/engine_stubs', '-Iengine/inc')
  $game = @('-Itests/host', '-Igame/inc')
  $algorithm = 'engine/src/renderer/core/3d/clipper/planes_clip_algorithm.cpp'
  $core = 'engine/src/renderer/3d/pipeline/static/core'
  $cases = @(
    @{Name='enemy-test'; Args=$game+@('tests/host/enemy_test.cpp', 'game/src/systems/enemy/enemy_test_system.cpp', 'game/src/events/event_bus.cpp')},
    @{Name='weapon-hud'; Args=$game+@('tests/host/weapon_hud_test.cpp', 'game/src/systems/ui/hud_system.cpp', 'game/src/systems/ui/ui_context_drawer.cpp', 'game/src/systems/ui/font_sprite.cpp', 'game/src/events/event_bus.cpp', 'game/src/hardware/ps2_scratchpad.cpp')},
    @{Name='weapon'; Args=$game+@('tests/host/weapon_test.cpp', 'game/src/systems/weapon/weapon_system.cpp', 'game/src/systems/weapon/weapon_geometry.cpp', 'game/src/systems/render3d/world_render_system.cpp', 'game/src/systems/audio/menu_audio_system.cpp', 'game/src/systems/narrative/radio_codec_system.cpp', 'game/src/systems/ui/ui_context_drawer.cpp', 'game/src/systems/ui/font_sprite.cpp', 'game/src/systems/input/input_system.cpp', 'game/src/events/event_bus.cpp', 'game/src/hardware/ps2_scratchpad.cpp')},
    @{Name='scene-intro'; Args=$game+@('tests/host/scene_intro_test.cpp', 'game/src/systems/ui/scene_intro_system.cpp', 'game/src/systems/ui/hud_system.cpp', 'game/src/systems/audio/menu_audio_system.cpp', 'game/src/systems/narrative/radio_codec_system.cpp', 'game/src/systems/ui/ui_context_drawer.cpp', 'game/src/systems/ui/font_sprite.cpp', 'game/src/systems/input/input_system.cpp', 'game/src/events/event_bus.cpp', 'game/src/hardware/ps2_scratchpad.cpp')},
    @{Name='radio'; Args=$game+@('tests/host/radio_codec_test.cpp', 'game/src/systems/audio/menu_audio_system.cpp', 'game/src/systems/narrative/radio_codec_system.cpp', 'game/src/systems/ui/ui_context_drawer.cpp', 'game/src/systems/ui/font_sprite.cpp', 'game/src/systems/input/input_system.cpp', 'game/src/events/event_bus.cpp', 'game/src/hardware/ps2_scratchpad.cpp')},
    @{Name='menu-audio'; Args=$game+@('tests/host/menu_audio_test.cpp', 'game/src/systems/audio/menu_audio_system.cpp', 'game/src/systems/input/input_system.cpp', 'game/src/events/event_bus.cpp', 'game/src/hardware/ps2_scratchpad.cpp')},
    @{Name='font'; Args=@('-Itests/host/font_stubs', '-Igame/inc', 'tests/host/font_batch_test.cpp', 'tests/host/font_reference.cpp', 'game/src/systems/ui/font_sprite.cpp')},
    @{Name='sprite-groups'; Args=@('-Itests/host/batch_stubs', 'tests/host/renderer_batch_test.cpp', 'engine/src/renderer/2d/renderer_2d.cpp')},
    @{Name='sprite-packets'; Args=@('-Itests/host/sprite_stubs', 'tests/host/sprite_packet_test.cpp', 'tests/host/renderer_core_2d_reference.cpp', 'engine/src/renderer/core/2d/renderer_core_2d.cpp')},
    @{Name='spatial'; Args=@('-Igame/inc', 'tests/host/spatial_order_test.cpp')},
    @{Name='pipeline'; Args=@('-Itests/host/pipeline_stubs', '-Itests/host/engine_stubs', 'tests/host/pipeline_culling_test.cpp', 'engine/src/renderer/3d/pipeline/static/static_pipeline.cpp')},
    @{Name='clipper'; Args=$engine+@('tests/host/clipper_test.cpp', $algorithm)},
    @{Name='frustum'; Args=@('-Itests/host/engine_stubs', 'tests/host/frustum_policy_test.cpp', 'engine/src/renderer/core/3d/bbox/render_bbox.cpp')},
    @{Name='equivalence'; Args=$engine+@('tests/host/clipper_equivalence_test.cpp', 'tests/host/clipper_reference.cpp', $algorithm)},
    @{Name='buffers'; Args=$engine+@('tests/host/clipper_buffer_test.cpp', 'tests/host/clipper_reference.cpp', $algorithm, "$core/stapip_clipper.cpp", "$core/stapip_qbuffer.cpp")},
    @{Name='packages'; Args=$engine+@('tests/host/packager_reuse_test.cpp', "$core/bag/packaging/stapip_bag_packager.cpp")},
    @{Name='interior'; Args=$game+@('tests/host/interior_test.cpp', 'game/src/systems/input/input_system.cpp', 'game/src/systems/player/player_controller_system.cpp', 'game/src/systems/physics/collision_system.cpp', 'game/src/hardware/ps2_scratchpad.cpp', 'game/src/events/event_bus.cpp')},
    @{Name='render'; Args=$game+@('tests/host/render_lifecycle_test.cpp', 'game/src/systems/render3d/world_render_system.cpp', 'game/src/systems/weapon/weapon_geometry.cpp', 'game/src/events/event_bus.cpp', 'game/src/hardware/ps2_scratchpad.cpp')}
  )
  foreach ($case in $cases) {
    if ($case.Args -contains 'game/src/systems/render3d/world_render_system.cpp') {
      $case.Args += 'game/src/systems/enemy/enemy_test_system.cpp'
    }
    $output = Join-Path $env:TEMP ('droid-optimization-' + $case.Name + '.exe')
    $compilerArgs = @('-std=c++17', '-O3', '-Wall', '-Wextra') + $case.Args + @('-o', $output)
    & $compilerPath @compilerArgs
    if ($LASTEXITCODE -ne 0) {throw "Compilation failed: $($case.Name)"}
    & $output 'game/res/'
    if ($LASTEXITCODE -ne 0) {throw "Test failed: $($case.Name)"}
  }
  & python tools/audit_etip_inside.py
  if ($LASTEXITCODE -ne 0) {throw 'Asset audit failed'}
} finally {
  Pop-Location
}
