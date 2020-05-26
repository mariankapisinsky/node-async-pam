/*!
 * node-auth-pam
 * Copyright(c) 2020 Marian Kapisinsky
 * MIT Licensed
 */

'use strict';

/**
 * Module dependencies.
 */

const yargs = require('yargs');
const fs = require('fs');
const crypto = require('crypto');
const WebSocketServer = require('ws').Server;

/**
 * Binding for node-auth-pam addon.
 */

const pam = require('bindings')('auth_pam');

/**
 * PAM related defines.
 */

const PAM_SUCCESS = 0;
const msgStyle = {
  PAM_PROMPT_ECHO_OFF: 1,
  PAM_PROMPT_ECHO_ON: 2,
  PAM_ERROR_MSG: 3,
  PAM_TEXT_INFO: 4
};

/**
 * node-auth-pam define - conversation in progress
 * 
 * Name of the cookie that carries the session ID.
 */

const NODE_PAM_JS_CONV = 50;
const cookieName = 'SID';

/**
 * Command line arguments parsing.
 */

const argv = yargs
  .usage('Usage: node $0 -p <path> -s <service> -w <wport>')
  .example('node main.js -p /app -s myservice -w 1234')
  .alias('p', 'path')
  .nargs('p', 1)
  .default('p', '/')
  .describe('p', 'Path to the application\'s root')
  .alias('s', 'service')
  .nargs('s', 1)
  .default('s', 'login')
  .describe('s', 'Name of the service')
  .alias('w', 'wport')
  .nargs('w', 1)
  .default('w', '1234')
  .describe('w', 'Port to run the WebSocket server on')
  .help('h')
  .alias('h', 'help')
  .argv;

/**
 *
 * Path to the application's root (default: /)
 * Port number for the WebSocket server (default: 1234)
 * 
 * Name of the service as configured
 * in /etc/pam.d/ (default: login)
 */

var path = argv.path;
var port = argv.wport;
var service = argv.service;

/**
 * Path to the sessions/ directory.
 * 
 * Path to the file that stores session information
 * named after the service as configured in /etc/pam.d/
 */

const dirPath = './sessions';
const sessionsFile = './sessions/' + service;

/**
 * On startup, create the sessions/ directory
 * if does not exist.
 */

fs.stat(dirPath, function(err, stats) {

  if (!stats) {
    fs.mkdir(dirPath, (err) => {
      if (err) throw err;
    });
  }
});

/**
 * On startup, create the file for storing session information
 * if does not exist.
 */

fs.stat(sessionsFile, function(err, stats) {

  if (!stats) {
    fs.writeFile(sessionsFile, '', (err) => {
      if (err) throw err;
    });
  }
});

/**
 * Start the WebSocket server.
 */

const wss = new WebSocketServer({ port: port });

console.log('Running on port ' + port + '...');

/**
 * On client's connection event.
 */

wss.on('connection', (ws) => {

  // variable to store nodepamCtx.
  var ctx;

  /** 
   * On client's message event.
   * 
   * The first (initial) message is the username,
   * every other message is the user's response
   * for the current prompt.
  */ 
  ws.on('message', (message) => {

    // If it is the client's inital message (does not have a nodepamCtx yet),
    // begin a PAM transaction, else look at the message as the user's response
    // to the current PAM message from the nodepamCtx.
    if (!ctx) {
      
      /**
       * The authenticate(service name, username, callback) binding creates
       * new nodepamCtx and authentication thread
       * (each connection has exactly one nodepamCtx and authentication thread).
       * 
       * The callback function is called from the PAM conversation function
       * (one or multiple times) and finally after the transaction is finished.
       * It passes the nodepamCtx from the addon to Node.js.
       * 
       * When the retval in the nodepamCtx is set to NODE_PAM_JS_CONV,
       * a conversation is in progress and waits for response.
       * The server sends the message and the message style to the client.
       * In case of PAM_ERROR_MSG or PAM_TEXT_INFO message styles, it has to call
       * the setResponse(nodepamCtx, response) binding with an empty string
       * due to synchronization issues.
       * 
       * When the retval in the nodepamCtx is set to PAM_SUCCESS,
       * the authentication was successful and the server generates a cookie,
       * stores it to the sessions file and sends it to a user along with the retval.
       * 
       * Any other case is an error case and the server sends the retval to the client
       * and deletes the stored context.
       */
      pam.authenticate(service, message, (nodepamCtx) => {
              
        if (nodepamCtx.retval === NODE_PAM_JS_CONV) {
            ws.send(JSON.stringify({'msg': nodepamCtx.msg, 'msgStyle': nodepamCtx.msgStyle}));
            ctx = nodepamCtx;
            if (nodepamCtx.msgStyle === msgStyle.PAM_ERROR_MSG || nodepamCtx.msgStyle === msgStyle.PAM_TEXT_INFO)
              pam.setResponse(ctx, '');
        } else if (nodepamCtx.retval === PAM_SUCCESS) {
            var cookie = generateCookie(nodepamCtx.user);
            ws.send(JSON.stringify({'msg': nodepamCtx.retval, 'cookie': cookie}));
            ctx = undefined;
        } else {
            ws.send(JSON.stringify({'msg': nodepamCtx.retval}));
            ctx = undefined;
        }
      });

    } else {
      
      // The setResponse(nodepamCtx, response) binding passes the user's response
      // to the waiting conversation function.
      pam.setResponse(ctx, message);
    }  
  });

  /** When the connection closes, the kill(nodepamCtx) binding kills
   * the authentication thread and deletes the nodepamCtx
   * if the transaction was in progress.
   */ 
  ws.on('close', () => {

    if (ctx)
      pam.kill(ctx);    
  });

});

/**
 * Generates a session ID (base64 encoded random 16 byte string),
 * generates an expiration date (now + 1d), creates a SID cookie 
 * with the expiration date and the path, stores the session ID 
 * to the sessions file in "sid::username" format
 * and starts the one-day timeout.
 * Returns the cookie.
 * @param {string} user 
 */

function generateCookie(user) {

  var sid = crypto.randomBytes(16).toString('base64');

  var expiresDate = new Date(new Date().getTime() + 86400000).toUTCString();

  var cookie = cookieName + '=' + sid + '; Path=' + path + '; Expires=' + expiresDate;

  fs.appendFile(sessionsFile, sid + '::' + user + '\n', err => {
    if (err) throw err;
  });

  startTimer(sid, 86400000);

  return cookie;
};

/**
 * Starts the one-day timeout after which
 * it deletes the corresponding line
 * with given session ID from 
 * the file with session information.
 * 
 * @param {string} sid 
 * @param {number} expiresDate 
 */

function startTimer(sid, expiresDate) {

  setTimeout( () => {
    fs.readFile(sessionsFile, (err, data) => {

      if (err) throw err;
  
      var lines = data.toString().split('\n');
  
      for ( let line of lines ) {
  
        if (line.startsWith(sid)) {
           var idx = lines.indexOf(line);
           lines.splice(idx, 1);
           fs.writeFile(sessionsFile, lines.join('\n'), (err) => {
             if (err) throw err;
           });
           break;             
        }
      }    
    });
  }, expiresDate);
};

/**
 * On interupt (ctrl+c),
 * clears the sessions file
 * and calls the cleanUp() binding
 * to prevent some memory leaks.
 */

process.on('SIGINT', () => {

  console.log('Stopping the server...');

  fs.writeFile(sessionsFile, '', (err) => {
    if (err) throw err;
  });
  
  pam.cleanUp();

  process.exit();
});
