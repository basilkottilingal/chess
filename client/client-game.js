/* ToDo. 1) Get board (rather) from chess.js
2) player can be any color. 
   BUt toggle between 'w' and 'b' while 'play again'
3) check fen v/s board validity.
4) Replay, undo, play again button
*/
import {Chess} from "./chess.js";
import {Client} from "./websocket.js";

export class ChessGame {

  constructor() {
    /* default board. Can be reloaded using this.load(fen) */
    this.chess = new Chess();
    /* Open a socket for communicating with server */
    this.socket = new Client();
    /* Enable all the buttons input field, etc */
    this.socket.eventListen();
    /* Ascii Board */    
    this.board = Array.from({ length: 8 }, () => Array(8).fill('.'));
  }

  msgMove(move) {
    let msg = 'm' + move;
    this.socket.socket.send(msg);
    console.log('Move: msg ' + msg);
  }

  piece(chesspiece){
    const img = document.createElement('img');
    img.id = 'rnbqkp'.includes(chesspiece) ? 'b' : 'w';
    img.src = "imgs/" + img.id + "/" + chesspiece + ".png"; 
    img.alt = chesspiece;
    img.draggable = "true";
    return img;
  }

  //can be used to load/reload chess board and the images . 
  load(fen) {
    //default only. now
    this.chess.load(fen ? fen : 
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    let b = this.chess.board();
    //console.log(b);
    //empty 8x8 char array
    for (let i = 0; i < 8; i++) {
      for (let j = 0; j < 8; j++) {
        if(b[i][j] != null) {
          let piece = b[i][j].type;
          this.board[i][j] = 
  b[i][j].color === 'b' ? piece.toLowerCase() :  piece.toUpperCase();
        }
        else
          this.board[i][j] = '.';
      }
    }
    console.log(this.board);
  
    // JavaScript to create an 8x8 grid using a for loop
    const gridContainer = 
      document.getElementById('grid-container');

    //remove all 'div' (In case of reloading);
    while (gridContainer.firstChild) {
        gridContainer.removeChild(gridContainer.firstChild);
    }

    // Loop through each chess sqquare
    for (let i = 0; i < 8; i++) {
      for (let j = 0; j < 8; j++) {
    
        const square = document.createElement('div');

        square.classList.add('grid-item');
        square.id = 
  String.fromCharCode('a'.charCodeAt(0) + j) + (8 - i);
    
         //Black Squares
        if( (i%2)^(j%2) ){
  square.style.backgroundColor = "#bbbbbb";
        }
  
        const piece = this.board[i][j];
        if ( piece != '.') {
  /* Image Element of Chesspieces on board */
  const img = this.piece(piece);
  square.appendChild(img);
        }

        // Append the square to the grid container
        gridContainer.appendChild(square);
      } 
    }
  } //End of function load()

  /* Trigger when you start dragging a piece */
  pieceDrag(e, img){
    /* Only chess pieces with valid moves can be dragged */
    this.moves = this.chess.moves({ 
      square: img.parentElement.id, 
      verbose: true});
    if(this.moves.length > 0){
      this.piece = e.target;
      setTimeout(() => {
        e.target.style.display = 'none';
      }, 0);
  
      /* Highlight all the possible target sqrs of this piece*/
      this.moves.forEach( move => {
        const div = document.getElementById(move.to);
        if(div){
  div.classList.add('move-highlight');
        } else {
  console.log('Error: div \'' + move.id + 
    '\' doesn\'t exists');
        }
      });
    }
  }

  /* Trigger when you stop dragging a piece */
  pieceDragEnd(e, img){
    setTimeout(async () => {
      e.target.style.display = 'block';
        if ( this.piece ) {
  this.piece = null;
  this.moves.forEach( move => {
    /*remove highlight of the valid target squares*/
    const div = document.getElementById(move.to);
    if(div){
      div.classList.remove('move-highlight');
    } else {
      console.log('Error: div \'' + move.id + 
        '\' doesn\'t exists');
    }
  });
  this.moves = null;
  if(this.move) {
    //Move the board in chess.js
    await this.playerMove(false);
    const flags = this.move.flags;
    this.move = null;
  }
        }
    }, 0);
  }

  pieceFree(img) {
    // remove eventListener of img element (chesspiece)
    img.removeEventListener('drag', this.pieceDrag);
    img.removeEventListener('dragend', this.pieceDragEnd);
  }
  squareFree(sq) {
    // remove eventListener of div element 
    sq.removeEventListener('dragstart', this.pieceDrag);
    sq.removeEventListener('dragover', this.pieceDragEnd);
    sq.removeEventListener('drop', this.pieceDragEnd);
  }

  /* Trigger when dragging a piece over this square ('div') */
  squareDragOver(e, square){
    if (this.piece) {
      if (square.classList.contains('move-highlight')) {
        /* This makes sure that only the cells with      ..
        .. 'move-highlight' (i.e possible target squares ..
        .. of the image/piece being dragged) can take part ..
        .. in 'dragover' (and thus 'drop'). You cannot drop a..
        .. piece in squares which is not 'move-highlight'-ed */
        e.preventDefault();
        square.classList.add('highlight');
      }
    }
  }

  /* Trigger when leave dragging over this square ('div') */
  squareDragLeave(square){
    if (this.piece) {
      square.classList.remove('highlight');
    }
  }

  /* Trigger when you drop a piece ('img') on this ..
  .. square ('div') */
  squareDrop(e, square){
    if ( this.piece ) { 
      e.preventDefault();
      square.classList.remove('highlight');

      if (square.querySelector('img')) {
        /* Capture */
        let capturedPiece = square.querySelector('img');
        square.replaceChild(this.piece, capturedPiece);
        // Memory management
        this.pieceFree(capturedPiece);
        capturedPiece = null;
      } else {
        /* Normal Chess Move / "Quiet Move" */ 
        square.appendChild( this.piece );
      }

      // Let chess.js know which move among ..
      // moves is chosen by mouse drag/drop. 
      // chess.move() will be called in 'dragend'
      let found = false;
      this.moves.forEach( move => {
        if (move.to == square.id) {
  found = true;
  this.move = move;
        }
      });
      if( found === false ){
        console.log('Error: Cannot find the move' +
  ' triggered by mouse drag');
      }
    }
  }

  /* list of eventListen related to drag and drop ..
  .. The order in which these happen are 
  ..  dragstart → dragleave (if applicable) → drop → dragend.
  .. dragstart and dragend are associated with 'img' ( like ..
  .. a chesspiece) while dragover, dragleave, drop are ..
  .. associated 'div' (like a square on the chessboard).  
  */
  gameEventListen(colors) { 
    const squares = document.querySelectorAll('.grid-item');
    squares.forEach(square => {
      const piece = square.querySelector('img');
      if (piece) {
        if(colors.includes(piece.id[0])) {
  /* Only pieces of player's color are draggable */      
  piece.addEventListener('dragstart', 
    (e) => {this.pieceDrag(e, piece)});
  
  piece.addEventListener('dragend',
    (e) => {this.pieceDragEnd(e, piece)});
        }
      } 

      square.addEventListener('dragover', 
        (e) => {this.squareDragOver(e, square)});

      square.addEventListener('dragleave',
        (e) => {this.squareDragLeave(square)});

      square.addEventListener('drop',
        (e) => {this.squareDrop(e, square)});
    });
  }

  // Function to show the promotion options
  pawnPromotion(option){
    return new Promise( (resolve) => {
      const p = option.getAttribute('data-piece'); 
      const promotion = 
       this.move.color === 'w' ? p.toUpperCase() : p;

      console.log('promotion '+promotion);
      this.move.promotion = p;
    
      const square = document.getElementById(this.move.to);
      const piece = square.querySelector('img');
      if(piece){
        piece.alt = promotion;
        piece.id = 
          'rnbqkp'.includes(promotion) ? 'b' : 'w';
        piece.src = 'imgs/' + piece.id + '/' + promotion + '.png';
      }

      const promotionOverlay = 
        document.getElementById('promotion-overlay');
      const boardOverlay = 
        document.getElementById('board-overlay');
      promotionOverlay.style.display = 'none';
      boardOverlay.style.display = 'none';

      resolve(); 
    });
  }

  /* Promote a pawn. 'p' -> 'q'/'b'/'r'/'n'*/
  async finishPromotion() {
    return new Promise ( (resolve) => {
      const promotionOverlay = 
        document.getElementById('promotion-overlay');
      const boardOverlay = 
        document.getElementById('board-overlay');
      promotionOverlay.style.display = 'block';
      boardOverlay.style.display = 'block';

      // Handle click on a promotion option
      const options = 
        document.querySelectorAll('.promotion-option');

      options.forEach( (option) => {
        /* Invert .png (black<->white) for black's promotion */
        if(this.move.color === 'w')
          option.classList.add('promotion-invert');
        else 
          option.classList.remove('promotion-invert');
        const handleClick = async () => {
          await this.pawnPromotion(option);
            
          /* Disable all the eventListen of type 'click' */
          options.forEach( (opt) => {
            opt.removeEventListener('click', handleClick);
          }); 
  
          resolve(); // Once a promotion piece is clicked, this function is resolved
        };
        // Make each option listening for a 'click' 
        option.addEventListener('click', handleClick); 
      }); //end of each option
    }); //end of Promise
  }

  finishCastling(){
    /* King is already moved on the board. ..
    .. Now move the rook on the board*/
    let rank = this.move.color === 'w' ? '1' : '8';
    let file = this.move.flags === 'q' ? 'a' : 'h';
    let square = document.getElementById(file+rank);
    const rook = square.querySelector('img');
    //Remove the rook from the corner square
    square.removeChild(rook);

    file = this.move.flags === 'q' ? 'd' : 'f';
    square = document.getElementById(file+rank);
    //New rook location
    square.appendChild(rook);
  }

  finishEnPassante(){
    /* Remove opponent pawn from the board ..
    .. if the move is an en-passante one*/
    const square = 
      document.getElementById(this.move.to[0]+this.move.from[1]);

    const capturedPawn = square.querySelector('img');
    //fixme: didn't remove eventListen related to image.
    square.removeChild(capturedPawn);
  }

  playerMove(bot){
    return new Promise(async (resolve) => {
     /* Update player's move (mouse drag) into ..
     .. the chess.js database and take care of special cases */
      let promotion = '';
      if (!bot) {
        if (this.move.flags.includes('p')) {
          /* In case of promotion, it has to wait till a promotion piece is selected */
          await this.finishPromotion(); // updates this.move.promotion
          promotion = this.move.promotion;
        }
        else if(this.move.flags === 'q' || this.move.flags === 'k') {
          /* See if it's a castling */
          console.log('Castling ' + this.move.flags);
          this.finishCastling();
        }  
        else if (this.move.flags === 'e') {
          /* See if it's an En-passante advance */
          console.log('en-passant');
          this.finishEnPassante();
        }
      } 
        
      // Reflect the move the chess.js
      this.chess.move (this.move);
      //Fixme. just return the move.
      //send the move to the server
      this.socket.encodeSend('m', this.move.from + this.move.to + promotion);
      //Fixme: It has to be explicitly called
      //wait(EventListen/wait for server) for the next move
      this.next();
      resolve();
    }); // end of Promise
  }
  

  botMove(move){

    /* Reflect the bot's move on the chessboard */
    /* Random move for the moment */
    const squareFrom = document.getElementById(move.from);
    const squareTo = document.getElementById(move.to);
    const piece = squareFrom.querySelector('img');
    if (squareTo.querySelector('img')) {
      /* Capture */
      let capturedPiece = squareTo.querySelector('img');
      squareTo.replaceChild( piece, capturedPiece );
      // Memory management
      this.pieceFree(capturedPiece);
      capturedPiece = null;
    } else {
      /* Normal Chess Move / "Quiet Move" */ 
      squareTo.appendChild( piece );
    }
    //squareFrom.removeChild( piece );
    if(move.flags.includes('p')){
      const promotion = move.color === 'w' ? 
  move.p.toUpperCase() : move.p;
      piece.id = 
        'rnbqkp'.includes(promotion) ? 'b' : 'w';
      piece.src = piece.id + '/' + promotion + '.png';
      piece.alt = promotion;
    } 
    /* Take care of special moves */
    this.move = move;
    this.playerMove(true);
    this.move = null;
  }

  randomMove(){
    const moves = this.chess.moves();  
    const imove = Math.floor(Math.random() * moves.length); 
    const _move = moves[imove];
    const move = this.chess.move(_move);
    this.botMove(move); 
  }

  game(fen){
    this.load(fen);
    /* Once you let the mouse listen to drag */
    this.gameEventListen('wb');
  }

  next(){
    this.socket.errorLog.textContent = this.chess.fen();
    this.socket.errorLog.style.color = "green";
    //console.log(this.chess.fen());
    if (this.chess.moves().length === 0) {
      console.log("Game over!");
      return;
    }
    /*
    this.randomMove();
    this.socket.errorLog.textContent = this.chess.fen();
    this.socket.errorLog.style.color = "green";
    //console.log(this.chess.fen());
    if (this.chess.moves().length === 0) {
      console.log("Game over!");
      return;
    }
    */
  }
  
} //End of Class ChessGame

const game = new ChessGame();
game.game(null);
