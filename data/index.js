let d = document;
let ws = null;

let columnNr = 0;
let nrOfColumns = 4;
let userFunId = "";
let htmlGenerated = false;
let jsonValues = {};

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
      let json = JSON.parse(e.data);
      if (!htmlGenerated) { //generate array of objects
        if (Array.isArray(json)) {
          console.log("WS receive generateHTML", json);
          generateHTML(null, json); //no parentNode
          htmlGenerated = true;
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
    for (var element of json) //if isArray then objects of array
      generateHTML(parentNode, element);
  }
  else {
    if (parentNode == null) {
      parentNode = gId("column" + columnNr);
      columnNr = (columnNr +1)%nrOfColumns;
    }
    var newNode = null;
    let labelNode = cE("label");
    labelNode.innerHTML = initCap(json.id);

    if (json.type == "group") {
      newNode = cE("div");
      newNode.id = json.id
      newNode.draggable = true;
      newNode.className = "box";
      let h2 = cE("h2");
      h2.innerHTML = initCap(json.id);
      newNode.appendChild(h2);
      setupBox(newNode);
    }
    else if (json.type == "many") {
      //add many label
      let node = cE("p");
      node.appendChild(labelNode);
      parentNode.appendChild(node); //add the many label to the parent

      //add many detail table
      newNode = cE("table");
      newNode.id = json.id;
      newNode.className = "table-style"

      let theadNode = cE("thead");
      let trNode = cE("tr");
      theadNode.appendChild(trNode);
      newNode.appendChild(theadNode); //row for header

      newNode.appendChild(cE("tbody"));
    }
    else if (json.type == "display") {
      if (parentNode.nodeName.toLocaleLowerCase() == "table") { //1:Many: add the id in the header
        // console.log("display table", parentNode);
        let tdNode = cE("th");
        tdNode.id = json.id;
        tdNode.innerHTML = initCap(json.id); //label uiFun response can change it
        parentNode.firstChild.firstChild.appendChild(tdNode); //<thead><tr>
      }
      else {
        newNode = cE("p");
        newNode.appendChild(labelNode);
        let fieldNode = cE("span");
        fieldNode.id = json.id;
        if (json.value) fieldNode.innerHTML = json.value;
        newNode.appendChild(fieldNode);
      }
    }
    else if (json.type == "dropdown") {
      newNode = cE("p");
      newNode.appendChild(labelNode);
      let fieldNode = cE("select");
      fieldNode.id = json.id;
      fieldNode.addEventListener('change', (event) => {console.log("dropdown change", event);setDropdown(event.target);});
      newNode.appendChild(fieldNode);
      //(default) value will be set in processUpdate
    }
    else if (json.type == "canvas") {
      var node = cE("p");
      node.appendChild(labelNode);
      parentNode.appendChild(node); //add the many label to the parent

      newNode = cE("canvas");
      newNode.id = json.id;
    }
    else { //input
      newNode = cE("p");
      let buttonSaveNode = null;
      let buttonCancelNode = null;
      if (json.type != "button") newNode.appendChild(labelNode);
      let fieldNode = cE("input");
      fieldNode.id = json.id;
      fieldNode.type = json.type;
      if (json.type == "checkbox") {
        if (json.value) fieldNode.checked = json.value;
        fieldNode.addEventListener('change', (event) => {console.log(json.type + " change", event);setCheckbox(event.target);});
      } else if (json.type == "button") {
        if (json.value) fieldNode.value = json.value;
        fieldNode.addEventListener('click', (event) => {console.log(json.type + " click", event);setButton(event.target);});
      } else {
        //input types: text, search, tel, url, email, and password.
        if (json.value) fieldNode.value = json.value;
        fieldNode.addEventListener('change', (event) => {console.log(json.type + " change", event);setInput(event.target);});
        if (["input", "password", "number"].includes(json.type) ) {
          buttonSaveNode = cE("text");
          buttonSaveNode.innerHTML = "✅";
          buttonSaveNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
          buttonCancelNode = cE("text");
          buttonCancelNode.innerHTML = "🛑";
          buttonCancelNode.addEventListener('click', (event) => {console.log(json.type + " click", event);});
        }
        if (json.type == "number") {
          fieldNode.setAttribute('size', '4');
          fieldNode.maxlength = 4;
          // fieldNode.size = 4;
        }
      }
      newNode.appendChild(fieldNode);
      if (buttonSaveNode) newNode.appendChild(buttonSaveNode);
      if (buttonCancelNode) newNode.appendChild(buttonCancelNode);
    }

    if (newNode) parentNode.appendChild(newNode); //add new node to parent

    //call ui Functionality, if defined (to set label, comment, lov etc)
    if (json.uiFun >= 0) { //>=0 as element in object
      var command = {};
      command["uiFun"] = json.id; //ask to run uiFun for object (to add the options)
      requestJson(command);
    }

    if (json.n) generateHTML(newNode, json.n); //details (e.g. group)

  }
}

