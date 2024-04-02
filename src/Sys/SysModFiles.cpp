/*
   @title     StarMod
   @file      SysModFiles.cpp
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModFiles.h"
#include "SysModUI.h"
#include "SysModWeb.h"
#include "SysModPrint.h"
#include "SysModModel.h"

// #include <FS.h>

SysModFiles::SysModFiles() :SysModule("Files") {
  if (!LittleFS.begin(true)) { //true: formatOnFail
    USER_PRINTF(" An Error has occurred while mounting File system");
    USER_PRINTF(" fail\n");
    success = false;
  }
};

//setup filesystem
void SysModFiles::setup() {
  SysModule::setup();
  parentVar = ui->initSysMod(parentVar, name, 2101);

  JsonObject tableVar = ui->initTable(parentVar, "fileTbl", nullptr, false, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Files");
      ui->setComment(var, "List of files");
      return true;
    case f_AddRow:
      rowNr = fileList.size();
      USER_PRINTF("chFun addRow %s[%d] = %s\n", mdl->varID(var), rowNr, var["value"].as<String>().c_str());
      web->getResponseObject()["addRow"]["rowNr"] = rowNr;
      //add a row with all defaults
      return true;
    case f_DelRow:
      if (rowNr != UINT8_MAX && rowNr < fileList.size()) {
        const char * fileName = fileList[rowNr].name;
        USER_PRINTF("chFun delRow %s[%d] = %s %s\n", mdl->varID(var), rowNr, var["value"].as<String>().c_str(), fileName);
        this->removeFiles(fileName, false);
      }
      print->printJson(" ", var);
      return true;
    default: return false;
  }});

  ui->initText(tableVar, "flName", nullptr, 32, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < fileList.size(); rowNr++)
        mdl->setValue(var, JsonString(fileList[rowNr].name, JsonString::Copied), rowNr);
      return true;
    case f_UIFun:
      ui->setLabel(var, "Name");
      return true;
    default: return false;
  }});

  ui->initNumber(tableVar, "flSize", UINT16_MAX, 0, UINT16_MAX, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < fileList.size(); rowNr++)
        mdl->setValue(var, fileList[rowNr].size, rowNr);
      return true;
    case f_UIFun:
      ui->setLabel(var, "Size (B)");
      return true;
    default: return false;
  }});

  ui->initURL(tableVar, "flLink", nullptr, true, [this](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      for (forUnsigned8 rowNr = 0; rowNr < fileList.size(); rowNr++) {
        char urlString[32] = "file/";
        strncat(urlString, fileList[rowNr].name, sizeof(urlString)-1);
        mdl->setValue(var, JsonString(urlString, JsonString::Copied), rowNr);
      }
      return true;
    case f_UIFun:
      ui->setLabel(var, "Show");
      return true;
    default: return false;
  }});

  ui->initFile(parentVar, "upload", nullptr, UINT16_MAX, false, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_UIFun:
      ui->setLabel(var, "Upload File");
    default: return false;
  }});

  ui->initProgress(parentVar, "drsize", UINT16_MAX, 0, files->totalBytes(), true, [](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    case f_ValueFun:
      mdl->setValue(var, files->usedBytes());
      return true;
    case f_UIFun:
      ui->setLabel(var, "FS Size");
      return true;
    case f_ChangeFun:
      var["max"] = files->totalBytes(); //makes sense?
      web->addResponseV(var["id"], "comment", "%d / %d B", files->usedBytes(), files->totalBytes());
      return true;
    default: return false;
  }});

}

void SysModFiles::loop() {
  // SysModule::loop();

  if (filesChanged) {
    filesChanged = false;

    File root = LittleFS.open("/");
    File file = root.openNextFile();

    //repopulate file list
    fileList.clear();
    while (file) {
      FileDetails details;
      strcpy(details.name, file.name());
      details.size = file.size();
      fileList.push_back(details);
      file.close();
      file = root.openNextFile();
    }
    root.close();

    ui->callVarFun(mdl->findVar("drsize")); //valueFun

    for (JsonObject childVar: mdl->varChildren("fileTbl"))
      ui->callVarFun(childVar, UINT8_MAX, f_ValueFun);
  }
}

void SysModFiles::loop10s() {
  ui->callVarFun(mdl->findVar("drsize"));
}

bool SysModFiles::remove(const char * path) {
  USER_PRINTF("File remove %s\n", path);
  return LittleFS.remove(path);
  filesChanged = true;
}

size_t SysModFiles::usedBytes() {
  return LittleFS.usedBytes();
}

size_t SysModFiles::totalBytes() {
  return LittleFS.totalBytes();
}

File SysModFiles::open(const char * path, const char * mode, const bool create) {
  return LittleFS.open(path, mode, create);
}

// https://techtutorialsx.com/2019/02/24/esp32-arduino-listing-files-in-a-spiffs-file-system-specific-path/
void SysModFiles::dirToJson(JsonArray array, bool nameOnly, const char * filter) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {

    if (filter == nullptr || strstr(file.name(), filter) != nullptr) {
      if (nameOnly) {
        array.add(JsonString(file.name(), JsonString::Copied));
      }
      else {
        JsonArray row = array.add<JsonArray>();
        row.add(JsonString(file.name(), JsonString::Copied));
        row.add(file.size());
        char urlString[32] = "file/";
        strncat(urlString, file.name(), sizeof(urlString)-1);
        row.add(JsonString(urlString, JsonString::Copied));
      }
      // USER_PRINTF("FILE: %s %d\n", file.name(), file.size());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

bool SysModFiles::seqNrToName(char * fileName, size_t seqNr) {

  File root = LittleFS.open("/");
  File file = root.openNextFile();

  size_t counter = 0;
  while (file) {
    if (counter == seqNr) {
      USER_PRINTF("seqNrToName: %s %d\n", file.name(), file.size());
      root.close();
      strncat(fileName, "/", 31); //add root prefix, fileName is 32 bytes but sizeof doesn't know so cheating
      strncat(fileName, file.name(), 31);
      file.close();
      return true;
    }

    file.close();
    file = root.openNextFile();
    counter++;
  }

  root.close();
  return false;
}

bool SysModFiles::readObjectFromFile(const char* path, JsonDocument* dest) {
  // if (doCloseFile) closeFile();
  File f = open(path, "r");
  if (!f) {
    USER_PRINTF("File %s open not successful\n", path);
    return false;
  }
  else { 
    USER_PRINTF(PSTR("File %s open to read, size %d bytes\n"), path, (int)f.size());
    DeserializationError error = deserializeJson(*dest, f, DeserializationOption::NestingLimit(20)); //StarMod requires more then 10
    if (error) {
      print->printJDocInfo("readObjectFromFile", *dest);
      USER_PRINTF("readObjectFromFile deserializeJson failed with code %s\n", error.c_str());
      f.close();
      return false;
    } else {
      f.close();
      return true;
    }
  }
}

//candidate for deletion as taken over by StarModJson
// bool SysModFiles::writeObjectToFile(const char* path, JsonDocument* dest) {
//   File f = open(path, "w");
//   if (f) {
//     print->println("  success");
//     serializeJson(*dest, f);
//     f.close();
//     filesChanged = true;
//     return true;
//   } else {
//     print->println("  fail");
//     return false;
//   }
// }

void SysModFiles::removeFiles(const char * filter, bool reverse) {
  File root = LittleFS.open("/");
  File file = root.openNextFile();

  while (file) {
    if (filter == nullptr || reverse?strstr(file.name(), filter) == nullptr: strstr(file.name(), filter) != nullptr) {
      char fileName[32] = "/";
      strncat(fileName, file.name(), sizeof(fileName)-1);
      file.close(); //close otherwise not removeable
      remove(fileName);
    }
    else
      file.close();

    file = root.openNextFile();
  }

  root.close();
}

bool SysModFiles::readFile(const char * path) {
  File f = open(path, "r");
  if (f) {

    while(f.available()) {
      Serial.print((char)f.read());
    }
    Serial.println();

    f.close();
    return true;
  }
  else 
    return false;
}
