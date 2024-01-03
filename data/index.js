// @title     StarMod
// @file      index.css
// @date      20231016
// @repo      https://github.com/ewowi/StarMod
// @Authors   https://github.com/ewowi/StarMod/commits/main
// @Copyright (c) 2023 Github StarMod Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007

let d = document;
let ws = null;

let mdlColumnNr = 0;
let nrOfMdlColumns = 4;
let userFunId = "";
let htmlGenerated = false;
let jsonValues = {};
let uiFunCommands = [];
let model = null; //model.json (as send by the server), used by FindVar
let savedView = null;
let savedData = {};
let varNodeMapping = {};

function gId(c) {return d.getElementById(c);}
function cE(e) { return d.createElement(e); }

function handleVisibilityChange() {
  console.log("handleVisibilityChange");
}

function onLoad() {
  makeWS();

  initMdlColumns();

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
            flushUIFunCommands();
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
    //if root (type module) add the html to one of the mdlColumns
    if (parentNode == null) {
      parentNode = gId("mdlColumn" + mdlColumnNr);
      mdlColumnNr = (mdlColumnNr +1)%nrOfMdlColumns;
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

    let divNode = null; //divNode will be appended to the parentNode after if then else and returned
    let varNode = null; //the node containing the variable
    let rangeValueNode = null;
    // let buttonSaveNode = null;
    // let buttonCancelNode = null;

    let ndivNeeded = true; //for details ("n"), module and table do not need an extra div for details
       
    let labelNode = cE("label");
    labelNode.innerText = initCap(variable.id); // the default when not overridden by uiFun
    
    let isPartOfTableRow = (rowNr != -1);

    let parentNodeType = parentNode.nodeName.toLocaleLowerCase();

    divNode = cE("div");

    if (parentNodeType != "td") {
      if (variable.type != "button") divNode.appendChild(labelNode); //add label (tbd:must be done by childs n table cell)
    }

    if (variable.type == "module") {
      ndivNeeded = false;

      varNode = cE("div");
      let h2Node = cE("h2");
      h2Node.innerText = initCap(variable.id);
      varNode.appendChild(h2Node);

      setupModule(varNode); //enable drag and drop of modules
    }
    else if (variable.type == "table") {
      ndivNeeded = false;

      //add table
      varNode = cE("table");

      let theadNode = cE("thead");
      theadNode.appendChild(cE("tr"));
      varNode.appendChild(theadNode); //row for header

      varNode.appendChild(cE("tbody"));

      //variable.n will add the columns
    }
    else if (parentNodeType == "table") { 
      // console.log("tableChild", parentNode, parentNode.firstChild.firstChild, variable);

      varNode = cE("th");
      varNode.innerText = initCap(variable.id); //label uiFun response can change it
      parentNode.firstChild.firstChild.appendChild(varNode); //<thead><tr> (containing th)

    } else if (variable.type == "select") {

      if (variable.ro) { //e.g. for reset/restart reason: do not show a select but only show the selected option
        varNode = cE("span");
      }
      else {
        varNode = cE("select");
        varNode.addEventListener('change', (event) => {console.log("select change", event);sendValue(event.target);});
      }

      if (json.value) varNode.setAttribute("value", json.value); //custom attribute
    }
    else if (variable.type == "canvas") {
      //3 lines of code to only add 🔍
      let spanNode = cE("span");
      spanNode.innerText= "🔍";
      divNode.appendChild(spanNode);
      divNode.appendChild(cE("br"));

      varNode = cE("canvas");
      varNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
    }
    else if (variable.type == "textarea") {

      //3 lines of code to only add 🔍
      let spanNode = cE("span");
      spanNode.innerText= "🔍";
      divNode.appendChild(spanNode);
      divNode.appendChild(cE("br"));

      varNode = cE("textarea");
      varNode.readOnly = variable.ro;
      varNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});

      if (variable.value) varNode.innerText = variable.value;
    }
    else if (variable.type == "url") {

      varNode = cE("a");
      varNode.setAttribute('href', variable.value);
      // varNode.setAttribute('target', "_blank"); //does not work well on mobile
      varNode.innerText = variable.value;

    } else { //input

      //type specific actions
      if (variable.type == "checkbox") {
        varNode = cE("input");
        varNode.type = variable.type;
        varNode.disabled = variable.ro;
        if (variable.value) varNode.checked = variable.value;
        varNode.addEventListener('change', (event) => {console.log(variable.type + " change", event);sendValue(event.target);});
      } else if (variable.type == "button") {
        varNode = cE("input");
        varNode.type = variable.type;
        varNode.disabled = variable.ro;
        varNode.value = initCap(variable.id);
        varNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);sendValue(event.target);});
      } else if (variable.type == "range") {
        varNode = cE("input");
        varNode.type = variable.type;
        varNode.min = variable.min?variable.min:0;
        varNode.max = variable.max?variable.max:255; //range slider default 0..255
        varNode.disabled = variable.ro;
        if (variable.value) varNode.value = variable.value;
        //numerical ui value changes while draging the slider (oninput)
        let rvNode = variable.id + (isPartOfTableRow?"#" + rowNr:"") + "_rv";
        varNode.addEventListener('input', (event) => {
          if (gId(rvNode)) {
            gId(rvNode).innerText = variable.log?linearToLogarithm(variable, event.target.value):event.target.value;
          }
        });
        //server value changes after draging the slider (onchange)
        varNode.addEventListener('change', (event) => {
          sendValue(event.target);
        });
        rangeValueNode = cE("span");
        rangeValueNode.id = rvNode; //rangeValue
        if (variable.value) rangeValueNode.innerText = variable.log?linearToLogarithm(variable, variable.value):variable.value;
      } else {
        //input types: text, search, tel, url, email, and password.

        if (variable.ro && variable.type != "button") {
          varNode = cE("span");
          if (variable.value) varNode.innerText = variable.value;
        } else {
          varNode = cE("input");
          varNode.type = variable.type;
          if (variable.value) varNode.value = variable.value;
          varNode.addEventListener('change', (event) => {console.log(variable.type + " change", event);sendValue(event.target);});
          // if (["text", "password", "number"].includes(variable.type) ) {
          //   buttonSaveNode = cE("text");
          //   buttonSaveNode.innerText = "✅";
          //   buttonSaveNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
          //   buttonCancelNode = cE("text");
          //   buttonCancelNode.innerText = "🛑";
          //   buttonCancelNode.addEventListener('click', (event) => {console.log(variable.type + " click", event);});
          // }
          if (variable.type == "number") {
            varNode.min = variable.min?variable.min:0; //if not specified then unsigned value (min=0)
            if (variable.max) varNode.max = variable.max;
          }
          else {
            if (variable.max) varNode.setAttribute('maxlength', variable.max); //for text and textarea set max length varNode.maxlength is not working for some reason
            if (variable.id == "serverName")
              gId("instanceName").innerText = variable.value;
          }
        }
      } //not checkbox or button or range

    } //input type
    
    if (parentNodeType == "table") {
      varNode.id = variable.id;
      divNode = varNode;
    } else {
      varNode.id = variable.id + (isPartOfTableRow?"#" + rowNr:"");
      varNode.className = variable.type;
      divNode.appendChild(varNode); //add to <div>
    }

    if (rangeValueNode) divNode.appendChild(rangeValueNode); //_rv value of range / sliders
    // if (buttonSaveNode) divNode.appendChild(buttonSaveNode);
    // if (buttonCancelNode) divNode.appendChild(buttonCancelNode);
    
    //disable drag of parent module
    if (variable.type != "module") {
      varNode.draggable = true;
      varNode.addEventListener('dragstart', (event) => {event.preventDefault(); event.stopPropagation();});
    }

    if (parentNodeType != "table") parentNode.appendChild(divNode); //add new node to parent

    if (variable.n && parentNodeType != "table") { //multiple details, not for table header
      //add a div with _n extension and details have this as parent
      if (ndivNeeded) {
        let ndivNode = cE("div");
        ndivNode.id = variable.id + (isPartOfTableRow?"#" + rowNr:"") + "_n";
        ndivNode.className = "ndiv";
        divNode.appendChild(ndivNode); // add to the parent of the node
        generateHTML(variable.n, ndivNode, rowNr);
      }
      else
        generateHTML(variable.n, varNode, rowNr); //details (e.g. module)
    }

    //don't call uiFun on table rows (the table header calls uiFun and propagate this to table row columns in changeHTML when needed - e.g. select)
    if (savedData[variable.id]) {
      // if (variable.value) savedData[variable.id].value = variable.value; // save default value (for select)
      // console.log("genHTML use savedData", variable, varNode, savedData[variable.id]);
      // console.log("genHTML select table get savedData", varNode, variable, savedData[variable.id]);
      changeHTML(variable, varNode, savedData[variable.id], rowNr);
    }
    else {
      // if (variable.value) {
      //   savedData[variable.id] = {};
      //   savedData[variable.id].value = variable.value; // save default value (for select)
      // }
    
      // if (!isPartOfTableRow && !savedData[variable.id]) {
      //call ui Functionality, if defined (to set label, comment, select etc)
      if (variable.uiFun >= 0) { //>=0 as element in var
        uiFunCommands.push(variable.id);
        if (uiFunCommands.length > 8) { //every 8 vars (to respect responseDoc size) check WS_EVT_DATA info
          flushUIFunCommands();
        }
      }
    }

    if (isPartOfTableRow) {
      if (!varNodeMapping[variable.id]) varNodeMapping[variable.id] = [];
      varNodeMapping[variable.id].push(varNode.id);
    }
      
    return varNode;
  } //not an array but variable
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

      //tbd: for each node of a variable (rowNr)

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
        let rowNr = variable["rowNr"]!=null?variable["rowNr"]:-1;
        let nodeId = variable.id + ((rowNr != -1)?"#" + rowNr:"");
        //if var object with .n, create .n (e.g. see setEffect and fixtureGenChFun, tbd: )
        console.log("receiveData details", key, variable);
        if (gId(nodeId + "_n")) gId(nodeId + "_n").remove(); //remove old ndiv

        //create new ndiv
        if (variable.n) {
          let ndivNode = cE("div");
          ndivNode.id = nodeId + "_n";
          ndivNode.className = "ndiv";
          gId(nodeId).parentNode.appendChild(ndivNode);
          generateHTML(variable.n, ndivNode);
        }
        flushUIFunCommands(); //make sure uiFuns of new elements are called
      }
      else if (key == "updrow") { //update the row of a table
        console.log("receiveData", key, value);
        for (var tableId of Object.keys(value)) { //currently only one table
          let tableRows = value[tableId];
          let tableNode = gId(tableId);
          let tableVar = findVar(tableId);
          // console.log("updrow main", tableId, tableRows, tableNode, tableVar);

          for (var nodeRowNr = 1, rowNode; rowNode = tableNode.rows[nodeRowNr]; nodeRowNr++) { //<table> rows starting with header row
            let rowNr = nodeRowNr - 1;
            // console.log("  noderow", rowNr, rowNode);

            if (Array.isArray(tableRows)) {
              for (let tableRow of tableRows) {
                // console.log("  tablerow", tableId, tableRow);

                //loop over all column vars
                let colNr = 0;
                let found = false;
                for (let colVar of tableVar.n) {
                  let colNode = gId(colVar.id + "#" + rowNr); 
                  if (colNode) {
                    let colValue = tableRow[colNr];
                    // console.log("    ", colVar, colNode, colValue);
  
                    if (colNr == 0) { //check on the value of the first table column: tbd: check other columns?
                      found = colNode.innerText == colValue;
                      //innerText is assuming span like node. tbd: others
                    } else if (found) { //columns 1..n
                      changeHTML(colVar, colNode, {value:colValue, "chk":1}, rowNr);
                    }
  
                  }
                  else
                    console.log("   node not found", colVar.id + "#" + rowNr, colVar);
                  colNr++;
                }

              }
            }

          }
        } //tableId
      }
      else { //{variable:{label:value}}
        let variable = findVar(key);

        if (variable) {
          if (varNodeMapping[variable.id]) {
            for (let nodeId of varNodeMapping[variable.id] ) {
              let rowNr = nodeId.split("#")[1];
              if (gId(nodeId)) { //is the key a var?
                changeHTML(variable, gId(nodeId), value, rowNr);
              }
              else
                console.log("receiveData id not found in dom", nodeId, value);
            }
  
          } else {
            if (gId(key)) { //is the key a var?
              changeHTML(variable, gId(key), value);
            }
            else
              console.log("receiveData id not found in dom", key, value);
          }
        }
        else
          console.log("receiveData key is no variable", key, value);
      }
    } //for keys
  } //isObject
  else
    console.log("receiveData no Object", object);
} //receiveData

