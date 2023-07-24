#pragma once
#include "Module.h"
#include "LittleFS.h"

#include "SysModPrint.h"
#include <vector>

class SysModFiles:public Module {

public:
  SysModFiles();
  void setup();
  void loop();

  // void handleUpload(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);

  bool remove(const char * path);

  size_t usedBytes();

  size_t totalBytes();

  File open(const char * path, const char * mode, const bool create = false);

  void filesChange();

  //get the file names and size in an array
  static void dirToJson(JsonArray array, bool nameOnly = false, const char * filter = nullptr);

  //get back the name of a file based on the sequence
  bool seqNrToName(char * fileName, size_t seqNr);

  //reads file and load it in json
  //name is copied from WLED but better to call it readJsonFrom file
  bool readObjectFromFile(const char* path, JsonDocument* dest);

  //write json into file
  //name is copied from WLED but better to call it readJsonFrom file
  //candidate for deletion as taken over by LazyJsonRDWS
  // bool writeObjectToFile(const char* path, JsonDocument* dest);

  //remove files meeting filter condition, if no filter, all, if reverse then all but filter
  void removeFiles(const char * filter = nullptr, bool reverse = false);

  bool readFile(const char * path);

private:
  static bool filesChanged;

};

static SysModFiles *files;

//Lazy Json Read Deserialize Write Serialize (write / serialize not implemented yet)
//ArduinoJson won't work on very large LedFix.json, this does
//only support what is currently needed: read / deserialize uint8/16/char object elements (arrays not yet)
class LazyJsonRDWS {

  public:
  bool foundAll = false;

  LazyJsonRDWS(const char * path, const char * mode = "r") {
    print->print("LazyJsonRDWS constructing %s %s\n", path, mode);
    f = files->open(path, mode);
    if (!f)
      print->print("LazyJsonRDWS open %s for %s failed", path, mode);
  }

  ~LazyJsonRDWS() {
    print->print("LazyJsonRDWS destructing\n");
    f.close();
  }

  void addExclusion(const char * key) {
    charList.push_back((char *)key);
  }

  //serializeJson
  void writeJsonDocToFile(JsonDocument* dest) {
    writeJsonVariantToFile(dest->as<JsonVariant>());
    files->filesChange();
  }

  //look for uint8 object
  void lookFor(const char * id, uint8_t * value) {
    // const char *p = (const char*)&value; //pointer trick
    uint8List.push_back(value);
    addToObjects(id, "uint8", uint8List.size()-1);
  }

  //look for uint16 object
  void lookFor(const char * id, uint16_t * value) {
    // const char *p = (const char*)&value; //pointer trick
    uint16List.push_back(value);
    addToObjects(id, "uint16", uint16List.size()-1);
  }

  //look for char object
  void lookFor(const char * id, char * value) {
    // const char *p = (const char*)&value; //pointer trick
    charList.push_back(value);
    addToObjects(id, "char", charList.size()-1);
  }

  //reads from file until all objects have been found (then stops reading)
  //returns false if not all objects to look for are found
  bool deserialize() {
    f.read(&character, sizeof(byte));
    while (f.available() && !foundAll)
      next();
    if (!foundAll)
      print->print("LazyJsonRDWS Not all objects looked for where found\n");
    return foundAll;
  }

private:
  struct ObjectDetails {
    const char * id;
    const char * type;
    size_t index;
  };

  File f;
  uint8_t character;
  std::vector<ObjectDetails> objects;
  std::vector<uint8_t *> uint8List;
  std::vector<uint16_t *> uint16List;
  std::vector<char *> charList;
  char lastObject[100] = "";
  size_t foundNumber = 0;

  void addToObjects(const char * id, const char * type, size_t index) {
    ObjectDetails object;
    object.id = id;
    object.type = type;
    object.index = index;
    objects.push_back(object);
  }

  void next() {
    if (character=='{') {
      // print->print("Object %c\n", character);
      print->print("Object [%s]\n", lastObject);
      strcpy(lastObject, "");
      f.read(&character, sizeof(byte));
    }
    else if (character=='[') {
      // print->print("Array %c\n", character);
      print->print("Array [%s]\n", lastObject);
      strcpy(lastObject, "");
      f.read(&character, sizeof(byte));
    }
    else if (character=='"') { //parse String
      char value[100] = "";
      f.readBytesUntil('"', value, sizeof(value));
    
      if (strcmp(lastObject, "") == 0) {
        // print->print("Element [%s]\n", value);
        strcpy(lastObject, value);
      }
      else {
        print->print("String %s: [%s]\n", lastObject, value);
        check(value);
        strcpy(lastObject, "");
      }

      f.read(&character, sizeof(byte));
    }
    else if (isDigit(character)) { //parse number
      char value[100] = "";

      size_t len = 0;
      //readuntil not number
      while (isDigit(character)) {
        // print->print("%c", character);
        value[len++] = character;
        f.read(&character, sizeof(byte));
      }
      value[len++] = '\0';

      print->print("Number %s: [%s]\n", lastObject, value);

      check(value);
  
      strcpy(lastObject, "");
    }
    else if (character==':') {
      // print->print("semicolon %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character==',') {
      // print->print("sep %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character==']') {
      // print->print("close %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character=='}') {
      print->print("close %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character=='\n') { //skip new lines
      // print->print("skip newline \n");
      f.read(&character, sizeof(byte));
    }
    else
      print->print("Element don't know %c\n", character);
  } //next

  void check(char *value) {
    for (std::vector<ObjectDetails>::iterator object=objects.begin(); object!=objects.end(); ++object) {
      if (strcmp(object->id, lastObject)==0) {
        print->print("LazyJsonRDWS found %s: %s\n", lastObject, value);
        if (strcmp(object->type, "uint8") ==0) *uint8List[object->index] = atoi(value);
        if (strcmp(object->type, "uint16") ==0) *uint16List[object->index] = atoi(value);
        if (strcmp(object->type, "char") ==0) strcpy(charList[object->index], value);
        foundNumber++;
      }
    }
    foundAll = foundNumber == objects.size();
    if (foundAll)
      print->print("Hooray, LazyJsonRDWS found all what we were looking for, no further search needed\n");
  }

  //writeJsonVariantToFile calls itself recursively until whole json document has been parsed
  void writeJsonVariantToFile(JsonVariant variant) {
    if (variant.is<JsonObject>()) {
      f.printf("{");
      char sep[3] = "";
      for (JsonPair pair: variant.as<JsonObject>()) {
        bool found = false;
        for (char *el:charList) {
          if (strcmp(el, pair.key().c_str())==0) {
            found = true;
            break;
          }
        }
        // std::vector<char *>::iterator itr = find(charList.begin(), charList.end(), pair.key().c_str());
        if (!found) { //not found
          f.printf("%s\"%s\":", sep, pair.key().c_str());
          strcpy(sep,",");
          writeJsonVariantToFile(pair.value());
        }
      }
      f.printf("}");
    }
    else if (variant.is<JsonArray>()) {
      f.printf("[");
      char sep[3] = "";
      for (JsonVariant variant2: variant.as<JsonArray>()) {
        f.print(sep);
        strcpy(sep,",");
        writeJsonVariantToFile(variant2);
      }      
      f.printf("]");
    }
    else if (variant.is<const char *>()) {
      f.printf("\"%s\"", variant.as<const char *>());      
    }
    else if (variant.is<int>()) {
      f.printf("%d", variant.as<int>());      
    }
    else if (variant.is<bool>()) {
      f.printf("%s", variant.as<bool>()?"true":"false");      
    }
    else
      print->print("%s not supported", variant.as<String>());
  }

};