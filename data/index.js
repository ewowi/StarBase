// @title     StarMod
// @file      index.css
// @date      20231016
// @repo      https://github.com/ewowi/StarMod
// @Authors   https://github.com/ewowi/StarMod/commits/main
// @Copyright (c) 2023 Github StarMod Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007

let d = document;
let ws = null;

let screenColumnNr = 0;
let nrOfScreenColumns = 4;
let userFunId = "";
let htmlGenerated = false;
let jsonValues = {};
let uiFunCommands = [];
let model = null; //model.json (as send by the server)

function gId(c) {return d.getElementById(c);}
function cE(e) { return d.createElement(e); }

function handleVisibilityChange() {
  console.log("handleVisibilityChange");
}

function onLoad() {
  makeWS();

  initScreenColumns();

  d.addEventListener("visibilitychange", handleVisibilityChange, false);
}

function makeWS() {
  if (ws) return;
  let url = (window.location.protocol == "https:"?"wss":"ws")+'://'+window.location.hostname+'/ws';
  console.log("makeWS url", url);
  ws = new WebSocket(url);
  ws.binaryType = "arraybuffer";
  ws.onmessage = (e)=>{
    if (e.data instanceof ArrayBuffer) { // preview packet
      if (userFun(userFunId, e.data))
        userFunId = "";
    } 
    else {
      clearTimeout(jsonTimeout);
      jsonTimeout = null;
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
        if (!htmlGenerated) { //generate array of variables
          if (Array.isArray(json)) {
            model = json; //this is the model
            console.log("WS receive generateHTML", model);
            generateHTML(null, model); //no parentNode
            htmlGenerated = true;
            //send request for uiFun
            if (uiFunCommands.length) { //flush commands not already send
              flushUIFunCommands();
            }
          }
          else
            console.log("Error: no valid model", json);
        }
        else { //update
          if (!Array.isArray(json)) //only the model is an array
            // console.log("WS receive update", json);
            processUpdate(json);
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
		reqsLegal = true;
  }
}

function linearToLogarithm(json, value) {
  if (value == 0) return 0;

  var minp = json.min?json.min:0;
  var maxp = json.max?json.max:255;

  // The result should be between 100 an 10000000
  var minv = minp?Math.log(minp):0;
  var maxv = Math.log(maxp);

  // calculate adjustment factor
  var scale = (maxv-minv) / (maxp - minp);

  let result = Math.exp(minv + scale*(value-minp));

  console.log(json, minv, maxv, scale, result);

  return Math.round(result);
}

function generateHTML(parentNode, json, rowNr = -1) {
  // console.log("generateHTML", parentNode, json);
  if (Array.isArray(json)) {
    json.sort(function(a,b) {
      // Turn your strings into dates, and then subtract them
      // to get a value that is either negative, positive, or zero.
      return a.o - b.o; //o is order nr
    });
    for (var node of json) //if isArray then variables of array
      generateHTML(parentNode, node);
  }
  else {
    //if root (type module) add the html to one of the screen columns
    if (parentNode == null) {
      parentNode = gId("screenColumn" + screenColumnNr);
      screenColumnNr = (screenColumnNr +1)%nrOfScreenColumns;
    }

    var newNode = null; //newNode will be appended to the parentNode after if then else

    let ndivNeeded = true; //for details ("n"), module and table do not need an extra div for details
    
    //set labelNode before if, will be used in if then else
    let labelNode = cE("label");
    labelNode.innerText = initCap(json.id);

    if (json.type == "module") {
      ndivNeeded = false;
      newNode = cE("div");
      newNode.id = json.id
      newNode.draggable = true;
      newNode.className = "screenBox";

      let h2Node = cE("h2");
      h2Node.innerText = initCap(json.id);
      newNode.appendChild(h2Node);

      setupScreenBox(newNode);
    }
    else if (json.type == "table") {
      ndivNeeded = false;
      //add label
      let pNode = cE("p");
      pNode.appendChild(labelNode);
      parentNode.appendChild(pNode); //add the table label to the parent

      //add table
      newNode = cE("table");
      newNode.id = json.id;
      newNode.className = "table-style"

      let theadNode = cE("thead");
      let trNode = cE("tr");
      theadNode.appendChild(trNode);
      newNode.appendChild(theadNode); //row for header

      newNode.appendChild(cE("tbody"));
    }
    else { //primitive types

      //table header
      if (parentNode.nodeName.toLocaleLowerCase() == "table") { //table add the id in the header
        let tdNode = cE("th");
        tdNode.id = json.id;
        tdNode.innerText = initCap(json.id); //label uiFun response can change it
        parentNode.firstChild.firstChild.appendChild(tdNode); //<thead><tr>
      } else {
        
        if (json.type == "select") {
          if (json.ro) { //e.g. for reset/restart reason: do not show a select but only show the selected option
            newNode = cE("p");
            newNode.appendChild(labelNode);
            let spanNode = cE("span");
            spanNode.id = json.id;
            if (json.value) spanNode.innerText = json.value;
            newNode.appendChild(spanNode);
          }
          else {
            //<p> with <label><select> (<comment> in processUpdate)
            newNode = cE("p");
            newNode.appendChild(labelNode);

            let selectNode = cE("select");
            selectNode.id = json.id;
            selectNode.addEventListener('change', (event) => {console.log("select change", event);setSelect(event.target);});

            newNode.appendChild(selectNode);
            //(default) value will be set in processUpdate
          }
        }
        else if (json.type == "canvas") {
          //<p><label><span><canvas>
          var pNode = cE("p");
          pNode.appendChild(labelNode);

          let spanNode = cE("span");
          spanNode.innerText= "🔍";
          // spanNode.addEventListener('click', (event) => {toggleModal(newNode);});
          pNode.appendChild(spanNode);
          
          parentNode.appendChild(pNode);

          newNode = cE("canvas");
          newNode.id = json.id;
          newNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
        }
        else if (json.type == "textarea") {
          pNode = cE("p");
          pNode.appendChild(labelNode);
          parentNode.appendChild(pNode);

          newNode = cE("textarea");
          newNode.id = json.id;
          newNode.readOnly = json.ro;
          newNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});

          if (json.value) newNode.innerText = json.value;
          // newNode.appendChild(textareaNode);
          pNode.innerText += "🔍";
        }
        else if (json.type == "url") {
          //tbd: th and table row outside this if
          if (rowNr == -1) {
            newNode = cE("p");
            newNode.appendChild(labelNode);
            //add label
            if (json.type != "button") newNode.appendChild(labelNode);
          }
    
          let valueNode = cE("a");
          valueNode.setAttribute('href', json.value);
          // valueNode.setAttribute('target', "_blank"); //does not work well on mobile
          valueNode.innerText = json.value;
    
          if (rowNr == -1) {
            valueNode.id = json.id;
            newNode.appendChild(valueNode);
          } else {
            valueNode.id = json.id + "#" + rowNr;
            newNode = valueNode;
          }
        } else { //input
          if (rowNr == -1) {
            newNode = cE("p");
            newNode.appendChild(labelNode);
            //add label
            if (json.type != "button") newNode.appendChild(labelNode);
          }  

          let rangeValueNode = null;
          let buttonSaveNode = null;
          let buttonCancelNode = null;

          let valueNode;

          //type specific actions
          if (json.type == "checkbox") {
            valueNode = cE("input");
            valueNode.type = json.type;
            valueNode.disabled = json.ro;
            if (json.value) valueNode.checked = json.value;
            valueNode.addEventListener('change', (event) => {console.log(json.type + " change", event);setCheckbox(event.target);});
          } else if (json.type == "button") {
            valueNode = cE("input");
            valueNode.type = json.type;
            valueNode.disabled = json.ro;
            valueNode.value = initCap(json.id);
            valueNode.addEventListener('click', (event) => {console.log(json.type + " click", event);setButton(event.target);});
          } else if (json.type == "range") {
            valueNode = cE("input");
            valueNode.type = json.type;
            valueNode.min = json.min?json.min:0;
            valueNode.max = json.max?json.max:255; //range slider default 0..255
            valueNode.disabled = json.ro;
            if (json.value) valueNode.value = json.value;
            //numerical ui value changes while draging the slider (oninput)
            valueNode.addEventListener('input', (event) => {
              gId(json.id + "_rv").innerText = json.log?linearToLogarithm(json, event.target.value):event.target.value;
            });
            //server value changes after draging the slider (onchange)
            valueNode.addEventListener('change', (event) => {
              setInput(event.target);
            });
            rangeValueNode = cE("span");
            rangeValueNode.id = json.id + "_rv"; //rangeValue
            if (json.value) rangeValueNode.innerText = json.log?linearToLogarithm(json, json.value):json.value;
          } else {
            //input types: text, search, tel, url, email, and password.

            if (json.ro && json.type != "button") {
              valueNode = cE("span");
              if (json.value) valueNode.innerText = json.value;
            } else {
              valueNode = cE("input");
              valueNode.type = json.type;
              if (json.value) valueNode.value = json.value;
              valueNode.addEventListener('change', (event) => {console.log(json.type + " change", event);setInput(event.target);});
              // if (["text", "password", "number"].includes(json.type) ) {
              //   buttonSaveNode = cE("text");
              //   buttonSaveNode.innerText = "✅";
              //   buttonSaveNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
              //   buttonCancelNode = cE("text");
              //   buttonCancelNode.innerText = "🛑";
              //   buttonCancelNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
              // }
              if (json.type == "number") {
                if (json.min) valueNode.min = json.min;
                if (json.max) valueNode.max = json.max;
                // valueNode.setAttribute('size', '4');
                // valueNode.maxlength = 4;
                // valueNode.size = 4;
              }
              else {
                if (json.max) valueNode.setAttribute('maxlength', json.max); //for text and textarea set max length valueNode.maxlength is not working for some reason
                if (json.id == "serverName")
                  gId("instanceName").innerText = json.value;
              }
            }
          } //not checkbox or button or range

          if (rowNr == -1) {
            valueNode.id = json.id;
            newNode.appendChild(valueNode);
          } else {
            valueNode.id = json.id + "#" + rowNr;
            newNode = valueNode;
          }

          if (rangeValueNode) newNode.appendChild(rangeValueNode);
          if (buttonSaveNode) newNode.appendChild(buttonSaveNode);
          if (buttonCancelNode) newNode.appendChild(buttonCancelNode);
        } //input type

        //disable drag of parent screenBox
        newNode.draggable = true;
        newNode.addEventListener('dragstart', (event) => {event.preventDefault(); event.stopPropagation();});
      } //table header
    } //primitive types

    if (newNode) parentNode.appendChild(newNode); //add new node to parent

    //don't call uiFun on rowNrs (for the moment)
    if (rowNr == -1) {
      //call ui Functionality, if defined (to set label, comment, select etc)
      if (json.uiFun >= 0) { //>=0 as element in var
        uiFunCommands.push(json.id);
        if (uiFunCommands.length > 8) { //every 10 vars (to respect responseDoc size) check WS_EVT_DATA info
          flushUIFunCommands();
        }
      }
      
      if (json.n) {
        //add a div with _n extension and details have this as parent
        if (ndivNeeded) {
          let divNode = cE("div");
          divNode.id = json.id + "_n";
          divNode.classList.add("ndiv");
          newNode.appendChild(divNode);
          generateHTML(divNode, json.n, rowNr);
        }
        else
          generateHTML(newNode, json.n, rowNr); //details (e.g. module)
      }
    }
    return newNode;
  } //not an array
}

