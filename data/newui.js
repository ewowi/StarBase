// @title     StarBase
// @file      newui.js
// @date      20240720
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

class Controller {

  //kept vars public as other classes uses them
  ws = null
  model = [] //model.json (as send by the server), used by FindVa
  themeClass = null //stores MainNav class instance at onLoad
  mainNav = null //stores MainNav class instance at onLoad

  onLoad() {
    if (window.location.href.includes("127.0.0.1"))
      this.fetchModelForLiveServer();
    else 
      this.makeWS();

    this.themeClass = new ThemeClass();
    this.themeClass.addHTML();
    this.themeClass.getTheme();
  
    this.mainNav = new MainNav(this.model);
    this.mainNav.addHTML();
  
    }

  async fetchModelForLiveServer() {
    // Mock fetch for testing while using Live Server. No error checking for brevity.
    // Replace with call to server websocket
    let localModel = await (await fetch('../misc/model.json')).json()
  
    //sort modules by order (o)
    localModel.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });
  
    for (let moduleJson of localModel) {
      this.addModule(moduleJson) //this will add moduleJson to this.model
    }
  }
  
  makeWS() {
    if (ws) return;
    let url = (window.location.protocol == "https:"?"wss":"ws")+'://'+window.location.hostname+'/ws';
    console.log("makeWS url", url);
    ws = new WebSocket(url);
    ws.binaryType = "arraybuffer";
    ws.onmessage = (e)=>{
      if (e.data instanceof ArrayBuffer) { // binary packet - e.g. for preview
        let buffer = new Uint8Array(e.data);
        if (buffer[0]==0) {
          let pviewNode = gId("board");
          // console.log(buffer, pviewNode);
          if (pviewNode)
            previewBoard(pviewNode, buffer);
        }
        else 
          userFun(buffer);
      } 
      else {
        // console.log("onmessage", e.data);
        let json = null;
        try {
          json = JSON.parse(e.data);
        } catch (error) {
            json = null;
            console.error("makeWS json error", error, e.data); // error in the above string (in this case, yes)!
        }
        if (json) {
          //receive model per module to stay under websocket size limit of 8192
          if (json.type && ["appmod","usermod", "sysmod"].includes(json.type)) { //generate array of variables
            let found = false;
            for (let module of model) {
              if (module.id == json.id)
                found = true;
            }
            if (!found) {
              addModule(json);
            }
            else
              console.log("html of module already generated", json);
          }
          else { //update
            if (!Array.isArray(json)) {//only the model is an array
              // console.log("WS receive update", json);
              // receiveData(json);
            }
            else
              console.log("dev array not expected", json);
          }
        }
      }
    }
    ws.onclose = (e)=>{
      console.log("WS close and retry", e);
      gId('connind').style.backgroundColor = "var(--c-r)";
      setTimeout(makeWS,1500); // retry WS connection
      ws = null;
    }
    ws.onopen = (e)=>{
      console.log("WS open", e);
    }
    ws.onerror = (e)=>{
      console.log("WS error", e);
    }
  }

  //used by fetchModel and by makeWS
  addModule(moduleJson) {
    this.model.push(moduleJson);

    //updateUI is made after all modules have been fetched, how to adapt to add one module?
    this.mainNav.updateUI(moduleJson, moduleFun); //moduleFun returns the html to show in the module panel of the UI
    //still doesn't maker sense  to call updateUI for every module ...

  }
} //class Controller

/**
 * Create an instance of the app on the global space
 */
window.controller = new Controller()