/*
   @title     StarMod
   @file      UserModInstances.h
   @date      20240114
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once

#ifdef STARMOD_USERMOD_E131
  #include "UserModE131.h"
#endif
// #include "Sys/SysModSystem.h" //for sys->version
#include <HTTPClient.h> //need to be replaced by udp messages as this is a memory sucker

struct DMX {
  uint8_t universe:3; //3 bits / 8
  uint16_t start:9; //9 bits / 512
  uint8_t count:4; // 4 bits / 16
}; //total 16 bits

struct SysData {
  unsigned long upTime;
  uint8_t type;
  uint8_t syncMaster;
  DMX dmx;
};

struct VarData {
  char id[3];
  uint8_t value;
}; //4

#define nrOfAppVars 20

struct AppData {
  VarData vars[nrOfAppVars]; //total 80

  int getVar(const char * varID) { //int to support -1
    for (int i=0; i< nrOfAppVars; i++) {
      if (strncmp(vars[i].id, "", 3) != 0 && strncmp(vars[i].id, varID, 3) == 0) {
        return vars[i].value;
      }
    }
    return -1;
  }
  
  void setVar(const char * varID, uint8_t value) {
    size_t foundAppVar;
    for (int i=0; i< nrOfAppVars; i++) {
      if (strncmp(vars[i].id, "", 3) == 0)
        foundAppVar = i;
      else if (strncmp(vars[i].id, varID, 3) == 0) {
        foundAppVar = i;
        break; //use this slot
      }
    }
    size_t i = foundAppVar;
    if (strncmp(vars[i].id, "", 3) == 0) strncpy(vars[i].id, varID, 3);
    vars[i].value = value;
  }

  void initVars() {
    for (int i=0; i< nrOfAppVars; i++) {
      strncpy(vars[i].id, "", 3);
      vars[i].value = 0;
    }
  }

};

//note: changing SysData and AppData sizes: all instances should have the same version so change with care

struct InstanceInfo {
  IPAddress ip;
  char name[32];
  uint32_t version;
  unsigned long timeStamp; //when was the package received, used to check on aging
  SysData sys;
  AppData app; //total 80

};

struct UDPWLEDMessage {
  byte token;       //0: 'binary token 255'
  byte id;          //1: id '1'
  byte ip0;         //2: uint32_t ip takes 6 bytes instead of 4 ?!?! so splitting it into bytes
  byte ip1;         //3
  byte ip2;         //4
  byte ip3;         //5
  char name[32];    //6..37: server name
  byte type;        //38: instance type id //esp32 tbd: CONFIG_IDF_TARGET_ESP32S3 etc
  byte insId;      //39: instance id  unit ID == last IP number
  uint32_t version; //40..43: version ID (here it takes 4 bytes (as it should)
}; //total 44 bytes!

//compatible with WLED instances as it only interprets first 44 bytes
struct UDPStarModMessage {
  UDPWLEDMessage header; // 44 bytes fixed!
  SysData sys;
  AppData app;
  char body[1460 - sizeof(UDPWLEDMessage) - sizeof(SysData) - sizeof(AppData)];
};

//WLED syncmessage
struct UDPWLEDSyncMessage { //see notify( in WLED
  byte protocol; //0
  byte callMode; //1
  uint8_t bri; //2
  byte rCol0; //3
  byte gCol0; //4
  byte bCol0; //5
  byte nightlightActive; //6
  byte nightlightDelayMins; //7
  uint8_t mainsegMode; //8
  uint8_t mainsegSpeed; //9
  byte wCol0; //10
  byte version; //11
  uint8_t col1[4]; //12
  uint8_t mainsegIntensity; //16
  uint8_t transitionDelay[2]; //17
  uint8_t palette; //19
  uint8_t col2[4]; //20
  byte followUp; //24
  uint8_t timebase[4]; //25
  byte tokiSource; //29
  uint8_t tokiTime[4]; //30
  uint8_t tokiMs[2]; //34
  uint8_t syncGroups; //36
  char body[1193 - 37]; //41 +(32*36)+0 = 1193
};

class UserModInstances:public SysModule {

public:

  std::vector<InstanceInfo> instances;

  UserModInstances() :SysModule("Instances") {
  };

  void addTblRow(JsonArray row, std::vector<InstanceInfo>::iterator instance) {
    row.add(JsonString(instance->name, JsonString::Copied));
    char urlString[32] = "http://";
    strncat(urlString, instance->ip.toString().c_str(), sizeof(urlString)-1);
    row.add(JsonString(urlString, JsonString::Copied));
    row.add(JsonString(instance->ip.toString().c_str(), JsonString::Copied));
    // row.add(instance->timeStamp / 1000);

    row.add(instance->sys.type?"StarMod":"WLED");

    row.add(instance->version);
    row.add(instance->sys.upTime);

    mdl->findVars("stage", true, [instance, row](JsonObject var) { //findFun
      //look for value in instance
      int value = instance->app.getVar(var["id"]);
      // USER_PRINTF("insTbl %s %s: %d\n", instance->name, varID, value);
      row.add(value);
    });
  }

  //setup filesystem
  void setup() {
    SysModule::setup();

    parentVar = ui->initSysMod(parentVar, name);
    if (parentVar["o"] > -1000) parentVar["o"] = -4200; //set default order. Don't use auto generated order as order can be changed in the ui (WIP)

    JsonObject tableVar = ui->initTable(parentVar, "insTbl", nullptr, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Instances");
        ui->setComment(var, "List of instances");
        return true;
      }
      default: return false;
    }});
    
    ui->initText(tableVar, "insName", nullptr, 32, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, JsonString(instances[rowNrL].name, JsonString::Copied), rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Name");
        return true;
      default: return false;
    }});

    ui->initURL(tableVar, "insLink", nullptr, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++) {
          char urlString[32] = "http://";
          strncat(urlString, instances[rowNrL].ip.toString().c_str(), sizeof(urlString)-1);
          mdl->setValue(var, JsonString(urlString, JsonString::Copied), rowNrL);
        }
        return true;
      case f_UIFun:
        ui->setLabel(var, "Show");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "insIp", nullptr, 16, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, JsonString(instances[rowNrL].ip.toString().c_str(), JsonString::Copied), rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "IP");
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "insType", nullptr, 16, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, instances[rowNrL].sys.type?"StarMod":"WLED", rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Type");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "insVersion", UINT16_MAX, 0, (unsigned long)-1, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, instances[rowNrL].version, rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Version");
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "insUp", UINT16_MAX, 0, (unsigned long)-1, true, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_ValueFun:
        for (uint8_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++)
          mdl->setValue(var, instances[rowNrL].sys.upTime, rowNrL);
        return true;
      case f_UIFun:
        ui->setLabel(var, "Uptime");
      default: return false;
    }});

    JsonObject currentVar;

    //default is 0 / None
    currentVar = ui->initSelect(parentVar, "sma", 0, false, [this](JsonObject var, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
      case f_UIFun: {
        ui->setLabel(var, "Sync Master");
        ui->setComment(var, "Instance to sync from");
        JsonArray options = ui->setOptions(var);
        JsonArray instanceObject = options.add<JsonArray>();
        instanceObject.add(0);
        instanceObject.add("no sync");
        for (auto instance=instances.begin(); instance!=instances.end(); ++instance) {
          char option[32] = { 0 };
          strncpy(option, instance->ip.toString().c_str(), sizeof(option)-1);
          strncat(option, " ", sizeof(option)-1);
          strncat(option, instance->name, sizeof(option)-1);
          instanceObject = options.add<JsonArray>();
          instanceObject.add(instance->ip[3]);
          instanceObject.add(option);
        }
        return true;
      }
      default: return false;
    }}); //syncMaster
    currentVar["stage"] = true;

    //find stage variables and add them to the table
    mdl->findVars("stage", true, [tableVar, this](JsonObject var) { //findFun

      USER_PRINTF("stage %s %s found\n", mdl->varID(var), var["value"].as<String>().c_str());

      char columnVarID[32] = "ins";
      strcat(columnVarID, var["id"]);
      JsonObject insVar; // = ui->cloneVar(var, columnVarID, [this, var](JsonObject insVar){});

      //create a var of the same type. InitVar is not calling chFun which is good in this situation!
      insVar = ui->initVar(tableVar, columnVarID, var["type"], false, [this, var](JsonObject insVar, uint8_t rowNr, uint8_t funType) { switch (funType) { //varFun
        case f_ValueFun:
          //should not trigger chFun
          // USER_PRINTF("initVar stage %s[%d]\n", mdl->varID(insVar), rowNr);
          for (uint8_t rowNrL = 0; rowNrL < instances.size() && (rowNr == UINT8_MAX || rowNrL == rowNr); rowNrL++) {
            //do what setValue is doing except calling changeFun
            // insVar["value"][rowNrL] = instances[rowNrL].app.getVar(mdl->varID(var)); //only int values...
            web->addResponse(insVar["id"], "value", instances[rowNrL].app.getVar(mdl->varID(var)), rowNrL); //only int values...);

            // mdl->setValue(insVar, instances[rowNr].app.getVar(var["id"]), rowNr);
          //send to ws?
          }
          return true;
        case f_UIFun:
          // call uiFun of the base variable for the new variable
          ui->varFunctions[var["fun"]](insVar, rowNr, f_UIFun);
          return true;
        case f_ChangeFun: {
          //do not set this initially!!!
          if (rowNr != UINT8_MAX) {
            //if this instance update directly, otherwise send over network
            if (instances[rowNr].ip == WiFi.localIP()) {
              mdl->setValue(var, mdl->getValue(insVar, rowNr).as<uint8_t>()); //this will call sendDataWS (tbd...), do not set for rowNr
            } else {
              // https://randomnerdtutorials.com/esp32-http-get-post-arduino/
              HTTPClient http;
              char serverPath[32];
              print->fFormat(serverPath, sizeof(serverPath)-1, "http://%s/json", instances[rowNr].ip.toString().c_str());
              http.begin(serverPath);
              http.addHeader("Content-Type", "application/json");
              char postMessage[32];
              print->fFormat(postMessage, sizeof(postMessage)-1, "{\"%s\":%d}", mdl->varID(var), mdl->getValue(insVar, rowNr).as<uint8_t>());

              USER_PRINTF("json post %s %s\n", serverPath, postMessage);

              int httpResponseCode = http.POST(postMessage);

              if (httpResponseCode>0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String payload = http.getString();
                Serial.println(payload);
              }
              else {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
              }
              // Free resources
              http.end();
            }
          }
          // print->printJson(" ", var);
          return true;
        }
        default: return false;
      }});

      if (insVar) {
        if (!var["min"].isNull()) insVar["min"] = var["min"];
        if (!var["max"].isNull()) insVar["max"] = var["max"];
        if (!var["log"].isNull()) insVar["log"] = var["log"];
        // insVar["fun"] = var["fun"]; //copy the uiFun
      }

    });

    if (sizeof(UDPWLEDMessage) != 44) {
      USER_PRINTF("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPWLEDMessage));
      // USER_PRINTF("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }
    if (sizeof(UDPStarModMessage) != 1460) { //size of UDP Packet
      // one udp frame should be 1460, not 1472 (then split by network in 1460 and 12)
      USER_PRINTF("Program error: Size of UDP message is not 44: %d\n", sizeof(UDPStarModMessage));
      // USER_PRINTF("udpMessage size %d = %d + %d + %d + ...\n", sizeof(UDPWLEDMessage), sizeof(udpMessage.ip0), sizeof(udpMessage.version), sizeof(udpMessage.name));
      success = false;
    }

    USER_PRINTF("UDPWLEDSyncMessage %d %d %d\n", sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage), sizeof(UDPWLEDSyncMessage));
  }

  void onOffChanged() {
    if (SysModules::isConnected && isEnabled) {
      udpConnected = notifierUdp.begin(notifierUDPPort); //sync
      udp2Connected = instanceUDP.begin(instanceUDPPort); //instances
    } else {
      udpConnected = false;
      udp2Connected = false;
      instances.clear();

      //not needed here as there is no connection
      // ui->processUiFun("insTbl");

      //udp off ??
    }
  }

  void loop() {
    // SysModule::loop();

    handleNotifications();

    if (ui->stageVarChanged) {
      ui->stageVarChanged = false;
      sendSysInfoUDP(); 
    }
  }

  void loop10s() {
    sendSysInfoUDP(); 
  }

  void handleNotifications()
  {
    if(!SysModules::isConnected) return;

    // instanceUDP.flush(); //tbd: test if needed

    int packetSize;

    //handle sync from WLED
    if (udpConnected) {
      int packetSize = notifierUdp.parsePacket();

      if (packetSize > 0) {
        IPAddress remoteIp = notifierUdp.remoteIP();

        // USER_PRINTF("handleNotifications sync ...%d %d\n", remoteIp[3], packetSize);

        UDPWLEDSyncMessage wledSyncMessage;
        uint8_t *udpIn = (uint8_t *)&wledSyncMessage;
        notifierUdp.read(udpIn, packetSize);

        // for (int i=0; i<40; i++) {
        //   Serial.printf("%d: %d\n", i, udpIn[i]);
        // }

        USER_PRINTF("   %d %d p:%d\n", wledSyncMessage.bri, wledSyncMessage.mainsegMode, packetSize);

        std::vector<InstanceInfo>::iterator instance = findNode(remoteIp); //if not exist, created

        instance->sys.upTime = (wledSyncMessage.timebase[0] * 256*256*256 + 256*256*wledSyncMessage.timebase[1] + 256*wledSyncMessage.timebase[2] + wledSyncMessage.timebase[3]) / 1000;
        instance->sys.syncMaster = wledSyncMessage.syncGroups; //tbd: change
        
        uint8_t syncMaster = mdl->getValue("sma");
        if (syncMaster == remoteIp[3]) {
          if (instance->app.getVar("bri") != wledSyncMessage.bri) mdl->setValue("bri", wledSyncMessage.bri);
          //only set brightness
        }

        instance->app.setVar("bri", wledSyncMessage.bri);
        instance->app.setVar("fx", wledSyncMessage.mainsegMode); //tbd: rowNr
        instance->app.setVar("pal", wledSyncMessage.palette); //tbd: rowNr

        // for (size_t x = 0; x < packetSize; x++) {
        //   char xx = (char)udpIn[x];
        //   Serial.print(xx);
        // }
        // Serial.println();

        USER_PRINTF("insTbl handleNotifications %d\n", remoteIp[3]);
        for (JsonObject childVar: mdl->varN("insTbl"))
          ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //rowNr //instance - instances.begin()

        return;
      }
    }
 
    //handle instances update
    if (udp2Connected) {
      packetSize = instanceUDP.parsePacket();

      if (packetSize > 0) {
        IPAddress remoteIp = instanceUDP.remoteIP();
        // USER_PRINTF("handleNotifications instances ...%d %d check %d or %d\n", remoteIp[3], packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage));

        if (packetSize == sizeof(UDPWLEDMessage)) { //WLED instance
          UDPStarModMessage starModMessage;
          uint8_t *udpIn = (uint8_t *)&starModMessage.header;
          instanceUDP.read(udpIn, packetSize);

          starModMessage.sys.type = 0; //WLED

          updateNode(starModMessage);
        }
        else if (packetSize == sizeof(UDPStarModMessage)) {
          UDPStarModMessage starModMessage;
          uint8_t *udpIn = (uint8_t *)&starModMessage;
          instanceUDP.read(udpIn, packetSize);
          starModMessage.sys.type = 1; //Starmod

          updateNode(starModMessage);
        }
        else {
          //read the rest of the data (flush)
          uint8_t udpIn[1472+1];
          notifierUdp.read(udpIn, packetSize);

          USER_PRINTF("packetSize %d not equal to %d or %d\n", packetSize, sizeof(UDPWLEDMessage), sizeof(UDPStarModMessage));
        }

      } //packetSize
      else {
      }
    }

    //remove inactive instances
    for (std::vector<InstanceInfo>::iterator instance=instances.begin(); instance!=instances.end(); ) {
      if (millis() - instance->timeStamp > 32000) { //assuming a ping each 30 seconds
        instance = instances.erase(instance);
        USER_PRINTF("insTbl remove inactive instances %d\n", instance->ip[3]);

        for (JsonObject childVar: mdl->varN("insTbl"))
          ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //no rowNr so all rows updated

        ui->callVarFun("ddpInst", UINT8_MAX, f_UIFun);
        ui->callVarFun("artInst", UINT8_MAX, f_UIFun);
      }
      else
        ++instance;
    }
  }

  void sendSysInfoUDP()
  {
    if(!SysModules::isConnected) return;
    if (!udp2Connected) return;

    IPAddress localIP = WiFi.localIP();
    if (!localIP || localIP == IPAddress(255,255,255,255)) localIP = IPAddress(4,3,2,1);

    UDPStarModMessage starModMessage;
    starModMessage.header.token = 255; //WLED only accepts 255
    starModMessage.header.id = 1; //WLED only accepts 1
    starModMessage.header.ip0 = localIP[0];
    starModMessage.header.ip1 = localIP[1];
    starModMessage.header.ip2 = localIP[2];
    starModMessage.header.ip3 = localIP[3];
    const char * serverName = mdl->getValue("serverName");
    strncpy(starModMessage.header.name, serverName?serverName:"StarMod", sizeof(starModMessage.header.name)-1);
    starModMessage.header.type = 32; //esp32 tbd: CONFIG_IDF_TARGET_ESP32S3 etc
    starModMessage.header.insId = localIP[3]; //WLED: used in map of instances as index!
    starModMessage.header.version = atoi(sys->version);
    starModMessage.sys.type = 1; //StarMod
    starModMessage.sys.upTime = millis()/1000;
    starModMessage.sys.syncMaster = mdl->getValue("sma");
    starModMessage.sys.dmx.universe = 0;
    starModMessage.sys.dmx.start = 0;
    starModMessage.sys.dmx.count = 0;
    #ifdef STARMOD_USERMOD_E131
      if (e131mod->isEnabled) {
        starModMessage.sys.dmx.universe = mdl->getValue("dun");
        starModMessage.sys.dmx.start = mdl->getValue("dch");
        starModMessage.sys.dmx.count = 3;//e131->varsToWatch.size();
      }
    #endif

    //stage values default 0
    starModMessage.app.initVars();

    //send stage values
    mdl->findVars("stage", true, [&starModMessage](JsonObject var) { //varFun
      starModMessage.app.setVar(var["id"], var["value"]);
    });

    updateNode(starModMessage); //temp? to show own instance in list as instance is not catching it's own udp message...

    IPAddress broadcastIP(255, 255, 255, 255);
    if (0 != instanceUDP.beginPacket(broadcastIP, instanceUDPPort)) {  // WLEDMM beginPacket == 0 --> error
      // USER_PRINTF("sendSysInfoUDP %s s:%d p:%d i:...%d\n", starModMessage.header.name, sizeof(UDPStarModMessage), instanceUDPPort, localIP[3]);
      // for (size_t x = 0; x < sizeof(UDPWLEDMessage) + sizeof(SysData) + sizeof(AppData); x++) {
      //   char * xx = (char *)&starModMessage;
      //   Serial.printf("%d: %d - %c\n", x, xx[x], xx[x]);
      // }

      instanceUDP.write((uint8_t*)&starModMessage, sizeof(UDPStarModMessage));
      instanceUDP.endPacket();
    }
    else {
      USER_PRINTF("sendSysInfoUDP error\n");
    }
  }

  void updateNode( UDPStarModMessage udpStarMessage) {
    IPAddress messageIP = IPAddress(udpStarMessage.header.ip0, udpStarMessage.header.ip1, udpStarMessage.header.ip2, udpStarMessage.header.ip3);

    bool instanceFound = false;
    for (auto instance=instances.begin(); instance!=instances.end(); ++instance)
    {
      if (instance->ip == messageIP)
        instanceFound = true;
    }

    // USER_PRINTF("updateNode Instance: ...%d n:%s found:%d\n", messageIP[3], udpStarMessage.header.name, instanceFound);

    if (!instanceFound) { //new instance
      InstanceInfo instance;
      instance.ip = messageIP;
      if (udpStarMessage.sys.type == 0) {//WLED only
        instance.sys.type = 0; //WLED
        //updated in udp sync message:
        instance.sys.upTime = 0;
        instance.sys.dmx.universe = 0;
        instance.sys.dmx.start = 0;
        instance.sys.dmx.count = 0;
        instance.sys.syncMaster = 0;
        //stage values default 0
        instance.app.initVars();
      }

      instances.push_back(instance);
      std::sort(instances.begin(),instances.end(), [](InstanceInfo &a, InstanceInfo &b){ return a.ip < b.ip; });//Sorting the vector strcmp(a.name,b.name);
    }

    //iterate vector pointers so we can update the instances
    // uint8_t rowNr = 0;
    for (std::vector<InstanceInfo>::iterator instance=instances.begin(); instance!=instances.end(); ++instance) {
      if (instance->ip == messageIP) {
        instance->timeStamp = millis(); //update timestamp
        strncpy(instance->name, udpStarMessage.header.name, sizeof(instance->name)-1);
        instance->version = udpStarMessage.header.version;
        if (udpStarMessage.sys.type == 1) {//StarMod only
          instance->sys = udpStarMessage.sys;

          //check for syncing
          uint8_t syncMaster = mdl->getValue("sma");
          if (syncMaster == messageIP[3]) {

            //find matching var
            for (int i=0; i< nrOfAppVars; i++) {
              //set instance kv

              VarData newVar = udpStarMessage.app.vars[i];

              if (strncmp(newVar.id, "", 3) != 0) {

                int value = instance->app.getVar(newVar.id);

                //if no value found or value has changed
                if (value == -1 || value != newVar.value) {
                  char varID[4];
                  strncpy(varID, newVar.id, 3);
                  varID[3]='\0';

                  USER_PRINTF("AppData3 %s %s %d\n", instance->name, varID, newVar.value);
                  mdl->setValue(varID, newVar.value);
                }
              }
            }
          } 

          //set values to instance
          for (int i=0; i< nrOfAppVars; i++)
            instance->app.vars[i] = udpStarMessage.app.vars[i];
        }

        //only update cell in instbl!
        //create a json string
        //send the json
        //ui to parse the json

        if (instanceFound) {
          // JsonObject responseObject = web->getResponseObject();

          // responseObject["updRow"]["id"] = "insTbl";
          // responseObject["updRow"]["rowNr"] = rowNr;
          // responseObject["updRow"]["value"].to<JsonArray>();
          // addTblRow(responseObject["updRow"]["value"], instance);

          // web->sendResponseObject();

          // USER_PRINTF("updateNode updRow[%d] %s\n", instance - instances.begin(), instances[instance - instances.begin()].name);

          for (JsonObject childVar: mdl->varN("insTbl"))
            ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //rowNr instance - instances.begin()

          //tbd: now done for all rows, should be done only for updated rows!
        }

      } //ip
      // rowNr++;
    } // for instances

    if (!instanceFound) {
      USER_PRINTF("insTbl new instance node %d\n", messageIP[3]);
      
      ui->callVarFun("ddpInst", UINT8_MAX, f_UIFun); //UiFun as select changes
      ui->callVarFun("artInst", UINT8_MAX, f_UIFun);

      // ui->processUiFun("insTbl");
      //run though it sorted to find the right rowNr
      // for (std::vector<InstanceInfo>::iterator instance=instances.begin(); instance!=instances.end(); ++instance) {
      //   if (instance->ip == messageIP) {
          for (JsonObject childVar: mdl->varN("insTbl")) {
            ui->callVarFun(childVar, UINT8_MAX, f_ValueFun); //no rowNr, update all
          }
      //   }
      // }
    }
  }

  std::vector<InstanceInfo>::iterator findNode(IPAddress nodeIP) {

    bool instanceFound = false;
    for (auto instance=instances.begin(); instance!=instances.end(); ++instance)
    {
      if (instance->ip == nodeIP)
        instanceFound = true;
    }

    if (!instanceFound) { //instance always found
      InstanceInfo instance;
      instance.ip = nodeIP;
      instances.push_back(instance);
      std::sort(instances.begin(),instances.end(), [](InstanceInfo &a, InstanceInfo &b){ return a.ip < b.ip; });//Sorting the vector strcmp(a.name,b.name);
    }

    std::vector<InstanceInfo>::iterator foundNode;
    //iterate vector pointers so we can update the instances
    for (auto instance=instances.begin(); instance!=instances.end(); ++instance) {
      if (instance->ip == nodeIP) {
        foundNode = instance;
      }
    }

    return foundNode;

  }

  private:
    //sync (only WLED)
    WiFiUDP notifierUdp;
    uint16_t notifierUDPPort = 21324;
    bool udpConnected = false;

    //instances (WLED and StarMod)
    WiFiUDP instanceUDP;
    uint16_t instanceUDPPort = 65506;
    bool udp2Connected = false;

};

static UserModInstances *instances;