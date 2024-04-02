/*
   @title     StarMod
   @file      SysModModel.h
   @date      20240228
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright © 2024 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
#include "SysModule.h"
#include "SysModPrint.h"
#include "SysModWeb.h"
#include "SysModules.h" //isConnected

typedef std::function<void(JsonObject)> FindFun;
typedef std::function<void(JsonObject, size_t)> ChangeFun;

struct Coord3D {
  int x;
  int y;
  int z;

  // Coord3D() {
  //   x = 0;
  //   y = 0;
  //   z = 0;
  // }
  // Coord3D(unsigned16 x, unsigned16 y, unsigned16 z) {
  //   this->x = x;
  //   this->y = y;
  //   this->z = y;
  // }

  //comparisons
  bool operator!=(Coord3D rhs) {
    // USER_PRINTF("Coord3D compare%d %d %d %d %d %d\n", x, y, z, rhs.x, rhs.y, rhs.z);
    // return x != rhs.x || y != rhs.y || z != rhs.z;
    return !(*this==rhs);
  }
  bool operator==(Coord3D rhs) {
    return x == rhs.x && y == rhs.y && z == rhs.z;
  }
  bool operator>=(Coord3D rhs) {
    return x >= rhs.x && y >= rhs.y && z >= rhs.z;
  }
  bool operator<=(Coord3D rhs) {
    return x <= rhs.x && y <= rhs.y && z <= rhs.z;
  }
  bool operator<(Coord3D rhs) {
    return x < rhs.x && y < rhs.y && z < rhs.z;
  }
  bool operator>=(uint16_t rhs) {
    return x >= rhs && y >= rhs && z >= rhs;
  }

  //assignments
  Coord3D operator=(Coord3D rhs) {
    // USER_PRINTF("Coord3D assign %d,%d,%d\n", rhs.x, rhs.y, rhs.z);
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
    return *this;
  }
  Coord3D operator-=(Coord3D rhs) {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }
  Coord3D operator/=(Coord3D rhs) {
    if (rhs.x) x /= rhs.x;
    if (rhs.y) y /= rhs.y;
    if (rhs.z) z /= rhs.z;
    return *this;
  }
  //Minus / delta (abs)
  Coord3D operator-(Coord3D rhs) {
    Coord3D result;
    // result.x = x > rhs.x? x - rhs.x : rhs.x - x;
    // result.y = y > rhs.y? y - rhs.y : rhs.y - y;
    // result.z = z > rhs.z? z - rhs.z : rhs.z - z;
    result.x = x - rhs.x;
    result.y = y - rhs.y;
    result.z = z - rhs.z;
    return result;
  }
  Coord3D operator+(Coord3D rhs) {
    return Coord3D{unsigned16(x + rhs.x), unsigned16(y + rhs.y), unsigned16(z + rhs.z)};
  }
  Coord3D operator/(Coord3D rhs) {
    return Coord3D{unsigned16(x / rhs.x), unsigned16(y / rhs.y), unsigned16(z / rhs.z)};
  }
  Coord3D operator%(Coord3D rhs) {
    return Coord3D{unsigned16(x % rhs.x), unsigned16(y % rhs.y), unsigned16(z % rhs.z)};
  }
  Coord3D minimum(Coord3D rhs) {
    return Coord3D{min(x, rhs.x), min(y, rhs.y), min(z, rhs.z)};
  }
  Coord3D maximum(Coord3D rhs) {
    return Coord3D{max(x, rhs.x), max(y, rhs.y), max(z, rhs.z)};
  }
  Coord3D operator*(unsigned8 rhs) {
    return Coord3D{unsigned16(x * rhs), unsigned16(y * rhs), unsigned16(z * rhs)};
  }
  Coord3D operator/(unsigned8 rhs) {
    return Coord3D{unsigned16(x / rhs), unsigned16(y / rhs), unsigned16(z / rhs)};
  }
  //move the coordinate one step closer to the goal, if difference in coordinates (used in GenFix)
  Coord3D advance(Coord3D goal) {
    if (x != goal.x) x += (x<goal.x)?1:-1;
    if (y != goal.y) y += (y<goal.y)?1:-1;
    if (z != goal.z) z += (z<goal.z)?1:-1;
    return *this;
  }
  unsigned distance(Coord3D rhs) {
    Coord3D delta = (*this-rhs);
    return sqrt((delta.x)*(delta.x) + (delta.y)*(delta.y) + (delta.z)*(delta.z));
  }
};

//used to sort keys of jsonobjects
struct ArrayIndexSortValue {
  size_t index;
  unsigned32 value;
};

//https://arduinojson.org/news/2021/05/04/version-6-18-0/
namespace ArduinoJson {
  template <>
  struct Converter<Coord3D> {
    static bool toJson(const Coord3D& src, JsonVariant dst) {
      // JsonObject obj = dst.to<JsonObject>();
      dst["x"] = src.x;
      dst["y"] = src.y;
      dst["z"] = src.z;
      // USER_PRINTF("Coord3D toJson %d,%d,%d -> %s\n", src.x, src.y, src.z, dst.as<String>().c_str());
      return true;
    }

    static Coord3D fromJson(JsonVariantConst src) {
      // USER_PRINTF("Coord3D fromJson %s\n", src.as<String>().c_str());
      return Coord3D{src["x"], src["y"], src["z"]};
    }

    static bool checkJson(JsonVariantConst src) {
      return src["x"].is<unsigned16>() && src["y"].is<unsigned16>() && src["z"].is<unsigned16>();
    }
  };
}

// https://arduinojson.org/v7/api/jsondocument/
struct RAM_Allocator: ArduinoJson::Allocator {
  void* allocate(size_t size) override {
    if (psramFound()) return ps_malloc(size); // use PSRAM if it exists
    else              return malloc(size);    // fallback
    // return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
  }
  void deallocate(void* pointer) override {
    free(pointer);
    // heap_caps_free(pointer);
  }
  void* reallocate(void* ptr, size_t new_size) override {
    if (psramFound()) return ps_realloc(ptr, new_size); // use PSRAM if it exists
    else              return realloc(ptr, new_size);    // fallback
    // return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
  }
};



class SysModModel:public SysModule {

public:

  RAM_Allocator allocator;
  JsonDocument *model = nullptr;

  JsonObject modelParentVar;

  bool doWriteModel = false;

  unsigned8 setValueRowNr = UINT8_MAX;
  unsigned8 getValueRowNr = UINT8_MAX;

  SysModModel();
  void setup();
  void loop();
  
  //scan all vars in the model and remove vars where var["o"] is negative or positive, if ro then remove ro values
  void cleanUpModel(JsonObject parent = JsonObject(), bool oPos = true, bool ro = false);

  //sets the value of var with id
  template <typename Type>
  JsonObject setValue(const char * id, Type value, unsigned8 rowNr = UINT8_MAX) {
    JsonObject var = findVar(id);
    if (!var.isNull()) {
      return setValue(var, value, rowNr);
    }
    else {
      USER_PRINTF("setValue Var %s not found\n", id);
      return JsonObject();
    }
  }

  template <typename Type>
  JsonObject setValue(JsonObject var, Type value, unsigned8 rowNr = UINT8_MAX) {

    bool changed = false;

    if (rowNr == UINT8_MAX) { //normal situation
      if (var["value"].isNull() || var["value"].as<Type>() != value) { //const char * will be JsonString so comparison works
        JsonString oldValue = JsonString(var["value"], JsonString::Copied);
        var["value"] = value;
        //trick to remove null values
        if (var["value"].isNull() || var["value"].as<unsigned16>() == UINT16_MAX) {
          var.remove("value");
          if (oldValue.size()>0)
            USER_PRINTF("dev setValue value removed %s %s\n", varID(var), oldValue.c_str()); //old value
        }
        else {
          USER_PRINTF("setValue changed %s %s -> %s\n", varID(var), oldValue.c_str(), var["value"].as<String>().c_str()); //old value
          web->addResponse(var["id"], "value", var["value"]);
          changed = true;
        }
      }
    }
    else {
      //if we deal with multiple rows, value should be an array, if not we create one

      if (var["value"].isNull() || !var["value"].is<JsonArray>()) {
        USER_PRINTF("setValue var %s[%d] value %s not array, creating\n", varID(var), rowNr, var["value"].as<String>().c_str());
        // print->printJson("setValueB var %s value %s not array, creating", id, var["value"].as<String>().c_str());
        var["value"].to<JsonArray>();
      }

      if (var["value"].is<JsonArray>()) {
        JsonArray valueArray = var["value"].as<JsonArray>();
        //set the right value in the array (if array did not contain values yet, all values before rownr are set to false)
        bool notSame = true; //rowNr >= size

        if (rowNr < valueArray.size())
          notSame = valueArray[rowNr].isNull() || valueArray[rowNr].as<Type>() != value;

        if (notSame) {
          // if (rowNr >= valueArray.size())
          //   USER_PRINTF("notSame %d %d\n", rowNr, valueArray.size());
          valueArray[rowNr] = value; //if valueArray[<rowNr] not exists it will be created
          // USER_PRINTF("  assigned %d %d %s\n", rowNr, valueArray.size(), valueArray[rowNr].as<String>().c_str());
          web->addResponse(var["id"], "value", var["value"]); //send the whole array to UI as response is in format value:<value> !!
          changed = true;
        }
      }
      else {
        USER_PRINTF("setValue %s could not create value array\n", varID(var));
      }
    }

    if (changed) callChangeFun(var, rowNr);
    
    return var;
  }

  //Set value with argument list with rowNr (rowNr cannot have a default)
  JsonObject setValueV(const char * id, unsigned8 rowNr = UINT8_MAX, const char * format = nullptr, ...) {
    va_list args;
    va_start(args, format);

    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    USER_PRINTF("setValueV %s[%d] = %s\n", id, rowNr, value);
    return setValue(id, JsonString(value, JsonString::Copied), rowNr);
  }

  void setUIValueV(const char * id, const char * format, ...) {
    va_list args;
    va_start(args, format);

    char value[128];
    vsnprintf(value, sizeof(value)-1, format, args);

    va_end(args);

    //no print
    web->addResponse(id, "value", JsonString(value, JsonString::Copied)); //setValue not necessary
  }

  JsonVariant getValue(const char * id, unsigned8 rowNr = UINT8_MAX) {
    JsonObject var = findVar(id);
    if (!var.isNull()) {
      return getValue(var, rowNr);
    }
    else {
      // USER_PRINTF("getValue: Var %s does not exist!!\n", id);
      return JsonVariant();
    }
  }
  JsonVariant getValue(JsonObject var, unsigned8 rowNr = UINT8_MAX) {
    if (var["value"].is<JsonArray>()) {
      JsonArray valueArray = var["value"].as<JsonArray>();
      if (rowNr == UINT8_MAX) rowNr = getValueRowNr;
      if (rowNr != UINT8_MAX && rowNr < valueArray.size())
        return valueArray[rowNr];
      else if (valueArray.size())
        return valueArray[0];
      else {
        USER_PRINTF("dev getValue no array or rownr wrong %s %s %d\n", varID(var), var["value"].as<String>().c_str(), rowNr);
        return JsonVariant();
      }
    }
    else
      return var["value"];
  }

  //returns the var defined by id (parent to recursively call findVar)
  JsonObject findVar(const char * id, JsonArray parent = JsonArray());
  void findVars(const char * id, bool value, FindFun fun, JsonArray parent = JsonArray());

  //recursively add values in  a variant
  void varToValues(JsonObject var, JsonArray values);

  //run the change function and send response to all? websocket clients
  void callChangeFun(JsonObject var, unsigned8 rowNr = UINT8_MAX);

  //pseudo VarObject: public JsonObject functions
  const char * varID(JsonObject var) {return var["id"];}

  bool varRO(JsonObject var) {return var["ro"];}
  void varRO(JsonObject var, bool value) {var["ro"] = value;}

  JsonArray varChildren(const char * id) {return varChildren(findVar(id));}
  JsonArray varChildren(JsonObject var) {return var["n"];}

  JsonArray varValArray(JsonObject var) {if (var["value"].is<JsonArray>()) return var["value"]; else return JsonArray(); }

  int varOrder(JsonObject var) {return var["o"];}
  void varOrder(JsonObject var, int value) {var["o"] = value;}
  
  void varInitOrder(JsonObject parent, JsonObject var) {
        //set order. make order negative to check if not obsolete, see cleanUpModel
    if (varOrder(var) >= 1000) //predefined! (modules) - positive as saved in model.json
      varOrder(var, -varOrder(var)); //leave the order as is
    else {
      if (!parent.isNull() && varOrder(parent) >= 0) // if checks on the parent already done so vars added later, e.g. controls, will be autochecked
        varOrder(var, varCounter++); //redefine order
      else
        varOrder(var, -varCounter++); //redefine order
    }
  }
  void varSetDefaultOrder(JsonObject var, int value) {if (varOrder(var) > -1000) varOrder(var, - value); } //set default order (in range >=1000). Don't use auto generated order as order can be changed in the ui (WIP)
  
  //recursively remove all value[rowNr] from children of var
  void varRemoveValuesForRow(JsonObject var, unsigned8 rowNr) {
    for (JsonObject childVar: varChildren(var)) {
      JsonArray valArray = varValArray(childVar);
      if (!valArray.isNull()) {
        valArray.remove(rowNr);
        //recursive
        varRemoveValuesForRow(childVar, rowNr);
      }
    }
  }

  void varPreDetails(JsonObject var, unsigned8 rowNr = UINT8_MAX) {
    for (JsonObject var: varChildren(var)) { //for all controls
      if (varOrder(var) >= 0) { //post init
        varOrder(var, -varOrder(var)); // set all negative
      }
    }
    setValueRowNr = rowNr;
    print->printJson("varPreDetails post", var);
  }

  void varPostDetails(JsonObject var, unsigned8 rowNr) {

    setValueRowNr = UINT8_MAX;
    if (rowNr != UINT8_MAX) {

      print->printJson("varPostDetails pre", var);

      //check if post init added: parent is already >=0
      if (varOrder(var) >= 0) {
        for (JsonArray::iterator childVar=varChildren(var).begin(); childVar!=varChildren(var).end(); ++childVar) { //use iterator to make .remove work!!!
          JsonArray valArray = varValArray(*childVar);
          if (!valArray.isNull())
          {

            if (varOrder(*childVar) < 0) { //if not updated
              valArray[rowNr] = (char*)0; // set element in valArray to 0

              USER_PRINTF("varPostDetails %s[%d] to null\n", varID(var), rowNr);
              // setValue(var, -99, rowNr); //set value -99
              varOrder(*childVar, -varOrder(*childVar)); //make positive again
              //if some values in array are not -99
            }

            //if all values null, remove value
            bool allNull = true;
            for (JsonVariant element: valArray) {
              if (!element.isNull())
                allNull = false;
            }
            if (allNull) {
              print->printJson("remove allnulls", *childVar);
              varChildren(var).remove(childVar);
            }
          }
          else {
            print->printJson("remove non valArray", *childVar);
            varChildren(var).remove(childVar);
          }

        }
      } //if new added
      print->printJson("varPostDetails post", var);

      web->addResponse("details", "rowNr", rowNr);
    }

    //post update details
    web->addResponse("details", "var", var);
  }

  unsigned8 varLinearToLogarithm(JsonObject var, unsigned8 value) {
    if (value == 0) return 0;

    float minp = var["min"].isNull()?var["min"]:0;
    float maxp = var["max"].isNull()?var["max"]:255;

    // The result should be between 100 an 10000000
    float minv = minp?log(minp):0;
    float maxv = log(maxp);

    // calculate adjustment factor
    float scale = (maxv-minv) / (maxp-minp);

    return round(exp(minv + scale*((float)value-minp)));
  }


private:
  bool doShowObsolete = false;
  bool cleanUpModelDone = false;
  int varCounter = 1; //start with 1 so it can be negative, see var["o"]

};

extern SysModModel *mdl;