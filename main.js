'use strict';

const yargs = require('yargs');
const fs = require('fs');
const crypto = require('crypto');
const WebSocketServer = require('ws').Server;
const pam = require('bindings')('auth_pam');

const PAM_SUCCESS = 0;
const msgStyle = {
  PAM_PROMPT_ECHO_OFF: 1,
  PAM_PROMPT_ECHO_ON: 2,
  PAM_ERROR_MSG: 3,
  PAM_TEXT_INFO: 4
};
const NODE_PAM_JS_CONV = 50;
const cookieName = 'SID=';

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

const dirPath = './sessions';
const sessionFile = './sessions/' + service;

fs.stat(dirPath, function(err, stats) {

  if (!stats) {
    fs.mkdir(dirPath, (err) => {
      if (err) throw err;
    });
  }
});

fs.stat(sessionFile, function(err, stats) {

  if (!stats) {
    fs.writeFile(sessionFile, '', (err) => {
      if (err) throw err;
    })
  }
});

const wss = new WebSocketServer({ port: port });

console.log('Runnning on port ' + port + '...');

wss.on('connection', (ws) => {

  var ctx;

  ws.on('message', (message) => {

    if (!ctx) {

      pam.authenticate(service, message, data => {
              
        if (data.retval === NODE_PAM_JS_CONV) {
            ws.send(JSON.stringify({'msg': data.msg, 'msgStyle': data.msgStyle}));
            ctx = data;
            if (data.msgStyle === msgStyle.PAM_ERROR_MSG || data.msgStyle === msgStyle.PAM_TEXT_INFO) pam.registerResponse(ctx, '');
        } else if (data.retval === PAM_SUCCESS) {
            var cookie = generateCookie(cookieName, data.user);
            ws.send(JSON.stringify({'msg': data.retval, 'cookie': cookie}));
            ctx = undefined;
        } else {
            ws.send(JSON.stringify({'msg': data.retval}));
            ctx = undefined;
        }
      });
    } else {

      pam.registerResponse(ctx, message);
    }  
  });

  ws.on('close', function() {

    if (ctx)
      pam.terminate(ctx);    
  });

});

function generateCookie(cookieName, user) {

  var expiresDate = new Date(new Date().getTime() + 86400000).toUTCString();

  var sid = crypto.randomBytes(16).toString('base64');

  var cookie = cookieName + sid + '; Expires=' + expiresDate;

  fs.appendFile(sessionFile, sid + '::' + user + '\n', err => {
    if (err) throw err;
  });

  startTimer(sid, 86400000);

  return cookie;
};

function startTimer(sid, expiresDate) {

  setTimeout( () => {
    fs.readFile(sessionFile, (err, data) => {

      if (err) throw err;
  
      var lines = data.toString().split('\n');
  
      for ( let line of lines ) {
  
        if (line.startsWith(sid)) {
           var idx = lines.indexOf(line);
           lines.splice(idx, 1);
           fs.writeFile(sessionFile, lines, (err) => {
             if (err) throw err;
           });
           break;             
        }
      }    
    });
  }, expiresDate);
};

process.on('SIGINT', () => {

  console.log('Stopping the server...');

  fs.unlink(sessionFile, (err) => {
    if (err) throw err;
  })
  
  pam.cleanUp();

  process.exit();
});
