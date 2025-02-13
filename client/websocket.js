import {ChessBoard} from "./client-game.js";

export class Client {
  
  constructor(){
    /* a game with front end interface */
    this.boardInterface = new ChessBoard();
    
    //Error output, FEN input, fen submit button
    this.errorLog = document.getElementById("errorLog");
    this.inputFEN = document.getElementById("fen");
    this.submitFEN = document.getElementById("fenEnter");
  
    //did it establish connection atleast once?
    this.onceConnected = 0;

    // This file should be written by wsserver.c !!!
    this.socket = 
      new WebSocket("ws://localhost:8080");
    // Ensures binary data (if any. not preferred ) 
    // ..is received as ArrayBuffer 
    this.socket.binaryType = "arraybuffer"; 

    this.socket.onopen = () => {
      console.log("Connected to wsServer");
      this.onceConnected = 1;
    };

    this.socket.onmessage = (event) => {
      //console.log("Received:", event.data);
      if (event.data instanceof ArrayBuffer)
        console.log("Error : Weird!! Only expect string");
      else if (event.data instanceof Blob)
        console.log("Error : Weird!! Only expect string");
      else
        this.recvDecode(event.data);
    };

    this.socket.onclose = () => {
      if(this.onceConnected === 1)
        this.error ("Server Error : Connection Lost");
      else
        this.error ("Server Error : Couldn't establish a connection");
    };

    /* Enable all the buttons input field, etc, on the page*/
    this.eventListen();

  } // End of the default contructor

  error(msg) {
    console.log(msg);
    this.errorLog.textContent = msg;
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
  
  encodeSend(type, msg) {
    console.log("send: "+ type + msg);
    this.socket.send(type + msg);
  }

  //function 
  recvDecode(msg) {
    let type = msg[0];
    if (type === 'S') {
      console.log("Server " + msg);
      return 1;
    }
    else if ('wWeE'.includes(type))  {  
      //Error/warning from server
      this.error('Server ' + msg);  
      return 0;
    }
    else if (type === 'f') {
      //let fen = msg.substring(1);
      //console.log('restarting with FEN ' + fen); 
      //this.boardInterface.load( fen );
      return 1;
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

  /* To play a game with server */
  async play(fen){
    this.boardInterface.load(fen);
    /* Once you let the mouse listen to drag */
    while(this.boardInterface.chess.moves()){
      /* See if the game is over */
      let color = this.boardInterface.chess.turn();
      //if player's turn
      if(1) {
        let move = await this.boardInterface.eventListen(color);
        if(move) {
          /* Finish the move on the board */
          await this.boardInterface.playerMove(move);
          /* Reflect the move in chess.js */
          this.boardInterface.chess.move(move);
          /* Encode a mesage that you can send to server*/
          let msg = 'm' + move.from + move.to + 
            (move.flags.includes('p') ? move.promotion : '');
          /* Send the move to the server*/
          this.socket.send(msg);
        }
        else {
          console.log("Client Error: Impossible Move");
        }
        this.boardInterface.move = null;
      }
      else {
        //await server
      }
    } 
    // Game Ended
    console.log("Game over!");
  }

} // End of the class "Client"

    
/* Create a socket */
const socket = new Client();
/* start a game with server */
/* keep on playing till connection fails */
socket.play(null);
