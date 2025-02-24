/**
TODO : Order class functions like
1) constructor
2) buttons etc.
3) reset/restart etc
3) Wait for message
3) functions that trigger client to server 
4) 
*/

import {ChessBoard} from "./client-game.js";

export class Client {
  
  constructor(){
    /* a game with front end interface */
    this.boardInterface = new ChessBoard();
    this.restartFen = '';

    //by default client plays for both b/w
    this.client = 'bw';
    
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

    this.socket.onclose = () => {
      this.error (this.onceConnected === 1 ?
        "Server Error : Connection Lost" :
        "Server Error : Couldn't establish a connection");
    };
    
    /* onmessage() is triggered when a msg is received by socket  
    ..*/
    this.socket.onmessage = (event) => {
      /* We don't expect binary msg*/
      if (event.data instanceof ArrayBuffer ||
          event.data instanceof Blob) {
        console.log("Error : We expect only string communication");
        return;
      }
      /* Let's decode the message */
      let msg = event.data;
      let type = msg[0];
      if (type === 'S') {
        /*Success msg from server. for verification*/
        console.log("Server " + msg);
        return 1;
      }
      if ('wWeE'.includes(type))  {  
        /*Error/warning from server */
        this.error('Server ' + msg);  
      }
      else if (type === 'f') {
        let fen = msg.substring(1);
        console.log('restarting with FEN ' + fen); 
        this.restart(fen);
      }
      else {
        console.log('Msg neglected in onmessage() ' + msg); 
        /* ANy other kind of message should be handled in waitForMessage() */
      }
    }

    /* Enable all the buttons input field, etc, on the page*/
    this.eventListen();

  } // End of the default contructor
    

  /* Wait for a particular message. This blocks thread,
  .. until a server msg is recvd*/
  waitForMessage( ) {
    return new Promise((resolve) => {
      /* Define the function that triggers to the eventListen 
      .. that listens to socket message */
      const msgHandler =  (event) => {
        /* Since msg is recvd, you can switch off listening 
        ..*/
        this.socket.removeEventListener("message", msgHandler);
        resolve(event.data);  
        console.log("Waited to recv this" + event.data);
      };
      /* Start Listening 
      ..*/
      this.socket.addEventListener("message", msgHandler);
    });
  }

  /* Display and log any error/warning msg from Server
  ..*/
  error(msg) {
    console.log(msg);
    this.errorLog.textContent = msg;
    this.errorLog.style.color = "red";
    setTimeout(()=>{}, 1000); //wait for .2s
  }

  /* Display msg in green color on page 
  ..*/
  success(msg) {
    console.log(msg);
    this.errorLog.textContent = msg;
    this.errorLog.style.color = "green";
  }

  /* "string" encoding for easier implementation in js (client side) 
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
  */
 
  /* Send a msg to server via socket 
  ..*/ 
  encodeSend(type, msg) {
    console.log("send: "+ type + msg);
    this.socket.send(type + msg);
  }
  
  /* Send an FEN from input field to server  via socket  
  .. by adding 'f' at the beginning
  ..*/
  async sendFen() {
    let fen = this.inputFEN.value.trim();
    if( fen.length <= 22 ) { 
      this.error("invalid fen! Please enter a valid one");
      return;
    }

    /* encode 'f' at the start and send it via socket*/
    let msg = this.encodeSend('f', fen); 
     
    this.inputFEN.value = "";
    this.inputFEN.disabled = true;
    this.submitFEN.disabled = true;
    this.errorLog.textContent = "";

    /* Now wait for a reply. Be it error/success, it
    .. will be handled by socket.onmessage()*/
    let reply = await this.waitForMessage();
  }

