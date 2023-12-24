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
let model = null; //model.json (as send by the server), used by FindVar
let savedView = null;
let savedData = {};

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
            generateHTML(model); //no parentNode
            htmlGenerated = true;

            if (savedView)
              showHideModules(gId(savedView));
            else
              showHideModules(gId("vApp")); //default
      
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
            receiveData(json);
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

  // console.log(json, minv, maxv, scale, result);

  return Math.round(result);
}

function generateHTML(json, parentNode = null, rowNr = -1) {
  // console.log("generateHTML", json, parentNode);
  if (Array.isArray(json)) {
    //sort according to o value
    json.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });
    for (var variable of json) //if isArray then variables of array
      generateHTML(variable, parentNode, rowNr);
  }
  else { // json is variable
    let  variable = json;
    //if root (type module) add the html to one of the screen columns
    if (parentNode == null) {
      parentNode = gId("screenColumn" + screenColumnNr);
      screenColumnNr = (screenColumnNr +1)%nrOfScreenColumns;
    }

    //if System, set the current view
    if (variable && variable.id) {
      if (variable.id == "System") {
        //get the current view
        console.log("view", variable);
        if (variable.view) 
          savedView = variable.view;
      }
    }
    else {
      console.log("genHTML no variable and id", variable, parentNode); //tbd: caused by more data then columns in table...
      return;
    }

    let newNode = null; //newNode will be appended to the parentNode after if then else and returned
    let valueNode; //the node containing the variable
    let rangeValueNode = null;
    // let buttonSaveNode = null;
    // let buttonCancelNode = null;

    let ndivNeeded = true; //for details ("n"), module and table do not need an extra div for details
       
    let labelNode = cE("label"); //set labelNode before if, will be used in if then else
    labelNode.innerText = initCap(variable.id); // the default when not overridden by uiFun
    
    let isPartOfTableRow = (rowNr != -1);

    if (!isPartOfTableRow) {
      newNode = cE("p");
      if (variable.type != "button") newNode.appendChild(labelNode); //add label (tbd:must be done by childs n table cell)
    }

    let parentNodeType = parentNode.nodeName.toLocaleLowerCase();
    
    if (variable.type == "module") {
      ndivNeeded = false;
      valueNode = cE("div");
      // valueNode.draggable = true;
      valueNode.className = "screenBox";

      let h2Node = cE("h2");
      h2Node.innerText = initCap(variable.id);
      valueNode.appendChild(h2Node);

      setupScreenBox(valueNode);
    }
    else if (variable.type == "table") {
      ndivNeeded = false;

      //add label in an extra row
      let pNode = cE("p");
      pNode.appendChild(labelNode);
      parentNode.appendChild(pNode); //add the table label to the parent

      //add table
      valueNode = cE("table");
      valueNode.className = "table-style"

      let theadNode = cE("thead");
      theadNode.appendChild(cE("tr"));
      valueNode.appendChild(theadNode); //row for header

      valueNode.appendChild(cE("tbody"));

      //variable.n will add the columns
    }
    else if (parentNodeType == "table") { 
      // console.log("tableChild", parentNode, parentNode.firstChild.firstChild, variable);
      //table header //no newNode created
      //table add the id in the header
      //rowNr = -1 for th so uiFun will be called and processed in changeHTML
      valueNode = cE("th");
      // valueNode.id = variable.id;
      valueNode.innerText = initCap(variable.id); //label uiFun response can change it
      parentNode.firstChild.firstChild.appendChild(valueNode); //<thead><tr> (containing th)

      // newNode = valueNode;
    } else if (variable.type == "select") {

      if (variable.ro) { //e.g. for reset/restart reason: do not show a select but only show the selected option
        valueNode = cE("span");
        if (variable.value) valueNode.innerText = variable.value;
        
        if (savedData[variable.id]) {
          // console.log("genHTML select table get savedData", valueNode, variable, savedData[variable.id]);
          changeHTML(valueNode, {"data":savedData[variable.id]});
        }
      }
      else {
        //<p> with <label><select> (<comment> in receiveData)

        valueNode = cE("select");
        valueNode.addEventListener('change', (event) => {console.log("select change", event);sendValue(event.target);});

        if (isPartOfTableRow) {
          //   valueNode.innerHTML = gId(variable.id).innerHTML; //gId(variable.id) is the <th> where uiFun assigned to option values to
          if (savedData[variable.id]) {
            // console.log("genHTML select table get savedData", valueNode, variable, savedData[variable.id]);
            changeHTML(valueNode, {"data":savedData[variable.id]});
          }
        }
      }
    }
    else if (variable.type == "canvas") {
      //<p><label><span><canvas>

      if (!isPartOfTableRow) {
        let pNode = cE("p");
        pNode.appendChild(labelNode);
        //3 lines of code to only add 🔍
        let spanNode = cE("span");
        spanNode.innerText= "🔍";
        pNode.appendChild(spanNode);
        parentNode.appendChild(pNode); //add the table label to the parent
      }

      valueNode = cE("canvas");
      // valueNode.id = variable.id;
      valueNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
    }
    else if (variable.type == "textarea") {
      if (!isPartOfTableRow) {
        let pNode = cE("p");
        pNode.appendChild(labelNode);
        //3 lines of code to only add 🔍
        let spanNode = cE("span");
        spanNode.innerText= "🔍";
        pNode.appendChild(spanNode);
        parentNode.appendChild(pNode); //add the table label to the parent
      }

      valueNode = cE("textarea");
      valueNode.readOnly = variable.ro;
      valueNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});

      if (variable.value) valueNode.innerText = variable.value;
    }
    else if (variable.type == "url") {

      valueNode = cE("a");
      valueNode.setAttribute('href', variable.value);
      // valueNode.setAttribute('target', "_blank"); //does not work well on mobile
      valueNode.innerText = variable.value;

    } else { //input

      //type specific actions
      if (variable.type == "checkbox") {
        valueNode = cE("input");
        valueNode.type = variable.type;
        valueNode.disabled = variable.ro;
        if (variable.value) valueNode.checked = variable.value;
        valueNode.addEventListener('change', (event) => {console.log(variable.type + " change", event);sendValue(event.target);});
      } else if (variable.type == "button") {
        valueNode = cE("input");
        valueNode.type = variable.type;
        valueNode.disabled = variable.ro;
        valueNode.value = initCap(variable.id);
        valueNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);sendValue(event.target);});
      } else if (variable.type == "range") {
        valueNode = cE("input");
        valueNode.type = variable.type;
        valueNode.min = variable.min?variable.min:0;
        valueNode.max = variable.max?variable.max:255; //range slider default 0..255
        valueNode.disabled = variable.ro;
        if (variable.value) valueNode.value = variable.value;
        //numerical ui value changes while draging the slider (oninput)
        valueNode.addEventListener('input', (event) => {
          if (gId(variable.id + "_rv")) {
            gId(variable.id + "_rv").innerText = variable.log?linearToLogarithm(variable, event.target.value):event.target.value;
          }
        });
        //server value changes after draging the slider (onchange)
        valueNode.addEventListener('change', (event) => {
          sendValue(event.target);
        });
        rangeValueNode = cE("span");
        rangeValueNode.id = variable.id + "_rv"; //rangeValue
        if (variable.value) rangeValueNode.innerText = variable.log?linearToLogarithm(variable, variable.value):variable.value;
      } else {
        //input types: text, search, tel, url, email, and password.

        if (variable.ro && variable.type != "button") {
          valueNode = cE("span");
          if (variable.value) valueNode.innerText = variable.value;
        } else {
          valueNode = cE("input");
          valueNode.type = variable.type;
          if (variable.value) valueNode.value = variable.value;
          valueNode.addEventListener('change', (event) => {console.log(variable.type + " change", event);sendValue(event.target);});
          // if (["text", "password", "number"].includes(variable.type) ) {
          //   buttonSaveNode = cE("text");
          //   buttonSaveNode.innerText = "✅";
          //   buttonSaveNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
          //   buttonCancelNode = cE("text");
          //   buttonCancelNode.innerText = "🛑";
          //   buttonCancelNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
          // }
          if (variable.type == "number") {
            valueNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
            if (variable.max) valueNode.max = variable.max;
          }
          else {
            if (variable.max) valueNode.setAttribute('maxlength', variable.max); //for text and textarea set max length valueNode.maxlength is not working for some reason
            if (variable.id == "serverName")
              gId("instanceName").innerText = variable.value;
          }
        }
      } //not checkbox or button or range

    } //input type
    
    if (variable.type == "module" || variable.type == "table" || variable.type == "canvas" || variable.type == "textarea" || parentNodeType == "table") {
      valueNode.id = variable.id;
      newNode = valueNode;
    } else if (!isPartOfTableRow) {
      // console.log(valueNode, variable);
      valueNode.id = variable.id;
      newNode.appendChild(valueNode); //add to <p>
    } else {
      valueNode.id = variable.id + "#" + rowNr;
      newNode = valueNode;
    }

    if (rangeValueNode) newNode.appendChild(rangeValueNode); //_rv value of range / sliders
    // if (buttonSaveNode) newNode.appendChild(buttonSaveNode);
    // if (buttonCancelNode) newNode.appendChild(buttonCancelNode);
    
    //disable drag of parent screenBox
    if (variable.type != "module") {
      valueNode.draggable = true;
      valueNode.addEventListener('dragstart', (event) => {event.preventDefault(); event.stopPropagation();});
    }

    if (parentNodeType != "table") parentNode.appendChild(newNode); //add new node to parent

    if (variable.n && parentNodeType != "table") { //multiple details
      //add a div with _n extension and details have this as parent
      if (ndivNeeded) {
        let divNode = cE("div");
        divNode.id = variable.id + "_n";
        divNode.className = "ndiv";
        newNode.parentNode.appendChild(divNode); // add to the parent of the node
        generateHTML(variable.n, divNode, rowNr);
      }
      else
        generateHTML(variable.n, newNode, rowNr); //details (e.g. module)
    }

    //don't call uiFun on table rows (the table header calls uiFun and propagate this to table row columns in changeHTML when needed - e.g. select)
    if (!isPartOfTableRow) {
      //call ui Functionality, if defined (to set label, comment, select etc)
      if (variable.uiFun >= 0) { //>=0 as element in var
        uiFunCommands.push(variable.id);
        if (uiFunCommands.length > 8) { //every 8 vars (to respect responseDoc size) check WS_EVT_DATA info
          flushUIFunCommands();
        }
      }
    }
      
    // if (rowNr != -1)
    //   newNode.id += "#" + rowNr;
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

