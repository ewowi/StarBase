// @title     StarMod
// @file      app.js
// @date      20231016
// @repo      https://github.com/ewowi/StarMod
// @Authors   https://github.com/ewowi/StarMod/commits/main
// @Copyright (c) 2023 Github StarMod Commit Authors
// @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007

var renderer = null;
var scene = null;
var camera = null;
var controls = null;
// var previousFunction;

function userFun(userFunId, data) {
  if (userFunId == "pview" && jsonValues.pview) {
    let leds = new Uint8Array(data);
    let pviewNode = gId("pview");

    //replace the canvas: in case we switch from 2D to 3D as they cannot be reused between them
    if (jsonValues.pview.new)
    {
      console.log("replace the canvas!", jsonValues.pview);
      let canvasNode = cE("canvas");
      canvasNode.width = pviewNode.width;
      canvasNode.height = pviewNode.height;
      canvasNode.draggable = true;
      canvasNode.addEventListener('dragstart', (event) => {event.preventDefault(); event.stopPropagation();});

      pviewNode.parentNode.replaceChild(canvasNode, pviewNode);
      pviewNode = canvasNode;
      pviewNode.id = "pview";
      pviewNode.addEventListener('dblclick', (event) => {toggleModal(event.target);});
    }

    // console.log("userFun", leds);

    if (jsonValues.pview.depth == 1)
      preview2D(pviewNode, leds);
    else
      preview3D(pviewNode, leds);

    return true;
  }
  else if (userFunId == "board") {
    let leds = new Uint8Array(data);
    let pviewNode = gId("board");
    // console.log(leds, pviewNode);
    previewBoard(pviewNode, leds);
  }
  return false;
}

function preview2D(node, leds) {
  let ctx = node.getContext('2d');
  let i = 4;
  let factor = 10;//fixed value: from mm to cm
  ctx.clearRect(0, 0, node.width, node.height);
  if (jsonValues.pview) {
    let pPL = Math.min(node.width / jsonValues.pview.width, node.height / jsonValues.pview.height); // pixels per LED (width of circle)
    let lOf = Math.floor((node.width - pPL*jsonValues.pview.width)/2); //left offeset (to center matrix)
    if (jsonValues.pview.outputs) {
      // console.log("preview2D jsonValues", jsonValues.pview);
      for (var output of jsonValues.pview.outputs) {
        if (output.leds) {
          for (var led of output.leds) {
            if (leds[i] + leds[i+1] + leds[i+2] > 20) { //do not show nearly blacks
              ctx.fillStyle = `rgb(${leds[i]},${leds[i+1]},${leds[i+2]})`;
              ctx.beginPath();
              ctx.arc(led[0]*pPL/factor + lOf, led[1]*pPL/factor, pPL*0.4, 0, 2 * Math.PI);
              ctx.fill();
            }
            i+=3;
          }
        }
        else {
          console.log("preview2D jsonValues no leds", jsonValues.pview);
          jsonValues.pview = null;
        }            
      }
    }
    else {
      console.log("preview2D jsonValues no outputs", jsonValues.pview);
      jsonValues.pview = null;
    }
    jsonValues.pview.new = false;
  }
}

