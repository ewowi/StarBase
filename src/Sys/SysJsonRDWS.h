/*
   @title     StarMod
   @file      SysJsonRDWS.h
   @date      20231016
   @repo      https://github.com/ewowi/StarMod
   @Authors   https://github.com/ewowi/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 */

//Lazy Json Read Deserialize Write Serialize (write / serialize not implemented yet)
//ArduinoJson won't work on very large fixture.json, this does
//only support what is currently needed: read / deserialize uint8/16/char var elements (arrays not yet)
class JsonRDWS {

  public:

  JsonRDWS(const char * path, const char * mode = "r") {
    USER_PRINTF("JsonRDWS constructing %s %s\n", path, mode);
    f = files->open(path, mode);
    if (!f)
      USER_PRINTF("JsonRDWS open %s for %s failed", path, mode);
  }

  ~JsonRDWS() {
    USER_PRINTF("JsonRDWS destructing\n");
    f.close();
  }

  void addExclusion(const char * key) {
    charList.push_back((char *)key);
  }

  //serializeJson
  void writeJsonDocToFile(JsonDocument* dest) {
    writeJsonVariantToFile(dest->as<JsonVariant>());
    f.close();
    files->filesChange();
  }

  //look for uint8 var
  void lookFor(const char * id, uint8_t * value) {
    // const char *p = (const char*)&value; //pointer trick
    uint8List.push_back(value);
    addToVars(id, "uint8", uint8List.size()-1);
  }

  //look for uint16 var
  void lookFor(const char * id, uint16_t * value) {
    uint16List.push_back(value);
    addToVars(id, "uint16", uint16List.size()-1);
  }

  //look for char var
  void lookFor(const char * id, char * value) {
    charList.push_back(value);
    addToVars(id, "char", charList.size()-1);
  }

  //look for array of integers
  void lookFor(const char * id, std::function<void(std::vector<uint16_t>)> fun) {
    funList.push_back(fun);
    addToVars(id, "fun", funList.size()-1);
  }

  //reads from file until all vars have been found (then stops reading)
  //returns false if not all vars to look for are found
  bool deserialize(bool lazy) {
    f.read(&character, sizeof(byte));
    while (f.available() && (!foundAll || !lazy))
      next();
    if (foundAll)
      USER_PRINTF("JsonRDWS found all what it was looking for %d >= %d\n", foundCounter, varDetails.size());
    else
      USER_PRINTF("JsonRDWS Not all vars looked for where found %d < %d\n", foundCounter, varDetails.size());
    f.close();
    return foundAll;
  }

private:
  struct VarDetails {
    const char * id;
    const char * type;
    size_t index;
  };

  File f;
  uint8_t character; //the last character parsed
  std::vector<VarDetails> varDetails; //details of vars looking for
  std::vector<uint8_t *> uint8List; //pointer of uint8 to assign found values to (index of list stored in varDetails)
  std::vector<uint16_t *> uint16List; //same for uint16
  std::vector<char *> charList; //same for char
  std::vector<std::function<void(std::vector<uint16_t>)>> funList; //same for function calls
  std::vector<String> varStack; //objects and arrays store their names in a stack
  bool collectNumbers = false; //array can ask to store all numbers found in array (now used for x,y,z coordinates)
  std::vector<uint16_t> uint16CollectList; //collected numbers
  char lastVarId[128] = ""; //last found var id in json
  char beforeLastVarId[128] = ""; //last found var id in json
  size_t foundCounter = 0; //count how many of the id's to lookFor have been actually found
  bool foundAll = false;

  //called by lookedFor, store the var details in varDetails
  void addToVars(const char * id, const char * type, size_t index) {
    VarDetails vd;
    vd.id = id;
    vd.type = type;
    vd.index = index;
    varDetails.push_back(vd);
  }

