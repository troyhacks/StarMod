/*
   @title     StarMod
   @file      SysModUI.h
   @date      20240227
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"
#include "SysModModel.h"

enum FunIDs
{
  f_ValueFun,
  f_UIFun,
  f_ChangeFun,
  f_LoopFun,
  f_AddRow,
  f_DelRow,
  f_count
};

// https://stackoverflow.com/questions/59111610/how-do-you-declare-a-lambda-function-using-typedef-and-then-use-it-by-passing-to
typedef std::function<unsigned8(JsonObject, unsigned8, unsigned8)> VarFun;

struct VarLoop {
  JsonObject var;
  VarFun loopFun;
  unsigned long lastMillis = 0;
  unsigned long counter = 0;
};

class SysModUI: public SysModule {

public:
  bool dashVarChanged = false; //tbd: move mechanism to UserModInstances as there it will be used
  std::vector<VarFun> varFunctions;

  SysModUI();

  //serve index.htm
  void setup();

  void loop();
  void loop1s();

  //order: order%4 determines the column (WIP)
  JsonObject initAppMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndUpdate<const char *>(parent, id, "appmod", (const char *)nullptr);
    if (order) mdl->varSetDefaultOrder(var, order + 1000);
    return var;
  }
  JsonObject initSysMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndUpdate<const char *>(parent, id, "sysmod", (const char *)nullptr);
    if (order) mdl->varSetDefaultOrder(var, order + 1000);
    return var;
  }
  JsonObject initUserMod(JsonObject parent, const char * id, int order = 0) {
    JsonObject var = initVarAndUpdate<const char *>(parent, id, "usermod", (const char *)nullptr);
    if (order) mdl->varSetDefaultOrder(var, order + 1000);
    return var;
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "table", value, 0, 0, readOnly, varFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, unsigned16 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "text", value, 0, max, readOnly, varFun);
  }

  JsonObject initFile(JsonObject parent, const char * id, const char * value = nullptr, unsigned16 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "file", value, 0, max, readOnly, varFun);
  }
  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, unsigned8 max = 32, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "password", value, 0, 0, readOnly, varFun);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "number", value, min, max, readOnly, varFun);
  }

  JsonObject initPin(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "pin", value, 0, NUM_DIGITAL_PINS, readOnly, varFun);
  }

  JsonObject initProgress(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "progress", value, min, max, readOnly, varFun);
  }

  JsonObject initCoord3D(JsonObject parent, const char * id, Coord3D value = {UINT16_MAX, UINT16_MAX, UINT16_MAX}, int min = 0, int max = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<Coord3D>(parent, id, "coord3D", value, min, max, readOnly, varFun);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value = UINT16_MAX, int min = 0, int max = 255, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "range", value, min, max, readOnly, varFun);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "canvas", value, 0, 0, readOnly, varFun);
  }

  //supports 3 state value: if UINT16_MAX it is indeterminated
  JsonObject initCheckBox(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "checkbox", value, 0, 0, readOnly, varFun);
  }

  //a button never gets a value
  JsonObject initButton(JsonObject parent, const char * id, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<bool>(parent, id, "button", false, 0, 0, readOnly, varFun);
  }

  //int value ?
  JsonObject initSelect(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "select", value, 0, 0, readOnly, varFun);
  }

  JsonObject initIP(JsonObject parent, const char * id, int value = UINT16_MAX, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "ip", value, 0, 255, readOnly, varFun);
  }

  //WIP pointer values
  JsonObject initSelect(JsonObject parent, const char * id, unsigned8 * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<unsigned8>(parent, id, "select", value, 0, 0, readOnly, varFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "textarea", value, 0, 0, readOnly, varFun);
  }

  JsonObject initURL(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = false, VarFun varFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "url", value, 0, 0, readOnly, varFun);
  }

  //WIP pointer values
  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type * value, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    // JsonObject var = initVar(parent, id, type, readOnly, varFun);
    JsonObject var = initVarAndUpdate(parent, id, type, *value, min, max, readOnly, varFun);
    // var["p"] = (const char *)value; //store pointer!
    return var;
  }

  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type value, int min = 0, int max = 255, bool readOnly = true, VarFun varFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, varFun);
    if (min) var["min"] = min;
    if (max && max != UINT16_MAX) var["max"] = max;

    bool valueNeedsUpdate = false;
    if (strcmp(type, "button") != 0) { //button never gets a value
      if (var["value"].isNull()) {
        valueNeedsUpdate = true;
        // print->printJson("initVarAndUpdate varFun value is null", var);
      } else if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = var["value"].as<JsonArray>();
        if (mdl->setValueRowNr != UINT8_MAX) { // if var in table
          if (mdl->setValueRowNr >= valueArray.size())
            valueNeedsUpdate = true;
          else if (valueArray[mdl->setValueRowNr].isNull())
            valueNeedsUpdate = true;
        }
      }
    }

    if (valueNeedsUpdate) {
      bool valueFunExists = false;
      if (varFun) {
        valueFunExists = varFun(var, mdl->setValueRowNr, f_ValueFun);
      }
      if (!valueFunExists) { //setValue provided (if not null)
        mdl->setValue(var, value, mdl->setValueRowNr); //does changefun if needed, if var in table, update the table row
      }
    }
    else { //do changeFun on existing value
      //no call of chFun for buttons otherwise all buttons will be fired which is highly undesirable
      if (strcmp(type,"button") != 0 && varFun ) { //!isPointer because 0 is also a value then && (!isPointer || value)
        bool changeFunExists = false;
        if (var["value"].is<JsonArray>()) {
          int rowNr = 0;
          for (JsonVariant val:var["value"].as<JsonArray>()) {
            changeFunExists |= varFun(var, rowNr++, f_ChangeFun);
          }
        }
        else {
          changeFunExists = varFun(var, UINT8_MAX, f_ChangeFun); //if no rowNr use rowNr 0
        }

        if (changeFunExists)
          USER_PRINTF("initVarAndUpdate chFun init %s[x] <- %s\n", mdl->varID(var), var["value"].as<String>().c_str());
      }
    }

    return var;
  }

  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, VarFun varFun = nullptr);

  bool callVarFun(const char * varID, unsigned8 rowNr = UINT8_MAX, unsigned8 funType = f_ChangeFun) {
    JsonObject var = mdl->findVar(varID);
    return callVarFun(var, rowNr, funType);
  }

  bool callVarFun(JsonObject var, unsigned8 rowNr = UINT8_MAX, unsigned8 funType = f_ValueFun) {
    bool result = false;

    if (!var["fun"].isNull()) {//isNull needed here!
      size_t funNr = var["fun"];
      if (funNr < varFunctions.size()) {
        result = varFunctions[funNr](var, rowNr, funType);
        // if (result) { //send rowNr = 0 if no rowNr
        //   //only print vars with a value
        //   if (!var["value"].isNull() && funType != f_ValueFun) {
          // if (var["id"] == "fixFirst") {
        //     USER_PRINTF("%sFun %s", funType==f_ValueFun?"val":funType==f_UIFun?"ui":funType==f_ChangeFun?"ch":funType==f_AddRow?"add":funType==f_DelRow?"del":"other", mdl->varID(var));
        //     if (rowNr != UINT8_MAX)
        //       USER_PRINTF("[%d] = %s\n", rowNr, var["value"][rowNr].as<String>().c_str());
        //     else
        //       USER_PRINTF(" = %s\n", var["value"].as<String>().c_str());
        //   }
        // }
      }
      else    
        USER_PRINTF("dev callVarFun function nr %s outside bounds %d >= %d\n", mdl->varID(var), funNr, varFunctions.size());
    }

    //for ro variables, call valueFun to add also the value in responseDoc (as it is not stored in the model)
    if (funType == f_UIFun && mdl->varRO(var)) {
      callVarFun(var, rowNr, f_ValueFun);
    }

    return result;
  }

  // assuming ui->callVarFun(varID, UINT8_MAX, f_UIFun); has been called before
  uint8_t selectOptionToValue(const char *varID, const char *label) {
    JsonArray options = web->getResponseObject()[varID]["options"];
    // USER_PRINTF("selectOptionToValue fileName %s %s\n", label, options[0].as<String>().c_str());
    uint8_t value = 0;
    for (JsonVariant option: options) {
      // USER_PRINTF("selectOptionToValue fileName2 %s %s\n", label, option.as<String>().c_str());
      if (strstr(option, label) != nullptr) //if label part of value
        return value;
      value++;
    }
    return UINT8_MAX; //not found
  }

  //interpret json and run commands or set values like deserializeJson / deserializeState / deserializeConfig
  void processJson(JsonVariant json); //must be Variant, not object for jsonhandler

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  // void processUiFun(const char * id);

  void setLabel(JsonObject var, const char * text) {
    web->addResponse(var["id"], "label", text);
  }
  void setComment(JsonObject var, const char * text) {
    web->addResponse(var["id"], "comment", text);
  }
  JsonArray setOptions(JsonObject var) {
    return web->addResponseA(var["id"], "options");
  }
  //return the options from valueFun (don't forget to clear responseObject)
  JsonArray getOptions(JsonObject var) {
    callVarFun(var, UINT8_MAX, f_UIFun); //tricky: fills the options table
    return web->getResponseObject()[mdl->varID(var)]["options"];
  }
  void clearOptions(JsonObject var) {
    web->getResponseObject().remove(mdl->varID(var));
  }

private:
  std::vector<VarLoop> loopFunctions;

};

extern SysModUI *ui;