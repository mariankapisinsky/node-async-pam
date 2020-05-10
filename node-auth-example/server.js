/*!
 * node-auth-example
 * Copyright(c) 2020 Marian Kapisinsky
 * MIT Licensed
 */

'use strict';

/**
 * Module dependencies.
 */

const express = require('express');
const cookieParser = require('cookie-parser');
const fs = require('fs');
const path = require('path');
const app = express();

/**
 * Name of the cookie that carries
 * the session ID.
 * 
 * Path to the file that stores session information
 * named after the service as configured in /etc/pam.d/
 */

const cookieName = 'SID';
const sessionsFile = '../sessions/webapp';

/**
 * 1. path to CSS, JS and public page content
 * 2. cookieParser() middleware
 * 3. the view engine - EJS
 */

app.use(express.static(path.join(__dirname, 'public')));
app.use(cookieParser());
app.set('view engine', 'ejs');

/**
 * The index page
 * If the 'SID' cookie is defined,
 * search for the corresponding username
 * and render the corresponding page content.
 */

app.get('/', (req, res) => {

    var user;

    if (req.cookies[cookieName]) {
		const sid = req.cookies[cookieName];
		user = getUser(sid);
    }

    if (!user) {
        res.locals.content = 'This is an Example Application - public view';
		res.locals.user = '';
		res.locals.link = '/login'
		res.locals.linkText = 'Log In'
    } else {
		res.locals.content = 'This is an Example Application - logged in as user: ';
		res.locals.user = user;
		res.locals.link = '/logout'
		res.locals.linkText = 'Log Out'
    }

    res.header('Pragma', 'no-cache');
    res.locals.title = 'Example Application';
    res.render('pages/index');
});

/**
 * The login page
 * If the authentication server restarted
 * (the file with session information was deleted),
 * remove the obsolete 'SID' cookie and re-authenticate.
 * Authentication is handled by the login.js script.
 */

app.get('/login', (req, res) => {

	var user;

    if (req.cookies[cookieName]) {
		const sid = req.cookies[cookieName];
		user = getUser(sid);
    }

    if (!user) res.clearCookie(cookieName);

    res.header('Pragma', 'no-cache');
    res.locals.title = 'Log In';
    res.locals.link = '/'
    res.locals.linkText = 'Back to the application';
    res.render('pages/login');
});

/**
 * The logout page
 * Check if a user is logged in
 * and remove the 'SID' cookie.
 */

app.get('/logout', (req, res) => {

    var user;
    var sid;

    if (req.cookies[cookieName]) {
		sid = req.cookies[cookieName];
		user = getUser(sid);
    }

    if (!user) {
		res.locals.content = 'No user is logged in';
    } else {
		removeSID(sid);
		res.clearCookie(cookieName);
		res.locals.content = 'User logged out';
    }
    
    res.header('Pragma', 'no-cache');
    res.header('Refresh', '3; url=/');
    res.locals.title = 'Log Out';
    res.locals.link = '/'
    res.locals.linkText = 'Back to the application';
    res.render('pages/logout');
});

/**
 * Get username by session ID.
 * Session ID is a base64 encoded
 * randomly generated 16 byte string.
 * @param {string} sid 
 */

function getUser(sid) {
	
	var user;

	data = fs.readFileSync(sessionsFile);

        if (data.toString().includes(sid)) {

		  const lines = data.toString().split('\n');

		  for ( let line of lines ) {

		    	if (line.startsWith(sid + "::")) {
		    		var cred = (line.split('::'));
		    	  	user = cred[1];
		    	  	break;             
		   	}
		  }
        }
	return user;
};

/**
 * Remove the corresponding line
 * with given session ID
 * from the file with session information
 * @param {string} sid 
 */

function removeSID(sid) {

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
};

/**
 * Run the Example Application
 */

app.listen(8080);
console.log('Express running on port 8080...');
