import {Chess} from "./chess.js";

export class ChessBoard {

  constructor() {
    /* default board. Can be reloaded using this.load(fen) */
    this.chess = new Chess();
  }

  pieceImage(chesspiece){
    const img = document.createElement('img');
    img.id = 'rnbqkp'.includes(chesspiece) ? 'b' : 'w';
    img.src = "imgs/" + img.id + "/" + chesspiece + ".png"; 
    img.classList.add('piece');
    img.alt = chesspiece;
    img.draggable = "true";
    return img;
  }

  //can be used to load/reload chess board and the images . 
  load(fen) {
    //default only. now
    this.chess.load(fen ? fen : 
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    let board = this.chess.board();
  
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
        if( (i%2)^(j%2) )
          square.style.backgroundColor = "#bbbbbb";
  
        if ( board[i][j] ) {
           /* Image Element of Chesspieces on board */
          let type =  board[i][j].type; 
          let piece = 
            board[i][j].color === 'b' ? type.toLowerCase() : type.toUpperCase();
          const img = this.pieceImage(piece);
          square.appendChild(img);
        }

        // Append the square to the grid container
        gridContainer.appendChild(square);
      } 
    }
   
    /* drag events related to chessboard squares */   
    const squares = document.querySelectorAll('.grid-item');
    squares.forEach(square => {
      square.addEventListener('dragover', this.squareDragOver.bind(this));
      square.addEventListener('dragleave',this.squareDragLeave.bind(this));
      square.addEventListener('drop',this.squareDrop.bind(this));
    });
  } //End of function load()

  /* Trigger when you start dragging a piece */
  pieceDragStart(e){
    let img = e.target;
    /* Only chess pieces with valid moves can be dragged */
    this.moves = this.chess.moves({ square: img.parentElement.id, verbose: true});

    if(this.moves.length > 0){
      this.piece = e.target;
      setTimeout(() => {
        e.target.style.display = 'none';
      }, 0);
  
      /* Highlight all the possible target sqrs of this piece*/
      this.moves.forEach( move => {
        const div = document.getElementById(move.to);
        if(div)
          div.classList.add('move-highlight');
      });
    }
  }

