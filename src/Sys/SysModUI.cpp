/*
   @title     StarMod
   @file      SysModUI.cpp
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModModel.h"

SysModUI::SysModUI() :SysModule("UI") {
};

//serve index.htm
void SysModUI::setup() {
  SysModule::setup();

  parentVar = initSysMod(parentVar, name, 4101);

  JsonObject tableVar = initTable(parentVar, "vlTbl", nullptr, true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Variable loops");
      ui->setComment(var, "Loops initiated by a variable");
      return true;
    default: return false;
  }});

  initText(tableVar, "vlVar", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < loopFunctions.size(); rowNr++)
        mdl->setValue(var, JsonString(loopFunctions[rowNr].var["id"], JsonString::Copied), rowNr);
      return true;
    case f_UIFun:
      ui->setLabel(var, "Name");
      return true;
    default: return false;
  }});

  initNumber(tableVar, "vlLoopps", UINT16_MAX, 0, 999, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < loopFunctions.size(); rowNr++)
        mdl->setValue(var, loopFunctions[rowNr].counter, rowNr);
      return true;
    case f_UIFun:
      ui->setLabel(var, "Loops p s");
      return true;
    default: return false;
  }});
}

void SysModUI::loop() {
  // SysModule::loop();

  for (VarLoop &varLoop : loopFunctions) {
    if (millis() - varLoop.lastMillis >= varLoop.var["interval"].as<int>()) {
      varLoop.lastMillis = millis();

      varLoop.loopFun(varLoop.var, 1, f_LoopFun); //rowNr..

      varLoop.counter++;
      // USER_PRINTF("%s %u %u %d %d\n", varLoop->mdl->varID(var), varLoop->lastMillis, millis(), varLoop->interval, varLoop->counter);
    }
  }
}

void SysModUI::loop1s() {
  //if something changed in vloops
  ui->callVarFun("vlLoopps", UINT8_MAX, f_ValueFun);
  for (VarLoop &varLoop : loopFunctions)
    varLoop.counter = 0;
}

JsonObject SysModUI::initVar(JsonObject parent, const char * id, const char * type, bool readOnly, VarFun varFun) {
  JsonObject var = mdl->findVar(id); //sets the existing modelParentVar
  const char * modelParentId = mdl->modelParentVar["id"];
  const char * parentId = parent["id"];

  bool differentParents = modelParentId != nullptr && parentId != nullptr && strcmp(modelParentId, parentId) != 0;
  //!mdl->modelParentVar.isNull() && !parent.isNull() && mdl->modelParentVar["id"] != parent["id"];
  if (differentParents) {
    USER_PRINTF("initVar parents not equal %s: %s != %s\n", id, modelParentId, parentId);
  }

  //create new var
  if (differentParents || var.isNull()) {
    USER_PRINTF("initVar create new %s var: %s->%s\n", type, parentId, id);
    if (parent.isNull()) {
      JsonArray vars = mdl->model->as<JsonArray>();
      var = vars.add<JsonObject>();
    } else {
      if (parent["n"].isNull()) parent["n"].to<JsonArray>(); //TO!!! if parent exist and no "n" array, create it
      var = parent["n"].add<JsonObject>();
      // serializeJson(model, Serial);Serial.println();
    }
    var["id"] = JsonString(id, JsonString::Copied);
  }
  // else {
  //   USER_PRINTF("initVar Var %s->%s already defined\n", modelParentId, id);
  // }

  if (!var.isNull()) {
    if (var["type"].isNull() || var["type"] != type) {
      var["type"] = JsonString(type, JsonString::Copied);
      print->printJson("initVar set type", var);
    }

    if (var["ro"].isNull() || mdl->varRO(var) != readOnly) mdl->varRO(var, readOnly);

    mdl->varInitOrder(parent, var);

    //if varFun, add it to the list
    if (varFun) {
      //if fun already in ucFunctions then reuse, otherwise add new fun in ucFunctions
      //lambda update: when replacing typedef void(*UCFun)(JsonObject); with typedef std::function<void(JsonObject)> UCFun; this gives error:
      //  mismatched types 'T*' and 'std::function<void(ArduinoJson::V6213PB2::JsonObject)>' { return *__it == _M_value; }
      //  it also looks like functions are not added more then once anyway
      // std::vector<UCFun>::iterator itr = find(ucFunctions.begin(), ucFunctions.end(), varFun);
      // if (itr!=ucFunctions.end()) //found
      //   var["varFun"] = distance(ucFunctions.begin(), itr); //assign found function
      // else { //not found
        varFunctions.push_back(varFun); //add new function
        var["fun"] = varFunctions.size()-1;
      // }
      
      if (varFun(var, UINT8_MAX, f_LoopFun)) { //test run if it supports loop
        //no need to check if already in...
        VarLoop loop;
        loop.loopFun = varFun;
        loop.var = var;

        loopFunctions.push_back(loop);
        var["loopFun"] = loopFunctions.size()-1;
        // USER_PRINTF("iObject loopFun %s %u %u %d %d\n", mdl->varID(var));
      }
    }
  }
  else
    USER_PRINTF("initVar could not find or create var %s with %s\n", id, type);

  return var;
}

void SysModUI::processJson(JsonVariant json) {
  if (json.is<JsonObject>()) //should be
  {
     //varFun adds object elements to json which would be processed in the for loop. So we freeze the original pairs in a vector and loop on this
    std::vector<JsonPair> pairs;
    for (JsonPair pair : json.as<JsonObject>()) { //iterate json elements
      pairs.push_back(pair);
    }

    for (JsonPair pair : pairs) { //iterate json elements
      const char * key = pair.key().c_str();
      JsonVariant value = pair.value();

      // commands
      if (pair.key() == "v") { //when called from jsonHandler
        // do nothing as it is no real var but the verbose command of WLED
        USER_PRINTF("processJson v type %s\n", pair.value().as<String>().c_str());
      }
      else if (pair.key() == "view" || pair.key() == "canvasData" || pair.key() == "theme") { //save the chosen view in System (see index.js)
        JsonObject var = mdl->findVar("System");
        USER_PRINTF("processJson %s v:%s n: %d s:%s\n", pair.key().c_str(), pair.value().as<String>().c_str(), var.isNull(), mdl->varID(var));
        var[JsonString(key, JsonString::Copied)] = JsonString(value, JsonString::Copied); //this is needed as key can become a dangling pointer
        // json.remove(key); //key should stay as all clients use this to perform the changeHTML action
      }
      else if (pair.key() == "addRow" || pair.key() == "delRow") {
        if (value.is<JsonObject>()) {
          JsonObject command = value;
          JsonObject var = mdl->findVar(command["id"]);
          stackUnsigned8 rowNr = command["rowNr"];
          USER_PRINTF("processJson %s - %s [%d]\n", key, value.as<String>().c_str(), rowNr);

          //first remove the deleted row both on server and on client(s)
          if (pair.key() == "delRow") {
            mdl->varRemoveValuesForRow(var, rowNr);
            print->printJson("deleted rows", var);
            web->sendResponseObject(); //async response //trigger receiveData->delRow
          }

          if (callVarFun(var, rowNr, pair.key() == "addRow"?f_AddRow:f_DelRow)) {
            web->sendResponseObject(); //async response
          }
        }
        json.remove(key); //key processed we don't need the key in the response
      }
      else if (pair.key() == "uiFun") { //JsonString can do ==
        //find the select var and collect it's options...
        if (value.is<JsonArray>()) { //should be
          for (JsonVariant varInArray: value.as<JsonArray>()) {
            JsonObject var = mdl->findVar(varInArray); //value is the id
            if (!var.isNull()) {
              callVarFun(var, UINT8_MAX, f_UIFun);
              //sendDataWs done in caller of processJson
            }
            else
              USER_PRINTF("dev processJson Command %s var %s not found\n", key, varInArray.as<String>().c_str());
          }
        } else
          USER_PRINTF("dev processJson value not array? %s %s\n", key, value.as<String>().c_str());
        json.remove(key); //key processed we don't need the key in the response
      } 
      else if (!value.isNull()) { // {"varid": {"value":value}} or {"varid": value}

        JsonVariant newValue;
        if (value["value"].isNull()) // if no explicit value field (e.g. jsonHandler)
          newValue = value;
        else
          newValue = value["value"]; //use the value field

        //check if we deal with multiple rows (from table type)
        char * rowNrC = strtok((char *)key, "#");
        if (rowNrC != NULL ) {
          key = rowNrC;
          rowNrC = strtok(NULL, " ");
        }
        stackUnsigned8 rowNr = rowNrC?atoi(rowNrC):UINT8_MAX;

        JsonObject var = mdl->findVar(key);

        USER_PRINTF("processJson var %s[%d] %s -> %s\n", key, rowNr, var["value"].as<String>().c_str(), newValue.as<String>().c_str());

        if (!var.isNull())
        {
          //a button never sets the value
          if (var["type"] == "button") { //button always
            ui->callVarFun(var, rowNr, f_ChangeFun);
            if (rowNr != UINT8_MAX) web->getResponseObject()[mdl->varID(var)]["rowNr"] = rowNr;
          }
          else {
            if (newValue.is<const char *>())
              mdl->setValue(var, JsonString(newValue, JsonString::Copied), rowNr);
            else if (newValue.is<Coord3D>()) //otherwise it will be treated as JsonObject and toJson / fromJson will not be triggered!!!
              mdl->setValue(var, newValue.as<Coord3D>(), rowNr);
            else
              mdl->setValue(var, newValue, rowNr);
          }
          // json.remove(key); //key / var["id"] processed we don't need the key in the response
        }
        else
          USER_PRINTF("dev Object %s[%d] not found\n", key, rowNr);
      } 
      else {
        USER_PRINTF("dev processJson command not recognized k:%s v:%s\n", key, value.as<String>().c_str());
      }
    } //for json pairs
  }
}