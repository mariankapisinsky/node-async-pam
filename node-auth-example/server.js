const express = require('express');
const cookieParser = require('cookie-parser');
const fs = require('fs');
const path = require('path');
const app = express();

const cookieName = 'SID';
const sessionsFile = '../sessions/webapp';

app.use(express.static(path.join(__dirname, 'public')));
app.use(cookieParser());
app.set('view engine', 'ejs');

app.locals.pretty = true;

app.get('/', (req, res) => {

    if (!req.cookies[cookieName]) {
        res.locals.content = 'This is an Example Application - public view';
	res.locals.user = '';
	res.locals.link = '/login'
	res.locals.linkText = 'Log In'
    } else {
	const sid = req.cookies[cookieName];

	const user = getUser(sid);	

	res.locals.content = 'This is an Example Application - logged in as user: ';
	res.locals.user = user;
	res.locals.link = '/logout'
	res.locals.linkText = 'Log Out'
    }

    res.header('Pragma', 'no-cache');
    res.locals.title = 'Example Application';
    res.render('pages/index');
});

app.get('/login', (req, res) => {
    res.header('Pragma', 'no-cache');
    res.locals.title = 'Log In';
    res.locals.link = '/'
    res.locals.linkText = 'Back to the application';
    res.render('pages/login');
});

app.get('/logout', (req, res) => {

    if (!req.cookies[cookieName]) {
	res.locals.content = 'No user is logged in';
    } else {
	const sid = req.cookies[cookieName];
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

app.listen(8080);
console.log('Express running on port 8080...');
