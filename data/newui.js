// @title     StarBase
// @file      index.css
// @date      20240411
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

let d = document;
let ws = null;
let navBar; //stores navBar class instance at onLoad

let model = []; //model.json (as send by the server), used by FindVar

//C++ equivalents
const UINT8_MAX = 255;
const UINT16_MAX = 65535;

function gId(c) {return d.getElementById(c);}
function qS(e) { return d.querySelector(e); }
function cE(e, id = null, classs = null, inner = null) {
  let node = d.createElement(e);
  if (id) node.id = id;
  if (classs) node.className = classs;
  if (inner) node.innerHTML = inner;
  return node;
}

class NavBar {

  //create navbar, menu, nav-list
  constructor() {
    let navBarNode = cE("nav", "navbar", "navbar");
    gId("body").appendChild(navBarNode);

    let menuNode = cE("div", "mobile-menu", "menu-toggle", '<span class="bar"></span><span class="bar"></span><span class="bar"></span>');
    menuNode.addEventListener('click', function () {
      qS('.nav-list').classList.toggle('active'); //toggle nav-list visibility
    });
    navBarNode.appendChild(menuNode);

    navBarNode.appendChild(cE("ul", "nav-list", "nav-list"));    
  }

  //create nav item and link
  addNavItem(name, clickFun = null) {
    let navItemNode = cE("li", null, "nav-item");
    gId("nav-list").appendChild(navItemNode); //add the li node to the nav-list

    let navLinkNode = cE("a", null, "nav-link", initCap(name));
    navLinkNode.addEventListener('click', (event) => {
      if (clickFun) clickFun(event);
      qS('.nav-list').classList.remove('active');
    });
    navItemNode.appendChild(navLinkNode);

    let dropdownNode = cE("ul", name, "dropdown");
    navItemNode.appendChild(dropdownNode);

    return dropdownNode; //return the node with id=name
  }

  //cretae dropdown item and link
  addDropdownItem(navItemName, name, clickFun) {
    let navItemNode = gId(navItemName);

    //if navItemNode does not exist, create the node
    if (!navItemNode)
      navItemNode = this.addNavItem(navItemName);

    let dropdownItemNode = cE("li", null, "dropdown-item");
    navItemNode.appendChild(dropdownItemNode);

    let dropdownLinkNode = cE("a", name, "dropdown-link", initCap(name));
    dropdownItemNode.appendChild(dropdownLinkNode);
    dropdownLinkNode.addEventListener('click', (event) => {
      if (clickFun) clickFun(event);
      qS('.nav-list').classList.remove('active');
    });

    return dropdownLinkNode; //return the node with id=name
  }

} //class NavBar

// https://www.w3schools.com/howto/howto_js_topnav_responsive.asp
function onLoad() {

  navBar = new NavBar(); //adds the navBar to <body>

  let body = gId("body");

  //additional html 
  let node;
  body.appendChild(cE("h1", null, null, "StarBase💫 by MoonModules 🌔"));

  node = cE("a", null, null, "ⓘ");
  node.href = "https://ewowi.github.io/StarDocs";
  body.appendChild(node);

  body.appendChild(cE("h2", "serverName"));
  body.appendChild(cE("h3", "vApp"));

  body.appendChild(cE("p", "screenSize"));

  body.appendChild(cE("h3", "modName"));
  body.appendChild(cE("pre", "modelJson"));
  body.appendChild(cE("div", "connind", null, "&#9790;"));


  node = cE("p", null, null, "© 2024 MoonModules ☾ - StarMod, StarBase and StarLight is licensed under GPL-v3");
  node.style = "color:grey;";
  body.appendChild(node);

  
  window.onresize = function() {
    // Setting the current height & width
    gId("screenSize").innerText = window.innerWidth + "x" + window.innerHeight;
  };

  makeWS();

  d.addEventListener('visibilitychange', function () {
    console.log("handleVisibilityChange");
    gId("screenSize").innerText = window.innerWidth + "x" + window.innerHeight;
  });

}

function makeWS() {
  if (ws) return;
  let url = (window.location.protocol == "https:"?"wss":"ws")+'://'+window.location.hostname+'/ws';
  console.log("makeWS url", url);
  ws = new WebSocket(url);
  ws.binaryType = "arraybuffer";
  ws.onmessage = (e)=>{
    if (e.data instanceof ArrayBuffer) { // preview packet
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
      gId('connind').style.backgroundColor = "var(--c-l)";

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
            let module = json;
            model.push((module)); //this is the model
            console.log("WS receive module", module);

            //create dropdownItem for module.id (level 2) under module.type (level 1 - module.type will be created if not exists)
            navBar.addDropdownItem(module.type, module.id, function(event) {
              console.log("DropdownItem click", event.target.innerText);
              for (let module of model) {
                if (module.id == event.target.innerText) {
                  gId("modName").innerText = module.id;
                  gId("modelJson").innerHTML = JSON.stringify(module, null, 2); //pretty
                }
              }
            });
           
            gId("vApp").innerText = appName(); //tbd: should be set by server
            gId("screenSize").innerText = window.innerWidth + "x" + window.innerHeight;
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

//utility function
function initCap(s) {
  if (typeof s !== 'string') return '';
  // https://www.freecodecamp.org/news/how-to-capitalize-words-in-javascript/
  return s.replace(/[\W_]/g,' ').replace(/(^\w{1})|(\s+\w{1})/g, l=>l.toUpperCase()); // replace - and _ with space, capitalize every 1st letter
}
