// @title     StarBase
// @file      modules.js
// @date      20240720
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

class Modules {

  model = [] //model.json (as send by the server), used by FindVar

  //generates html for a module
  //for some strange reason, this method does not accept this.otherMethodInThisClass ???
  createHTML(moduleJson) {
    let code = ""
    controller.modules.test("ew")
    // this.test("ew") //this.otherMethodInThisClass: [Error] Unhandled Promise Rejection: TypeError: this.test is not a function. (In 'this.test("ew")', 'this.test' is undefined)

    // sort module variables
    moduleJson.n.sort(function(a,b) {
      return Math.abs(a.o) - Math.abs(b.o); //o is order nr (ignore negatives for the time being)
    });

    for (let variable of moduleJson.n) {

      let variableClass = varJsonToClass(variable);
      code += variableClass.createHTML()

      if (variable.fun >= 0) { //>=0 as element in var

        let command = {};
        command.onUI = [variable.id];
        // console.log("flushOnUICommands", command);
        controller.requestJson(command);

        variable.fun = -1; //requested
      }
    }
    code += '<pre>' + JSON.stringify(moduleJson, null, 2) + '</pre>'
    return code
  }

  //temporary to test issue above
  test(name) {
    console.log(name)
  }

  //used by fetchModel and by makeWS
  addModule(moduleJson) {
    moduleJson.type = moduleJson.type=="appmod"?appName():moduleJson.type=="usermod"?"User":"System"; 
    this.model.push(moduleJson);

    //updateUI is made after all modules have been fetched, how to adapt to add one module?
    controller.mainNav.updateUI(moduleJson, this.createHTML); //moduleFun returns the html to show in the module panel of the UI
    //still doesn't maker sense  to call updateUI for every module ...
  }

  //walkthrough, if a variable is returned by fun, it stops the walkthrough with this variable
  walkThroughModel(fun, parent = this.model) {
    for (var variable of parent) {
      let foundVar = null
      foundVar = fun(variable);

      if (foundVar) return foundVar;
 
      if (variable.n) {
        foundVar = this.walkThroughModel(fun, variable.n); //recursive
        if (foundVar) return foundVar;
      }
    }
  }

  //loop through the model hierarchy and generateData data for each variable in the model
  generateData() {
    this.walkThroughModel(function(variable) {
      let variableClass = varJsonToClass(variable);
      variableClass.generateData();
    })
  }

  //finds a var with id in the model, if found returns it
  findVar(id) {
    // console.log("findVar", id, parent, model);
    return this.walkThroughModel(function(variable){
      if (variable.id == id) //found variable
        return variable; //this stops the walkThrough
    })
  
  }

} //class Modules

//see this.otherMethodInThisClass, should be moved in Modules class
//creates the right class based on variable.type
function varJsonToClass(variable) {
  switch (variable.type) {
    case "button":
      return new ButtonVariable(variable);
    case "progress":
      return new ProgressVariable(variable);
    case "text":
      return new TextVariable(variable);
    case "select":
      return new SelectVariable(variable);
    case "checkbox":
      return new CheckboxVariable(variable);
    default:
      return new Variable(variable);
  }
}

class Variable {

  constructor(variable) {
    this.variable = variable
    this.node = document.getElementById(variable.id);
  }

  createHTML(node = `<input id=${this.variable.id} type=${this.variable.type} class="${this.variable.type}" value="${this.variable.value}"></input>`) { //base
    return `<p>
            <label>${initCap(this.variable.id)}</label>
            ${node}
            <p>`
  }

  //sets the value of the node to value of properties
  receiveData(properties) { //base
    if (this.node) {
      if (properties.label) {
        let labelNode = this.node.parentNode.querySelector("label")
        if (labelNode)
          labelNode.innerText = properties.label;
        else
          this.node.parentNode.innerHTML = `<label>${properties.label}</label>` + this.node.parentNode.innerHTML
      }
      if (properties.value != null && this.node.value != null) {
        this.node.value = properties.value;
      }
      if (properties.comment) {
        let commentNode = this.node.parentNode.querySelector("comment")
        if (commentNode)
          commentNode.innerText = properties.comment
        else
          this.node.parentNode.innerHTML += `<comment>${properties.comment}</comment>`
      }
    } 
  }

  generateData(custom = `"value":${Math.random() * 1000}`) {
    if (custom != "") custom += ", "
    controller.receiveData(JSON.parse(`{"${this.variable.id}":{${custom}"comment":"${Math.random().toString(36).slice(2)}"}}`));
  }

} //class Variable

class TextVariable extends Variable {

  createHTML() { //base
    return super.createHTML(`<input id=${this.variable.id} type=${this.variable.type} class="${this.variable.type}" value="${this.variable.value}"></input>`);
  }

  generateData() {
    super.generateData(`"value":"${Math.random().toString(36).slice(2)}"`)
  }

} //class TextVariable

class CheckboxVariable extends Variable {

  receiveData(properties) { //base
    super.receiveData(properties)
    if (this.node && properties.value != null)
      this.node.checked = properties.value
  }

  generateData() {
    super.generateData(`"value":${(Math.random()<0.5)?1:0}`)
  }

} //class CheckboxVariable

class ButtonVariable extends Variable {

  createHTML() { //override
    return super.createHTML(`<input id=${this.variable.id} type=${this.variable.type} class="${this.variable.type}" value="${initCap(this.variable.id)}"></input>`)
  }

  generateData() {
    super.generateData("") //no value update
  }

} //class ButtonVariable

class SelectVariable extends Variable {

  createHTML() { //override
    return super.createHTML(`<select id="${this.variable.id}" class="select"></select>`)
  }

  generateData() {
    //add sample options
    if (this.node && this.node.childNodes.length == 0)
      this.node.innerHTML+='<option value=0>One</option><option value=1>Two</option><option value=2>Three</option>'

    super.generateData(`"value":${Math.random() <.33?0:Math.random() <.66?1:2}`)
  }

} //class SelectVariable

class ProgressVariable extends Variable {

  createHTML() { //override
    return super.createHTML(`<progress max="${this.variable.max}" id="${this.variable.id}" class="progress"></progress>`)
  }

} //class ProgressVariable