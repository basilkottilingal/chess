import {ChessBoard} from "./client-game.js";

/*
.. the class "Client" handles frontend that connects to the server
.. via websocket. Common routines are communicating moves, board reset,
.. undo, change player, game status, etc
*/
export class Client
{
  
  constructor()
  {
    /* a game with front end interface */
    this.boardInterface = new ChessBoard();
    this.restartFen = '';

    /* by default client plays for both b/w */
    this.client = 'bw';
    
    /* Error output, FEN input, fen submit button */
    this.errorLog  = document.getElementById ("errorLog");
    this.inputfen  = document.getElementById ("fen");
    this.submitfen = document.getElementById ("fenEnter");
  
    /* did it establish connection atleast once? */
    this.onceConnected = 0;

    /*
    .. fixme : This file should be have some mechanism to change port. 
    .. maybe communicate with src/.server
    */
    this.socket = new WebSocket ("ws://localhost:8080");

    /* Ensures binary data (if any. not preferred ) is received as ArrayBuffer */
    this.socket.binaryType = "arraybuffer"; 

    this.socket.onopen = () =>
      {
        console.log ("connected to src/.server");
        this.onceConnected = 1;
      };

    this.socket.onclose = () =>
      {
        this.error (this.onceConnected === 1 ?
          "server error : connection lost" :
          "server error : couldn't establish a connection");
      };
    
    /* onmessage() is triggered when a msg is received by socket */
    this.socket.onmessage = (event) =>
      {
        /* We don't expect binary msg*/
        if ( event.data instanceof ArrayBuffer ||
             event.data instanceof Blob)
        {
          console.log("error : we expect only string communications");
          return;
        }

        /* Let's decode the message */
        let msg = event.data;
        let type = msg[0];

        console.log ("{server >> client}" + msg);

        if (type === 'S')
        {
          /*Success msg from server. for verification*/
          return 1;
        }
        if (type === 'g')
        {
          this.displayGreen(msg.substring(1));
        }
        if ('wWeE'.includes(type))
        {
          /* error or warning from server */
          this.error('server ' + msg);  
        }
        else if (type === 'f')
        {
          let fen = msg.substring(1);
          this.restart(fen);
        }
        /*
        .. Any other kind of message should be handled in waitForMessage()
        .. console.log('msg neglected in onmessage() ' + msg); 
        */
      }

    this.serverPlayer('b'); //server by default plays black
    /* Enable all the buttons input field, etc, on the page*/
    this.eventListen();

  } /* End of the default contructor */
    

  /*
  .. wait for a particular message. this blocks thread, until a server msg is recvd
  */
  waitForMessage ()
  {
    return new Promise ((resolve) => 
      {
        /* 
        .. define the function that triggers to the eventListen 
        .. that listens to socket message
        */
        const msgHandler =  (event) => 
          {
            /* Since msg is recvd, you can switch off listening */
            this.socket.removeEventListener ("message", msgHandler);
            resolve(event.data);  
            console.log("{await : server -> client}" + event.data);
          };

        /* start listening */
        this.socket.addEventListener("message", msgHandler);
      });
  }

  /* display and log any error or warning msg from server */
  error (msg)
  {
    console.log (msg);
    this.errorLog.textContent = msg;
    this.errorLog.style.color = "red";
    setTimeout (() => {}, 1000); //wait for ?? s
  }

  /* display msg in green color on page */
  displayGreen (msg)
  {
    console.log (msg);
    this.errorLog.textContent = msg;
    this.errorLog.style.color = "green";
  }
 
  /* send a msg to the server via socket */
  encodeSend (type, msg)
  {
    console.log ("{client to server} "+ type + msg);
    this.socket.send (type + msg);
  }
  
  /* 
  .. send an fen from input field to server  via socket by adding 'f' at
  .. the beginning
  */
  async sendFen ()
  {
    let fen = this.inputfen.value.trim ();
    if( fen.length <= 22 )
    {
      this.error("invalid fen! Please enter a valid one");
      return;
    }
    /* encode 'f' at the start and send it via socket*/
    let msg = this.encodeSend ('f', fen);
    this.inputfen.value       = "";
    /*
    .. this.inputfen.disabled    = true;
    .. this.submitfen.disabled   = true;
    .. this.errorLog.textContent = "";
    */
    /*
    .. Now wait for a reply. Be it error/success, it will be handled by
    .. socket.onmessage ()
    */
    let reply = await this.waitForMessage ();
  }

  /* Send a restart command 'r' to server */
  async restartGame ()
  {
    this.socket.send ("r");
    console.log ("game restart");
    let reply = await this.waitForMessage ();
    if (reply[0] === 'S')
      this.restart ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    /* If any error, it will be handled by onmessage() */
  }

