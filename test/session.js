"use strict";

$( document ).ready(function() {

	var ws = new WebSocket('ws://localhost:1234');

	var user;

	ws.onopen = function(e) {
		
		if (document.cookie) ws.send('sid:' + getSID());
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

/*https://www.the-art-of-web.com/javascript/getcookie/*/
function getSID() {

 	var re = new RegExp('SID' + "=([^;]+)");
   	var value = re.exec(document.cookie);
   	return (value != null) ? unescape(value[1]) : null;
};