//process json from server, json is assumed to be an object
function receiveData(json) {
  // console.log("receiveData", json);

  if (Object.keys(json)) {
    for (var key of Object.keys(json)) {
      let value = json[key];
      //special commands
      if (key == "uiFun") {
        console.log("receiveData no action", key, value); //should not happen anymore
      }
      else if (key == "view") { //should not happen anymore
        console.log("receiveData no action", key, value);
      }
      else if (key == "canvasData") { //should not happen anymore
        console.log("receiveData no action", key, value);
      }
      else if (key == "details") {
        let variable = value;
        //if var object with .n, create .n (e.g. see setEffect and fixtureGenChFun, tbd: )
        console.log("receiveData details", key, variable);
        if (gId(variable.id + "_n")) gId(variable.id + "_n").remove(); //remove old ndiv

        //create new ndiv
        if (variable.n) {
          let divNode = cE("div");
          divNode.id = variable.id + "_n";
          divNode.className = "ndiv";
          gId(variable.id).parentNode.appendChild(divNode);
          generateHTML(variable.n, divNode);
        }
        flushUIFunCommands(); //make sure uiFuns of new elements are called
      }
      else if (key == "updrow") { //update the row of a table
        // console.log("receiveData", key, value);
        for (var tableId of Object.keys(value)) {
          let tableRows = value[tableId];
          let tableNode = gId(tableId);
          // console.log("  ", tableNode);
          if (Array.isArray(tableRows)) {
            for (let tableRow of tableRows) {
              // console.log("  ", tableId, tableRow);
            }
          }
          for (var rowNr = 0, row; row = tableNode.rows[rowNr]; rowNr++) {
            if (rowNr != 0 && row.cells[0].innerText == tableRows[0][0]) {
              // console.log("  row", i, row);
              for (var colNr = 0, col; col = row.cells[colNr]; colNr++) { //coll is a <td>
                // console.log("  cell", i, j, col);
                changeHTML(col.firstChild, {value:tableRows[0][colNr]}); //<td>.firstChild is the cell e.g. <select>
              }  
            }
          }
        } //tableId
      }
      else { //{variable:{label:value}}
        if (gId(key)) { //is the key a var?
          changeHTML(gId(key), value);
        }
        else
          console.log("receiveData id not found in dom", key, value);
      }
    } //for keys
  } //isObject
  else
    console.log("receiveData no Object", object);
} //receiveData

