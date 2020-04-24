"use strict";

var ws = new WebSocket('ws://localhost:1234');

var user;

ws.onopen = function (e) {
	$("#promptLabel").text(['Username:']);
	$("#promptForm").show();
};

ws.onmessage = function(e) {
	var response = JSON.parse(e.data);

	if (response.message === 0) {
		$("#promptForm").hide();
		$("#status").text('Authenticated');
	}
	else if (typeof response.message === 'string') {
		$("#promptLabel").text(response.message);
	} else if (response.message === 10) {
		$("#promptLabel").text(['Username:']);
		$("#status").text('You are not yet registered to the service');
		user = undefined;
	}
	else {
		$("#promptLabel").text(['Username:']);
		$("#status").text('An error occured, please try again');
		user = undefined;
	}
	$('#prompt').val('');
}

function sendUserInput() {
	if (!user) user = $('#prompt').val();
	var userInput = user + ':' + $('#prompt').val();
	ws.send(userInput);
	$("#status").text('');
}
