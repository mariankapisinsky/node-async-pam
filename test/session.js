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
		
		if (user === '') {
			ws.close();
		} else {
			sendAuthInfo( user);
			ws.close();
		}
	}
});

function sendAuthInfo(user) {

	$.ajax({
		type: 'POST',
		url: window.location,
		data: { user: user },
		success: function(data) {
			$("html").html(data);
		}
	});
};
