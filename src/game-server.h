#include "game.h"
#include <ws.h>

/*
enum MSG_TYPE {
  MSG_IS_START =  "s", // send/recv
  MSG_IS_SUCCESS =  "S", // send/recv
  MSG_IS_FEN   =  "f", // send only
  MSG_IS_BOARD =  "b", // recv only
  MSG_IS_MOVE  =  "m", // send/recv
  MSG_IS_MOVES =  "M", // recv only
  MSG_IS_UNDO  =  "u", // send only
  MSG_IS_RESTART = "r", // send only
  MSG_IS_META  =  "x", // send/recv
  MSG_IS_TEXT  =  "t", // send/recv
  MSG_IS_GAME_STATUS =  "g", // send/recv 
  MSG_IS_WARNING =  "w", // send/recv
  MSG_IS_ERROR =  "e", // send/recv
  MSG_IS_DEBUG =  "d"  // send/recv
}
*/

_Game * GAME_SERVER = NULL;

void ServerDestroy() {
  if(GAME_SERVER)
    GameDestroy(GAME_SERVER);
  GAME_SERVER = NULL;
}

static inline 
void ServerError (ws_cli_conn_t client, char err[]) {
  ws_sendframe_txt(client, err);
  fprintf(stderr, "\nERROR: %s", &(err[1]));
  fflush(stderr);
}

void ServerCommandSuccess (ws_cli_conn_t client) {
  char success[] = "S";
  ws_sendframe_txt(client, success);
}

Flag 
ServerInit (ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type)
{
    
  if (  type != WS_FR_OP_TXT  ) {
    char err[] = "eClient Message should be String (Start/Restart)";
	  ServerError(client, err);
    return GAME_STATUS_ERROR;
  }

  char cmd = (char ) msg[0];
  if( cmd == 'f' ) // if the message is fen
  {
    if ( size <= 25 ) {   // FEN is short
      char err[] = "eShort FEN";
	    ServerError(client, err);
      return GAME_STATUS_ERROR;
    }
  }
  else if ( cmd == 'r' ){  // "restart"
    if( size != 1)  {   // FEN is short
      char err[] = "eWrong Encoding (Restart)";
	    ServerError(client, err);
      return GAME_STATUS_ERROR;
    }
  }
  else {
    char err[] = "eMessage is neither an FEN nor a Restart ";
	  ServerError(client, err);
    return GAME_STATUS_ERROR;
  }
  
  // Kill the game if any running
  ServerDestroy();

  // Start new game
  char * fen = (cmd == 'r') ? NULL : (char *) (&msg[1]);
  GAME_SERVER = GameNew(fen);

  //in case game cannot be loaded
  if(!GAME_SERVER) {
    char err[] = "eWrong FEN";
	  ServerError(client, err);
    return GAME_STATUS_ERROR;
  }

  GamePrintBoard(GAME_SERVER, 0);
    
  //Succesfully created a new game
  ServerCommandSuccess(client);
  return GAME_SERVER->board->status;
}

/*
Flag ServerBoard() {
  assert(GAME_SERVER); 
  //encode new board info to
  Flag status = GameMove(GAME_SERVER, move); 
  mempcy(msg, &GAME_SERVER->board, sizeof(_Board));
  return status;
}

Flag ServerMoves(_BoardMove * move, char * msg) {
  assert(GAME_SERVER); 
  //encode new board info to
  Flag status = GameMove(GAME_SERVER, move); 
  mempcy(msg, &GAME_SERVER->board, sizeof(_Board));
  return status;
}
*/

Flag ClientMove( ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type) 
{
  if( ( msg[0] != 'm' ) || // if the message is not 'move'
      ( type != WS_FR_OP_TXT )  ||  // Wrong msg format
      ( ! ( size == 5 || size == 6 ) ) )  //wrong length
  {   
    ServerError(client, "eWrong Client Message (Move)");
    return GAME_STATUS_ERROR;
  }
  if ( !GAME_SERVER  )  // cannot find game
  {   
    ServerError(client, 
      "eCannot find the game. Start/Restart a game");
    return GAME_STATUS_ERROR;
  }
  _Board * board = GAME_SERVER->board; 
  //Decode move
  Square from = BoardSquareParse ((char *) (msg+1)),
           to = BoardSquareParse ((char *) (msg+3));

  Piece promotion 
    = (size == 6) ? MAPPING2[msg[5]] : EMPTY;

  if(promotion) //Not empty
    promotion |= board->color;

  if( ( promotion > 15 ) || // invalid promotion piece
      ( from >= OUTSIDE )  ||  
      ( to >= OUTSIDE ) )
  {   
    ServerError(client, 
      "eCannot identify squares or promotion piece");
    return GAME_STATUS_ERROR;
  }

  BoardMakeAvailable(board);
  
  //GameStatus(GAME_SERVER);

  Flag nmoves = 
    (Flag) (GAME_SERVER->moves->len/sizeof(_BoardMove) );
  _BoardMove * move = (_BoardMove *) GAME_SERVER->moves->p;
  for(int i=0; i<nmoves; ++i, ++move) {
    if(move->from.square == from && move->to.square == to &&
       move->promotion == promotion) {
      Flag status = GameMove(GAME_SERVER, move);
      if(status == GAME_STATUS_ERROR) {
        ServerError(client, "eCannot Move the board");
        return GAME_STATUS_ERROR;
      }
      else {
        ServerCommandSuccess(client);
        return GAME_SERVER->board->status;
      } 
    }
  }

  ServerError(client, 
    "eCannot find the move in the list of moves");
  return GAME_STATUS_ERROR;
}



