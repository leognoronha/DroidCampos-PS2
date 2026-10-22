#include <tyra>
#include "droid_campos_game.hpp"

int main() {
  Tyra::Engine engine;
  DroidCampos::DroidCamposGame game(&engine);
  engine.run(&game);
  SleepThread();
  return 0;
}