function flushUIFunCommands() {
  if (uiFunCommands.length > 0) { //if something to flush
    var command = {};
    command["uiFun"] = uiFunCommands; //ask to run uiFun for vars (to add the options)
    // console.log("flushUIFunCommands", command);
    requestJson(command);
    uiFunCommands = [];
  }
}

function processUpdate(json) {
  // console.log("processUpdate", json);
  if (json.id) { //this is a var object
    console.log("processUpdate variable", gId(json.id), json.n);
    if (gId(json.id + "_n")) 
      gId(json.id + "_n").remove();
    if (json.n) {
      let divNode = cE("div");
      divNode.id = json.id + "_n";
      divNode.classList.add("ndiv");
      gId(json.id).parentNode.appendChild(divNode);
      generateHTML(divNode, json.n);
    }
    flushUIFunCommands(); //make sure uiFuns of new elements are called
  }
  else { //uiFun or {variable:{label:value}}
    for (var key of Object.keys(json)) {
      if (key != "uiFun") { //was the request
        if (gId(key)) { //is the key a var?
          processVarNode(gId(key), key, json[key]);
        }
        else
          console.log("processUpdate id not found in dom", key, json[key]);
      } //key != uiFun
    } //for keys
  } //not id
} //processUpdate

function processVarNode(node, key, json) {
  let overruleValue = false;
  
  if (json.hasOwnProperty("label")) {
    if (key != "insTbl") // tbd: table should not update
      console.log("processVarNode label", key, json.label);
    if (node.nodeName.toLocaleLowerCase() == "input" && node.type == "button") {
      node.value = initCap(json.label);
    }
    else {
      let labelNode;
      if (node.nodeName.toLocaleLowerCase() == "canvas" || node.nodeName.toLocaleLowerCase() == "table")
        labelNode = node.previousSibling.firstChild; //<p><label> before <canvas>/<table>
      else if (node.nodeName.toLocaleLowerCase() == "th") //table header
        labelNode = node; //the <th>
      else
        labelNode = node.parentNode.firstChild; //<label> before <span or input> within <p>
      labelNode.innerText = initCap(json.label);
    }
  }
  if (json.hasOwnProperty("comment")) {
    if (key != "insTbl") // tbd: table should not update
      console.log("processVarNode comment", key, json.comment);
    // normal: <p><label><input id><comment></p>
    // table or canvas <p><label><comment></p><canvas id>
    // 1) if exist then replace else add
    let parentNode;
    if (node.nodeName.toLocaleLowerCase() == "canvas" || node.nodeName.toLocaleLowerCase() == "table")
      parentNode = node.previousSibling; //<p><label> before <canvas>/<table>
    else
      parentNode = node.parentNode;
    let commentNode = parentNode.querySelector('comment');
    // console.log("commentNode", commentNode);
    if (!commentNode) { //create if not exist
      commentNode = cE("comment");
      //if a div node exists (for details - ndiv) then place the comment before the div node
      let divNode = parentNode.querySelector('div');
      if (divNode)
        parentNode.insertBefore(commentNode, divNode);
      else
        parentNode.appendChild(commentNode);
    }
    commentNode.innerText = json.comment;        
  }
  if (json.hasOwnProperty("select")) {
    console.log("processVarNode select", key, json.select);
    if (node.nodeName.toLocaleLowerCase() == "span") { //readonly. tbd: only the displayed value needs to be in the select
      var index = 0;
      for (var value of json.select) {
        if (parseInt(node.textContent) == index) {
          // console.log("processVarNode select1", value, node, node.textContent, index);
          node.textContent = value; //replace the id by its value
          // console.log("processVarNode select2", value, node, node.textContent, index);
          overruleValue = true; //in this case we do not want the value set
        }
        index++;
      }
    }
    else { //select
      var index = 0;
      //remove all old options first
      while (node.options && node.options.length > 0) {
        node.remove(0);
      }
      for (var value of json.select) {
        let optNode = cE("option");
        optNode.value = index;
        optNode.text = value;
        node.appendChild(optNode);
        index++;
      }
    }
  }
  if (json.hasOwnProperty("table")) {
    //remove table rows
    let tbodyNode = cE('tbody');
    
    //find model info
    let variable = findVar(key);
    if (key != "insTbl") // tbd: table should not update
      console.log("processVarNode table", key, variable, json.table);

    //add each row
    let rowNr = 0;
    for (var row of json.table) {
      let trNode = cE("tr");
      //add each column
      let colNr = 0;
      for (var columnRow of row) {              
        let tdNode = cE("td");

        //call generateHTML to create the variable in the UI
        // console.log("table cell generateHTML", tdNode, variable, variable.n, colNr, rowNr);
        let newNode = generateHTML(tdNode, variable.n[colNr], rowNr); //no <p><label>
        if (newNode) {
          //very strange: gId(newNode.id) is not working here. Delay before it is in the dom??? (workaround create processVarNode function)
          let updateJson;
          if (variable.n[colNr].type == "checkbox" || variable.n[colNr].type == "number")
            updateJson = `{"value":${columnRow}}`;
          else
            updateJson = `{"value":"${columnRow}"}`
          // console.log("tablecolumn", rowNr, colNr, newNode, variable.n[colNr], updateJson, JSON.parse(updateJson), gId(newNode.id));
          //call processVarNode to give the variable a value
          processVarNode(newNode, newNode.id, JSON.parse(updateJson));
        }

        trNode.appendChild(tdNode);
        colNr++;
      }
      tbodyNode.appendChild(trNode);
      rowNr++;
    }
    //replace the table body
    node.replaceChild(tbodyNode, node.lastChild); //replace <table><tbody>
  }
  if (json.hasOwnProperty("value") && !overruleValue) { //after select, in case used
    //hasOwnProperty needed to catch also boolean json.value when it is false
    // if (key=="mdlEnabled" || key=="clIsFull" || key=="pin2")
    //   console.log("processVarNode value", key, json, json.value, node);
    if (node.nodeName.toLocaleLowerCase() == "span") //read only vars
      node.textContent = json.value;
    else if (node.nodeName.toLocaleLowerCase() == "a") { //url links
      node.innerText = "🔍";
      node.setAttribute('href', json.value);
    } else if (node.nodeName.toLocaleLowerCase() == "canvas")
      userFunId = key; //prepare for websocket data
    else if (node.type == "checkbox")
      node.checked = json.value;
    else if (Array.isArray(json.value)) { //table column
      let rowNr = 0;
      for (let x of json.value) {
        // console.log(key, gId(key + "#" + rowNr), x);
        if (gId(key + "#" + rowNr) && gId(key + "#" + rowNr).checked)
          gId(key + "#" + rowNr).checked = x; //tbd support all types!!
        rowNr++;
      }
      // node.checked = json.value;
    } else {//inputs
      node.value = json.value;
      node.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input
    }
  }
  if (json.hasOwnProperty("json")) { //json send html nodes cannot process, store in jsonValues array
    console.log("processVarNode json", key, json.json, node);
    jsonValues[key] = json.json;
  }
  if (json.hasOwnProperty("file")) { //json send html nodes cannot process, store in jsonValues array
    console.log("processVarNode file", key, json.file, node);
  
    //we need to send a request which the server can handle using request variable
    let url = `http://${window.location.hostname}/file`;
    fetchAndExecute(url, json.file, key, function(key,text) { //send key as parameter
      // console.log("fetchAndExecute", text); //in case of invalid json
      var ledmapJson = JSON.parse(text);
      jsonValues[key] = ledmapJson;
      jsonValues[key].new = true;
      console.log("fetchAndExecute", jsonValues);
    }); 
  }
}


