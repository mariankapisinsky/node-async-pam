"use strict";

var ws = new WebSocket('ws://localhost:1234');

var user, tm;

ws.onopen = function (e) {
	$("#promptLabel").text(['Username:']);
	$("#promptForm").show();
};

ws.onmessage = function(e) {
	var response = JSON.parse(e.data);

	if (response.message === 0) {
		ws.close();
		var cookie = 'ok:' + user;
		$("#promptForm").hide();
		$("#status").text('Authenticated');
		sendAuthInfo(user, cookie);
	}
	else if (typeof response.message === 'string') {
		$("#promptLabel").text(response.message);

		tm = setTimeout( () => {
			ws.close();
			$("#promptForm").hide();
			$("#status").text('Connection timeout');
			user = undefined;
		}, 60000);
	} else {
		$("#promptLabel").text(['Username:']);
		$("#status").text('An error occured, please try again');
		user = undefined;
	}
	$('#prompt').val('');
}

function sendUserInput() {
	clearTimeout(tm);
	$("#status").text('');
	if(!user) user = $('#prompt').val();
	var userInput = $('#prompt').val();
	if (!userInput) {
		$("#status").text('Input has to non empty!');
	} else {
		ws.send(userInput);
	}
}

function sendAuthInfo(user, cookie) {

	$.ajax({
		type: 'POST',
		url: 'http://localhost/app/login',
		data: { user: user, cookie: cookie }
	});
};

