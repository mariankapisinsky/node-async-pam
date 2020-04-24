"use strict";

var ws = new WebSocket('ws://localhost:1234');

var user, status, tm;

ws.onopen = function (e) {
	$("#promptLabel").text(['Username:']);
	$("#promptForm").show();
};

ws.onmessage = function(e) {
	var response = JSON.parse(e.data);

	if (response.message === 0) {
		$("#promptForm").hide();
		status = 'Authenticated';
		sendAuthInfo(user, status);
	}
	else if (typeof response.message === 'string') {
		$("#promptLabel").text(response.message);

		tm = setTimeout( () => {
			ws.close();
			$("#promptForm").hide();
			status = 'Connection timeout';
			sendAuthInfo(user, status);
			user = undefined;
		}, 60000);
	} else {
		$("#promptLabel").text(['Username:']);
		status = 'An error occured, please try again';
		sendAuthInfo(user, status);
		user = undefined;
	}
	$('#prompt').val('');
}

function sendUserInput() {
	clearTimeout(tm);
	if(!user) user = $('#prompt').val();
	var userInput = $('#prompt').val();
	ws.send(userInput);
	$("#status").text('');
}

function sendAuthInfo(user, status) {

 	$.ajax({
        	type: 'POST',
        	url: 'http://localhost/app/login',
        	data: { user: user, status: status },
		success: success
        });
};