//do something with an existing (variable) node, key is an existing node, json is what to do with it
function changeHTML(variable, node, commandJson, rowNr = -1) {

  if (!node)
    console.log("changeHTML", variable, node, commandJson, rowNr);
  let nodeType = node.nodeName.toLocaleLowerCase();
  let isPartOfTableRow = (rowNr != -1);

  if (commandJson.hasOwnProperty("label")) {
    // if (node.id != "insTbl") // tbd: table should not update
    //   console.log("changeHTML label", node.id, commandJson.label);
    if (nodeType == "input" && node.type == "button") {
      node.value = initCap(commandJson.label);
    }
    else {
      let labelNode;
      // if (nodeType == "canvas" || nodeType == "table")
      //   labelNode = node.previousSibling.firstChild; //<p><label> before <canvas> or <table>
      if (nodeType == "th") //table header
        labelNode = node; //the <th>
      else {
        labelNode = gId(node.id).parentNode.querySelector("label");
      }
      if (labelNode) labelNode.innerText = initCap(commandJson.label);
    }

    if (!savedData[variable.id]) savedData[variable.id] = {};
    savedData[variable.id].label = commandJson.label;
  } //label

  if (commandJson.hasOwnProperty("comment")) {
    
    if (nodeType != "th") {
      // if (node.id != "insTbl") // tbd: table should not update
      //   console.log("changeHTML comment", node, commandJson.comment);

      //only add comment if there is a label
      let labelNode = node.parentNode.querySelector('label');
      if (labelNode) {
        let commentNode = node.parentNode.querySelector('comment');
        // console.log("commentNode", commentNode);
        if (!commentNode) { //create if not exist
          commentNode = cE("comment");
          node.parentNode.appendChild(commentNode);
        }
        commentNode.innerText = commandJson.comment;
      }
    }
    else { //th
      // console.log("changeHTML comment", node, commandJson.comment);
      let ttdivNode = cE("div");
      ttdivNode.innerText = node.innerText;
      node.innerText = "";
      ttdivNode.classList.add("tooltip");
      let spanNode = cE("span");
      spanNode.innerHTML = commandJson.comment;
      spanNode.classList.add("tooltiptext");
      ttdivNode.appendChild(spanNode);

      node.appendChild(ttdivNode);

    }
    if (!savedData[variable.id]) savedData[variable.id] = {};
    savedData[variable.id].comment = commandJson.comment;
  } //comment

  if (commandJson.hasOwnProperty("data")) { //replace the body of a table
    // console.log("changeHTML data", node, commandJson);

    if (nodeType == "table") {
      //remove table rows
      let tbodyNode = cE('tbody'); //the tbody of node will be replaced
      //replace the table body
      node.replaceChild(tbodyNode, node.lastChild); //replace <table><tbody> by tbodyNode  //add to dom asap

      // if (node.id != "insTbl") // tbd: table should not update
      //   console.log("changeHTML table", node.id, commandJson.data);

      //add each row
      let rowNr = 0;
      for (var row of commandJson.data) {
        let trNode = cE("tr");
        tbodyNode.appendChild(trNode); //add to dom asap

        //add each column
        let colNr = 0;
        for (var columnRow of row) {              
          let tdNode = cE("td");
          trNode.appendChild(tdNode); //add to dom asap

          //call generateHTML to create the variable in the UI
          // if (variable.id == "insTbl")
          //   console.log("table cell generateHTML", tdNode, variable, variable.n, colNr, rowNr);
          let columnVar = variable.n[colNr]; //find the column definition in the model
          //table cell at row e.g. id: "flName"; type: "text"...
          let varNode = generateHTML(columnVar, tdNode, rowNr); //no <p><label>
          //find the node with id

          if (varNode) {
            let updateJson;
            if (typeof columnRow == 'number' || typeof columnRow == 'boolean')
              updateJson = `{"value":${columnRow}, "chk":2}`;
            else
              updateJson = `{"value":"${columnRow}", "chk":3}`;
            //call changeHTML to give the variable a value
            changeHTML(columnVar, varNode, JSON.parse(updateJson), rowNr);
          }
          else
            console.log("chHTML tablecolumn not found", rowNr, colNr, varNode, columnVar, columnVar.id + "#" + rowNr);

          colNr++;
        }
        rowNr++;
      }

      flushUIFunCommands(); //make sure uiFuns of new elements are called

      if (variable.id == "insTbl")
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
      let valuex = node.getAttribute("value");
      if (valuex) node.value = valuex;
    }
      
    if (!savedData[variable.id]) savedData[variable.id] = {};
    savedData[variable.id].data = commandJson.data;
  } //data

  if (commandJson.hasOwnProperty("value")) {
    //hasOwnProperty needed to catch also boolean commandJson.value when it is false
    // if (node.id=="insName#0" || node.id=="fx")// || node.id=="mdlEnabled" || node.id=="clIsFull" || node.id=="pin2")
    //   console.log("changeHTML value", node.id, commandJson, commandJson.value, node);
    if (Array.isArray(commandJson.value)) { //table column

      if (rowNr == -1) {
        let rowNr = 0;
        for (let val of commandJson.value) {
          let nodeId = node.id + "#" + rowNr; //tbd: not variable id? using node.id var#x#y possible for nested tables?
          if (gId(nodeId)) {
            // console.log("changeHTML value array recursive", variable, node.id, gId(nodeId), val);
            changeHTML(variable, gId(nodeId), {"value":val, "chk":4}, rowNr); //recursive set value for variable in row
          }
          else
            console.log("changeHTML node not found", nodeId, node, commandJson);
          rowNr++;
        }
      }
      else {
        changeHTML(variable, node, {"value":commandJson.value[rowNr], "chk":5}, rowNr); //recursive set value for variable in row
      }
      // node.checked = commandJson.value;
    } else if (nodeType == "span") //read only vars
      if (node.className == "select") {
        var index = 0;
        // if (!savedData[variable.id])
        
        if (savedData[variable.id] && savedData[variable.id].data) { // not always the case e.g. data / table / uiFun. Then value set if uiFun returns
          for (var value of savedData[variable.id].data) {
            if (parseInt(commandJson.value) == index) {
              // console.log("changeHTML select1", value, node, node.textContent, index);
              node.textContent = value; //replace the id by its value
              // console.log("changeHTML select2", value, node, node.textContent, index);
            }
            index++;
          }
        } else
          node.textContent = commandJson.value;
          // console.log("changehtml no saveddata", savedData[variable.id], variable, node, commandJson);
      }
      else
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
    else {//inputs or select
      node.value = commandJson.value;
      node.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input
    }
    // savedData[variable.id].value = commandJson.value;
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
function initMdlColumns() {

  let columns = document.querySelectorAll('.mdlContainer .mdlColumn');
  columns.forEach(function(column) {
    column.addEventListener('dragover', handleDragOver);
    column.addEventListener('dragenter', handleDragEnter);
    column.addEventListener('dragleave', handleDragLeave);
    column.addEventListener('drop', handleDrop);
  });

  setupModules();
  
}

function setupModules() {
  let modules = document.querySelectorAll('.mdlContainer .module');
  modules.forEach(function(box) {
    setupModule(box);
  });

}

// var lastPage;
function setupModule(item) {
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

  let modules = document.querySelectorAll('.mdlContainer .module');
  modules.forEach(function (item) {
    item.classList.remove('over');
  });

  let columns = document.querySelectorAll('.mdlContainer .mdlColumn');
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
    setupModule(clone);
    removeDragStyle(clone);

    if (this.id.includes("mdlColumn")) {
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
  let mdlContainer = gId("mdlContainer");
  // let isStageView = tbl.parentNode.parentNode.parentNode.className != "mdlColumn";
  let isStageView = !mdlContainer.contains(tbl);
  let thead = tbl.getElementsByTagName('thead')[0];
  let tbody = tbl.getElementsByTagName('tbody')[0];

  function showHideColumn(colNr, doHide) {
    // console.log("showHideColumn", thead.parentNode.parentNode, colNr, doHide);
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
    showHideColumn(columnNr, isStageView);
  }
  for (; columnNr<thead.firstChild.childNodes.length; columnNr++) {
    showHideColumn(columnNr, !isStageView);
  }

  gId("sma").parentNode.hidden = isStageView; //hide sync master label field and comment
}

function showHideModules(node) {

  function toggleInstances(isStageView) {
    let module = gId("Instances").parentNode;
    let container = gId("mdlContainer");
    // console.log("toggleInstances", module, container, isStageView);
    if ((isStageView && container.contains(module)) || (!isStageView && !container.contains(module))) {

      modalPlaceHolder = cE("div");
      modalPlaceHolder.id = isStageView?"instPH2":"instPH";
      module.parentNode.replaceChild(modalPlaceHolder, module); //replace by modalPlaceHolder
      let element = gId(isStageView?"instPH":"instPH2");
      element.parentNode.replaceChild(module, element); //replace by module
      // gId("instPH").remove();
    }
    setInstanceTableColumns();
  }

  let sysMods = ["Files", "Print", "System", "Network", "Model", "Pins", "Modules", "Web", "UI", "Instances"];
  let mdlContainerNode = gId("mdlContainer"); //class mdlContainer
  // console.log("showHideModules", node, node.value, node.id, mdlContainerNode, mdlContainerNode.childNodes);

  gId("vApp").style.background = "none";
  gId("vStage").style.background = "none";
  // gId("vUser").style.background = "none";
  gId("vSys").style.background = "none";
  gId("vAll").style.background = "none";
  node.style.backgroundColor = "#FFFFFF";

  switch (node.id) {
    case "vApp":
    case "vSys":
      toggleInstances(false); //put Instance back if needed

      //hide all system modules, show the rest
      for (let mdlColumn of mdlContainerNode.childNodes) {
        for (let module of mdlColumn.childNodes) {
          module.hidden = sysMods.includes(module.id);
          let found = false;
          for (let sysMod of sysMods) {
            if (module.contains(gId(sysMod))) {
              found = true;
              break;
            }
          }
          module.hidden = (node.id=="vApp"?found:!found);
        }
      }

      break;
    case "vStage":
      
      //hide all modules but show instances
      for (let mdlColumn of mdlContainerNode.childNodes) {  //all mdlColumn Nodes
        for (let module of mdlColumn.childNodes) {
          module.hidden = !module.contains(gId("Instances"));
        }
      }

      toggleInstances(true);

      // for (let child of insTrNode.childNodes) {
      //   child.hidden = false;
      // }
      // for (let i=4 ; insTrNode.childNodes.length; i++)
      //   show_hide_column("insTbl", i, true)

      break;
    case "vSysxx":
      //set all modules but sys hidden
      for (let mdlColumn of mdlContainerNode.childNodes) {
        for (let module of mdlColumn.childNodes) {
          module.hidden = !sysMods.includes(module.id);
        }
      }

      toggleInstances(false);
      
      // for (let child of insTrNode.childNodes) {
      //   child.hidden = !insCols.includes(child.id + "_d");
      // }
      // for (let i=4 ; insTrNode.childNodes.length; i++)
      //   show_hide_column("insTbl", i, false)

      break;
    case "vAll":
      toggleInstances(false); //put Instance back if needed
      setInstanceTableColumns();

      //set all modules visible
      for (let mdlColumn of mdlContainerNode.childNodes) {
        for (let module of mdlColumn.childNodes) {
          module.hidden = false;
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