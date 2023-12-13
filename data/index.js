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
let savedView = null;

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

  // console.log(json, minv, maxv, scale, result);

  return Math.round(result);
}

function generateHTML(parentNode, json, rowNr = -1) {
  // console.log("generateHTML", parentNode, json);
  if (Array.isArray(json)) {
    //sort according to o value
    json.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });
    for (var node of json) //if isArray then variables of array
      generateHTML(parentNode, node, rowNr);
  }
  else {
    //if root (type module) add the html to one of the screen columns
    if (parentNode == null) {
      parentNode = gId("screenColumn" + screenColumnNr);
      screenColumnNr = (screenColumnNr +1)%nrOfScreenColumns;
    }

    if (json) {
      if (json.id == "System") {
        //get the current view
        console.log("view", json);
        if (json.view) 
          savedView = json.view;
      }
    }
    else {
      console.log("json no ?", json);
      return;
    }

    var newNode = null; //newNode will be appended to the parentNode after if then else

    let ndivNeeded = true; //for details ("n"), module and table do not need an extra div for details
    
    //set labelNode before if, will be used in if then else
    let labelNode = cE("label");
    labelNode.innerText = initCap(json.id); // the default when not overridden by uiFun

    let isPartOfTable = (rowNr != -1);

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

      //json.n will add the columns
    }
    else { //primitive types

      //table header //no newNode created
      if (parentNode.nodeName.toLocaleLowerCase() == "table") { //table add the id in the header
        //rowNr = -1 for th so uiFun will be called here and processed in processVarNode
        let thNode = cE("th");
        thNode.id = json.id;
        thNode.innerText = initCap(json.id); //label uiFun response can change it "wait for uiFun";// 
        parentNode.firstChild.firstChild.appendChild(thNode); //<thead><tr>
      } else {
        
        if (!isPartOfTable) {
          newNode = cE("p");
          if (json.type != "button") newNode.appendChild(labelNode); //add label
        }

        let valueNode;
        let rangeValueNode = null;
        // let buttonSaveNode = null;
        // let buttonCancelNode = null;

        if (json.type == "select") {

          if (json.ro) { //e.g. for reset/restart reason: do not show a select but only show the selected option
            valueNode = cE("span");
            if (json.value) valueNode.innerText = json.value;
          }
          else {
            //<p> with <label><select> (<comment> in processUpdate)

            valueNode = cE("select");
            valueNode.addEventListener('change', (event) => {console.log("select change", event);sendValue(event.target);});

            //if part of a table, use the saved list of options, otheriwise create select and uiFun will get the options
            if (isPartOfTable) {
              // if (json.id == "insfx")
              //   console.log("genHTML select table", parentNode, json, rowNr, gId(json.id), gId(json.id).innerHTML);
              valueNode.innerHTML = gId(json.id).innerHTML; //gId(json.id) is the <th> where uiFun assigned to option values to
              // let index = 0;
              // // for (var value of json.select) {
              // for (var i=0; i<20;i++) {
              //   let optNode = cE("option");
              //   optNode.value = index;
              //   optNode.text = i;
              //   valueNode.appendChild(optNode);
              //   index++;
              // }
            }
          }

        }
        else if (json.type == "canvas") {
          //<p><label><span><canvas>

          if (!isPartOfTable) {
            newNode.appendChild(labelNode);
            //3 lines of code to only add 🔍
            let spanNode = cE("span");
            spanNode.innerText= "🔍";
            newNode.appendChild(spanNode);
          }

          valueNode = cE("canvas");
          // valueNode.id = json.id;
          valueNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
        }
        else if (json.type == "textarea") {
          if (!isPartOfTable) {
            newNode.appendChild(labelNode);
            //3 lines of code to only add 🔍
            let spanNode = cE("span");
            spanNode.innerText= "🔍";
            newNode.appendChild(spanNode);
          }

          valueNode = cE("textarea");
          valueNode.readOnly = json.ro;
          valueNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});

          if (json.value) valueNode.innerText = json.value;
        }
        else if (json.type == "url") {
    
          valueNode = cE("a");
          valueNode.setAttribute('href', json.value);
          // valueNode.setAttribute('target', "_blank"); //does not work well on mobile
          valueNode.innerText = json.value;
    
        } else { //input

          //type specific actions
          if (json.type == "checkbox") {
            valueNode = cE("input");
            valueNode.type = json.type;
            valueNode.disabled = json.ro;
            if (json.value) valueNode.checked = json.value;
            valueNode.addEventListener('change', (event) => {console.log(json.type + " change", event);sendValue(event.target);});
          } else if (json.type == "button") {
            valueNode = cE("input");
            valueNode.type = json.type;
            valueNode.disabled = json.ro;
            valueNode.value = initCap(json.id);
            valueNode.addEventListener('click', (event) => {console.log(json.type + " click", event);sendValue(event.target);});
          } else if (json.type == "range") {
            valueNode = cE("input");
            valueNode.type = json.type;
            valueNode.min = json.min?json.min:0;
            valueNode.max = json.max?json.max:255; //range slider default 0..255
            valueNode.disabled = json.ro;
            if (json.value) valueNode.value = json.value;
            //numerical ui value changes while draging the slider (oninput)
            valueNode.addEventListener('input', (event) => {
              if (gId(json.id + "_rv")) {
                gId(json.id + "_rv").innerText = json.log?linearToLogarithm(json, event.target.value):event.target.value;
              }
            });
            //server value changes after draging the slider (onchange)
            valueNode.addEventListener('change', (event) => {
              sendValue(event.target);
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
              valueNode.addEventListener('change', (event) => {console.log(json.type + " change", event);sendValue(event.target);});
              // if (["text", "password", "number"].includes(json.type) ) {
              //   buttonSaveNode = cE("text");
              //   buttonSaveNode.innerText = "✅";
              //   buttonSaveNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
              //   buttonCancelNode = cE("text");
              //   buttonCancelNode.innerText = "🛑";
              //   buttonCancelNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
              // }
              if (json.type == "number") {
                valueNode.min = json.min?json.min:0; //if not specified then unsigned value (min=0)
                if (json.max) valueNode.max = json.max;
              }
              else {
                if (json.max) valueNode.setAttribute('maxlength', json.max); //for text and textarea set max length valueNode.maxlength is not working for some reason
                if (json.id == "serverName")
                  gId("instanceName").innerText = json.value;
              }
            }
          } //not checkbox or button or range

        } //input type
        
        if (!isPartOfTable) {
          valueNode.id = json.id;
          newNode.appendChild(valueNode);
        } else {
          valueNode.id = json.id + "#" + rowNr;
          newNode = valueNode;
        }

        if (rangeValueNode) newNode.appendChild(rangeValueNode); //_rv value of range / sliders
        // if (buttonSaveNode) newNode.appendChild(buttonSaveNode);
        // if (buttonCancelNode) newNode.appendChild(buttonCancelNode);
        
        //disable drag of parent screenBox
        newNode.draggable = true;
        newNode.addEventListener('dragstart', (event) => {event.preventDefault(); event.stopPropagation();});
      } //table header
    } //primitive types

    if (newNode) parentNode.appendChild(newNode); //add new node to parent

    //don't call uiFun on table rows (the table header calls uiFun and propagate this to table row columns in processVarNode when needed - e.g. select)
    if (!isPartOfTable) {
      //call ui Functionality, if defined (to set label, comment, select etc)
      if (json.uiFun >= 0) { //>=0 as element in var
        uiFunCommands.push(json.id);
        if (uiFunCommands.length > 8) { //every 8 vars (to respect responseDoc size) check WS_EVT_DATA info
          flushUIFunCommands();
        }
      }
    }
      
    if (json.n) { //multple details
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

//process json from server
function processUpdate(json) {
  // console.log("processUpdate", json);
  if (json.id) { //this is a var object so create this object using generateHTML
    console.log("processUpdate variable", gId(json.id), json);
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
  else { //loop over keys
    for (var key of Object.keys(json)) {
      //special commands
      if (key == "uiFun") {
        console.log("processUpdate no action", key, json[key]);
      }
      else if (key == "view") {
        console.log("processUpdate no action", key, json[key]);
      }
      else if (key == "updrow") { //update the row of a table
        // console.log("processUpdate", key, json[key]);
        let tablerowObject = json[key];
        for (var tableId of Object.keys(tablerowObject)) {
          let tableRows = tablerowObject[tableId];
          let tableNode = gId(tableId);
          // console.log("  ", tableNode);
          if (Array.isArray(tableRows)) {
            for (let tableRow of tableRows) {
              // console.log("  ", tableId, tableRow);
            }
          }
          for (var i = 0, row; row = tableNode.rows[i]; i++) {
            if (i != 0 && row.cells[0].innerText == tableRows[0][0]) {
              // console.log("  row", i, row);
              for (var j = 0, col; col = row.cells[j]; j++) { //coll is a <td>
                // console.log("  cell", i, j, col);
                processVarNode(col.firstChild, col.firstChild.id, {value:tableRows[0][j]}); //<td>.firstChild is the cell e.g. <select>
              }  
            }
          }
        } //tableId
      }
      else { //{variable:{label:value}}
        if (gId(key)) { //is the key a var?
          processVarNode(gId(key), key, json[key]);
        }
        else
          console.log("processUpdate id not found in dom", key, json[key]);
      }
    } //for keys
  } //not id
} //processUpdate

//do something with an existing (variable) node, key is an existing node, json is what to do with it
function processVarNode(node, key, json) {
  let overruleValue = false;

  // let node = gId(key);
  // if (rowNr != -1)
  //   node = gId(key + "#" + rowNr);
  
  if (json.hasOwnProperty("label")) {
    // if (key != "insTbl") // tbd: table should not update
    //   console.log("processVarNode label", key, json.label);
    if (node.nodeName.toLocaleLowerCase() == "input" && node.type == "button") {
      node.value = initCap(json.label);
    }
    else {
      let labelNode;
      if (node.nodeName.toLocaleLowerCase() == "canvas" || node.nodeName.toLocaleLowerCase() == "table")
        labelNode = node.previousSibling.firstChild; //<p><label> before <canvas> or <table>
      else if (node.nodeName.toLocaleLowerCase() == "th") //table header
        labelNode = node; //the <th>
      else
        labelNode = node.parentNode.firstChild; //<label> before <span or input> within <p>
      labelNode.innerText = initCap(json.label);
    }
  } //label

  if (json.hasOwnProperty("comment")) {
    
    if (node.nodeName.toLocaleLowerCase() != "th") { //no comments on table header
      // normal: <p><label><input id><comment></p>
      // table or canvas <p><label><comment></p><canvas id>
      // 1) if exist then replace else add
      let parentNode;
      if (node.nodeName.toLocaleLowerCase() == "canvas" || node.nodeName.toLocaleLowerCase() == "textarea" || node.nodeName.toLocaleLowerCase() == "table")
        parentNode = node.previousSibling; //<p><label> before <canvas> or <table> or <textarea>
      else
        parentNode = node.parentNode;
      
      // if (key != "insTbl") // tbd: table should not update
      //   console.log("processVarNode comment", key, json.comment);

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
  } //comment

  if (json.hasOwnProperty("select")) { //replace the select options
    // console.log("processVarNode select", key, json.select);

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

    //if node is a table header, propagate to all column cells
    if (node.nodeName.toLocaleLowerCase() == "th") {
      let tableNode = node.parentNode.parentNode.parentNode; //<table><thead><tr><th>

      let columnNr = -1;

      for (var i = 0, row; row = tableNode.rows[i]; i++) { //row 0 is header
        // console.log("  row", i, row);
        for (var j = 0, col; col = row.cells[j]; j++) {

          if (i == 0 && col.id == key)
            columnNr = j;
          else if (columnNr == j) {
            // console.log("    cell", col, node);
            col.firstChild.innerHTML = node.innerHTML; //firstChild is <select>
          }
        }  
      }
    } // th
  } //select

  if (json.hasOwnProperty("table")) { //replace the body of a table

    //remove table rows
    let tbodyNode = cE('tbody'); //the tbody of node will be replaced
    
    //find model info
    let variable = findVar(key); //key is the table where no are the columns
    // if (key != "insTbl") // tbd: table should not update
    //   console.log("processVarNode table", key, json.table);

    //add each row
    let rowNr = 0;
    for (var row of json.table) {
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
        let newNode = generateHTML(tdNode, columnVar, rowNr); //no <p><label>
        if (newNode) {
          //very strange: gId(newNode.id) is not working here. Delay before it is in the dom??? (workaround create processVarNode function)
          let updateJson;
          if (typeof columnRow == 'number' || typeof columnRow == 'boolean')
            updateJson = `{"value":${columnRow}}`;
          else
            updateJson = `{"value":"${columnRow}"}`
          // console.log("tablecolumn", rowNr, colNr, newNode, columnVar, updateJson, JSON.parse(updateJson), gId(newNode.id));
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
    node.replaceChild(tbodyNode, node.lastChild); //replace <table><tbody> by tbodyNode

    if (node.id == "insTbl")
      setInstanceTableColumns();
  } //table

  if (json.hasOwnProperty("value") && !overruleValue) { //overruleValue: select sets already the option
    //hasOwnProperty needed to catch also boolean json.value when it is false
    // if (key=="fx")// || key=="mdlEnabled" || key=="clIsFull" || key=="pin2")
      // console.log("processVarNode value", key, json, json.value, node);
    if (node.nodeName.toLocaleLowerCase() == "span") //read only vars
      node.textContent = json.value;
    else if (node.nodeName.toLocaleLowerCase() == "a") { //url links
      node.innerText = "🔍";
      node.setAttribute('href', json.value);
    } else if (node.nodeName.toLocaleLowerCase() == "canvas")
      userFunId = key; //prepare for websocket data
    else if (node.type == "checkbox")
      node.checked = json.value;
    else if (node.type == "button") {
      // console.log("button", node, json);
      if (json.value) node.value = json.value; //else the id / label is used as button label
    }
    else if (Array.isArray(json.value)) { //table column
      let rowNr = 0;
      for (let x of json.value) {
        // console.log(key, gId(key + "#" + rowNr), x);
        if (gId(key + "#" + rowNr) && gId(key + "#" + rowNr).checked)
          gId(key + "#" + rowNr).checked = x; //tbd support all types!!
        rowNr++;
      }
      // node.checked = json.value;
    } else {//inputs or select
      node.value = json.value;
      node.dispatchEvent(new Event("input")); // triggers addEventListener('input',...). now only used for input type range (slider), needed e.g. for qlc+ input
    }
  } //value

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
} //processVarNode

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
  let panelParentNode = gId("System").parentNode.parentNode;
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