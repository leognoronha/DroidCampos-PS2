#include <tyra>
#include "core/game_app.hpp"

int main() {
  Tyra::EngineOptions options;
  options.loadUsbDriver = true;
  Tyra::Engine engine(options);
  DroidCampos::Core::GameApp game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}
