"use strict";

$( document ).ready(function() {

	var ws = new WebSocket('ws://localhost:1234');

	var user;

	ws.onopen = function(e) {

		if (document.cookie) ws.send('sid:'+document.cookie);
	};

	ws.onmessage = function(e) {
		
		var message = JSON.parse(e.data);

		var user = message.user;
		var cookie = message.cookie;

		sendAuthInfo(document.url, user, cookie);
		console.log(user + cookie);
	}
});

function sendAuthInfo(url, user, cookie) {

	$.ajax({
		type: 'POST',
		url: url,
		data: { user: user, cookie: cookie }
	});
}
