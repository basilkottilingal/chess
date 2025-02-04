/* ToDo. 1) Get board (rather) from chess.js
2) player can be any color. 
   BUt toggle between 'w' and 'b' while 'play again'
3) check fen v/s board validity.
4) Replay, undo, play again button
*/
import {Chess} from "./chess.js";


class ChessGame {

  constructor() {
    this.chess = new Chess();
    this.blackpieces = 'rnbqkp';
  
    //Random number
    let array = new Uint32Array(1);
    window.crypto.getRandomValues(array);
    let r = array[0]/4294967296;
    this.color = Math.floor ( r + 0.5 ) ? 'w' : 'b';
    console.log('Hi Player, your are ' + this.color);
  }

  piece(chesspiece){
    const img = document.createElement('img');
    img.id = this.blackpieces.includes(chesspiece) ? 'b' : 'w';
    img.src = "imgs/" + img.id + "/" + chesspiece + ".png"; 
    img.alt = chesspiece;
    img.draggable = "true";
    return img;
  }

  board(){
    this.board = [
      ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],
      ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],
      ['', '', '', '', '', '', '', ''],
      ['', '', '', '', '', '', '', ''],
      ['', '', '', '', '', '', '', ''],
      ['', '', '', '', '', '', '', ''],
      ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],
      ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'],
    ];
  
    // JavaScript to create an 8x8 grid using a for loop
    const gridContainer = 
      document.getElementById('grid-container');

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
        if ( piece != '') {
  /* Image Element of Chesspieces on board */
  const img = this.piece(piece);
  square.appendChild(img);
        }

        // Append the square to the grid container
        gridContainer.appendChild(square);
      } 
    }
  } //End of function board()

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
    setTimeout(() => {
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
    this.playerMove(this.move, false);
    const flags = this.move.flags;
    this.move = null;
    /* Finish th bot move if any */
    if(!flags.includes('p'))
      this.next();
  }
        }
    }, 0);
  }

  pieceFree(img) {
    // remove eventListener of img element (chesspiece)
    img.removeEventListener('drag', this.pieceDrag);
    img.removeEventListener('dragend', this.pieceDragEnd);
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
  'triggered by mouse drag');
      }
    }
  }
 
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
  pawnPromotion(option, move){
    const p = option.getAttribute('data-piece'); 
    const promotion = 
     move.color === 'w' ? p.toUpperCase() : p;

    console.log('promotion '+promotion);
    const _move = this.chess.move({
      from: move.from, 
      to: move.to,  
      promotion: p});
    
    if(!_move)
      console.log("Error: Couldn't promote");

    const square = document.getElementById(move.to);
    const piece = square.querySelector('img');
    if(piece){
      piece.alt = promotion;
      piece.id = 
        this.blackpieces.includes(promotion) ? 'b' : 'w';
      piece.src = 'imgs/' + piece.id + '/' + promotion + '.png';
    }

    const promotionOverlay = 
      document.getElementById('promotion-overlay');
    const boardOverlay = 
      document.getElementById('board-overlay');
    promotionOverlay.style.display = 'none';
    boardOverlay.style.display = 'none';

    /* Disable all the eventListen of type 'click' */
    const options = 
      document.querySelectorAll('.promotion-option');
    options.forEach( (_option) => {
      //Remove the 'click' eventListener 
      _option.replaceWith(_option.cloneNode(true));
    });

    this.next();
  }

  /* Promote a pawn. 'p' -> 'q'/'b'/'r'/'n'*/
  finishPromotion(move) {
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
      if(move.color === 'w'){
        option.classList.add('promotion-invert');
      } else {
        option.classList.remove('promotion-invert');
      }
      option.addEventListener('click', () => {
        this.pawnPromotion(option, move);
      });
    });
  }

  finishCastling(move){
    /* King is already moved on the board. ..
    .. Now move the rook on the board*/
    let rank = move.color === 'w' ? '1' : '8';
    let file = move.flags === 'q' ? 'a' : 'h';
    let square = document.getElementById(file+rank);
    const rook = square.querySelector('img');
    //Remove the rook from the corner square
    square.removeChild(rook);

    file = move.flags === 'q' ? 'd' : 'f';
    square = document.getElementById(file+rank);
    //New rook location
    square.appendChild(rook);
  }

  finishEnPassante(move){
    /* Remove opponent pawn from the board ..
    .. if the move is an en-passante one*/
    const square = 
      document.getElementById(move.to[0]+move.from[1]);

    const capturedPawn = square.querySelector('img');
    //fixme: didn't remove eventListen related to image.
    square.removeChild(capturedPawn);
  }

  playerMove(move, bot){
    /* Update player's move (mouse drag) into ..
    .. the chess.js database and take care of special cases */
    if (move.flags.includes('p')) {
      /* See if it's a promotion */
      if (!bot) {
        //Means:  The functioned is not called by botMove();
        const p = this.finishPromotion(move);
      }
    } else {
      if (!bot) {
  //Means:  The functioned is not called by botMove();
  this.chess.move({
    from: move.from, 
    to: move.to });
      }
      if(move.flags === 'q' || move.flags === 'k') {
        /* See if it's a castling */
        console.log('Castling ' + move.flags);
        this.finishCastling(move);
      }  else if (move.flags === 'e') {
        /* See if it's an En-passante advance */
        console.log('en-passant');
        this.finishEnPassante(move);
      }
    }
    /* Necessar toggle; when you are playing with bot */
    /* See if the Game Over */
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
        this.blackpieces.includes(promotion) ? 'b' : 'w';
      piece.src = piece.id + '/' + promotion + '.png';
      piece.alt = promotion;
    } 
    /* Take care of special moves */
    this.playerMove(move, true);
  }

  randomMove(){
    const moves = this.chess.moves();  
    const imove = Math.floor(Math.random() * moves.length); 
    const _move = moves[imove];
    const move = this.chess.move(_move);
    this.botMove(move); 
  }

  game(){
    this.board();
    /* Once you let the mouse listen to drag */
    this.gameEventListen('wb');
  }

  next(){
    console.log(this.chess.fen());
    if (this.chess.moves().length === 0) {
      console.log("Game over!");
      return;
    }
    this.randomMove();
    console.log(this.chess.fen());
    if (this.chess.moves().length === 0) {
      console.log("Game over!");
      return;
    }
  }
  
} //End of Class ChessGame

const game = new ChessGame();
game.game();

