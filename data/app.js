function userFun(userFunId, data) {
  if (userFunId == "pview") {
    preview2D(gId(userFunId), data);
    return true;
  }
  return false;
}

function preview2D(node, data) {
  let leds = new Uint8Array(data);
  // console.log(node,leds);
  let ctx = node.getContext('2d');
  let mW = leds[0]; // matrix width
  let mH = leds[1]; // matrix height
  let pPL = Math.min(node.width / mW, node.height / mH); // pixels per LED (width of circle)
  let lOf = Math.floor((node.width - pPL*mW)/2); //left offeset (to center matrix)
  let i = 4;
  ctx.clearRect(0, 0, node.width, node.height);
  for (y=0.5;y<mH;y++) for (x=0.5; x<mW; x++) {
    if (leds[i] + leds[i+1] + leds[i+2] != 0) { //do not show blacks
      ctx.fillStyle = `rgb(${leds[i]},${leds[i+1]},${leds[i+2]})`;
      ctx.beginPath();
      ctx.arc(x*pPL+lOf, y*pPL, pPL*0.4, 0, 2 * Math.PI);
      ctx.fill();
    }
    i+=3;
  }

}

