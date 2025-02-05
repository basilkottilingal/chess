
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

class Client {
  
  constructor(){

    //Error 
    this.errorLog = document.getElementById("errorLog");
    // FEN Input 
    this.inputFEN = document.getElementById("fen");
    this.submitFEN = document.getElementById("fenEnter");

    // This file should be written by wsserver.c !!!
    this.socket = 
      new WebSocket("ws://localhost:8080");
    // Ensures data is received as ArrayBuffer 
    this.socket.binaryType = "arraybuffer"; 

    this.socket.onopen = function () {
      console.log("Connected to wsServer");
    };

    this.socket.onmessage = function (event) {
      //console.log("Received:", event.data);
      if (event.data instanceof ArrayBuffer) {
        let uint8Array = new Uint8Array(event.data);
        console.log(uint8Array);
        // console.log("Received Binary Data:", uint8Array);
        // Example: Convert binary to string 
        this.decode(event.data);
      } else if (event.data instanceof Blob) {
          console.log("ERROR : Weird!! socket binary type is arraybuffer");
      }
      else {
        console.log("Received Non-Binary Data:", event.data);
      }
    };

    this.socket.onclose = function () {
      console.log("Connection Lost");
      this.errorLog.textContent = "Server Error: Connection Lost!";
    };
  } // End of the default contructor

  //function 
  encode(msgtype, binaryMsg) {
    let combined =  
      new Uint8Array(1 + binaryMsg.length);
    combined[0] = msgtype;
    combined.set(binaryMsg, 1);
    return combined.buffer;
  }

  //function 
  encodeText(info, msg) {
    let combined =  
      new Uint8Array(2 + msg.length);
    combined[0] = ENCODE.text;
    combined[1] = info;
    let encoder = new TextEncoder();
    combined.set(encoder.encode(msg), 2);
    console.log("Sending:: type:",info," msg:", msg )
    return combined.buffer;
  }

  //function 
  decode(msg) {
    let type = msg[0];
    let start = (type === ENCODE.text) ? 2 : 1;
  }
    
  // Send input FEN to server
  //function 
  sendFen() {
    let fen = this.inputFEN.value.trim();
    if( fen.length > 22 ) {
      this.socket.send(fen);
      //if invalid
      this.inputFEN.value = "";
      this.inputFEN.disabled = true;
      this.submitFEN.disabled = true;
      this.errorLog.textContent = "";
    }
    else {
      this.errorLog.textContent = "invalid fen! Please enter a valid one";
      console.log("Invalid FEN");
    }
  }

  //List all eventListen of buttons, etc
  //function 
  eventListen(){
 
    // Restart button 
    document.addEventListener('DOMContentLoaded', function() {
      // Get the button element by ID
      const button = document.getElementById('restart');
      // Add the event listener for the click event
      button.addEventListener('click', function() {
        //alert('Button clicked!');
        this.socket.send("Restart");
      });
    });
   
    // Undo button
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

    let debounceTimer;

    // Handle the input event with debounce
    this.inputFEN.addEventListener("input", function() {
      clearTimeout(debounceTimer); // Clear the previous timer
      debounceTimer = setTimeout(function() {
        //
      }, 500); // Adjust 500ms as needed
    });

    // Trigger on Enter key press
    this.inputFEN.addEventListener("keydown", function(event) {
      if (event.key === "Enter") {
        //event.preventDefault(); // Prevent form submission
        this.sendFen();
      }
    });

    // Trigger on button click
    this.submitFEN.addEventListener("click", function() {
      this.sendFen();
    });
  } // End of eventListen()
} // End of the class "Client"


/* COnstructor opens socket*/
const socket = new Client();
/* Enable all the buttons input field, etc */
socket.eventListen();
