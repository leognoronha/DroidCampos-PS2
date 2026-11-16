/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022-2022, tyra - https://github.com/h4570/tyrav2
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
# Wellington Carvalho <wellcoj@gmail.com>
*/

#include <tamtypes.h>
#include <loadfile.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>
#include "debug/debug.hpp"
#include "pad/pad.hpp"

namespace Tyra {

/** Init vars, load modules, opens pad port and initializes pad */
Pad::Pad() {}

Pad::~Pad() {}

// ----
// Methods
// ----

void Pad::init() {
  this->oldPad = 0;
  validButtons_ = false;
  clearAnalog();
  padInit(0);
  this->port = 0;  // 0 -> Connector 1, 1 -> Connector 2
  this->slot = 0;  // Always zero if not using multitap

  this->ret = padPortOpen(this->port, this->slot, padBuf);
  TYRA_ASSERT(this->ret != 0,
              "padPortOpen failed! padPortOpen returned: ", this->ret);
  TYRA_ASSERT(this->initPad(), "initPad failed!");
}

/** Wait when pad will be ready (stable and ready) */
int Pad::waitPadReady() {
  int state;
  int lastState;
  char* stateString = new char[16];
  state = padGetState(this->port, this->slot);
  lastState = -1;
  while ((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
    if (state != lastState) {
      padStateInt2String(state, stateString);
      TYRA_LOG("Pad state changed");
      printf("Curent pad(%d,%d) status: %s\n", this->port, this->slot,
             stateString);
    }
    lastState = state;
    state = padGetState(this->port, this->slot);
  }
  // Were the pad ever 'out of sync'?
  if (lastState != -1) TYRA_LOG("Pad is ready!");

  delete[] stateString;
  return 0;
}

/** Initializes and checks type of pad */
int Pad::initPad() {
  TYRA_LOG("Initializing pad");
  this->waitPadReady();

  int modes = padInfoMode(this->port, this->slot, PAD_MODETABLE, -1);
  TYRA_ASSERT(modes, "Connected device is not a dual shock controller!");

  // Verify that the controller has a DUAL SHOCK mode
  int i = 0;
  do {
    if (padInfoMode(this->port, this->slot, PAD_MODETABLE, i) ==
        PAD_TYPE_DUALSHOCK)
      break;
    i++;
  } while (i < modes);

  TYRA_ASSERT(i < modes, "Connected device is not a dual shock controller!");

  // If ExId != 0x0 => This controller has actuator engines
  // This check should always pass if the Dual Shock test above passed
  this->ret = padInfoMode(this->port, this->slot, PAD_MODECUREXID, 0);
  TYRA_ASSERT(this->ret, "Connected device is not a dual shock controller!");

  TYRA_LOG("Enabling dual shock functions.");

  // When using MMODE_LOCK, user cant change mode with Select button
  padSetMainMode(this->port, this->slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);

  this->waitPadReady();
  TYRA_LOG("Pad has pressure sensitive buttons? ",
           padInfoPressMode(this->port, this->slot));
  this->waitPadReady();
  padEnterPressMode(this->port, this->slot);  // Set pressure sensitive mode
  this->waitPadReady();
  this->actuators = padInfoAct(this->port, this->slot, -1, 0);
  TYRA_LOG("# of actuators: ", this->actuators);
  if (actuators != 0) {
    this->actAlign[0] = 0;  // Enable small engine
    this->actAlign[1] = 1;  // Enable big engine
    this->actAlign[2] = 0xff;
    this->actAlign[3] = 0xff;
    this->actAlign[4] = 0xff;
    this->actAlign[5] = 0xff;
    this->waitPadReady();
    TYRA_LOG("padSetActAlign: ",
             padSetActAlign(this->port, this->slot, actAlign));
  } else
    TYRA_LOG("Did not find any actuators.");
  this->waitPadReady();
  TYRA_LOG("Pad initialized!");
  return 1;
}

/** Updates state of joys/buttons. Called by engine */
void Pad::update() {
  clicked = {};
  pressed = {};
  buttons = {};
  // A failed poll or a controller negotiating its mode must not reuse axes.
  const int bytes = padGetState(port, slot) == PAD_STATE_STABLE
      ? padRead(port, slot, &buttons) : 0;
  if (!isCompletePadPacket(bytes, buttons.ok, buttons.mode)) {
    validButtons_ = false;
    clearAnalog();
    return;
  }

  padData = 0xffff ^ buttons.btns;
  newPad = validButtons_ ? padData & ~oldPad : 0;
  oldPad = padData;
  validButtons_ = true;
  decodeButtons(clicked, newPad);
  decodeButtons(pressed, padData);

  if (!isAnalogPadMode(buttons.mode)) {
    clearAnalog();
    return;
  }
  const auto updateStick = [](PadJoy& stick, AnalogAxis& horizontal,
                              AnalogAxis& vertical, u8 h, u8 v) {
    stick.h = horizontal.update(h);
    stick.v = vertical.update(v);
    stick.isCentered = stick.h == AnalogAxis::kCenter && stick.v == AnalogAxis::kCenter;
    stick.isMoved = !stick.isCentered;
  };
  updateStick(leftJoyPad, leftHorizontal_, leftVertical_, buttons.ljoy_h, buttons.ljoy_v);
  updateStick(rightJoyPad, rightHorizontal_, rightVertical_, buttons.rjoy_h, buttons.rjoy_v);
}

void Pad::decodeButtons(PadButtons& output, u32 mask) {
  output.Cross = (mask & PAD_CROSS) != 0;
  output.Square = (mask & PAD_SQUARE) != 0;
  output.Triangle = (mask & PAD_TRIANGLE) != 0;
  output.Circle = (mask & PAD_CIRCLE) != 0;
  output.DpadUp = (mask & PAD_UP) != 0;
  output.DpadDown = (mask & PAD_DOWN) != 0;
  output.DpadLeft = (mask & PAD_LEFT) != 0;
  output.DpadRight = (mask & PAD_RIGHT) != 0;
  output.L1 = (mask & PAD_L1) != 0;
  output.L2 = (mask & PAD_L2) != 0;
  output.L3 = (mask & PAD_L3) != 0;
  output.R1 = (mask & PAD_R1) != 0;
  output.R2 = (mask & PAD_R2) != 0;
  output.R3 = (mask & PAD_R3) != 0;
  output.Start = (mask & PAD_START) != 0;
  output.Select = (mask & PAD_SELECT) != 0;
}

void Pad::clearAnalog() {
  leftHorizontal_.reset();
  leftVertical_.reset();
  rightHorizontal_.reset();
  rightVertical_.reset();
  leftJoyPad = rightJoyPad = {AnalogAxis::kCenter, AnalogAxis::kCenter, 1, 0};
}

}  // namespace Tyra