//do something with an existing (variable) node, key is an existing node, json is what to do with it
function changeHTML(node, commandJson) {
  let overruleValue = false;

  let nodeType = node.nodeName.toLocaleLowerCase();

  // let node = gId(node.id);
  // if (rowNr != -1)
  //   node = gId(node.id + "#" + rowNr);
  
  if (commandJson.hasOwnProperty("label")) {
    // if (node.id != "insTbl") // tbd: table should not update
    //   console.log("changeHTML label", node.id, commandJson.label);
    if (nodeType == "input" && node.type == "button") {
      node.value = initCap(commandJson.label);
    }
    else {
      let labelNode;
      if (nodeType == "canvas" || nodeType == "table")
        labelNode = node.previousSibling.firstChild; //<p><label> before <canvas> or <table>
      else if (nodeType == "th") //table header
        labelNode = node; //the <th>
      else
        labelNode = node.parentNode.firstChild; //<label> before <span or input> within <p>
      labelNode.innerText = initCap(commandJson.label);
    }
  } //label

  if (commandJson.hasOwnProperty("comment")) {
    
    if (nodeType != "th") { //no comments on table header
      // normal: <p><label><input id><comment></p>
      // table or canvas <p><label><comment></p><canvas id>
      // 1) if exist then replace else add
      let parentNode;
      if (nodeType == "canvas" || nodeType == "textarea" || nodeType == "table")
        parentNode = node.previousSibling; //<p><label> before <canvas> or <table> or <textarea>
      else
        parentNode = node.parentNode;
      
      // if (node.id != "insTbl") // tbd: table should not update
      //   console.log("changeHTML comment", node, commandJson.comment);

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
      commentNode.innerText = commandJson.comment;
    }
    else { //th
      // console.log("changeHTML comment", node, commandJson.comment);
      let divNode = cE("div");
      divNode.innerText = node.innerText;
      node.innerText = "";
      divNode.classList.add("tooltip");
      let spanNode = cE("span");
      spanNode.innerHTML = commandJson.comment;
      spanNode.classList.add("tooltiptext");
      divNode.appendChild(spanNode);

      node.appendChild(divNode);

    }
  } //comment

  if (commandJson.hasOwnProperty("data")) { //replace the body of a table
    // console.log("changeHTML data", node, commandJson);

    if (nodeType == "table") {
      //remove table rows
      let tbodyNode = cE('tbody'); //the tbody of node will be replaced
      
      //find model info
      let variable = findVar(node.id); //node.id is the table where no are the columns
      // if (node.id != "insTbl") // tbd: table should not update
      //   console.log("changeHTML table", node.id, commandJson.data);

      //add each row
      let rowNr = 0;
      for (var row of commandJson.data) {
        let trNode = cE("tr");
        //add each column
        let colNr = 0;
        for (var columnRow of row) {              
          let tdNode = cE("td");

          //call generateHTML to create the variable in the UI
          // if (variable.id == "insTbl")
          //   console.log("table cell generateHTML", tdNode, variable, variable.n, colNr, rowNr);
          let columnVar = variable.n[colNr]; //find the column definition in the model
          //table cell at row e.g. id: "flName"; type: "text"...
          let newNode = generateHTML(columnVar, tdNode, rowNr); //no <p><label>
          if (newNode) {
            //very strange: gId(newNode.id) is not working here. Delay before it is in the dom??? (workaround create changeHTML function)
            let updateJson;
            if (typeof columnRow == 'number' || typeof columnRow == 'boolean')
              updateJson = `{"value":${columnRow}}`;
            else
              updateJson = `{"value":"${columnRow}"}`
            // console.log("tablecolumn", rowNr, colNr, newNode, columnVar, updateJson, JSON.parse(updateJson), gId(newNode.id));
            //call changeHTML to give the variable a value
            changeHTML(newNode, JSON.parse(updateJson));
          }

          trNode.appendChild(tdNode);
          colNr++;
        }
        tbodyNode.appendChild(trNode);
        rowNr++;
      }
      //replace the table body
      node.replaceChild(tbodyNode, node.lastChild); //replace <table><tbody> by tbodyNode

      if (node.id == "insTbl")
        setInstanceTableColumns();
    }
    else if (nodeType == "select") {
      //remove all old options first
      var index = 0;
      while (node.options && node.options.length > 0) {
        node.remove(0);
      }
      for (var value of commandJson.data) {
        let optNode = cE("option");
        if (Array.isArray(value)) {
          optNode.value = value[0];
          optNode.text = value[1];
        }
        else {
          optNode.value = index;
          optNode.text = value;
        }
        node.appendChild(optNode);
        index++;
      }
    }
    else if (nodeType == "span") { //readonly select. tbd: only the displayed value needs to be in the select
      var index = 0;
      for (var value of commandJson.data) {
        if (parseInt(node.textContent) == index) {
          // console.log("changeHTML select1", value, node, node.textContent, index);
          node.textContent = value; //replace the id by its value
          // console.log("changeHTML select2", value, node, node.textContent, index);
          overruleValue = true; //in this case we do not want the value set
        }
        index++;
      }
    }
    else { //data not assigned (e.g. table headers) -> store when needed
        // console.log("changeHTML savedData", nodeType, node, commandJson);
        savedData[node.id] = commandJson.data;
    }

  } //data

  if (commandJson.hasOwnProperty("value") && !overruleValue) { //overruleValue: select sets already the option
    //hasOwnProperty needed to catch also boolean commandJson.value when it is false
    // if (node.id=="pro" || node.id=="insfx")// || node.id=="mdlEnabled" || node.id=="clIsFull" || node.id=="pin2")
    //   console.log("changeHTML value", node.id, commandJson, commandJson.value, node);
    if (nodeType == "span") //read only vars
      node.textContent = commandJson.value;
    else if (nodeType == "a") { //url links
      node.innerText = "🔍";
      node.setAttribute('href', commandJson.value);
    } else if (nodeType == "canvas")
      userFunId = node.id; //prepare for websocket data
    else if (node.type == "checkbox")
      node.checked = commandJson.value;
    else if (node.type == "button") {
      // console.log("button", node, commandJson);
      if (commandJson.value) node.value = commandJson.value; //else the id / label is used as button label
    }
    else if (Array.isArray(commandJson.value)) { //table column
      let rowNr = 0;
      for (let val of commandJson.value) {
        // console.log(node.id, gId(node.id + "#" + rowNr), val);
        if (gId(node.id + "#" + rowNr)) {
          if (gId(node.id + "#" + rowNr).type == "checkbox")
            gId(node.id + "#" + rowNr).checked = val;
          else
            gId(node.id + "#" + rowNr).value = val;
        }
        rowNr++;
      }
      // node.checked = commandJson.value;
    } else {//inputs or select
      node.value = commandJson.value;
      node.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input
    }
  } //value

  if (commandJson.hasOwnProperty("json")) { //json send html nodes cannot process, store in jsonValues array
    console.log("changeHTML json", node.id, commandJson.json, node);
    jsonValues[node.id] = commandJson.json;
  }

  if (commandJson.hasOwnProperty("file")) { //json send html nodes cannot process, store in jsonValues array
    console.log("changeHTML file", node.id, commandJson.file, node);
  
    //we need to send a request which the server can handle using request variable
    let url = `http://${window.location.hostname}/file`;
    fetchAndExecute(url, commandJson.file, node.id, function(id, text) { //send node.id as parameter
      // console.log("fetchAndExecute", text); //in case of invalid commandJson
      var ledmapJson = JSON.parse(text);
      jsonValues[id] = ledmapJson;
      jsonValues[id].new = true;
      console.log("fetchAndExecute", jsonValues);
    }); 
  }
} //changeHTML