  void next() {
    if (character=='{') { //object begin
      // USER_PRINTF("Object %c\n", character);
      varStack.push_back(lastVarId); //copy!!
      // USER_PRINTF("Object push %s %d\n", lastVarId, varStack.size());
      strcpy(lastVarId, "");
      f.read(&character, sizeof(byte));
    }
    else if (character=='}') { //object end
      strncpy(lastVarId, varStack[varStack.size()-1].c_str(), sizeof(lastVarId)-1);
      // USER_PRINTF("Object pop %s %d\n", lastVarId, varStack.size());
      check(lastVarId);
      varStack.pop_back();
      f.read(&character, sizeof(byte));
    }
    else if (character=='[') { //array begin
      // USER_PRINTF("Array %c\n", character);
      varStack.push_back(lastVarId); //copy!!
      // USER_PRINTF("Array push %s %d\n", lastVarId, varStack.size());
      strcpy(lastVarId, "");
      f.read(&character, sizeof(byte));

      //now we want to collect the array elements
      collectNumbers = true;
      uint16CollectList.clear(); //to be sure not to have old numbers (e.g. pin)
    }
    else if (character==']') { //array end
      //assign back the popped var id from [
      strncpy(lastVarId, varStack[varStack.size()-1].c_str(), sizeof(lastVarId)-1);
      // USER_PRINTF("Array pop %s %d %d\n", lastVarId, varStack.size(), uint16CollectList.size());
      check(lastVarId);

      //check the parent array, if exists
      if (varStack.size()-2 >=0) {
        // USER_PRINTF("  Parent check %s\n", varStack[varStack.size()-2].c_str());
        strncpy(beforeLastVarId, varStack[varStack.size()-2].c_str(), sizeof(beforeLastVarId)-1);
        check(beforeLastVarId);
      }
      varStack.pop_back(); //remove var id of this array
      collectNumbers = false;
      uint16CollectList.clear();
      f.read(&character, sizeof(byte));
    }
    else if (character=='"') { //parse String
      char value[128] = "";
      f.readBytesUntil('"', value, sizeof(value)-1);
    
      //if no lastVar then var found
      if (strcmp(lastVarId, "") == 0) {
        // USER_PRINTF("Element [%s]\n", value);
        strncpy(lastVarId, value, sizeof(lastVarId)-1);
      }
      else { // if lastvar then string value found
        // USER_PRINTF("String var %s: [%s]\n", lastVarId, value);
        check(lastVarId, value);
        strcpy(lastVarId, "");
      }

      f.read(&character, sizeof(byte));
    }
    else if (isDigit(character)) { //parse number
      char value[100] = "";

      size_t len = 0;
      //readuntil not number
      while (isDigit(character)) {
        // USER_PRINTF("%c", character);
        value[len++] = character;
        f.read(&character, sizeof(byte));
      }
      value[len++] = '\0';

      //number value found
      // USER_PRINTF("Number var %s: [%s]\n", lastVarId, value);
      if (collectNumbers)
        uint16CollectList.push_back(atoi(value));

      check(lastVarId, value);
  
      strcpy(lastVarId, "");
    }
    else if (character==':') {
      // USER_PRINTF("semicolon %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character==',') {
      // USER_PRINTF("sep %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character==']') {
      // USER_PRINTF("close %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character=='}') {
      // USER_PRINTF("close %c\n", character);
      f.read(&character, sizeof(byte));
    }
    else if (character=='\n') { //skip new lines
      // USER_PRINTF("skip newline \n");
      f.read(&character, sizeof(byte));
    }
    else {
      // USER_PRINTF("%c", character);
      f.read(&character, sizeof(byte));
    }
  } //next

  void check(char * varId, char * value = nullptr) {
    //check if var is in lookFor list
    for (std::vector<VarDetails>::iterator vd=varDetails.begin(); vd!=varDetails.end(); ++vd) {
      // USER_PRINTF("check %s %s %s\n", vd->id, varId, value);
      if (strcmp(vd->id, varId)==0) {
        // USER_PRINTF("JsonRDWS found %s:%s %d %s\n", varId, vd->type, vd->index, value?value:"", uint16CollectList.size());
        if (strcmp(vd->type, "uint8") ==0) *uint8List[vd->index] = atoi(value);
        if (strcmp(vd->type, "uint16") ==0) *uint16List[vd->index] = atoi(value);
        if (strcmp(vd->type, "char") ==0) strncpy(charList[vd->index], value, 31); //assuming size 32-1 here
        if (strcmp(vd->type, "fun") ==0) funList[vd->index](uint16CollectList);
        foundCounter++;
      }
    }

    foundAll = foundCounter >= varDetails.size();
  }

  //writeJsonVariantToFile calls itself recursively until whole json document has been parsed
  void writeJsonVariantToFile(JsonVariant variant) {
    if (variant.is<JsonObject>()) {
      f.printf("{");
      char sep[2] = "";
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
          strcpy(sep, ",");
          writeJsonVariantToFile(pair.value());
        }
      }
      f.printf("}");
    }
    else if (variant.is<JsonArray>()) {
      f.printf("[");
      char sep[2] = "";
      for (JsonVariant variant2: variant.as<JsonArray>()) {
        f.print(sep);
        strcpy(sep, ",");
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
      USER_PRINTF("%s not supported", variant.as<String>());
  }

};