function findVar(id, parent = null) {
  // console.log("findVar", id, parent, model);

  if (!parent)
    parent = model;

  let foundVar = null;
  for( var variable of parent) {
    if (!foundVar) {
      if (variable.id == id)
        foundVar = variable;
      else if (variable.n)
        foundVar = findVar(id, variable.n);
    }
  }
  return foundVar;
}

var jsonTimeout;
var reqsLegal = false;

function requestJson(command) {
  gId('connind').style.backgroundColor = "var(--c-y)";
	if (command && !reqsLegal) return; // stop post requests from chrome onchange event on page restore
	if (!jsonTimeout) jsonTimeout = setTimeout(()=>{if (ws) ws.close(); ws=null; console.log("connection failed")}, 3000);

  // if (!ws) return;
  let req = JSON.stringify(command);
  
  console.log("requestJson", command);

  if (req.length > 1340)
  console.log("too big???");
  
  ws.send(req?req:'{"v":true}');

  return;
  
  let url = `http://${window.location.hostname}/json`;
  //not used at the moment as WebSockets only
  fetch(url, {
    method: 'post',
    headers: {
      "Content-type": "application/json; charset=UTF-8"
    },
    body: req
  })
  .then(res => {
    if (res) console.log("requestJson res", res, res.json());
  })
  .then(json => {
    if (json) console.log("requestJson json", json);
  })
  .catch((e)=>{
    console.log("requestJson catch", e);
  });
}

