#ifndef _CHESS_BOARD_H_
#define _CHESS_BOARD_H_

  #include "common.h"

  #define a8           0
  #define b8           1
  #define c8           2
  #define d8           3
  #define e8           4  /* WK (starting position) */
  #define f8           5
  #define g8           6
  #define h8           7
  #define a1          56
  #define b1          57
  #define c1          58
  #define d1          59
  #define e1          60 /* BK (starting position) */
  #define f1          61
  #define g1          62
  #define h1          63

  const uint8_t BOARD [12][12] =
  {
    {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
    {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
    {64, 64, a8, b8, c8, d8, e8, f8, g8, h8, 64, 64},
    {64, 64,  8,  9, 10, 11, 12, 13, 14, 15, 64, 64},
    {64, 64, 16, 17, 18, 19, 20, 21, 22, 23, 64, 64},
    {64, 64, 24, 25, 26, 27, 28, 29, 30, 31, 64, 64},
    {64, 64, 32, 33, 34, 35, 36, 37, 38, 39, 64, 64},
    {64, 64, 40, 41, 42, 43, 44, 45, 46, 47, 64, 64},
    {64, 64, 48, 49, 50, 51, 52, 53, 54, 55, 64, 64},
    {64, 64, a1, b1, c1, d1, e1, f1, g1, h1, 64, 64},
    {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
    {64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64}
  };

  #define START       2
  #define END         9
  #define BOARDSQ(s)  & (BOARD [s/8 + START][ (s%8) + START])

  /* represent a move */
  typedef struct
  {
    struct
    {
      uint8_t square;
      uint8_t piece;
    } from, to;
    uint8_t flags;
    uint8_t promotion;
  } _Move;

  /* board config (except halfclock, enpassante/castling information) */
  uint8_t  PIECES [64];
  uint8_t  kings [2];
  uint8_t  color;
  uint8_t  npieces;
  uint16_t fullclock;
  
  /* additional informaion required to uniquley represent the board */
  typedef struct
  {
    uint8_t enpassante;
    uint8_t castling;
    uint8_t status;
    uint8_t totalMoves; /* including illegal moves */
    uint16_t halfclock;
    uint16_t moveLoc;
    uint64_t zobrist;
  } _Board;

  #ifndef MAX_STACK_SIZE
  #define MAX_STACK_SIZE 64
  #endif

  _Board BoardStack [ MAX_STACK_SIZE ];
  const _Board * BoardLast = & BoardStack [MAX_STACK_SIZE - 1];
  Array movesall = {.p = NULL, .max = 0, .len = 0};
  #define MOVES_AT(b) ( & ((_Move * ) movesall.p) [b->moveLoc] )

  #define NOT_UNUSED(x) (void)(x)

  #define COLOR        1
  #define BLACK        0
  #define WHITE        1
  #define BROOK      ((0<<1) | BLACK)
  #define WROOK      ((0<<1) | WHITE)
  #define BKNIGHT    ((1<<1) | BLACK)
  #define WKNIGHT    ((1<<1) | WHITE)
  #define BBISHOP    ((2<<1) | BLACK)
  #define WBISHOP    ((2<<1) | WHITE)
  #define BQUEEN     ((3<<1) | BLACK)
  #define WQUEEN     ((3<<1) | WHITE)
  #define BPAWN      ((4<<1) | BLACK)
  #define WPAWN      ((4<<1) | WHITE)
  #define BKING      ((5<<1) | BLACK)
  #define WKING      ((5<<1) | WHITE)
  #define EMPTY        12
  #define INVALID      64

  #define FEN_MAXSIZE 80

  /*
  .. For faster translation b/w chesspiece ID ( [0:11] ) and their ASCII
  .. repesentations ({r, R, n, N, b, B, q, Q, p, P, k, K, .} where
  .. '.' represents empty square 
  */

  const char ASCII [13] =
    { 
      'r', 'R', 'n', 'N', 'b', 'B',
      'q', 'Q', 'p', 'P', 'k', 'K',
      '.'
    };
  const uint8_t CHESSPIECE[50] =
    {
      'A',      WBISHOP,   'C',     'D',   'E',   'F',       'G',   'H',
      'I',      'J',       WKING,   'L',   'M',   WKNIGHT,   'O',   WPAWN,
      WQUEEN,   WROOK,     'S',     'T',   'U',   'V',       'W',   'X',
      'Y',      'Z',       ' ',     ' ',   ' ',   ' ',       ' ',   ' ',
      'a',      BBISHOP,   'c',     'd',   'e',   'f',       'g',   'h',
      'i',      'j',       BKING,   'l',   'm',   BKNIGHT,   'o',   BPAWN,
      BQUEEN,   BROOK,
    };
  #define BoardSquareParse(sq)   (8 * (8 - sq[1] + '0' ) + sq[0] - 'a')
  #define PieceParse(p)          CHESSPIECE [p - 'A']

  /*
  .. Identifying chess piece & it's info from square pointer 'S'
  */
  #define OUTSIDE         64
  #define IS_OUTSIDE(S)   ( (*S) == OUTSIDE )
  #define PIECE(S)        ( PIECES [*S] )
  #define IS_EMPTY(S)     ( PIECE(S) == EMPTY )
  #define IS_PIECE(S)     ( !IS_OUTSIDE (S) && !IS_EMPTY (S) )
  #define PIECE_COLOR(S)  ( PIECE (S) & COLOR)
  #define SQUARE_FILE(S)  ('a' + (*S)%8)
  #define SQUARE_RANK(S)  ('0' + 8 - (*S)/8)
  #define PIECE_ASCII(S)  ( ASCII [PIECE (S)] )

  /*
  .. Identifying the type of move
  */
  #define CASTLING          15
  #define MOVE_NORMAL        0
  #define CASTLING_BQ        1  /* warning : Don't change this castling */
  #define CASTLING_BK        2  /* .. order. This is the inverse of the */
  #define CASTLING_WQ        4  /* .. order stipulated for castling in  */
  #define CASTLING_WK        8  /* .. the standard FEN (i.e K, Q, k, q) */
  #define MOVE_CAPTURE      16
  #define MOVE_PROMOTION    32
  #define MOVE_ENP_CAPTURE  64
  #define MOVE_ILLEGAL     128
  /* #define MOVE_CHECK       128 */

  /* convert FEN string to board */
  _Board * BoardSetFromFEN (const char * fen)
  {
    _Board *  b = BoardStack;
    b->totalMoves = 0;  /* Note : not yet evaluated. */
    b->moveLoc = 0;
    movesall.len = 0;
  
    npieces = 0;
    kings [WHITE] = kings [BLACK] = OUTSIDE;

    /* Set board from FEN */
    uint8_t * piece = PIECES, square = 0;
    char c;
 
    while ( (c = *fen++) != '\0' )
    {

      if ( !(square <= OUTSIDE) )
        return NULL;

      /* Empty squares */
      if (isdigit (c))
      {
        int nempty = c - '0';
        /* cannot overfille a row */
        if ( nempty == 0 || nempty + (square % 8) > 8 )
          return NULL;
        for (int i=0; i<nempty; ++i)
          *piece++ = EMPTY;
        square += nempty;
        continue;
      }

      /* row breaker */
      if (c == '/')
      {
        /* Make sure all squares of this rank are filled */
        if ( !(square%8 == 0 ) )
          return NULL;
        continue;
      }

      /* end of board */
      if (c == ' ')
      {
        /* Make sure all squares are filled */
        if ( square != OUTSIDE )
          return NULL;
        break;
      }

      /* expects a chesspiece ascii */
      if (!(c == 'p' || c == 'P' ||  c == 'b' || c == 'B' ||
          c == 'n' || c == 'N' ||  c == 'r' || c == 'R' ||
          c == 'q' || c == 'Q' ||  c == 'k' || c == 'K') )
      {
        return NULL;
      }

      /* Identify chesspiece ID from (valid) ASCII */

      if (c == 'K' || c == 'k')
      {
        uint8_t color = (c == 'K') ? WHITE : BLACK;

        /* There cannot be multiple kings of same color */
        if ( kings [color] != OUTSIDE )
          return NULL;

        kings [color] = square;
      }
      else
      {
        /* There are a max of 30 chesspieces excluding the two kings*/
        if ( !((npieces)++ < 30) )
          return NULL;
      }

      *piece++ =  PieceParse (c);
      ++square;
    }

    /* Make sure that there are exacly one each of 'k' and 'K' in the FEN; */
    if (kings [WHITE] == OUTSIDE || kings [BLACK] == OUTSIDE)
      return NULL;

    /* Let's see whose turn is now ('w'/'b') */
    c = *fen++;
    if ( c != 'w' && c != 'b' )
      return NULL;
    color = *fen == 'w' ? WHITE : BLACK;
    if ( (c = *fen++) != ' ' )
      return NULL;

    /* read castling information */
    b->castling = 0;
    if ( (c = *fen++) == '-' )
      c = *fen++;
    else
    {
      uint8_t which;
      do
      {
        switch (c) {
          case 'K' :
            if ( PIECES [e1] != WKING || PIECES [h1] != WROOK )
              return NULL;
            which = CASTLING_WK;
            break;
          case 'Q' :
            if ( PIECES [e1] != WKING || PIECES [a1] != WROOK )
              return NULL;
            which = CASTLING_WQ;
            break;
          case 'k' :
            if ( PIECES [e8] != BKING || PIECES [h8] != BROOK )
              return NULL;
            which = CASTLING_BK;
            break;
          case 'q' :
            if ( PIECES [e8] != BKING || PIECES [a8] != BROOK )
              return NULL;
            which = CASTLING_BQ;
            break;
          default :
            /* expects only K, Q, k or q */
            return NULL;
        }
        if ( (2*which-1) & b->castling )
        {
          /*
          .. bit ordering of castling is done such a way that, any violation of
          .. the stipulated castling order(KQKq) or any multiplicity of K/Q/k/q
          .. will be caught here
          */
          return NULL;
        }
        b->castling |= which;
      } while ((c = *fen++) != '\0' && c != ' ');
    }
    if (c != ' ')
      return NULL;

    /* read if any square is enpassante */
    b->enpassante = OUTSIDE;
    if ( (c = *fen++) != '-')
    {
      char d = *fen++;
      if (c > 'h' || c < 'a' || d > '8' || d < '1')
        return NULL;
      b->enpassante = (8 * (8 - d + '0' ) + c - 'a');
    }
    if (*fen++ != ' ')
      return NULL;

    b->halfclock = 0;
    c = *fen++;
    do {
      if ( !(isdigit (c)) )
        return NULL;
      b->halfclock = 10*b->halfclock + (uint16_t) (c - '0');
      /* FIDE limits automatic draw @ 150 */
      if ( b->halfclock > /*150*/ 20000 )
        return NULL;
    } while ( (c = *fen++) != '\0' && c != ' ');
    if (c != ' ')
      return NULL;

    /* full fullclock information */
    fullclock = 0;
    c = *fen++;
    do {
      if ( !(isdigit (c)) )
        return NULL;
      fullclock = 10* fullclock + (uint16_t) (c - '0');
      /*
      .. No recorded FIDE game exceeded 300 moves. I don't know the theoretical
      .. limit. I think draw (by 50 moves rule) would have occured before 1000
      .. moves??
      */
      if ( fullclock > 10000 )
        return NULL;
    } while ( (c = *fen++) != '\0' && c != ' ');
    /* min {fullclocks} = 1 */
    if ( fullclock == 0 )
      return NULL;

    /* FEN is valid */
    return b;
  }

  /* create fen string for a board */
  void BoardFEN (_Board * b, char * fen)
  {
    uint8_t * piece = PIECES;
    unsigned char nempty;

    /* config of pieces */
    for (int i=0; i<8; ++i)
    {
      nempty = 0;
      for (int j=0; j<8; ++j)
      {
        if (*piece != EMPTY)
        {
          if (nempty)
          {
            *fen++ = '0' + nempty;
            nempty = 0;
          }
          *fen++ = ASCII [*piece];
        }
        else
          nempty++;
        piece++;
      }
      if (nempty)
        *fen++ = '0' + nempty;
      *fen++ = i<7 ? '/' : ' ';
    }

    /* whose turn */
    *fen++ = color ? 'w' : 'b';
    *fen++ = ' ';

    /* castling information */
    if (!b->castling)
      *fen++ = '-';
    else
    {
      if (b->castling & CASTLING_WK)
        *fen++ = 'K';
      if (b->castling & CASTLING_WQ)
        *fen++ = 'Q';
      if (b->castling & CASTLING_BK)
        *fen++ = 'k';
      if (b->castling & CASTLING_BQ)
        *fen++ = 'q';
    }
    *fen++ = ' ';

    /* enpassante */
    if (b->enpassante == OUTSIDE)
      *fen++ = '-';
    else
    {
      *fen++ = 'a' + (b->enpassante % 8);
      *fen++ = '0' + 8 - b->enpassante/8;
    }
    *fen++ = ' ';

    /* half & full fullclock */
    uint16_t gameclock[2] =
      {b->halfclock, fullclock};

    for (int i=0; i<2; ++i)
    {
      uint16_t n = gameclock[i], pos = 4;

      /* otherwise: weird fullclocknumbers */
      assert (n <= (i ? 5000 : 50));
      unsigned char h[5];
      h[pos] = i ? '\0' : ' ';
      while (pos)
      {
        h[--pos] = '0' + n%10;
        n /= 10;
        if (!n) break;
      }
      for (int j=pos; j<5; ++j)
        *fen++ = h[j];
    }
  }

  /* print ASCII Board */
  void BoardPrint (_Board * board)
  {
    uint8_t * piece = PIECES;
    fprintf (stdout, "\n        BOARD");
    for (int i=0; i<8; ++i)
    {
      fprintf (stdout, "\n %c ", '0'+8-i);
      for (int j=0; j<8; ++j)
        fprintf (stdout, " %c", ASCII[*piece++]);
    }

    fprintf (stdout, "\n\n   ");
    for (int j=0; j<8; ++j)
      fprintf (stdout, " %c", 'a'+j);
    fprintf (stdout, "\n");
  }

  /* conditions to check while moving piece from 'FROM' to 'TO' */

  #define IS_NORMAL(FROM,TO)         ( IS_EMPTY (TO) )
  #define IS_BLOCKED(FROM,TO)        ( IS_PIECE (TO) &&                       \
                                     (PIECE_COLOR (FROM) == PIECE_COLOR (TO)) )
  #define IS_CAPTURE(FROM,TO)        ( IS_PIECE (TO) &&                       \
                                     (PIECE_COLOR (FROM) != PIECE_COLOR (TO)) )
  /* use (both) carefully, assumes moving piece is WPAWN/BPAWN */
  #define IS_PROMOTION(TO) (SQUARE_RANK (TO) == '8' || SQUARE_RANK (TO) == '1')
  #define IS_ENP_CAPTURE(TO,BOARD)   ((*TO) == (BOARD)->enpassante)


  char * BoardMoveSAN (_Move * m)
  {
    /* fixme: Not et impelemented */
    assert (0);
    NOT_UNUSED (m);
    return "\0";
  }

  enum GAME_STATUS
  {
    /* Board status */
    GAME_CONTINUE          = 0,
    GAME_IS_A_WIN          = 16,
    GAME_IS_A_DRAW         = 32,

    /* Encode unknown error*/
    GAME_STATUS_ERROR      = 128,

    /* Info on WIN */
    GAME_WHO_WINS          = 1,
    GAME_IS_WON_BY_TIME    = 2,
    GAME_IS_WON_BY_FORFEIT = 4,

    /* Info on draw = (STATS & GAME_DRAW_INFO)*/
    GAME_DRAW_INFO         = 15,
    GAME_STALEMATE         = 0,
    GAME_INSUFFICIENT      = 1,
    GAME_FIFTY_MOVES       = 2,
    GAME_THREE_FOLD        = 3,
    GAME_WHITE_CANNOT      = 4,
    GAME_BLACK_CANNOT      = 5,
    GAME_AGREES            = 6
  };

  void BoardMove (_Board * b, _Move * move)
  {
    assert (b != BoardLast);
    
    uint8_t from = move->from.square,
      to = move->to.square;
    uint8_t piece = PIECES [to] = (move->flags & MOVE_PROMOTION) ?
      move->promotion : move->from.piece;
    assert (PIECES [from] == move->from.piece);
    PIECES [from] = EMPTY;

    #if 0
    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces --; 
    color = !color;
    fullclock++;
    #endif

    b[1].enpassante = 
      (piece == WPAWN && from - to == 16) ? from - 8 :
      (piece == BPAWN && to - from == 16) ? from + 8 : OUTSIDE;
    b[1].castling = b[0].castling & ~move->flags;
    b[1].halfclock = 
      ((move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE) ) || piece == WPAWN ||
        piece == BPAWN ) ? 0 : b[0].halfclock + 1;
    b[1].moveLoc = b[0].moveLoc + b[0].totalMoves;

    /*
    .. Switching off (respective) castling ability when rook moves/atacked.
    .. An edge case, that you can miss
    */
    if (b[1].castling)
    {
      /* Switching off castling if corner rooks move/captured */
      if (from == a8 || to == a8)
        b[1].castling &= ~CASTLING_BQ;
      if (from == h8 || to == h8)
        b[1].castling &= ~CASTLING_BK;
      if (from == a1 || to == a1)
        b[1].castling &= ~CASTLING_WQ;
      if (from == h1 || to == h1)
        b[1].castling &= ~CASTLING_WK;
    }

    if (piece == WKING)
    {
      kings [WHITE] = to;
      b[1].castling &= ~(CASTLING_WQ | CASTLING_WK);
      if (move->flags & CASTLING_WQ)
      {
        /* rank 1 :  "..KR.xxx"  (x : unknown)*/
        PIECES [a1] = EMPTY;
        PIECES [d1] = WROOK;
        return;
      }
      if (move->flags & CASTLING_WK)
      {
        /* rank 1 :  "xxxx.RK."  (x : unknown)*/
        PIECES [h1] = EMPTY;
        PIECES [f1] = WROOK;
        return;
      }
      return;
    }

    if (piece == BKING)
    {
      kings [BLACK] = to;
      b[1].castling &= ~(CASTLING_BQ | CASTLING_BK);
      if (move->flags & CASTLING_BQ)
      {
        /* rank 8 :  "..kr.xxx"  (x : unknown)*/
        PIECES [a8] = EMPTY;
        PIECES [d8] = BROOK;
        return;
      }
      if (move->flags & CASTLING_BK)
      {
        /* rank 8 :  "xxxx.rk."  (x : unknown)*/
        PIECES [h8] = EMPTY;
        PIECES [f8] = BROOK;
        return;
      }
      return;
    }

    if (move->flags & MOVE_ENP_CAPTURE)
    {
      PIECES [to + (piece == WPAWN ? 8 : -8)] = EMPTY;
    }
  }

  void BoardUnmove (_Board * b, _Move * move)
  {

    uint8_t from = move->from.square, to = move->to.square;

    uint8_t piece = PIECES [from] = move->from.piece;
    PIECES [to]   = move->to.piece;

    #if 0
    if (move->flags & ( MOVE_CAPTURE | MOVE_ENP_CAPTURE ))
      npieces ++; 
    color = !color;
    fullclock--;
    #endif

    if (piece == WKING)
    {
      kings [WHITE] = from;
      if (move->flags & CASTLING_WQ)
      {
        /* rank 1 :  "R...Kxxx"  (x : unknown)*/
        PIECES [a1] = WROOK;
        PIECES [d1] = EMPTY;
        return;
      }
      if (move->flags & CASTLING_WK)
      {
        /* rank 1 :  "xxxxK..R"  (x : unknown)*/
        PIECES [h1] = WROOK;
        PIECES [f1] = EMPTY;
        return;
      }
      return;
    }

    if (piece == BKING)
    {
      kings [BLACK] = from;
      if (move->flags & CASTLING_BQ)
      {
        /* rank 8 :  "r...kxxx"  (x : unknown)*/
        PIECES [a8] = BROOK;
        PIECES [d8] = EMPTY;
        return;
      }
      if (move->flags & CASTLING_BK)
      {
        /* rank 8 :  "xxxxk..r"  (x : unknown)*/
        PIECES [h8] = BROOK;
        PIECES [f8] = EMPTY;
        return;
      }
      return;
    }

    if (move->flags & MOVE_ENP_CAPTURE)
    {
      PIECES [to + (piece == WPAWN ? -8 : 8)] =
        piece == WPAWN ? BPAWN : WPAWN;
    }
  }
#endif
