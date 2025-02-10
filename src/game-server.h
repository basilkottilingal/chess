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
  if(! (err[0] == 'e' || err[0] == 'E' ||  // error
       err[0] == 'w' || err[0] == 'W' ) )  // or a warning
  {
    fprintf(stderr, "\nError : Wrong encoding by server for the ");
    fprintf(stderr, "following error/warning msg.\n\t[%s]", err);
  }
  else {
    ws_sendframe_txt(client, err);
    fprintf(stderr, "\n%s", err);
  }
  fflush(stderr);
}

static inline
void ServerCommandSuccess (ws_cli_conn_t client) {
  //return 'S' (i.e. success) to the client
  ws_sendframe_txt(client, "S"); 
}

Flag 
ServerInit (ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type)
{
    
  if (  type != WS_FR_OP_TXT  ) {
    ServerError(client, 
      "Error : Client Message should be String (Start/Restart)");
    return GAME_STATUS_ERROR;
  }

  char cmd = (char ) msg[0];
  if( cmd == 'f' ) // if the message is fen
  {
    if ( size <= 25 ) {   // FEN is short
      ServerError(client, "Error : Short FEN");
      return GAME_STATUS_ERROR;
    }
  }
  else if ( cmd == 'r' ){  // 'r' for restart 
    if( size != 1)  {   // msg should be simply "r" 
      ServerError(client, "Error : Wrong Encoding (Restart)");
      return GAME_STATUS_ERROR;
    }
  }
  else {
	  ServerError(client, 
      "Error : Message is neither an FEN nor a Restart ");
    return GAME_STATUS_ERROR;
  }
  
  // Start new game
  char * fen = (cmd == 'r') ? NULL : (char *) (&msg[1]);
  Game * newGame = GameNew(fen);

  //in case game cannot be loaded
  if(!newGame) {
	  ServerError(client, "Error : Wrong FEN");
    return GAME_STATUS_ERROR;
  }

  // Kill the old game if any running
  ServerDestroy();
  GAME_SERVER = newGame;

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

Flag ClientUnmove( ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type) 
{
  if( ( msg[0] != 'u' ) || // message should be 'u' (undo)
      ( type != WS_FR_OP_TXT )  ||  // Wrong msg format
      ( size != 1 ) )  //wrong length
  {   
    ServerError(client, 
      "Error : Wrong Client Message (Undo)!");
    return GAME_STATUS_ERROR;
  }
  if ( !GAME_SERVER  )  // cannot find game
  {   
    ServerError(client, 
      "Error : Cannot find the game! Start/Restart a game.");
    return GAME_STATUS_ERROR;
  }
  return GameUnmove(GAME_SERVER);
}

Flag ClientMove( ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type) 
{
  if( ( msg[0] != 'm' ) || // if the message is not 'move'
      ( type != WS_FR_OP_TXT )  ||  // Wrong msg format
      ( ! ( size == 5 || size == 6 ) ) )  //wrong length
  {   
    ServerError(client, 
      "Error : Wrong Client Message (Move)!");
    return GAME_STATUS_ERROR;
  }
  if ( !GAME_SERVER  )  // cannot find game
  {   
    ServerError(client, 
      "Error : Cannot find the game! Start/Restart a game.");
    return GAME_STATUS_ERROR;
  }
  _Board * board = GAME_SERVER->board; 
  //Decode move
  Square from = BoardSquareParse ((char *) (msg+1)),
           to = BoardSquareParse ((char *) (msg+3));

  Piece promotion 
    = (size == 6) ? PieceParse((char) msg[5]) : EMPTY;

  if(promotion) //Not empty
    promotion |= board->color;
  if( ( promotion > 15 ) || // invalid promotion piece
      ( from >= OUTSIDE )  ||  
      ( to >= OUTSIDE ) )
  {   
    ServerError(client, 
      "Error : Cannot identify squares or promotion piece");
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
        ServerError(client, "Error : Cannot Move the board");
        return GAME_STATUS_ERROR;
      }
      else {
        GamePrintBoard(GAME_SERVER, 0); //0 delay
        ServerCommandSuccess(client);
        return GAME_SERVER->board->status;
      } 
    }
  }

  ServerError(client, 
    "Error : Cannot find the move in the list of moves");
  return GAME_STATUS_ERROR;
}

/**
  main server function, that decodes the message ..
  .. friom client and send back appropriate responses ..
  .. or error/warnign messsages 
*/
  
    
Flag Server( ws_cli_conn_t client,
  const unsigned char *msg, uint64_t size, int type)
{
  if(!size) {
    ServerError(client, "Error : Message of zero length!");
    return 0;
  }
  char cmd = (char) msg[0];
  if( cmd == 'r' || cmd == 'f' )
    // 'r' : restart, 'f' : start with fen specified
    ServerInit (client, msg, size, type);
  else if (cmd == 'm') 
    /* reflect client's move in the server */
    ClientMove (client, msg, size, type);
  else if (cmd == 'u') 
    /* reflect client's undo command in the server */
    ClientUnmove (client, msg, size, type);
  else {
    ServerError (client,  "Error: Unknown Command from Client");
    return 0;
  }
    
  return 1; //successful
}