  /* eventListeners associated with input, button, etc on the page. */
  eventListen ()
  {
    /* Restart button. */ 
    document.addEventListener ('DOMContentLoaded', () =>
      {
        const button = document.getElementById ('restart');
        button.addEventListener ('click', () =>
          {
            this.restartGame();
          });
      });
   
    /* Undo button */
    document.addEventListener ('DOMContentLoaded', () =>
      {
        const button = document.getElementById('undo');
        button.addEventListener ('click', () =>
          {
            this.undo();
          });
      });

    /* Input field to enter fen */
    let debounceTimer;
    this.inputfen.addEventListener ("input", () =>
      {
        /* Clear the previous timer */
        clearTimeout (debounceTimer); 
        debounceTimer = setTimeout ( () =>
          {
          }, 500); 
      });

    /* Trigger on 'Enter' key */
    this.inputfen.addEventListener("keydown", (event) =>
      {
        if (event.key === "Enter")
        {
          //event.preventDefault(); // Prevent form submission
          this.sendFen ();
        }
      });

    /* Trigger on clicking on button 'submit' */
    this.submitfen.addEventListener("click", () =>
      {
        this.sendFen ();
      });
  }  /* end of eventListen () {} */

  /* Set a color 'w'/'b' to the server */
  async serverPlayer (serverColor)
  {
    this.client = serverColor == 'w' ? 'b' : 'w' ;
    let msg = 'p' + serverColor;
    this.socket.send (msg);
    /* Now wait for a reply. Be it error/success, it
    .. will be handled by socket.onmessage()*/
    let reply = await this.waitForMessage ();
  }

  /* Send a command 'M' to server which means asking to make a move */
  async serverMove()
  {
    return new Promise (async (resolve, reject) =>
      {
        try
        {
          this.socket.send('M');
          /* Now wait for a reply. If it's a move ('m'), 
          .. decode the move here. Any other type of replies 
          .. will be handled by socket.onmessage()*/
          let msg = await this.waitForMessage();
          let type = msg[0];

          if (type === 'm')
          {
            let from = msg[1] + msg[2];
            let to = msg[3] + msg[4];
            if(msg.length === 5)
              resolve({from : from, to : to});
            else if(msg.length === 6)
              resolve({from : from, to : to, promotion :msg[5]});
            else
              resolve(null);
          }
          else
          {
            /* Be it error / success, it will be handled by socket.onmessage() */
            resolve(null);
          }
          /* Resolve the promise with the received message */
        } catch (error)
        {
          reject(error); /*  Ensure errors don’t leave the Promise hanging */
        }
      });
  }

  /* Ask server to undo the last move by sending 'u' via socket */
  async undo ()
  {
    this.socket.send ("u");
    /*
    .. Now wait for a reply. be it error orfen, it will be handled by
    .. socket.onmessage()
    */
    let reply = await this.waitForMessage ();
  }

  /*
  .. setting the variable restartFen to an 'fen' so that eventListen in
  .. listenForRestart () will be triggered. 
  */
  restart (fen)
  {
    this.restartFen = fen;
    this.serverPlayer ('b');
  }

  /*
  .. Every interval of 500ms, it listen if any this.restartfen is set to something,
  .. so that it restarts the game 
  */
  async listenForRestart (signal)
  {
    return new Promise ((resolve, reject) =>
      {
        /*
        .. this function resolves if " restartFen != '' " . 
        .. it is checked every "interval" of 500ms
        */
        const interval = setInterval( () =>
          {
            if(this.restartFen != '')
            {
              clearInterval(interval);
              resolve("restart");
            }
          }, 500);

        /*
        .. this is rejected if it receives an "abort" signal,
        .. which happens usually when game over
        */
        signal.addEventListener ( "abort", () =>
          {
            clearInterval(interval);
            reject("stopped listening for a Restart. Game should be over!");
          });
      });
  }

