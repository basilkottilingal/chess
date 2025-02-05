/*
 * Copyright (C) 2016-2023 Davidson Francis <davidsondfgl@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ws.h>

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
  printf("I receive a message: from: %s\n", cli);
      
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
	 */
  if(size) {
    if(type == WS_FR_OP_TXT) // is a textmsg
	    ws_sendframe_txt_bcast(8080, (char *)msg);
    else if(type == WS_FR_OP_BIN) // Binary msg
	    ws_sendframe_bin_bcast(8080, msg, size);
  }
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
