#!/bin/bash

# lauch the server. fixme : works only if port 8080 is available
(cd src/ && make server && ./server) &
SERVER_PID=$!

# open an http server to serve the pages
python3 -m http.server 8000 &
HTTP_PID=$!

# now, you can kill both the processes by ctrl+c
cleanup() {
  kill "$SERVER_PID" "$HTTP_PID" 2>/dev/null
}
trap cleanup EXIT INT TERM

# once  the "./server" is launched let's wait a bit before
# .. the webpage is opened
sleep 0.2
xdg-open http://127.0.0.1:8000/client/client.html

wait "$SERVER_PID"
