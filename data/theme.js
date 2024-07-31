// @title     StarBase
// @file      theme.js
// @date      20240720
// @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com


class ThemeClass {

  addHTML() {
    let body = document.getElementById("body");//gId("body");

    body.innerHTML+= `
    <label>Theme</label> 
    <select name="theme-select" id="theme-select" onchange="controller.themeClass.setTheme(this)">
      <option value="starbase">StarBase</option>
      <option value="starlight">StarLight</option>
      <option value="wled">WLED</option>
      <option value="grayeen">Grayeen</option>
      <option value="dev">Dev</option>
      <option value="light">Light</option>
      <option value="dark">DeathStar</option>
      <option value="blue">Blue</option>
      <option value="pink">Pink</option>
      <option value="space">Space</option>
      <option value="nyan">Nyan</option>
    </select>
    `
  
  }

  setTheme(node) {
    // console.log("setTheme", node.value)
    localStorage.setItem('theme', node.value);
    document.documentElement.className = node.value;
    // if (document.getElementById("theme-select").value != node.value)
    //   document.getElementById("theme-select").value = node.value;
  }

  getTheme() {
    let value = localStorage.getItem('theme');
    // console.log("getTheme", value, document.getElementById("theme-select"))
    if (value && value != "null") 
      document.getElementById("theme-select").value = value
  }

}