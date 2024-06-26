/*
   @title     StarMod
   @file      SysModPins.cpp
   @date      20240226
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModPins.h"
#include "SysModPrint.h"
#include "SysModUI.h"
#include "SysModWeb.h"

SysModPins::SysModPins() :SysModule("Pins") {
#if CONFIG_IDF_TARGET_ESP32
  // softhack007: configuring these pins on S3/C3/S2 may cause major problems (crashes included)
  // pinMode(2, OUTPUT);  // softhack007 default LED pin on some boards, so don't play around with gpio2
  pinMode(4, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(33, OUTPUT);
#endif

  //start with no pins allocated
  for (int i=0; i<NUM_DIGITAL_PINS; i++) {
    deallocatePin(i, pinObjects[i].owner);
  }
};

void SysModPins::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name, 2202);

  //show table of allocated pins
  JsonObject tableVar = ui->initTable(parentVar, "pinTbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun: {
      ui->setLabel(var, "Allocated Pins");
      return true; }
    default: return false;
  }});

  ui->initPin(tableVar, "pinNr", UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++)
        mdl->setValue(var, getPinNr(rowNr), rowNr);
      return true;
    case f_UIFun:
      ui->setLabel(var, "Pin");
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "pinOwner", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++)
        mdl->setValue(var, JsonString(getNthAllocatedPinObject(rowNr).owner, JsonString::Copied), rowNr);
      return true;
    case f_UIFun:
      ui->setLabel(var, "Owner");
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "pinDetails", nullptr, 256, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < getNrOfAllocatedPins(); rowNr++)
        USER_PRINTF("pinDetails[%d] d1:%s d2:%s\n", rowNr, getNthAllocatedPinObject(rowNr).details, pinObjects[0].details);
        mdl->setValue(var, JsonString(getNthAllocatedPinObject(rowNr).details, JsonString::Copied), rowNr);
      return true;
    case f_UIFun:
      ui->setLabel(var, "Details");
      return true;
    default: return false;
  }});

  #ifdef STARMOD_DEVMODE

  ui->initCanvas(parentVar, "board", UINT16_MAX, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Board layout");
      ui->setComment(var, "WIP");
      return true;
    case f_LoopFun:
      var["interval"] = 10*10*10; //every 10 sec from cs to ms

      web->sendDataWs([](AsyncWebSocketMessageBuffer * wsBuf) {
        byte* buffer;

        buffer = wsBuf->get();

        // send leds preview to clients
        for (size_t i = 0; i < 20; i++)
        {
          buffer[i*3+5] = random(256);// (digitalRead(i)+1) * 50;
          buffer[i*3+5+1] = random(256);
          buffer[i*3+5+2] = random(256);
        }
        //new values
        buffer[0] = 0; //userFun id

      }, 20 * 3 + 5, true);
      return true;

    default: return false;
  }});

  #endif //STARMOD_DEVMODE
  // ui->initCheckBox(parentVar, "pin2", true, UINT8_MAX, false, nullptr, updateGPIO);
  // ui->initCheckBox(parentVar, "pin4");
#if CONFIG_IDF_TARGET_ESP32
  ui->initCheckBox(parentVar, "pin19", true, false, updateGPIO);
#endif
  // ui->initCheckBox(parentVar, "pin33", true);
}

void SysModPins::loop1s() {
  if (pinsChanged) {
    pinsChanged = false;

    for (JsonObject childVar: mdl->varChildren("pinTbl"))
      ui->callVarFun(childVar, UINT8_MAX, f_ValueFun);
  }
}

bool SysModPins::updateGPIO(JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ChangeFun:
      if (var["value"].is<bool>()) {
        bool pinValue = var["value"];
        JsonString id = var["id"];

        USER_PRINTF("updateGPIO %s:=%d\n", id.c_str(), pinValue);

#if CONFIG_IDF_TARGET_ESP32
  // softhack007: writing these pins on S3/C3/S2 may cause major problems (crashes included)
        // if (id == "pin2") digitalWrite(2, pinValue?HIGH:LOW); // softhack007 default LED pin on some boards, so don't play around with gpio2
        if (id == "pin4") digitalWrite(4, pinValue?HIGH:LOW);
        if (id == "pin19") digitalWrite(19, pinValue?HIGH:LOW);
        if (id == "pin33") digitalWrite(33, pinValue?HIGH:LOW);
#endif
      }
      return true;
    default: return false;
  }};

void SysModPins::allocatePin(unsigned8 pinNr, const char * owner, const char * details) {
  USER_PRINTF("allocatePin %d %s %s\n", pinNr, owner, details);
  if ((pinNr < NUM_DIGITAL_PINS) && (digitalPinIsValid(pinNr))) {
    if (strcmp(pinObjects[pinNr].owner, "") != 0 && strcmp(pinObjects[pinNr].owner, owner) != 0)
      USER_PRINTF("allocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strncpy(pinObjects[pinNr].owner, owner, sizeof(PinObject::owner)-1);  
      strncpy(pinObjects[pinNr].details, details, sizeof(PinObject::details)-1);  
      pinsChanged = true;
    }
  }
}

void SysModPins::deallocatePin(unsigned8 pinNr, const char * owner) {
  // USER_PRINTF("deallocatePin %d %s\n", pinNr, owner);
  if (pinNr < NUM_DIGITAL_PINS) {
    if (strcmp(pinObjects[pinNr].owner, owner) != 0)
      USER_PRINTF("deallocatePin %d: not owner %s!=%s", pinNr, owner, pinObjects[pinNr].owner);
    else {
      strcpy(pinObjects[pinNr].owner, "");  
      strcpy(pinObjects[pinNr].details, "");  
      pinsChanged = true;
    }
  }
}