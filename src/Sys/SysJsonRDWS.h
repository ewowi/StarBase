/*
   @title     StarMod
   @file      SysJsonRDWS.h
   @date      20230729
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

//Lazy Json Read Deserialize Write Serialize (write / serialize not implemented yet)
//ArduinoJson won't work on very large LedFix.json, this does
//only support what is currently needed: read / deserialize uint8/16/char object elements (arrays not yet)
class JsonRDWS {

  public:

  JsonRDWS(const char * path, const char * mode = "r") {
    print->print("JsonRDWS constructing %s %s\n", path, mode);
    f = files->open(path, mode);
    if (!f)
      print->print("JsonRDWS open %s for %s failed", path, mode);
  }

  ~JsonRDWS() {
    print->print("JsonRDWS destructing\n");
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

  //look for array of integers
  void lookFor(const char * id, void(*fun)(std::vector<uint16_t>)) {
    // const char *p = (const char*)&value; //pointer trick
    funList.push_back(fun);
    addToObjects(id, "fun", funList.size()-1);
  }

  //reads from file until all objects have been found (then stops reading)
  //returns false if not all objects to look for are found
  bool deserialize() {
    f.read(&character, sizeof(byte));
    while (f.available()) // && !foundAll
      next();
    bool foundAll = foundCounter >= objectDetails.size();
    if (foundAll)
      print->print("JsonRDWS found all what it was looking for %d >= %d\n", foundCounter, objectDetails.size());
    else
      print->print("JsonRDWS Not all objects looked for where found %d < %d\n", foundCounter, objectDetails.size());
    f.close();
    return foundAll;
  }

private:
  struct ObjectDetails {
    const char * id;
    const char * type;
    size_t index;
  };

  File f;
  uint8_t character; //the last character parsed
  std::vector<ObjectDetails> objectDetails; //details of objects looking for
  std::vector<uint8_t *> uint8List; //pointer of uint8 to assign found values to (index of list stored in objectDetails)
  std::vector<uint16_t *> uint16List; //same for uint16
  std::vector<char *> charList; //same for char
  std::vector<void(*)(std::vector<uint16_t>)> funList; //same for function calls
  std::vector<String> objectStack; //objects and arrays store their names in a stack
  bool collectNumbers = false; //array can ask to store all numbers found in array (now used for x,y,z coordinates)
  std::vector<uint16_t> uint16CollectList; //collected numbers
  char lastObjectId[100] = ""; //last found object id in json
  char beforeLastObjectId[100] = ""; //last found object id in json
  size_t foundCounter = 0; //count how many of the id's to lookFor have been actually found

  //called by lookedFor, store the object details in objectDetails
  void addToObjects(const char * id, const char * type, size_t index) {
    ObjectDetails od;
    od.id = id;
    od.type = type;
    od.index = index;
    objectDetails.push_back(od);
  }

  void next() {
    if (character=='{') { //object begin
      // print->print("Object %c\n", character);
      objectStack.push_back(lastObjectId); //copy!!
      print->print("Object push %s %d\n", lastObjectId, objectStack.size());
      strcpy(lastObjectId, "");
      f.read(&character, sizeof(byte));
    }
    else if (character=='}') { //object end
      strcpy(lastObjectId, objectStack[objectStack.size()-1].c_str());
      print->print("Object pop %s %d\n", lastObjectId, objectStack.size());
      check(lastObjectId);
      objectStack.pop_back();
      f.read(&character, sizeof(byte));
    }
    else if (character=='[') { //array begin
      // print->print("Array %c\n", character);
      objectStack.push_back(lastObjectId); //copy!!
      // print->print("Array push %s %d\n", lastObjectId, objectStack.size());
      strcpy(lastObjectId, "");
      f.read(&character, sizeof(byte));

      //now we want to collect the array elements
      collectNumbers = true;
      uint16CollectList.clear(); //to be sure not to have old numbers (e.g. pin)
    }
    else if (character==']') { //array end
      //assign back the popped object id from [
      strcpy(lastObjectId, objectStack[objectStack.size()-1].c_str());
      // print->print("Array pop %s %d %d\n", lastObjectId, objectStack.size(), uint16CollectList.size());
      check(lastObjectId);

      // print->print("1\n");
      //check the parent array, if exists
      if (objectStack.size()-2 >=0) {
      // print->print("2\n");
        // print->print("  Parent check %s\n", objectStack[objectStack.size()-2].c_str());
        strcpy(beforeLastObjectId, objectStack[objectStack.size()-2].c_str());
        check(beforeLastObjectId);
      // print->print("3\n");
      }
      objectStack.pop_back(); //remove objectid of this array
      // print->print("4\n");
      collectNumbers = false;
      // print->print("5\n");
      uint16CollectList.clear();
      // print->print("6\n");
      f.read(&character, sizeof(byte));
    }
    else if (character=='"') { //parse String
      char value[100] = "";
      f.readBytesUntil('"', value, sizeof(value));
    
      if (strcmp(lastObjectId, "") == 0) {
        // print->print("Element [%s]\n", value);
        strcpy(lastObjectId, value);
      }
      else {
        print->print("String %s: [%s]\n", lastObjectId, value);
        check(lastObjectId, value);
        strcpy(lastObjectId, "");
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

      // print->print("Number %s: [%s]\n", lastObjectId, value);
      if (collectNumbers)
        uint16CollectList.push_back(atoi(value));

      check(lastObjectId, value);
  
      strcpy(lastObjectId, "");
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
    else {
      print->print("Element don't know %c\n", character);
      f.read(&character, sizeof(byte));
    }
  } //next

  void check(char * objectId, char * value = nullptr) {
      // print->print("2.1\n");
    for (std::vector<ObjectDetails>::iterator od=objectDetails.begin(); od!=objectDetails.end(); ++od) {
      // print->print("2.2\n");
      // print->print("check %s %s %s\n", od->id, object, value);
      if (strcmp(od->id, objectId)==0) {
      // print->print("2.3\n");
        // print->print("JsonRDWS found %s:%s %d %s\n", objectId, od->type, od->index, value?value:"", uint16CollectList.size());
        if (strcmp(od->type, "uint8") ==0) *uint8List[od->index] = atoi(value);
        if (strcmp(od->type, "uint16") ==0) *uint16List[od->index] = atoi(value);
        if (strcmp(od->type, "char") ==0) strcpy(charList[od->index], value);
        if (strcmp(od->type, "fun") ==0) funList[od->index](uint16CollectList);
        foundCounter++;
      // print->print("2.4\n");
      }
    }
    // foundAll = foundCounter >= objectDetails.size();
    // if (foundAll)
    //   print->print("Hooray, LazyJsonRDWS found all what we were looking for, no further search needed\n");
      // print->print("2.5\n");
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