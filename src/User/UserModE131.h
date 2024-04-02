/*
   @title     StarMod
   @file      UserModE131.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include <ESPAsyncE131.h>

#define maxChannels 513

class UserModE131:public SysModule {

public:

  UserModE131() :SysModule("E131") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name, 6201);

    ui->initNumber(parentVar, "dun", universe, 0, 7, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "DMX Universe");
        return true;
      case f_ChangeFun:
        universe = var["value"];
        return true;
      default: return false;
    }});

    JsonObject currentVar = ui->initNumber(parentVar, "dch", 1, 1, 512, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "DMX Channel");
        ui->setComment(var, "First channel");
        return true;
      case f_ChangeFun:
        for (JsonObject childVar: mdl->varChildren("e131Tbl"))
          ui->callVarFun(childVar, UINT8_MAX, f_ValueFun);
        return true;
      default: return false;
    }});
    currentVar["dash"] = true;

    JsonObject tableVar = ui->initTable(parentVar, "e131Tbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_UIFun:
        ui->setLabel(var, "Vars to watch");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "e131Channel", UINT16_MAX, 1, 512, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, varsToWatch[rowNr].channel + mdl->getValue("dch").as<unsigned8>(), rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Channel");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "e131Name", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, varsToWatch[rowNr].id, rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Name");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "e131Max", UINT16_MAX, 0, UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, varsToWatch[rowNr].max, rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Max");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "e131Value", UINT16_MAX, 0, 255, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (forUnsigned8 rowNr = 0; rowNr < varsToWatch.size(); rowNr++)
          mdl->setValue(var, varsToWatch[rowNr].savedValue, rowNr);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Value");
        return true;
      default: return false;
    }});

  }

  // void connectedChanged() {
  //   setOnOff();
  // }

  // void enabledChanged() {
  //   setOnOff();
  // }

  void onOffChanged() {

    if (mdls->isConnected && isEnabled) {
      USER_PRINTF("UserModE131::connected && enabled\n");

      if (e131Created) { // TODO: crashes here - no idea why!
        USER_PRINTF("UserModE131 - ESPAsyncE131 created already\n");
        return;
      }
      USER_PRINTF("UserModE131 - Create ESPAsyncE131\n");

      e131 = ESPAsyncE131(universeCount);
      if (this->e131.begin(E131_MULTICAST, universe, universeCount)) { // TODO: multicast igmp failing, so only works with unicast currently
        USER_PRINTF("Network exists, begin e131.begin ok\n");
        success = true;
      }
      else {
        USER_PRINTF("Network exists, begin e131.begin FAILED\n");
      }
      e131Created = true;
    }
    else {
      // e131.end()???
      e131Created = false;
    }
  }

  void loop() {
    // SysModule::loop();
    if(!e131Created) {
      return;
    }
    if (!e131.isEmpty()) {
      e131_packet_t packet;
      e131.pull(&packet);     // Pull packet from ring buffer

      for (VarToWatch &varToWatch : varsToWatch) {
        for (int i=0; i < maxChannels; i++) {
          if (i == varToWatch.channel) {
            if (packet.property_values[i] != varToWatch.savedValue) {

              USER_PRINTF("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH%d: %u -> %u",
                      htons(packet.universe),                 // The Universe for this packet
                      htons(packet.property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
                      e131.stats.num_packets,                 // Packet counter
                      e131.stats.packet_errors,               // Packet error counter
                      i,
                      varToWatch.savedValue,
                      packet.property_values[i]);             // Dimmer data for Channel i

              varToWatch.savedValue = packet.property_values[i];

              if (varToWatch.id != nullptr && varToWatch.max != 0) {
                USER_PRINTF(" varsToWatch: %s\n", varToWatch.id);
                mdl->setValue(varToWatch.id, varToWatch.savedValue%(varToWatch.max+1)); // TODO: ugly to have magic string 
              }
              else
                USER_PRINTF("\n");
            }//!= savedValue
          }//if channel
        }//maxChannels
      } //for varToWatch
    } //!e131.isEmpty()
  } //loop

  void patchChannel(unsigned8 channel, const char * id, unsigned8 max = 255) {
    VarToWatch varToWatch;
    varToWatch.channel = channel;
    varToWatch.id = id;
    varToWatch.savedValue = 0; // Always reset when (re)patching so variable gets set to DMX value even if unchanged
    varToWatch.max = max;
    varsToWatch.push_back(varToWatch);
  }

  // unsigned8 getValue(const char * id) {
  //   for (int i=0; i < maxChannels; i++) {
  //       if(varsToWatch[i].id == id) {
  //         return varsToWatch[i].savedValue;
  //       }
  //   }
  //   USER_PRINTF("ERROR: failed to find param %s\n", id);
  //   return 0;
  // }

  private:
    struct VarToWatch {
      unsigned16 channel;
      const char * id = nullptr;
      unsigned16 max = -1;
      unsigned8 savedValue = -1;
    };

    std::vector<VarToWatch> varsToWatch;

    ESPAsyncE131 e131;
    boolean e131Created = false;
    unsigned16 universe = 1;
    unsigned8 universeCount = 1;

};

extern UserModE131 *e131mod;