function setInput(element) {
  var command = {};
  command[element.id] = element.value;
  // console.log("setInput", command);

  requestJson(command);
}
function setCheckbox(element) {
  var command = {};
  command[element.id] = element.checked;
  // console.log("setCheckbox", command);

  requestJson(command);
}
function setButton(element) {
  var command = {};
  command[element.id] = element.value;
  // console.log("setCheckbox", command);

  requestJson(command);
}

function setSelect(element) {
  var command = {};
  command[element.id] = element.value;
  // console.log("setInput", command);

  requestJson(command);
}

let isModal = false;
let modalPlaceHolder;

function toggleModal(element) {
  // console.log("toggleModal", element);
  isModal = !isModal;

	if (isModal) {

    modalPlaceHolder = cE(element.nodeName.toLocaleLowerCase());
    modalPlaceHolder.width = element.width;
    modalPlaceHolder.height = element.height;

    element.parentNode.replaceChild(modalPlaceHolder, element);

    // let btn = cE("button");
    // btn.innerText = "close";
    // btn.addEventListener('click', (event) => {toggleModal(element);});
    // gId('modalView').appendChild(btn);

    gId('modalView').appendChild(element);
    element.width = window.innerWidth;;
    element.height = window.innerHeight;
    // console.log("toggleModal +", element, modalPlaceHolder, element.getBoundingClientRect(), modalPlaceHolder.getBoundingClientRect().width, modalPlaceHolder.getBoundingClientRect().height, modalPlaceHolder.width, modalPlaceHolder.height);
	}
  else {    
    element.width = modalPlaceHolder.getBoundingClientRect().width;
    element.height = modalPlaceHolder.getBoundingClientRect().height;
    // if (renderer) renderer.setSize( element.width, element.height);

    // console.log("toggleModal -", element, modalPlaceHolder, element.getBoundingClientRect(), modalPlaceHolder.getBoundingClientRect().width, modalPlaceHolder.getBoundingClientRect().height, modalPlaceHolder.width, modalPlaceHolder.height);
    
    modalPlaceHolder.parentNode.replaceChild(element, modalPlaceHolder); //modalPlaceHolder loses rect
  }

	gId('modalView').style.transform = (isModal) ? "translateY(0px)":"translateY(100%)";
}
// https://stackoverflow.com/questions/324303/cut-and-paste-moving-nodes-in-the-dom-with-javascript

