"use strict"; // + session management ak sa da

const pam = require('bindings')('async_pam');
const yargs = require('yargs');
var WebSocketServer = require('ws').Server;

const argv = yargs
  .usage('Usage: node $0 -p <port> -s <service>')
  .example('node main.js -p 1234 -s myservice')
  .alias('p', 'port')
  .nargs('p', 1)
  .default('p', '1234')
  .describe('p', 'Port to run the WebSocket server on')
  .alias('s', 'service')
  .nargs('s', 1)
  .default('s', 'login')
  .describe('s', 'Name of the service')
  .help('h')
  .alias('h', 'help')
  .argv;

var port = argv.port;
var service = argv.service;

const wss = new WebSocketServer({ port: port });

console.log('Runnning on port ' + port + '...');

wss.on('connection', function(ws) {

  console.log('Connected');
  var ctx;

  ws.on('message', function(message) {

    if ( !ctx ) {
      
      pam.authenticate(service, message, data => {

        ctx = data;
        
        if (data.retval === -1) {
          ws.send(JSON.stringify({"message": data.prompt}));
        } else {
          ws.send(JSON.stringify({"message": data.retval}));
          ctx = undefined;
          console.log(data.retval);
        }
      });

    } else {

      pam.registerResponse(ctx, message);
    }
  });

  ws.on('close', function() {
    if (ctx) {
      pam.terminate(ctx);
      ctx = undefined;
      console.log('term');
    }
  });
});


