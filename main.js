"use strict";

const pam = require('bindings')('async_pam');

var WebSocketServer = require('ws').Server;

var WebSocketsServerPort = 1234;

var wss = new WebSocketServer({ port: WebSocketsServerPort });

console.log('Runnning on port ' + WebSocketsServerPort + '...');

let users = new Map;

wss.on('connection', function(ws) {

  console.log('Connected');

  ws.on('message', function(message) {

    var cred = message.split(':'); 

    if ( !users.has(cred[0]) ) {
      
      pam.authenticate('webapp', cred[0], data => {

        users.set(data.user, data);
        
        if (data.retval === -1) {
          ws.send(JSON.stringify({"message": data.prompt}));
        } else {
          ws.send(JSON.stringify({"message": data.retval}));
          console.log(data.retval);
          users.delete(data.user);
        }

        console.log(users.entries());
      });

    } else {

      pam.registerResponse(users.get(cred[0]), cred[1]);
    }
  });
});
