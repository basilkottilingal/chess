#include "game.h"
#include <ws.h>

/*
enum MSG_TYPE {
  MSG_IS_START =  "s", // send/recv
  MSG_IS_SUCCESS =  "S", // send/recv
  MSG_IS_FEN   =  "f", // send only
  MSG_IS_BOARD =  "b", // recv only
  MSG_IS_MOVE  =  "m", // send/recv
  MSG_IS_PLAYER  =  "p", // send/recv
  MSG_IS_MAKE_MOVE =  "M", // recv only
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
void ServerCommandSuccess (ws_cli_conn_t client, char msg[]) {
  if(! (msg[0] == 'S')) 
  {
    fprintf(stderr, "\nError : Wrong encoding by server for the ");
    fprintf(stderr, "following \"success\" msg.\n\t[%s]", msg);
    fflush(stderr);
  }
  else {
    ws_sendframe_txt(client, msg);
  }
}

Flag 
ServerInit (ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type)
{
    
  if (  type != WS_FR_OP_TXT  ) {
    ServerError(client, 
      "Error : Client Msg should be String (Start/Restart)");
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
  _Game * newGame = GameNew(fen);

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
  ServerCommandSuccess(client,"Success : New game");
  return GAME_SERVER->board->status;
}

Flag 
ServerEngineInit (ws_cli_conn_t client,                            
  const unsigned char *msg, uint64_t size, int type)
{
    
  if (  type != WS_FR_OP_TXT  ) {
    ServerError(client, 
      "Error : Client Message should be String (Start/Restart)");
    return GAME_STATUS_ERROR;
  }

  if( !(  (msg[0] == 'p') && 
          (msg[1] == 'w' || msg[1] == 'b') &&
          (size == 2) ) )
  {
	  ServerError(client, 
      "Error : Player Setting Message should be 'pw' or 'pb'");
    return GAME_STATUS_ERROR;
  }

  _Engine * e = EngineNew(GAME_SERVER->board, 
    msg[1] == 'w' ? WHITE : BLACK);

  if(!e) {
	  ServerError(client, 
      "Error : Couldn't start game engine");
    return GAME_STATUS_ERROR;
  }

  if(GAME_SERVER->engine) 
    EngineDestroy(GAME_SERVER->engine);

  GAME_SERVER->engine = e;  

  return GameStatus(GAME_SERVER);
}

/*
Flag ServerBoard() {
  assert(GAME_SERVER); 
  //encode new board info to
  Flag status = GameMove(GAME_SERVER, move); 
  mempcy(msg, &GAME_SERVER->board, sizeof(_Board));
  return status;
}
*/

Flag ServerMove( ws_cli_conn_t client,
  const unsigned char *msg, uint64_t size, int type) 
{
  if( ( msg[0] != 'M' ) || 
        // message should be 'M' (i.e Make a move)
      ( type != WS_FR_OP_TXT )  ||  // Wrong msg format
      ( size != 1 ) )  //wrong length
  {   
    ServerError(client, 
      "Error : Wrong Client Message (Make a move)!");
    return GAME_STATUS_ERROR;
  }

  if(!GAME_SERVER) {
    ServerError(client, "Error : No game running");
    return GAME_STATUS_ERROR;
  }
  
  _Engine * engine = GAME_SERVER->engine;
  if(!engine) {
    ServerError(client, 
      "Error : Hasn't started engine");
    return GAME_STATUS_ERROR;
  }

  //Engine Make a move on GAME_SERVER.
  _BoardMove * m = engine->engine(engine);
  if(!m) {
    ServerError(client, 
      "Error : Engine Failed to make a move");
    return GAME_STATUS_ERROR;
  }
 
  //Encode return message with the move information 
  char ret_msg[7] = {'m',
    'a' + m->from.square%8,
    '0' + 8 - m->from.square/8,
    'a' + m->to.square%8,
    '0' + 8 - m->to.square/8,
    !(m->flags & MOVE_PROMOTION) ? '\0' : 
          MAPPING[m->promotion&~WHITE],
    '\0'
  };
  ws_sendframe_txt(client, ret_msg);

  return GameStatus(GAME_SERVER);
}

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
  Flag status = GameUnmove(GAME_SERVER);
  if(status == GAME_CONTINUE) {
    ServerCommandSuccess(client, "Success : Undo");
    //send the new fen
    char fen[FEN_MAXSIZE + 1];
    sprintf(fen, "%c%s", 'f', GAME_SERVER->fen);
    ws_sendframe_txt (client, fen);
    //Print the board 
    GamePrintBoard(GAME_SERVER, 0); //0 delay
  }
  else { 
    ServerError(client, "Error : Couldn't undo a move.");
  }

  return status;
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
        ServerCommandSuccess(client, "Success : Board Moved");
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
    return ServerInit (client, msg, size, type);
  else if (cmd == 'm') 
    /* reflect client's move in the server */
    return ClientMove (client, msg, size, type);
  else if( cmd == 'p' )
    // 'p' : server player
    return ServerEngineInit (client, msg, size, type);
  else if (cmd == 'u') 
    /* reflect client's undo command in the server */
    return ClientUnmove (client, msg, size, type);
  else if (cmd == 'M') 
    /* reflect client's command to server to make a move */
    return ServerMove (client, msg, size, type);
    
  char * err = (char *) malloc (100 + size);
  sprintf(err, "%s%s",
    "Error: Unknown Command from Client. \n Msg :", msg); 
  ServerError (client, err);
  free(err);
  return GAME_STATUS_ERROR; //error
}
