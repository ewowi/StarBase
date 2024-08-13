// @title     StarBase
// @file      app.js
// @date      20240720
// @repo      https://github.com/ewowi/StarBase
// @Authors   https://github.com/ewowi/StarBase/commits/main
// @Copyright © 2024 Github StarBase Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
// @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com

function appName() {
  return "Demo";
}

function userFun(buffer) {
  // console.log("userFun", buffer)
  
  if (buffer[0]==1) {
    return true;
  }
  
  return false;
}