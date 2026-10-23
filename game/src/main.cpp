#include <tyra>
#include "core/game_app.hpp"

int main() {
  Tyra::Engine engine;
  DroidCampos::Core::GameApp game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}
