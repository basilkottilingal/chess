// error log for user, that will be displayed on the page
const errorLog = document.getElementById("errorLog");

const ENCODE = {
  handshake: 0x0, // send/recv
  fen:     0x1, // send only
  board:   0x2, // recv only
  move:    0x3, // send/recv
  moves:   0x4, // recv only
  undo:    0x5, // send only
  restart: 0x6, // send only
  meta:    0x7, // send/recv
  text:    0x8, // send/recv
  warning: 0xc, // send/recv
  error:   0xd, // send/recv
  crash:   0xe, // send/recv
  debug:   0xf  // send/recv
};

function encode(msgtype, binaryMsg) {
  let combined =  
    new Uint8Array(1 + binaryMsg.length);
  combined[0] = msgtype;
  combined.set(binaryMsg, 1);
  return combined.buffer;
}

function encodeText(info, msg) {
  let combined =  
    new Uint8Array(2 + msg.length);
  combined[0] = ENCODE.text;
  combined[1] = info;
  let encoder = new TextEncoder();
  combined.set(encoder.encode(msg), 2);
  console.log("Sending:: type:",info," msg:", msg )
  return combined.buffer;
}

function decode(msg) {
  let type = msg[0];
  let start = (type === ENCODE.text) ? 2 : 1;

  //let decoder = new TextDecoder();
  //let text = decoder.decode(msg.subarray(start));
  console.log("Decode received:", text, "len", msg.length, " start", start, " type", type);
}


// This file should be written by wsserver.c !!!
const socket = 
  new WebSocket("ws://localhost:8080");
// Ensures data is received as ArrayBuffer 
socket.binaryType = "arraybuffer"; 

  
socket.onopen = function () {
  console.log("Connected to wsServer");
};

socket.onmessage = function (event) {
  //console.log("Received:", event.data);
  if (event.data instanceof ArrayBuffer) {
    let uint8Array = new Uint8Array(event.data);
    console.log(uint8Array);
    // console.log("Received Binary Data:", uint8Array);
    // Example: Convert binary to string 
    decode(event.data);
  } else if (event.data instanceof Blob) {
    console.log("ERROR : Weird!! socket binary type is arraybuffer");
  }
  else {
    console.log("Received Non-Binary Data:", event.data);
  }
};

socket.onclose = function () {
  console.log("Connection Lost");
  errorLog.textContent = "Server Error: Connection Lost!";
};

    
// Wait for the DOM to load before executing the function
document.addEventListener('DOMContentLoaded', function() {
      // Get the button element by ID
  const button = document.getElementById('restart');
      // Add the event listener for the click event
  button.addEventListener('click', function() {
    //alert('Button clicked!');
    socket.send("Restart");
  });
});
   
/* 
document.addEventListener('DOMContentLoaded', function() {
      // Get the button element by ID
  const button = document.getElementById('handshake');
      // Add the event listener for the click event
  button.addEventListener('click', function() {
    //alert('Button clicked!');
    socket.send("Hello Server!");
  });
});
*/

document.addEventListener('DOMContentLoaded', function() {
      // Get the button element by ID
  const button = document.getElementById('undo');
      // Add the event listener for the click event
  button.addEventListener('click', function() {
    //alert('Button clicked!');
    //socket.send("undo");
    console.log("Sending:: type:",ENCODE.undo," msg:", "undo" )
    socket.send(encodeText(ENCODE.undo, "undo"));
  });
});

  
/*
// Add an event listener for the 'input' event
document.getElementById("fen").addEventListener("blur", function(event) {
  // Update the output element with the value of the input field
  document.getElementById("fenValidity").textContent = "FEN: " + event.target.value;

});
*/

const inputFEN = document.getElementById("fen");
const submitFEN = document.getElementById("fenEnter");


let debounceTimer;

// Function to handle the result
function sendFen() {
  let fen = inputFEN.value.trim();
  if( fen.length > 22 ) {
    socket.send(fen);
    //if invalid
    inputFEN.value = "";
    inputFEN.disabled = true;
    submitFEN.disabled = true;
    errorLog.textContent = "";
  }
  else {
    errorLog.textContent = "invalid fen! Please enter a valid one";
    console.log("Invalid FEN");
  }
}

// Handle the input event with debounce
inputFEN.addEventListener("input", function() {
  clearTimeout(debounceTimer); // Clear the previous timer
  debounceTimer = setTimeout(function() {
  }, 500); // Adjust 500ms as needed
});

// Trigger on Enter key press
inputFEN.addEventListener("keydown", function(event) {
  if (event.key === "Enter") {
    //event.preventDefault(); // Prevent form submission
    sendFen();
  }
});

// Trigger on button click
submitFEN.addEventListener("click", function() {
  sendFen();
});
