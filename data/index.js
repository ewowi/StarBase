// @title     StarMod
// @file      index.css
// @date      20230730
// @repo      https://github.com/ewoudwijma/StarMod
// @Authors   https://github.com/ewoudwijma/StarMod/commits/main
// @Copyright (c) 2023 Github StarMod Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007

let d = document;
let ws = null;

let columnNr = 0;
let nrOfColumns = 4;
let userFunId = "";
let htmlGenerated = false;
let jsonValues = {};
let uiFunCommands = [];

function gId(c) {return d.getElementById(c);}
function cE(e) { return d.createElement(e); }

function handleVisibilityChange() {
  console.log("handleVisibilityChange");
}

function onLoad() {
  makeWS();

  initColumns();

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
            console.log("WS receive generateHTML", json);
            generateHTML(null, json); //no parentNode
            htmlGenerated = true;
            //send request for uiFun
            if (uiFunCommands.length) { //flush commands not already send
              flushUIFunCommands();
            }
          }
          else
            console.log("Error: no array", json);
        }
        else { //update
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
  }
}

function generateHTML(parentNode, json) {
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
      parentNode = gId("column" + columnNr);
      columnNr = (columnNr +1)%nrOfColumns;
    }

    var newNode = null; //newNode will be appended to the parentNode after if then else

    //set labelNode before if, will be used in if then else
    let labelNode = cE("label");
    labelNode.innerText = initCap(json.id);

    let ndivNeeded = true; //for details ("n"), module and table do not need an extra div for details

    if (json.type == "module") {
      ndivNeeded = false;
      newNode = cE("div");
      newNode.id = json.id
      newNode.draggable = true;
      newNode.className = "box";

      let h2Node = cE("h2");
      h2Node.innerText = initCap(json.id);
      newNode.appendChild(h2Node);

      setupBox(newNode);
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
    else if (json.type == "select") {
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

      spanNode = cE("span");
      spanNode.innerText= "🔍";
      pNode.appendChild(spanNode);
      
      parentNode.appendChild(pNode);

      newNode = cE("canvas");
      newNode.id = json.id;
      newNode.addEventListener('click', (event) => {toggleModal(event.target);});
    }
    else if (json.type == "textarea") {
      pNode = cE("p");
      pNode.appendChild(labelNode);
      parentNode.appendChild(pNode);

      newNode = cE("textarea");
      newNode.id = json.id;
      newNode.readOnly = json.ro;
      newNode.addEventListener('click', (event) => {toggleModal(event.target);});

      if (json.value) newNode.innerText = json.value;
      // newNode.appendChild(textareaNode);
      pNode.innerText += "🔍";
    }
    else { //input
      if (parentNode.nodeName.toLocaleLowerCase() == "table") { //table add the id in the header
        let tdNode = cE("th");
        tdNode.id = json.id;
        tdNode.innerText = initCap(json.id); //label uiFun response can change it
        parentNode.firstChild.firstChild.appendChild(tdNode); //<thead><tr>
      }
      else {
        if (json.ro && json.type != "button") { //pka display
          newNode = cE("p");
          newNode.appendChild(labelNode);
          if (json.type == "url") {
            let a = cE("a")
            a.setAttribute('href', json.value);
            a.setAttribute('target', "_blank");
            a.innerText = json.value;
            newNode.appendChild(a);
          } else {
            let spanNode = cE("span");
            spanNode.id = json.id;
            if (json.value) spanNode.innerText = json.value;
            newNode.appendChild(spanNode);
          }
        }
        else { //not ro or button
          newNode = cE("p");
          let rangeValueNode = null;
          let buttonSaveNode = null;
          let buttonCancelNode = null;
          if (json.type != "button") newNode.appendChild(labelNode);
          let inputNode = cE("input");
          inputNode.id = json.id;
          inputNode.type = json.type;
          if (json.type == "checkbox") {
            if (json.value) inputNode.checked = json.value;
            inputNode.addEventListener('change', (event) => {console.log(json.type + " change", event);setCheckbox(event.target);});
          } else if (json.type == "button") {
            inputNode.value = initCap(json.id);
            inputNode.addEventListener('click', (event) => {console.log(json.type + " click", event);setButton(event.target);});
          } else {
            //input types: text, search, tel, url, email, and password.
            if (json.value) inputNode.value = json.value;
            if (json.type == "range") {
              inputNode.max = 255;
              // inputNode.addEventListener('input', (event) => {console.log(json.type + " input", event);gId(json.id + "_rv").innerText = this.value;});
              inputNode.addEventListener('change', (event) => {
                console.log(json.type + " change", event.target, json.id);
                gId(json.id + "_rv").innerText = event.target.value;
                setInput(event.target);
              });
              rangeValueNode = cE("span");
              rangeValueNode.id = json.id + "_rv"; //rangeValue
              if (json.value) rangeValueNode.innerText = json.value;
            } else {
              inputNode.addEventListener('change', (event) => {console.log(json.type + " change", event);setInput(event.target);});
              // if (["text", "password", "number"].includes(json.type) ) {
              //   buttonSaveNode = cE("text");
              //   buttonSaveNode.innerText = "✅";
              //   buttonSaveNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
              //   buttonCancelNode = cE("text");
              //   buttonCancelNode.innerText = "🛑";
              //   buttonCancelNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
              // }
              if (json.type == "number") {
                inputNode.setAttribute('size', '4');
                inputNode.maxlength = 4;
                // inputNode.size = 4;
              }
            }
          }
          newNode.appendChild(inputNode);
          if (rangeValueNode) newNode.appendChild(rangeValueNode);
          if (buttonSaveNode) newNode.appendChild(buttonSaveNode);
          if (buttonCancelNode) newNode.appendChild(buttonCancelNode);
        }
      }
    }

    if (newNode) parentNode.appendChild(newNode); //add new node to parent

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
        divNode = cE("div");
        divNode.id = json.id + "_n";
        divNode.classList.add("ndiv");
        newNode.appendChild(divNode);
        generateHTML(divNode, json.n);
      }
      else
        generateHTML(newNode, json.n); //details (e.g. module)
    }
  }
}

