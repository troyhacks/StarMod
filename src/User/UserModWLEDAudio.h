/*
   @title     StarMod
   @file      UserModWLEDAudio.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#include <WLED-sync.h> // https://github.com/netmindz/WLED-sync

#define MAX_FREQUENCY   11025          // sample frequency / 2 (as per Nyquist criterion)

enum UM_SoundSimulations {
  UMS_BeatSin = 0,
  UMS_WeWillRockYou
};


class UserModWLEDAudio:public SysModule {

public:

  WLEDSync sync;
  byte fftResults[NUM_GEQ_CHANNELS]= {0};
  float volumeSmth;

  UserModWLEDAudio() :SysModule("WLED Audio Sync Receiver") {
  };

  //setup filesystem
  void setup() {
    SysModule::setup();
    parentVar = ui->initUserMod(parentVar, name, 6300);
  
    ui->initText(parentVar, "wledAudioStatus", nullptr, 16, true, [](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Status:");
      return true;
    default: return false;
   }});

  }

  void onOffChanged() {
    if (mdls->isConnected && isEnabled) {
      sync.begin();
    } else {
      // sync.end();???
    }
  }

  bool isTimeout() {
    return ((millis() - lastData) > 10000);
  }

  void loop() {
    // SysModule::loop();
    if (mdls->isConnected && sync.read()) {
      lastData = millis();
      if(debug) USER_PRINTF("WLED-Sync: ");
      for (int b = 0; b < NUM_GEQ_CHANNELS; b++) {
        byte val = sync.fftResult[b];
        fftResults[b] = val;
        if(debug) USER_PRINTF("%u ", val);
      }
      volumeSmth = sync.volumeSmth;
      if(debug) USER_PRINTF("\n");
    }
    else if((lastData == 0) || isTimeout()) { // Could also check for non-silent
      simulateSound(UMS_BeatSin);
    }
  }

  void loop10s() {
    // USER_PRINTF("%d, %s, %d\n", sync.receivedFormat, sync.sourceIP.toString().c_str(), sync.lastPacketTime);
    String msg = "";
    if((lastData != 0) && isTimeout()) {
      msg = sync.sourceIP.toString() + " Timeout " + ((millis() - lastData) / 1000)  +"s";
    }
    else {
      switch(sync.receivedFormat) {
        case -1: msg = "Not connected";
          break;
        case 1: msg = "V1 from " + sync.sourceIP.toString();
          break;
        case 2: msg = "V2 from " + sync.sourceIP.toString();
          break;
        default: msg = "Unknown";
          break;
      }
    }
    mdl->setUIValueV("wledAudioStatus", "%s", msg.c_str());
  }

  private:
    boolean debug = false;
    unsigned long lastData = 0; 

    void simulateSound(uint8_t simulationId)
    {
      uint8_t samplePeak;
      float   FFT_MajorPeak;
      uint8_t maxVol;
      uint8_t binNum;

      uint16_t volumeRaw;
      float    my_magnitude;

      uint32_t ms = millis();

      switch (simulationId) {
        default:
        case UMS_BeatSin:
          for (int i = 0; i<16; i++)
            fftResults[i] = beatsin8(120 / (i+1), 0, 255);
            // fftResults[i] = (beatsin8(120, 0, 255) + (256/16 * i)) % 256;
            volumeSmth = fftResults[8];
          break;
        case UMS_WeWillRockYou:
          if (ms%2000 < 200) {
            volumeSmth = random8(255);
            for (int i = 0; i<5; i++)
              fftResults[i] = random8(255);
          }
          else if (ms%2000 < 400) {
            volumeSmth = 0;
            for (int i = 0; i<16; i++)
              fftResults[i] = 0;
          }
          else if (ms%2000 < 600) {
            volumeSmth = random8(255);
            for (int i = 5; i<11; i++)
              fftResults[i] = random8(255);
          }
          else if (ms%2000 < 800) {
            volumeSmth = 0;
            for (int i = 0; i<16; i++)
              fftResults[i] = 0;
          }
          else if (ms%2000 < 1000) {
            volumeSmth = random8(255);
            for (int i = 11; i<16; i++)
              fftResults[i] = random8(255);
          }
          else {
            volumeSmth = 0;
            for (int i = 0; i<16; i++)
              fftResults[i] = 0;
          }
          break;
      }

      // samplePeak    = random8() > 250;
      // FFT_MajorPeak = 21 + (volumeSmth*volumeSmth) / 8.0f; // WLEDMM 21hz...8200hz
      // maxVol        = 31;  // this gets feedback fro UI
      // binNum        = 8;   // this gets feedback fro UI
      // volumeRaw = volumeSmth;
      // my_magnitude = 10000.0f / 8.0f; //no idea if 10000 is a good value for FFT_Magnitude ???
      // if (volumeSmth < 1 ) my_magnitude = 0.001f;             // noise gate closed - mute

    }
 

};

extern UserModWLEDAudio *wledAudioMod;