function processUpdate(json) {
  // console.log("processUpdate", json);
  for (var key of Object.keys(json)) {
    // console.log("processUpdate", key, json[key]);
    if (key != "uiFun") { //was the request
      // let id = gId(key);
      if (gId(key)) {

        if (json[key].label) {
          console.log("processUpdate label", key, json[key].label);
          let node;
          if (gId(key).nodeName.toLocaleLowerCase() == "canvas" || gId(key).nodeName.toLocaleLowerCase() == "table")
            node = gId(key).previousSibling.firstChild; //<p><label> before <canvas>/<table>
          else if (gId(key).nodeName.toLocaleLowerCase() == "th") //table header
            node = gId(key); //the <th>
          else
            node = gId(key).parentNode.firstChild; //<label> before <span or input> within <p>
          node.innerHTML = json[key].label;
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
          if (!commentNode) {
            commentNode = cE("comment");
            parentNode.appendChild(commentNode);
          }
          commentNode.innerHTML = json[key].comment;        
        }
        if (json[key].lov) {
          console.log("processUpdate lov", key, json[key].lov);
          var index = 0;
          //remove all old options first
          while (gId(key).options && gId(key).options.length > 0) {
            gId(key).remove(0);
          }
          for (var value of json[key].lov) {
            let optNode = cE("option");
            optNode.value = index;
            optNode.text = value;
            gId(key).appendChild(optNode);
            index++;
          }
        }
        if (json[key].many) {
          console.log("processUpdate many", key, json[key].many);
          //remove table rows
          let tbodyNode = cE('tbody');
  
          for (var row of json[key].many) {
            let trNode = cE("tr");
            for (var columnRow of row) {
              let tdNode = cE("td");
              tdNode.innerHTML = columnRow;
              trNode.appendChild(tdNode);
            }
            tbodyNode.appendChild(trNode);
          }
          gId(key).replaceChild(tbodyNode, gId(key).lastChild); //replace <table><tbody>
        }
        if (json[key].value) { //after lov, in case used
          if (key=="ledFix" || key =="ledFixGen")
            console.log("processUpdate value", key, json[key].value, gId(key));
          if (gId(key).nodeName.toLocaleLowerCase() == "span") //display
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
          fetchAndExecute(url, json[key].file, jsonValues, function(jsonValues,text) {
            // console.log("fetchAndExecute", text); //in case of invalid json
            var ledmapJson = JSON.parse(text);
            jsonValues[key] = ledmapJson;
            jsonValues[key].new = true;
            console.log("fetchAndExecute", jsonValues);
          }); 

        }
      }
      else
        console.log("Id not found", key);
    } //key != uiFun
  } //for keys
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

function setDropdown(element) {
  var command = {};
  command[element.id] = element.value;
  // console.log("setInput", command);

  requestJson(command);
}

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
  e.dataTransfer.setData('text/html', this.innerHTML);
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
