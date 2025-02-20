import {ChessBoard} from "./client-game.js";

export class Client {
  
  constructor(){
    /* a game with front end interface */
    this.boardInterface = new ChessBoard();
    this.restartFen = '';
    
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

  waitForMessage() {
    return new Promise((resolve) => {

      const msgHandler =  (event) => {
        this.socket.removeEventListener("message", msgHandler); 
        // Clean up
        resolve(event.data);  
      };
  
      this.socket.addEventListener("message", msgHandler);
          // Resolve when message arrives
    });
  }

  error(msg) {
    console.log(msg);
    this.errorLog.textContent = msg;
    this.errorLog.style.color = "red";
    setTimeout(()=>{}, 1000); //wait for .2s
  }

  success(msg) {
    console.log(msg);
    this.errorLog.textContent = msg;
    this.errorLog.style.color = "green";
  }

  /* "string" encoding for easier implementation in js (client side) */
  ENCODE = {
    start:   's', // send/recv   : command to "start" the game. (client will send the starting time)
    start:   'S', // send/recv   : "SUCCESS"
    fen:     'f', // send only   : user defined FEN (send from client to server); will wait for the 'board' (if valid)
    board:   'b', // recv only   : 'board' 8x8 char of pieces.
    boardMeta: 'B', // recv only : board meta data
    move:    'm', // send/recv   : will encode starting and ending square in Algerbraic Notation. 
    move:    'p', // send only   : which player ('w'/'b')  does server represent
                  //              (other details have to be locall computed (like promotion, capture, enpassante, etc))
    moves:   'M', // send only   : Server asked to make a move
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
      let fen = msg.substring(1);
      console.log('restarting with FEN ' + fen); 
      this.restart(fen);
      return 1;
    }
    else if (type === 'm' ) {
      console.log('server moved ' + msg);
      return 1;
    }
      
    console.log("Unknown msg Received : ", msg);
    return 0;
  }
    
  // Send input FEN to server
  //function 
  async sendFen() {
    let fen = this.inputFEN.value.trim();
    if( fen.length > 22 ) {
      let msg = this.encodeSend(this.ENCODE.fen, fen); //encode fen.
      //if invalid
      this.inputFEN.value = "";
      this.inputFEN.disabled = true;
      this.submitFEN.disabled = true;
      this.errorLog.textContent = "";

      /* Now wait for a reply */
      let reply = await this.waitForMessage();
      if(reply[0] === 'S') {
        this.restart(fen);
      }
      else {
        this.recvDecode(reply);
        //otherwise will be handled by recvDecode
      }
    }
    else {
      this.error("invalid fen! Please enter a valid one");
    }
  }

  async restartGame() {
    //let fen = msg.substring(1);
    this.socket.send("r");
    console.log("Game Restart ");
    let reply = await this.waitForMessage();
    if(reply[0] === 'S') {
      this.restart("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }
    else {
      //decode the error
      this.recvDecode(reply);
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
        this.restartGame();
      });
    });
   
    // Undo button
    document.addEventListener('DOMContentLoaded', () => {
      // Get the button element by ID
      const button = document.getElementById('undo');
      // Add the event listener for the click event
      button.addEventListener('click', () => {
        //alert('Button clicked!');
        this.undo();
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

  async undo() {
    this.socket.send("u");
    //recvDecode will do the rest
  }

  restart(fen) {
    this.restartFen = fen;
  }

  async listenForRestart(signal){
    return new Promise((resolve, reject) => {
      /* This function resolves if "isGameOn === 0". 
      .. It is checked every "inetrval" which 0.5s here */
      const interval = setInterval( () => {
        if(this.restartFen != '') {
          clearInterval(interval);
          resolve("restart");
        }
      }, 500); //runs every 0.5s
      /* This is rejected if it receives an "abort"signal*/
      signal.addEventListener ( "abort", () => {
        clearInterval(interval);
        reject("stopped listening for a Restart. Game should be over!");
      });
    });
  }

  /* To play a game with server */
  async play(fen, signal){

    return new Promise ( (resolve, reject) => {
        
      //Game is on. 
      this.restartFen = '';
      //Load the game with fen.
      this.boardInterface.load(fen);

      const somebodyMakeAMove = async () => {
        //let server or player make a move

        //Get the list of moves using chess.j
        let allMoves = this.boardInterface.chess.moves(); 
        if(allMoves.length === 0) {
          console.log( "Game is Over" );
          resolve("over");
          return;
        }
        // ask chess.js whose turn it is.  
        let color = this.boardInterface.chess.turn();
        if(1) {

          try {
            //Let player Make a move by dragging+dropping a chesspiece;
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
              /* Send the move to the server*/
              this.success(this.boardInterface.chess.fen());
            }
            else {
              console.log("Client Error: Impossible Move");
            }
            this.boardInterface.move = null;
          }
          catch(error) {
            //To avoid crashing
            console.log("An error occured" + error.message );
          }
        }
        else {
          //Let's wait for server to send a move or throw an error
          //await server
        }

        // check if an Abort is activated because the
        // .. other task(listenForAbort()) is completed.
        if(!signal.aborted) {
          //if not make a move
          setTimeout(somebodyMakeAMove, 0);  
        }

      }; //End of definition of "somebodyMakeAMove()"

      //Start Making a move
      setTimeout(somebodyMakeAMove, 0);  
 
      //add an event listener to abort this.play()
      signal.addEventListener( "abort", () => {
        reject("stopped playing.")
      });
    
    });// end of Promise()
  }//end of this.play()

  async playTillAbortOrOver(fen) {
    const controller = new AbortController();
    const signal = controller.signal;
    try {
      const result = await Promise.race([ this.play(fen, signal), this.listenForRestart(signal) ]);
      console.log("status = ", result);
      if (!signal.aborted)
        controller.abort(); // Stop the other functioin
      return result;
    } catch (error) {
      console.log("Error:", error);
      if (!signal.aborted)
        controller.abort(); // Stop the other functioin
      return "error";
    }
  }

} // End of the class "Client"

    
/* Create a socket */
const socket = new Client();
/* start a game with server */
/* keep on playing till connection fails */
let result = "restart";
while(1) {
  if(result === "restart") {
    let fen = socket.restartFen === '' ? null : socket.restartFen;
    socket.success(fen);
    result = await socket.playTillAbortOrOver(fen);
  }
  else if(result === "over") {
    const controller = new AbortController();
    const signal = controller.signal;
    socket.inputFEN.disabled = false;
    socket.submitFEN.disabled = false;
    //if the game is over you can wait for user to restart
    result = await socket.listenForRestart(signal);
  }
  else {
    console.log("weird error :" + result);
    socket.error("Client Error : Reload the page");
    break;
  }
  console.log("Game Over/Restarted/Thrown an error?: " + result);
}
