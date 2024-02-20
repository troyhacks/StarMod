/*
   @title     StarMod
   @file      UserModDDP.h
   @date      20240114
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#define DDP_DEFAULT_PORT 4048
#define DDP_HEADER_LEN 10
#define DDP_SYNCPACKET_LEN 10

#define DDP_FLAGS1_VER 0xc0  // version mask
#define DDP_FLAGS1_VER1 0x40 // version=1
#define DDP_FLAGS1_PUSH 0x01
#define DDP_FLAGS1_QUERY 0x02
#define DDP_FLAGS1_REPLY 0x04
#define DDP_FLAGS1_STORAGE 0x08
#define DDP_FLAGS1_TIME 0x10

#define DDP_ID_DISPLAY 1
#define DDP_ID_CONFIG 250
#define DDP_ID_STATUS 251

// 1440 channels per packet
#define DDP_CHANNELS_PER_PACKET 1440 // 480 leds

#define DDP_TYPE_RGB24  0x0B // 00 001 011 (RGB , 8 bits per channel, 3 channels)
#define DDP_TYPE_RGBW32 0x1B // 00 011 011 (RGBW, 8 bits per channel, 4 channels)

class UserModDDP:public SysModule {

public:

  IPAddress targetIp; //tbd: targetip also configurable from fixtures, and ddp instead of pin output

  UserModDDP() :SysModule("DDP") {
    isEnabled = false; //default off
  };

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initUserMod(parentVar, name);
    if (parentVar["o"] > -1000) parentVar["o"] = -3000; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

    ui->initSelect(parentVar, "ddpInst", UINT16_MAX, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
    
      case f_UIFun: {
        ui->setLabel(var, "Instance");
        ui->setComment(var, "Instance to send data");
        JsonArray options = ui->setOptions(var);
        JsonArray instanceObject = options.add<JsonArray>();
        instanceObject.add(0);
        instanceObject.add("no sync");
        for (auto node=instances->instances.begin(); node!=instances->instances.end(); ++node) {
          if (node->ip != WiFi.localIP()) {
            char option[32] = { 0 };
            strncpy(option, node->ip.toString().c_str(), sizeof(option)-1);
            strncat(option, " ", sizeof(option)-1);
            strncat(option, node->name, sizeof(option)-1);
            instanceObject = options.add<JsonArray>();
            instanceObject.add(node->ip[3]);
            instanceObject.add(option);
          }
        }
        return true;
      }
      case f_ChangeFun: {
        size_t ddpInst = var["value"];
        if (ddpInst >=0 && ddpInst < instances->instances.size()) {
          targetIp = instances->instances[ddpInst].ip;
          USER_PRINTF("Start DDP to %s\n", targetIp.toString().c_str());
        }
        return true;
      }
      default: return false;
    }}); //ddpInst
  }

  void loop() {
    // SysModule::loop();

    if(!SysModules::isConnected) return;

    if(!targetIp) return;

    if(!lds->newFrame) return;

    // calculate the number of UDP packets we need to send
    bool isRGBW = false;

    const size_t channelCount = lds->fixture.nrOfLeds * (isRGBW? 4:3); // 1 channel for every R,G,B,(W?) value
    const size_t packetCount = ((channelCount-1) / DDP_CHANNELS_PER_PACKET) +1;

    uint32_t channel = 0; 
    size_t bufferOffset = 0;

    sequenceNumber++;

    WiFiUDP ddpUdp;

    int bri = mdl->getValue("bri");

    for (size_t currentPacket = 0; currentPacket < packetCount; currentPacket++) {

      if (sequenceNumber > 15) sequenceNumber = 0;

      if (!ddpUdp.beginPacket(targetIp, DDP_DEFAULT_PORT)) {  // port defined in ESPAsyncE131.h
        USER_PRINTF("DDP WiFiUDP.beginPacket returned an error\n");
        return; // borked
      }

      // the amount of data is AFTER the header in the current packet
      size_t packetSize = DDP_CHANNELS_PER_PACKET;

      uint8_t flags = DDP_FLAGS1_VER1;
      if (currentPacket == (packetCount - 1U)) {
        // last packet, set the push flag
        // TODO: determine if we want to send an empty push packet to each destination after sending the pixel data
        flags = DDP_FLAGS1_VER1 | DDP_FLAGS1_PUSH;
        if (channelCount % DDP_CHANNELS_PER_PACKET) {
          packetSize = channelCount % DDP_CHANNELS_PER_PACKET;
        }
      }

      // write the header
      /*0*/ddpUdp.write(flags);
      /*1*/ddpUdp.write(sequenceNumber++ & 0x0F); // sequence may be unnecessary unless we are sending twice (as requested in Sync settings)
      /*2*/ddpUdp.write(isRGBW ?  DDP_TYPE_RGBW32 : DDP_TYPE_RGB24);
      /*3*/ddpUdp.write(DDP_ID_DISPLAY);
      // data offset in bytes, 32-bit number, MSB first
      /*4*/ddpUdp.write(0xFF & (channel >> 24));
      /*5*/ddpUdp.write(0xFF & (channel >> 16));
      /*6*/ddpUdp.write(0xFF & (channel >>  8));
      /*7*/ddpUdp.write(0xFF & (channel      ));
      // data length in bytes, 16-bit number, MSB first
      /*8*/ddpUdp.write(0xFF & (packetSize >> 8));
      /*9*/ddpUdp.write(0xFF & (packetSize     ));

      for (size_t i = 0; i < lds->fixture.nrOfLeds; i++) {
        CRGB pixel = lds->fixture.ledsP[i];
        ddpUdp.write(scale8(pixel.r, bri)); // R
        ddpUdp.write(scale8(pixel.g, bri)); // G
        ddpUdp.write(scale8(pixel.b, bri)); // B
        // if (isRGBW) ddpUdp.write(scale8(buffer[bufferOffset++], bri)); // W
      }

      if (!ddpUdp.endPacket()) {
        USER_PRINTF("DDP WiFiUDP.endPacket returned an error\n");
        return; // problem
      }
      channel += packetSize;
    }
  }

  private:
    size_t sequenceNumber = 0;

};

static UserModDDP *ddpmod;