/* Trigger when you stop dragging a piece */
  pieceDragEnd(e){
    return new Promise( (resolve) => {
    let img = e.target;
      setTimeout(async () => {
        if(img)
          img.style.display = 'block';
        else 
          console.log("Weird");
        if ( this.piece ) {
          this.piece = null;
          this.moves.forEach( move => {
            //remove highlight of the valid target squares
            const div = document.getElementById(move.to);
            if(div)
              div.classList.remove('move-highlight');
          });
          this.moves = null;
          if(this.move) {
            /* A move detected by piece drag */
            if (this.move.flags.includes('p')) {
              /* In case of promotion, it has to wait till a 
              .. promotion piece is selected by the player*/
              this.move.promotion = await this.promotion(this.move); 
              /* Switch off promotion overlay display */
              document.getElementById('promotion-overlay').style.display = 'none';
              document.getElementById('board-overlay').style.display = 'none';
            }
            /* player Made a valid move.*/
            resolve(this.move); 
            this.move = null;
          }
          else {
            resolve(null);
          }
        }
        else {
          resolve(null);
        }
      }, 0);
    });
  }

  /* Trigger when dragging a piece over this square ('div') */
  squareDragOver(e){
    let square = 
      e.target.tagName === 'IMG' ? e.target.parentElement : e.target;
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
  squareDragLeave(e){
    let square = 
      e.target.tagName === 'IMG' ? e.target.parentElement : e.target;
    if (this.piece) {
      square.classList.remove('highlight');
    }
  }

  /* Trigger when you drop a piece ('img') on this ..
  .. square ('div') */
  squareDrop(e){
    let square = 
      e.target.tagName === 'IMG' ? e.target.parentElement : e.target;
    if ( this.piece ) { 
      e.preventDefault();
      square.classList.remove('highlight');

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
  eventListen(colors) {
    return new  Promise( (resolve) => {
      /* It is very important to use a map, that maps each image to ..
      .. to it's dragstart/dragend event handle functions. Otherwise ..
      .. removing these eventListers is messy*/
      const boundHandlers = new Map(); 
      const pieces = document.querySelectorAll('.piece');
      pieces.forEach(piece => {
        if(colors.includes(piece.id[0])) {
          const dragStartHandler = this.pieceDragStart.bind(this); 
          piece.addEventListener('dragstart', dragStartHandler);

          const dragEndHandler = async (e) => {
            let move = await this.pieceDragEnd(e);
            resolve(move); // move or null
            if(move) 
              resolve(move); 

            //delete all the eventListening of dragend/start
            //.. even if move is null
            pieces.forEach(img => {
              /* Remove all the drag related eventListeners attached to ..
              .. chess pieces */ 
              const handlers = boundHandlers.get(img);
              if(handlers) {
                img.removeEventListener('dragend', handlers.dragEndHandler);
                img.removeEventListener('dragstart', handlers.dragStartHandler);
              }
            })
          }
          piece.addEventListener('dragend', dragEndHandler);

          // Store the bound handlers in a map to reference them later
          boundHandlers.set(piece, { dragStartHandler, dragEndHandler });

        }
      }); 
    }); // End of Promise((resolve), ...)
  }

  /* Promote a pawn. 'p' -> 'q'/'b'/'r'/'n'*/
  async promotion(move) {
    return new Promise ( (resolve) => {
      //Appear the piece moved on board
      this.startMove(move);

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
        if(move.color === 'w')
          option.classList.add('promotion-invert');
        else 
          option.classList.remove('promotion-invert');
        const handleClick = async () => {
          //await this.pawnPromotion(option);
          const p = option.getAttribute('data-piece');

          resolve(p); 
          //console.log(p);  
          /* Disable all the eventListen of type 'click' */
          options.forEach( (opt) => {
            opt.removeEventListener('click', handleClick);
          });
  
          // Once a promotion piece is clicked, this function is resolved
        };
        // Make each option listening for a 'click' 
        option.addEventListener('click', handleClick); 
      }); //end of each option
    }); //end of Promise
  }

  startMove(move){
    /* move piece from 'from' to 'to' square*/
    let square = document.getElementById(move.to);
    let piece = 
      document.getElementById(move.from).querySelector('img');
    let capturedPiece  = square.querySelector('img');
    if (capturedPiece) {
      square.replaceChild(piece, capturedPiece);
      // Memory management
      capturedPiece = null;
    } 
    else {
      /* Normal Chess Move / "Quiet Move" */ 
      square.appendChild(piece);
    }
  }

  finishPromotion(move) {
    let piece = 
      document.getElementById(move.to).querySelector('img');
      /* In case of promotion.*/
    let promotion = move.color === 'w' ? 
        move.promotion.toUpperCase() : 
        move.promotion.toLowerCase();
    piece.alt = promotion;
    piece.id = move.color;
    piece.src = 'imgs/' + piece.id + '/' + promotion + '.png';
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
      document.getElementById(move.to[0] + move.from[1]);

    const capturedPawn = square.querySelector('img');
    //fixme: didn't remove eventListen related to image.
    square.removeChild(capturedPawn);
  }

  /*Use this function to reflect a move on the board */
  playerMove(move, isServer){
    return new Promise(async (resolve) => {
     /* Update player's move (mouse drag) into ..
     .. the chess.js database and take care of special cases */
      if (move) {
        let isProm = move.flags.includes('p');
        if(isServer?true:isProm?false:true)
        {
          //console.log('Piece Moved');
          this.startMove(move);
        }
      
        /* SPECIAL CASES */
        if(isProm) {
          /* See if it's a promotion */
          //console.log('Promotion ' + move.flags);
          this.finishPromotion(move);
        }
        if(move.flags === 'q' || move.flags === 'k') {
          /* See if it's a castling */
          console.log('Castling ' + move.flags);
          this.finishCastling(move);
        }  
        else if (move.flags === 'e') {
          /* See if it's an En-passante advance */
          console.log('En-passant');
          this.finishEnPassante(move);
        }
        resolve("success");
      } 
      else {
        resolve("fail");
      }
    }); // end of Promise
  }
  
  
} //End of Class ChessGame