//https://stackoverflow.com/questions/8426822/rotate-camera-in-three-js-with-mouse
function preview3D(node, leds) {
  //3D vars
  // let mW = leds[0];
  // let mH = leds[1];
  // let mD = leds[2];
  import ('three').then((THREE) => {
    import ('three/addons/controls/OrbitControls.js').then((OCModule) => {

      let factor = 10;//fixed value: from mm to cm
      let d = 5 / factor; //distanceLED;

      if (!renderer || (jsonValues.pview && jsonValues.pview.new)) { //init 3D

        console.log("preview3D create new renderer");

        // node.width  = 0;
        // node.height = 0;
        console.log("before", node);
        renderer = new THREE.WebGLRenderer({canvas: node, antialias: true, alpha: true });
        console.log("after", node);
        // THREE.Object3D.DefaultUp = new THREE.Vector3(0,1,1);
        renderer.setClearAlpha(0)
        renderer.setClearColor( 0x000000, 0 );
        // renderer.setSize( 300, 150);
        // node.parentNode.appendChild( renderer.domElement );

        camera = new THREE.PerspectiveCamera( 45, node.width/node.height, 1, 500);
        // const camera = new THREE.PerspectiveCamera( 45, window.innerWidth / window.innerHeight, 0.1, 2000 );
        camera.position.set( 0, 0, d*Math.sqrt(jsonValues.pview.width*jsonValues.pview.width + jsonValues.pview.height*jsonValues.pview.height + jsonValues.pview.depth*jsonValues.pview.depth) );
        camera.lookAt( 0, 0, 0 );

        scene = new THREE.Scene();
        scene.background = null; //new THREE.Color( 0xff0000 );

        controls = new OCModule.OrbitControls( camera, renderer.domElement );
        controls.target.set( 0, 0.5, 0 );
        controls.update();
        controls.enablePan = false;
        controls.enableDamping = true;

      } //new

      if (jsonValues.pview && jsonValues.pview.new) { //set the new coordinates
        var offset_x = -d*(jsonValues.pview.width-1)/2;
        var offset_y = -d*(jsonValues.pview.height-1)/2;
        var offset_z = -d*(jsonValues.pview.depth-1)/2;

        console.log("preview3D new jsonValues", jsonValues.pview);

        if (jsonValues.pview.outputs) {
          // console.log("preview2D jsonValues", jsonValues.pview);
          for (var output of jsonValues.pview.outputs) {
            if (output.leds) {
              for (var led of output.leds) {
                const geometry = new THREE.SphereGeometry( 1/factor);
                const material = new THREE.MeshBasicMaterial({transparent: true, opacity: 0.5});
                // material.color = new THREE.Color(`${x/mW}`, `${y/mH}`, `${z/mD}`);
                const sphere = new THREE.Mesh( geometry, material );
                sphere.position.set(offset_x + d*led[0]/factor, offset_y + d*led[1]/factor, offset_z + d*led[2]/factor);
                scene.add( sphere );
              }
            }
            else {
              console.log("preview3D jsonValues no leds", jsonValues.pview);
              jsonValues.pview = null;
            }
          }
        }
        else {
          console.log("preview3D jsonValues no outputs", jsonValues.pview);
          jsonValues.pview = null;
        }
        jsonValues.pview.new = false;
      }

      if (jsonValues.pview) {
        if (renderer.width != gId("pview").width || renderer.height != gId("pview").height)
          renderer.setSize( gId("pview").width, gId("pview").height);
        //light up the cube
        let firstLed = 4;
        var i = 1;
        if (jsonValues.pview.outputs) {
          // console.log("preview2D jsonValues", jsonValues.pview);
          for (var output of jsonValues.pview.outputs) {
            if (output.leds) {
              for (var led of output.leds) {
                if (i < scene.children.length) {
                  scene.children[i].visible = leds[i*3 + firstLed] + leds[i*3 + firstLed + 1] + leds[i*3 + firstLed+2] > 10; //do not show blacks
                  if (scene.children[i].visible)
                    scene.children[i].material.color = new THREE.Color(`${leds[i*3 + firstLed]/255}`, `${leds[i*3 + firstLed + 1]/255}`, `${leds[i*3 + firstLed + 2]/255}`);
                }
                i++;
              }
            }
            else {
              console.log("preview3D jsonValues no leds", jsonValues.pview);
              jsonValues.pview = null;
            }
          }
        }
        else {
          console.log("preview3D jsonValues no outputs", jsonValues.pview);
          jsonValues.pview = null;
        }
      }

      controls.update(); // apply orbit controls

      renderer.render( scene, camera);
    }); //import OrbitControl
  }); //import Three
} //preview3D

function previewBoard(node, leds) {
  let ctx = node.getContext('2d');
  //assuming 20 pins
  let mW = 10; // matrix width
  let mH = 2; // matrix height
  let pPL = Math.min(node.width / mW, node.height / mH); // pixels per LED (width of circle)
  let lOf = Math.floor((node.width - pPL*mW)/2); //left offeset (to center matrix)
  let i = 4;
  ctx.clearRect(0, 0, node.width, node.height);
  for (let y=0.5;y<mH;y++) for (let x=0.5; x<mW; x++) {
    if (leds[i] + leds[i+1] + leds[i+2] > 20) { //do not show nearly blacks
      ctx.fillStyle = `rgb(${leds[i]},${leds[i+1]},${leds[i+2]})`;
      ctx.beginPath();
      ctx.arc(x*pPL+lOf, y*pPL, pPL*0.4, 0, 2 * Math.PI);
      ctx.fill();
    }
    i+=3;
  }
}