function flushUIFunCommands() {
  if (uiFunCommands.length > 0) { //if something to flush
    var command = {};
    command["uiFun"] = uiFunCommands; //ask to run uiFun for vars (to add the options)
    console.log("uiFunCommands", command);
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
      divNode = cE("div");
      divNode.id = json.id + "_n";
      divNode.classList.add("ndiv");
      gId(json.id).parentNode.appendChild(divNode);
      generateHTML(divNode, json.n);
    }
    flushUIFunCommands(); //make sure uiFuns of new elements are called
  }
  else {
    for (var key of Object.keys(json)) {
      if (key != "uiFun") { //was the request
        // let id = gId(key);
        if (gId(key)) {
          let overruleValue = false;
  
          if (json[key].label) {
            console.log("processUpdate label", key, json[key].label);
            if (gId(key).nodeName.toLocaleLowerCase() == "input" && gId(key).type == "button") {
              gId(key).value = initCap(json[key].label);
            }
            else {
              let labelNode;
              if (gId(key).nodeName.toLocaleLowerCase() == "canvas" || gId(key).nodeName.toLocaleLowerCase() == "table")
                labelNode = gId(key).previousSibling.firstChild; //<p><label> before <canvas>/<table>
              else if (gId(key).nodeName.toLocaleLowerCase() == "th") //table header
                labelNode = gId(key); //the <th>
              else
                labelNode = gId(key).parentNode.firstChild; //<label> before <span or input> within <p>
              labelNode.innerText = initCap(json[key].label);
            }
          }
          if (json[key].comment) {
            console.log("processUpdate comment", key, json[key].comment);
            // normal: <p><label><input id><comment></p>
            // table or canvas <p><label><comment></p><canvas id>
            // 1) if exist then replace else add
            let parentNode;
            if (gId(key).nodeName.toLocaleLowerCase() == "canvas" || gId(key).nodeName.toLocaleLowerCase() == "table")
              parentNode = gId(key).previousSibling; //<p><label> before <canvas>/<table>
            else
              parentNode = gId(key).parentNode;
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
            commentNode.innerText = json[key].comment;        
          }
          if (json[key].select) {
            console.log("processUpdate select", key, json[key].select);
            if (gId(key).nodeName.toLocaleLowerCase() == "span") { //readonly. tbd: only the displayed value needs to be in the select
              var index = 0;
              for (var value of json[key].select) {
                if (parseInt(gId(key).textContent) == index) {
                  // console.log("processUpdate select1", value, gId(key), gId(key).textContent, index);
                  gId(key).textContent = value; //replace the id by its value
                  // console.log("processUpdate select2", value, gId(key), gId(key).textContent, index);
                  overruleValue = true; //in this case we do not want the value set
                }
                index++;
              }
            }
            else { //select
              var index = 0;
              //remove all old options first
              while (gId(key).options && gId(key).options.length > 0) {
                gId(key).remove(0);
              }
              for (var value of json[key].select) {
                let optNode = cE("option");
                optNode.value = index;
                optNode.text = value;
                gId(key).appendChild(optNode);
                index++;
              }
            }
          }
          if (json[key].table) {
            console.log("processUpdate table", key, json[key].table);
            //remove table rows
            let tbodyNode = cE('tbody');
    
            for (var row of json[key].table) {
              let trNode = cE("tr");
              for (var columnRow of row) {              
                let tdNode = cE("td");
                if (typeof(columnRow) == "boolean") { //if column is a checkbox
                  let checkBoxNode = cE("input");
                  checkBoxNode.type = "checkbox";
                  checkBoxNode.checked = columnRow;
                  tdNode.appendChild(checkBoxNode);
                }
                else {
                  tdNode.innerText = columnRow;
                }
                trNode.appendChild(tdNode);
              }
              tbodyNode.appendChild(trNode);
            }
            gId(key).replaceChild(tbodyNode, gId(key).lastChild); //replace <table><tbody>
          }
          if (json[key].value && !overruleValue) { //after select, in case used
            // if (key=="ledFix" || key =="ledFixGen"|| key =="reset0")
            //   console.log("processUpdate value", key, json[key].value, gId(key));
            if (gId(key).nodeName.toLocaleLowerCase() == "span") //read only vars
              gId(key).textContent = json[key].value;
            else if (gId(key).nodeName.toLocaleLowerCase() == "canvas") {
              userFunId = key; //prepare for websocket data
            } else if (gId(key).type == "checkbox") //checkbox
              gId(key).checked = json[key].value;
            else //inputs
              gId(key).value = json[key].value;
          }
          if (json[key].json) { //json send html nodes cannot process, store in jsonValues array
            console.log("processUpdate json", key, json[key].json, gId(key));
            jsonValues[key] = json[key].json;
          }
          if (json[key].file) { //json send html nodes cannot process, store in jsonValues array
            console.log("processUpdate file", key, json[key].file, gId(key));
          
            //we need to send a request which the server can handle using request variable
            let url = `http://${window.location.hostname}/file`;
            fetchAndExecute(url, json[key].file, key, function(key,text) { //send key as parameter
              // console.log("fetchAndExecute", text); //in case of invalid json
              var ledmapJson = JSON.parse(text);
              jsonValues[key] = ledmapJson;
              jsonValues[key].new = true;
              console.log("fetchAndExecute", jsonValues);
            }); 
  
          }
        }
        else
          console.log("processUpdate id not found in json", key, json[key]);
      } //key != uiFun
    } //for keys
  } //not id
} //processUpdate

function requestJson(command) {
  gId('connind').style.backgroundColor = "var(--c-y)";
  if (!ws) return;
  let req = JSON.stringify(command);
  
  console.log("requestJson", command);
  
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
function initColumns() {

  let columns = document.querySelectorAll('.container .column');
  columns.forEach(function(column) {
    column.addEventListener('dragover', handleDragOver);
    column.addEventListener('dragenter', handleDragEnter);
    column.addEventListener('dragleave', handleDragLeave);
    column.addEventListener('drop', handleDrop);
  });

  setupBoxes();
  
}

function setupBoxes() {
  let boxes = document.querySelectorAll('.container .box');
  boxes.forEach(function(box) {
    setupBox(box);
  });

}

// var lastPage;
function setupBox(item) {
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

  let boxes = document.querySelectorAll('.container .box');
  boxes.forEach(function (item) {
    item.classList.remove('over');
  });

  let columns = document.querySelectorAll('.container .column');
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
    setupBox(clone);
    removeDragStyle(clone);

    if (this.id.includes("column")) {
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