function initCap(s) {
  if (typeof s !== 'string') return '';
  // https://www.freecodecamp.org/news/how-to-capitalize-words-in-javascript/
  return s.replace(/[\W_]/g,' ').replace(/(^\w{1})|(\s+\w{1})/g, l=>l.toUpperCase()); // replace - and _ with space, capitalize every 1st letter
}


//drag and drop functionality
//===========================

var dragSrcEl;

// https://stackoverflow.com/questions/75698658/how-can-i-drag-and-drop-like-browser-tabs-in-javascript
function initScreenColumns() {

  let columns = document.querySelectorAll('.container .screenColumn');
  columns.forEach(function(column) {
    column.addEventListener('dragover', handleDragOver);
    column.addEventListener('dragenter', handleDragEnter);
    column.addEventListener('dragleave', handleDragLeave);
    column.addEventListener('drop', handleDrop);
  });

  setupScreenBoxes();
  
}

function setupScreenBoxes() {
  let boxes = document.querySelectorAll('.container .screenBox');
  boxes.forEach(function(box) {
    setupScreenBox(box);
  });

}

// var lastPage;
function setupScreenBox(item) {
  item.addEventListener('dragstart', handleDragStart);
  item.addEventListener('dragover', handleDragOver);
  item.addEventListener('dragenter', handleDragEnter);
  item.addEventListener('dragleave', handleDragLeave);
  item.addEventListener('dragend', handleDragEnd);
  item.addEventListener('drop', handleDrop);
  // item.onclick = function() {
  //   console.log("click", this, lastPage);
  //   if (lastPage) document.getElementById(lastPage.id+"-page").hidden = true;
  //   document.getElementById(this.id+"-page").hidden = false;
  //   lastPage = this;
  // };
}

