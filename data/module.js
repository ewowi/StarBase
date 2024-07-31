// @title     StarBase
// @file      module.js
// @date      20240720
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

function moduleFun(moduleJson) {
  let code = "";
  for (variable of moduleJson.n) {
    // console.log(variable);
    code += `<p>`
    code += `<label>${variable.id}</label>`
    switch (variable.type) {
      case "progress":
        code += `<progress max="${variable.max}" id="${variable.id}" class="progress"></progress>`
        break
      default:
        code += `<input type=${variable.type} class="${variable.type}"></input>`
    }
    code += `<p>`
  }
  code += '<pre>' + JSON.stringify(moduleJson, null, 2) + '</pre>'
  return code
}