# chess
A simple chess engine.

[chess.js](https://github.com/jhlywa/chess.js) is employed for move generation and validation.
Working on backend cpp code for better move suggestion.

## Latest version of chess.js 
### Directly load chess.js from CDN. 
The link is here
https://cdnjs.cloudflare.com/ajax/libs/chess.js/1.0.0/chess.min.js

### Install latest version chess.js
Install chess.js (https://github.com/jhlywa/chess.js) using npm. chess.js is a chess siulator and NOT a chess engine.
  $npm install chess.js

Now you can load this header in javascript using require().
  const { Chess } = require('chess.js')

## Notation used by chess.js
Notation for board position: FEN (Forsyth-Edwards Notation)
Notation for a move: Algebraic Notation 

#Engine.
  (1) Board eval using NNUE.
      A sample [nnue](https://tests.stockfishchess.org/nns?network_name=04cf2b&user=vdv) the kind of ones used in      
      [Stockfish](https://github.com/official-stockfish/Stockfish).
      A cpp interface for nnu-probing downloaded  from [dshawul](https://github.com/dshawul/nnue-probe/)
  (2) 
