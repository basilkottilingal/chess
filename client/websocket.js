export class Client {
  
  constructor(){

    //Error 
    this.errorLog = document.getElementById("errorLog");
    // FEN Input 
    this.inputFEN = document.getElementById("fen");
    this.submitFEN = document.getElementById("fenEnter");
  
    //did it establish connection atleast once?
    this.onceConnected = 0;

    // This file should be written by wsserver.c !!!
    this.socket = 
      new WebSocket("ws://localhost:8080");
    // Ensures data is received as ArrayBuffer 
    this.socket.binaryType = "arraybuffer"; 

    this.socket.onopen = () => {
      console.log("Connected to wsServer");
      this.onceConnected = 1;
    };

    this.socket.onmessage = (event) => {
      //console.log("Received:", event.data);
      if (event.data instanceof ArrayBuffer) {
          console.log("Error : Weird!! Only expect string");
      } else if (event.data instanceof Blob) {
          console.log("Error : Weird!! Only expect string");
      }
      else {
        this.recvDecode(event.data);
      }
    };

    this.socket.onclose = () => {
      if(this.onceConnected === 1)
        this.error ("Server Error : Connection Lost");
      else
        this.error ("Server Error : Couldn't establish a connection");
    };

  } // End of the default contructor

  error(errorMsg) {
    console.log(errorMsg);
    this.errorLog.textContent = errorMsg;
    this.errorLog.style.color = "red";
  }

  /* "string" encoding for easier implementation in js (client side) */
  ENCODE = {
    start:   's', // send/recv   : command to "start" the game. (client will send the starting time)
    start:   'S', // send/recv   : "SUCCESS"
    fen:     'f', // send only   : user defined FEN (send from client to server); will wait for the 'board' (if valid)
    board:   'b', // recv only   : 'board' 8x8 char of pieces.
    boardMeta: 'B', // recv only : board meta data
    move:    'm', // send/recv   : will encode starting and ending square in Algerbraic Notation. 
                  //              (other details have to be locall computed (like promotion, capture, enpassante, etc))
    moves:   'M', // recv only   : List of moves. (Only for debugging moves of the server side "move.h" . Compare with chess.js)
    undo:    'u', // send only   : Undo a move. (only in debug mode)
    restart: 'r', // send only   : Restart a game 
    meta:    'x', // send/recv   : Any complicated data. have to  give info on the type of msg..
    text:    't', // send/recv   : simple text.
    game_status: 'g', // send/recv  : Gamestatus
    warning: 'w', // send/recv   : warning message
    error:   'e', // send/recv   : error message 
    debug:   'd'  // send/recv   : ask server to run in debug mode.
  };

  //function
  /* 
  encode(msgtype, binaryMsg) {
    let combined =  
      new Uint8Array(1 + binaryMsg.length);
    combined[0] = msgtype;
    combined.set(binaryMsg, 1);
    return combined.buffer;
  }

  //function 
  encodeText(msgtype, msg) {
    let combined =  
      new Uint8Array(1 + msg.length);
    combined[0] = msgtype;
    let encoder = new TextEncoder();
    combined.set(encoder.encode(msg), 1);
    console.log("Sending:: msg", combined )
    return combined.buffer;
  }
  */
  
  encodeSend(type, msg) {
    console.log("send: "+ type + msg);
    this.socket.send(type + msg);
  }

  //function 
  recvDecode(msg) {
    let type = msg[0];
    if (type === 'S') {
      console.log("Server send :\"success\"");
      return 1;
    }
    else if ('wWeE'.includes(type))  {  
      //Error/warning from server
      this.error('Server ' + msg);  
      return 0;
    }
    else {
      console.log("Unknown msg Received : ", msg);
    }
    return 0;
  }
    
  // Send input FEN to server
  //function 
  sendFen() {
    let fen = this.inputFEN.value.trim();
    if( fen.length > 22 ) {
      let msg = this.encodeSend(this.ENCODE.fen, fen); //encode fen.
      //if invalid
      this.inputFEN.value = "";
      this.inputFEN.disabled = true;
      this.submitFEN.disabled = true;
      this.errorLog.textContent = "";
    }
    else {
      this.error("invalid fen! Please enter a valid one");
    }
  }

  //List all eventListen of buttons, etc
  //function 
  eventListen(){
 
    // Restart button 
    document.addEventListener('DOMContentLoaded', () => {
      // Get the button element by ID
      const button = document.getElementById('restart');
      // Add the event listener for the click event
      button.addEventListener('click', () => {
        //alert('Button clicked!');
        this.socket.send("r");
      });
    });
   
    // Undo button
    document.addEventListener('DOMContentLoaded', () => {
      // Get the button element by ID
      const button = document.getElementById('undo');
      // Add the event listener for the click event
      button.addEventListener('click', () => {
        //alert('Button clicked!');
        //socket.send("undo");
        this.socket.send("u");
      });
    });

    let debounceTimer;

    // Handle the input event with debounce
    this.inputFEN.addEventListener("input", () => {
      clearTimeout(debounceTimer); // Clear the previous timer
      debounceTimer = setTimeout( () => {
        //
      }, 500); // Adjust 500ms as needed
    });

    // Trigger on Enter key press
    this.inputFEN.addEventListener("keydown", (event) => {
      if (event.key === "Enter") {
        //event.preventDefault(); // Prevent form submission
        this.sendFen();
      }
    });

    // Trigger on button click
    this.submitFEN.addEventListener("click", () => {
      this.sendFen();
    });
  } // End of eventListen()
} // End of the class "Client"

