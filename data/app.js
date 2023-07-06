function preview2D(node, data) {
  let leds = new Uint8Array(data);
  // console.log(c,leds);
  let ctx = node.getContext('2d');
  let mW = leds[0]; // matrix width
  let mH = leds[1]; // matrix height
  let pPL = Math.min(node.width / mW, node.height / mH); // pixels per LED (width of circle)
  let lOf = Math.floor((node.width - pPL*mW)/2); //left offeset (to center matrix)
  var i = 3;
  ctx.clearRect(0, 0, node.width, node.height); //WLEDMM
  // function colorAmp(color) {
  //   if (color == 0) return 0;
  //   return 25+225*color/255;
  // } //WLEDMM in range 55 - 205
  for (y=0.5;y<mH;y++) for (x=0.5; x<mW; x++) {
    if (leds[i]!= 0 || leds[i+1]!= 0 || leds[i+2]!= 0) { //WLEDMM: do not show blacks
      // ctx.fillStyle = `rgb(${colorAmp(leds[i])},${colorAmp(leds[i+1])},${colorAmp(leds[i+2])})`;
      ctx.fillStyle = `rgb(${leds[i]},${leds[i+1]},${leds[i+2]})`;
      ctx.beginPath();
      ctx.arc(x*pPL+lOf, y*pPL, pPL*0.4, 0, 2 * Math.PI);
      ctx.fill();
    }
    i+=3;
  }

}