  /* Send a restart command 'r' to server
  */
  async restartGame() {
    this.socket.send("r");
    console.log("Game Restart ");
    let reply = await this.waitForMessage();
    if(reply[0] === 'S') 
      this.restart("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    /* If any error, it will be handled by onmessage()
    ..*/
  }

  /* eventListeners associated with input, button, etc on the page. */
  eventListen(){
    /* Restart button. */ 
    document.addEventListener('DOMContentLoaded', () => {
      const button = document.getElementById('restart');
      button.addEventListener('click', () => {
        this.restartGame();
      });
    });
   
    /* Undo button */
    document.addEventListener('DOMContentLoaded', () => {
      const button = document.getElementById('undo');
      button.addEventListener('click', () => {
        this.undo();
      });
    });

    /* Input field to enter fen */
    let debounceTimer;
    this.inputFEN.addEventListener("input", () => {
      /* Clear the previous timer */
      clearTimeout(debounceTimer); 
      debounceTimer = setTimeout( () => {
      }, 500); 
    });

    /* Trigger on 'Enter' key */
    this.inputFEN.addEventListener("keydown", (event) => {
      if (event.key === "Enter") {
        //event.preventDefault(); // Prevent form submission
        this.sendFen();
      }
    });

    /* Trigger on clicking on button 'submit' */
    this.submitFEN.addEventListener("click", () => {
      this.sendFen();
    });
  } 

  /* Set a color 'w'/'b' to the server */
  async serverPlayer(serverColor) {
    this.client = serverColor == 'w' ? 'b' : 'w' ;
    let msg = 'p' + serverColor;
    this.socket.send(msg);
    /* Now wait for a reply. Be it error/success, it
    .. will be handled by socket.onmessage()*/
    let reply = await this.waitForMessage();
  }

  /* Send a command 'M' to server which means asking
  .. to make a move 
  */
  async serverMove() {
    return new Promise(async (resolve, reject) => {
      try {
        this.socket.send('M');
        /* Now wait for a reply. If it's a move ('m'), 
        .. decode the move here. Any other type of replies 
        .. will be handled by socket.onmessage()*/
        let msg = await this.waitForMessage();
        let type = msg[0];
        if (type === 'm'){
          let from = msg[1] + msg[2];
          let to = msg[3] + msg[4];
          if(msg.length === 5)
            resolve({from : from, to : to});
          else if(msg.length === 6)
            resolve({from : from, to : to, promotion :msg[5]});
          else
            resolve(null);
        }
        else {
          /* Be it error/success, it
          .. will be handled by socket.onmessage()*/
          resolve(null);
        }
        // Resolve the promise with the received message
      } catch (error) {
        reject(error); // Ensure errors don’t leave the Promise hanging
      }
    });
  }

  /* Ask server to undo the last move by sending 'u' via socket
  ..*/
  async undo() {
    this.socket.send("u");
    /* Now wait for a reply. Be it error/fen, it
    .. will be handled by socket.onmessage()*/
    let reply = await this.waitForMessage();
  }

  /* setting the variable restartFen to an 'fen' 
  .. so that eventListen in listenForRestart()
  .. will be triggered. 
  */
  restart(fen) {
    this.restartFen = fen;
    this.serverPlayer('b'); //server by default plays black
  }

  /* Every interval of 500ms, it listen if any 
  .. this.restartFen is set to something, so that 
  .. it restarts the game 
  */
  async listenForRestart(signal){
    return new Promise((resolve, reject) => {
      /* This function resolves if " restartFen != '' " . 
      .. It is checked every "interval" of 500ms */
      const interval = setInterval( () => {
        if(this.restartFen != '') {
          clearInterval(interval);
          resolve("restart");
        }
      }, 500);
      /* This is rejected if it receives an "abort" signal,
      .. which happens usually when game over */
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
        if( this.client.includes(color) ) {

          try {
            //Let player Make a move by dragging+dropping a chesspiece;
            let move = await this.boardInterface.eventListen(color);
            if(move) {
              /* Finish the move on the board */
              await this.boardInterface.playerMove(move, false);
              /* Reflect the move in chess.js */
              this.boardInterface.chess.move(move);
              /* Encode a mesage that you can send to server*/
              let msg = 'm' + move.from + move.to + 
                (move.flags.includes('p') ? move.promotion : '');
              /* Send the move to the server*/
              this.socket.send(msg);
              /* Wait for a success
              .. FIXME : Might not be ideal to expect a "success" msg every time
              ..*/
              let reply = await this.waitForMessage();
              /* Update board  in chess.js*/
              this.success(this.boardInterface.chess.fen());
              /* In case of not success 'S', Abrupt end of play() */
              if(reply[0] != 'S')
                resolve("error:abort:ServerCannotMove");
              this.boardInterface.move = null;
            }
          }
          catch(error) {
            /* To avoid crashing */
            console.log("An error occured" + error.message );
          }
        }
        else {
          /* Servers turn*/
          try {
            /* Send 'M' (make a move) to server and wait for the move
            ..*/
            let move = await this.serverMove();
            if(move) {
              let _move_ = this.boardInterface.chess.move(move);
              /* Finish the move on the board */
              await this.boardInterface.playerMove(_move_, true);
              /* Reflect the move in chess.js */
            }
            else {
              /* In case no move made by server, Abrupt end of play() */
              resolve("error:abort:NoServerMove");
            }
          }
          catch(error) {
            console.log("An error occured" + error.message);
          }
        }

        // check if an Abort is activated because the
        // .. other task(listenForAbort()) is completed.
        // Otherwwise continue playing
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
    const controller = new AbortController();
    const signal = controller.signal;
    console.log("Error in play(). Restart");
    socket.restartFen = '';
    socket.inputFEN.disabled = false;
    socket.submitFEN.disabled = false;
    result = await socket.listenForRestart(signal);
  }
  console.log("Game Over/Restarted/Thrown an error?: " + result);
}