function findVar(id, parent = model) {
  // console.log("findVar", id, parent, model);

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

function sendValue(element) {
  let varId;
  if (element.id == "saveModel" || element.id == "bSave") {
    varId = "saveModel";
    gId("bSave").value = "Save";
    gId("bSave").disabled = true;
  }
  else 
  {
    varId = element.id;
    gId("bSave").value = "Save*";
    gId("bSave").disabled = false;
  }

  var command = {};
  if (element.type == "checkbox")
    command[varId] = element.checked;
  else if (element.nodeName.toLocaleLowerCase() == "span")
    command[varId] = element.innerText;
  else
    command[varId] = Number(element.value)?Number(element.value):element.value; //type number is default but html converts numbers in <option> to string
  // console.log("sendValue", command);

  
  requestJson(command);
}

let isModal = false;
let modalPlaceHolder;

function toggleModal(element) {
  // console.log("toggleModal", element);
  isModal = !isModal;

	if (isModal) {

    modalPlaceHolder = cE(element.nodeName.toLocaleLowerCase()); //create canvas or textarea
    modalPlaceHolder.width = element.width;
    modalPlaceHolder.height = element.height;

    element.parentNode.replaceChild(modalPlaceHolder, element); //replace by modalPlaceHolder

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
    
    modalPlaceHolder.parentNode.replaceChild(element, modalPlaceHolder); // //replace by element. modalPlaceHolder loses rect
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

function setInstanceTableColumns() {
  // let insCols = ["insName", "insLink", "insIp","insType"];
  // let insTrNode = gId("insName").parentNode;

  let tbl = gId("insTbl");
  let toStage = tbl.parentElement.parentElement.className != "screenColumn";
  let thead = tbl.getElementsByTagName('thead')[0];
  let tbody = tbl.getElementsByTagName('tbody')[0];

  function showHideColumn(colNr, doHide) {
    thead.firstChild.childNodes[colNr].hidden = doHide;
    for (let row of tbody.childNodes) {
      // console.log("   row", row, row.childNodes, i);
      if (colNr < row.childNodes.length) //currently there are comments in the table header ...
        row.childNodes[colNr].hidden = doHide;
    }
  }

  // console.log("setInstanceTableColumns", tbl, thead, tbody);
  columnNr = 2;
  for (; columnNr<6; columnNr++) {
    showHideColumn(columnNr, toStage);
  }
  for (; columnNr<thead.firstChild.childNodes.length; columnNr++) {
    showHideColumn(columnNr, !toStage);
  }

  gId("sma").parentNode.hidden = toStage; //hide sync master label field and comment
}

function showHideModules(node) {

  function toggleInstances(toStage) {
    let child = gId("Instances");
    if ((toStage && child.parentElement.className == "screenColumn") || (!toStage && child.parentElement.className != "screenColumn")) {
      //move back to screenColumn3
      modalPlaceHolder = cE("div");
      modalPlaceHolder.id = toStage?"instPH2":"instPH";
      child.parentNode.replaceChild(modalPlaceHolder, child); //replace by modalPlaceHolder
      let element = gId(toStage?"instPH":"instPH2");
      element.parentNode.replaceChild(child, element); //replace by child
      // gId("instPH").remove();
    }
    setInstanceTableColumns();
  }

  let sysMods = ["Files", "Print", "System","Network","Model", "Pins", "Modules", "Web", "UI", "Instances"];
  let panelParentNode = gId("screenColumn0").parentNode;
  // console.log("showHideModules", node, node.value, node.id, panelParentNode, panelParentNode.childNodes);

  gId("vApp").style.background = "none";
  gId("vStage").style.background = "none";
  // gId("vUser").style.background = "none";
  gId("vSys").style.background = "none";
  gId("vAll").style.background = "none";
  node.style.backgroundColor = "#FFFFFF";

  switch (node.id) {
    case "vApp":
      toggleInstances(false); //put Instance back if needed

      //hide all system modules, show the rest
      for (let screenColumn of panelParentNode.childNodes) {
        for (let child of screenColumn.childNodes) {
          child.hidden = sysMods.includes(child.id);
        }
      }

      break;
    case "vStage":
      
      //hide all modules but show instances
      for (let screenColumn of panelParentNode.childNodes) {
        for (let child of screenColumn.childNodes) {
          child.hidden = child.id != "Instances";
        }
      }

      toggleInstances(true);

      // for (let child of insTrNode.childNodes) {
      //   child.hidden = false;
      // }
      // for (let i=4 ; insTrNode.childNodes.length; i++)
      //   show_hide_column("insTbl", i, true)

      break;
    case "vSys":
      //set all modules but sys hidden
      for (let screenColumn of panelParentNode.childNodes) {
        for (let child of screenColumn.childNodes) {
          child.hidden = !sysMods.includes(child.id);
        }
      }

      toggleInstances(false);
      
      // for (let child of insTrNode.childNodes) {
      //   child.hidden = !insCols.includes(child.id);
      // }
      // for (let i=4 ; insTrNode.childNodes.length; i++)
      //   show_hide_column("insTbl", i, false)

      break;
    case "vAll":
      toggleInstances(false); //put Instance back if needed
      setInstanceTableColumns();

      //set all modules visible
      for (let screenColumn of panelParentNode.childNodes) {
        for (let child of screenColumn.childNodes) {
          child.hidden = false;
        }
      }
      break;
  }

  //save the current view
  var command = {};
  command["view"] = node.id;
  // console.log("setInput", command);

  requestJson(command);
} //showHideModules

function saveModel(node) {
  console.log("saveModel", node);

  sendValue(node);
}