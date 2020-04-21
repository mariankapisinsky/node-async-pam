"use strict";

const pam = require('bindings')('async_pam');

var WebSocketServer = require('ws').Server;

var WebSocketsServerPort = 1234;

var wss = new WebSocketServer({ port: WebSocketsServerPort });

console.log('Runnning on port ' + WebSocketsServerPort + '...');

wss.on('connection', function(ws) {

  console.log('Connected');

  var test;

  ws.on('message', function(message) {

    if ( message === "marian" ) {

      pam.authenticate('webapp', message, item => {
        test = item;
        if (item.retval === -1) {
          ws.send(JSON.stringify({"message": item.prompt}));
        } else {
          ws.send(JSON.stringify({"message": item.retval}));
        }
      });

    } else {

      pam.registerResponse(test, message);
    }
  });
});