  /* To play a game with server */
  async play (fen, signal)
  {
    return new Promise ( (resolve, reject) =>
      {
        /* Game is on. */
        this.restartFen = '';
        this.boardInterface.load(fen);

        /*
        .. define a function "somebodyMakeAMove ()", that makes a move (either
        .. wait for client to drag chesspiece or wait for server to make the
        .. move)
        */
        const somebodyMakeAMove = async () =>
          {
            /* let server or player make a move */

            /* Get the list of moves using chess.j */
            let color = this.boardInterface.chess.turn (); 
            let allMoves = this.boardInterface.chess.moves ();
            if (allMoves.length === 0)
            {
              this.displayGreen ("game over");
              this.socket.send ("g"); /* wait for exact game result */ 
              resolve ("over");
              return;
            }
        
            if (this.client.includes(color)) /* if client's turn */
            {
              try
              {
                /* let player make a move by dragging + dropping a chesspiece */
                let move = await this.boardInterface.eventListen(color);
                if (move)
                {
                  /* Finish the move on the board */
                  await this.boardInterface.playerMove (move, false);

                  /* Reflect the move in chess.js */
                  this.boardInterface.chess.move (move);

                  /* Encode a mesage that you can send to server*/
                  let msg = 'm' + move.from + move.to + 
                    (move.flags.includes ('p') ? move.promotion : '');

                  /* Send the move to the server*/
                  this.socket.send(msg);

                  /* 
                  .. wait for a success
                  .. fixme : Might not be ideal to expect a "success" msg every time
                  */
                  let reply = await this.waitForMessage ();

                  /* Update board  in chess.js  */
                  this.displayGreen (this.boardInterface.chess.fen ());

                  /* In case of not success 'S', Abrupt end of play() */
                  if (reply[0] != 'S')
                    resolve ("error:abort:ServerCannotMove");
                  this.boardInterface.move = null;
                }
              } catch (error)
              {
                /* To avoid crashing */
                console.log("An error occured" + error.message );
              }
            }  /* end of client's turn */
            else /* if server's turn */
            {
              try
              {
                /* send 'M' (make a move) to server and wait for the move */
                let move = await this.serverMove ();
                if (move)
                {
                  let _move_ = this.boardInterface.chess.move (move);
                  /* Finish the move on the board */
                  await this.boardInterface.playerMove (_move_, true);
                  /* Reflect the move in chess.js */
                  this.displayGreen (this.boardInterface.chess.fen ());
                }
                else
                {
                  /* In case no move made by server, Abrupt end of play() */
                  resolve ("error:abort:NoServerMove");
                }
              } catch(error)
              {
                console.log ("An error occured" + error.message);
              }
            }

            /*
            .. Asynchronous recursion of somebodyMakeAMove
            .. check if an Abort is activated because the other task (listenForAbort())
            .. is completed. Otherwwise continue playing
            */
            if (!signal.aborted)
            {
              /* if not make a move */
              setTimeout (somebodyMakeAMove, 0);  
            }

          }; /* End of "somebodyMakeAMove () {}" */

      /*
      .. Start Making the asynchronous recursive call of somebodyMakeAMove ().
      .. As you can see, it will run until "signal.aborted" is set by the
      .. parallel racing thread "this.listenForRestart ()".
      */
      setTimeout (somebodyMakeAMove, 0);
 
      /* add an event listener to abort this.play() */
      signal.addEventListener ("abort", () =>
        {
          reject ("stopped playing.")
        });
    
    }); /* end of Promise ((resolve, reject) => {}); */
  } /* end of this.play () */

  async playTillAbortOrOver (fen)
  {
    const controller = new AbortController ();
    const signal = controller.signal;
    try
    {
      const result =
        await Promise.race ([ this.play(fen, signal), this.listenForRestart(signal) ]);

      console.log ("status = ", result);
      if (!signal.aborted)
        controller.abort (); // Stop the other functioin
      return result;
    } catch (error)
    {
      console.log ("error:", error);
      if (!signal.aborted)
        controller.abort (); // Stop the other functioin
      return "error";
    }
  }

} /* End of the class "Client" */

    
/* Create a socket */
const socket = new Client();

/*
.. start a game with server. keep on playing till connection fails.
.. fixme : need to refresh in case server is disconnected and you want to 
.. reconnect. (bug)
*/
let result = "restart";
while (1)
{
  if (result === "restart")
  {
    let fen = socket.restartFen === '' ? null : socket.restartFen;
    socket.displayGreen (fen);
    result = await socket.playTillAbortOrOver (fen);
  }
  else if(result === "over")
  {
    const controller = new AbortController();
    const signal = controller.signal;
    socket.inputfen.disabled = false;
    socket.submitfen.disabled = false;
    /* if the game is over you can wait for user to restart */
    result = await socket.listenForRestart (signal);
  }
  else
  {
    const controller = new AbortController ();
    const signal = controller.signal;
    console.log ("Error in play(). Restart");
    socket.restartFen         = '';
    socket.inputfen.disabled  = false;
    socket.submitfen.disabled = false;
    result = await socket.listenForRestart(signal);
  }
  console.log("Game Over/Restarted/Thrown an error?: " + result);
}
