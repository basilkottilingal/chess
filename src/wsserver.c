/**
.. TODO: 
.. (1) multithread few functions, like
.. expanding tree compuattion , while waiting for
.. clients move.
.. (2) alphabeta pruning. expand pool a bit.
.. (3) Optimized tree data structure. (minimal and aligned).
.. (4) Tree search even if out of treepool memory.
*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ws.h>

#include "game-server.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
  non compilable line //Not yet implemented
#endif


/**
 * @dir examples/
 * @brief wsServer examples folder
 */

/*
 * @dir examples/echo
 * @brief Echo example directory.
 * @file echo.c
 * @brief Simple echo example.
 */

/**
 * @brief Called when a client connects to the server.
 *
 * @param client Client connection. The @p client parameter is used
 * in order to send messages and retrieve informations about the
 * client.
 */
void onopen(ws_cli_conn_t client)
{
	char *cli, *port;
	cli  = ws_getaddress(client);
	port = ws_getport(client);
#ifndef DISABLE_VERBOSE
	printf("Connection opened, addr: %s, port: %s\n", cli, port);
#endif

  //create a game with default board
  if(!GAME_SERVER)  {
    GAME_SERVER = GameNew(NULL);
    ServerSend(client, 
      "Success: Server Connected: New game.");
  }
  else {
    //Reload the client with current game
    ServerError(client, 
      "Warning : There is an ongoing game. Restart?"); 
    char fen[FEN_MAXSIZE + 1];
    sprintf(fen, "%c%s", 'f', GAME_SERVER->fen);
    ServerSend(client, fen); 
  }
}

/**
 * @brief Called when a client disconnects to the server.
 *
 * @param client Client connection. The @p client parameter is used
 * in order to send messages and retrieve informations about the
 * client.
 */
void onclose(ws_cli_conn_t client)
{
	char *cli;
	cli = ws_getaddress(client);
#ifndef DISABLE_VERBOSE
	printf("Connection closed, addr: %s\n", cli);
#endif
}

/**
 * @brief Called when a client connects to the server.
 *
 * @param client Client connection. The @p client parameter is used
 * in order to send messages and retrieve informations about the
 * client.
 *
 * @param msg Received message, this message can be a text
 * or binary message.
 *
 * @param size Message size (in bytes).
 *
 * @param type Message type.
 */
void onmessage(ws_cli_conn_t client,
	const unsigned char *msg, uint64_t size, int type)
{
	char *cli;
	cli = ws_getaddress(client);
#ifndef DISABLE_VERBOSE
  if (type == WS_FR_OP_BIN) {
    printf("Received binary data: ");
    for (uint64_t i = 0; i < size; i++)
      //printf("%02X ", (unsigned char)msg[i]);  // Print as hex
      printf("%d ", (unsigned char)msg[i]);  // Print as hex
    printf("\n");
  } 
  else
    printf("Received text: %s\n", msg);
#endif
  //send the message to game-server to decode 
  if (Server (client, msg, size, type) != GAME_STATUS_ERROR) 
    ServerIsGameOver(client);
  else
    ServerError (client, 
      "Error : Server failed to decode/implement client msg");

	/**
	 * Mimicks the same frame type received and re-send it again
	 *
	 * Please note that we could just use a ws_sendframe_txt()
	 * or ws_sendframe_bin() here, but we're just being safe
	 * and re-sending the very same frame type and content
	 * again.
	 *
	 * Alternative functions:
	 *   ws_sendframe()
	 *   ws_sendframe_txt()
	 *   ws_sendframe_txt_bcast()
	 *   ws_sendframe_bin()
	 *   ws_sendframe_bin_bcast()
  if(size) {
    if(type == WS_FR_OP_TXT) // is a textmsg
	    ws_sendframe_txt(client, (char *)msg);
	    //ws_sendframe_txt_bcast(8080, (char *)msg);
    else if(type == WS_FR_OP_BIN) // Binary msg
	    ws_sendframe_bin(client, msg, size);
	    //ws_sendframe_bin_bcast(8080, msg, size);
  }
	 */
}
//delete these 2 lines as you redo such that client.js is written by this file
#define PORT_START 8080
#define PORT_END 8080

#ifndef PORT_START
#define PORT_START 8080
#endif
#ifndef PORT_END
#define PORT_END 8090
#endif

int is_port_available(int port) 
{
    
  int sock = socket(AF_INET, SOCK_STREAM, 0);
    
  if (sock < 0) return 0;

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
  close(sock);
    
  return result == 0;
}

/**
 * @brief Main routine.
 *
 * @note After invoking @ref ws_socket, this routine never returns,
 * unless if invoked from a different thread.
 */

int main(void)
{

  // See if 8080 is free in shell
  // $  sudo lsof -i :8080
  // $  netstat -tuln | grep 8080

  int available = 0;
  for(uint32_t port = PORT_START; port <= PORT_END; ++port) 
  {
    if( !is_port_available(port) )
      continue; // port not handle
    
    available = 1;
    fprintf(stdout, 
      "Server listening to 127.0.0.1:%d\n", port); 
  
  	ws_socket(&(struct ws_server){
	  	/*
		   * Bind host:
		  * localhost -> localhost/127.0.0.1
		   * 0.0.0.0   -> global IPv4
		  * ::        -> global IPv4+IPv6 (DualStack)
		  */
  		.host = "0.0.0.0",
	   	.port = port,
		  .thread_loop   = 0,
		  .timeout_ms    = 1000,
  		.evs.onopen    = &onopen,
	  	.evs.onclose   = &onclose,
		  .evs.onmessage = &onmessage
	  });
  }
  
  if(!available)  
    fprintf(stderr, 
      "ERROR: No port between %d and %d are available",
      PORT_START, PORT_END);

	/*
	 * If you want to execute code past ws_socket(), set
	 * .thread_loop to '1'.
	 */

	return (0);
}