function handleDragStart(e) {
  this.style.opacity = '0.4';

  dragSrcEl = this;

  e.dataTransfer.effectAllowed = 'move';
  e.dataTransfer.setData('text/html', this.innerText);
  console.log("handleDragStart", this, e, e.dataTransfer);
  e.dataTransfer.setData('text/plain', this.id);
}

function removeDragStyle(item) {
  item.style.opacity = '1';

  let boxes = document.querySelectorAll('.container .screenBox');
  boxes.forEach(function (item) {
    item.classList.remove('over');
  });

  let columns = document.querySelectorAll('.container .screenColumn');
  columns.forEach(function (item) {
    item.classList.remove('over');
  });

}

function handleDragEnd(e) {
  console.log("handleDragEnd", this, e);
  removeDragStyle(this);
}

function handleDragOver(e) {
  e.preventDefault();
  return false;
}

function handleDragEnter(e) {
  this.classList.add('over');
}

function handleDragLeave(e) {
  this.classList.remove('over');
}

function handleDrop(e) {
  e.stopPropagation();

  if (dragSrcEl !== this) {
    console.log("handleDrop", dragSrcEl, this, e, e.dataTransfer);

    const clone = dragSrcEl.cloneNode(true);
    setupScreenBox(clone);
    removeDragStyle(clone);

    if (this.id.includes("screenColumn")) {
      console.log("coladd");
      this.appendChild(clone);
    } else {
      this.parentNode.insertBefore(clone, this.nextSibling);
    }

    dragSrcEl.remove();
  }

  return false;
}

//WLEDMM: utility function to load contents of file from FS (used in draw)
function fetchAndExecute(url, name, parms, callback, callError = null)
{
  fetch
  (url+name, {
    method: 'get'
  })
  .then(res => {
    if (!res.ok) {
		callError("File " + name + " not found");
    	return "";
    }
    // console.log("res", res, res.text(), res.text().result);
    return res.text();
  })
  .then(text => {
    // console.log("text", text);
    callback(parms, text);
  })
  .catch(function (error) {
	if (callError) callError(parms, "Error getting " + name);
	console.log(error);
  })
  .finally(() => {
    // if (callback) setTimeout(callback,99);
  });
}
