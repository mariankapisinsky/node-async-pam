"use strict";

var ws = new WebSocket('ws://localhost:1234');

var tm;

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

		tm = setTimeout( () => {
			ws.close();
			$("#promptForm").hide();
			$("#status").text('Connection timeout');
		}, 60000);
	} else {
		$("#promptLabel").text(['Username:']);
		$("#status").text('An error occured, please try again');
	}
	$('#prompt').val('');
}

function sendUserInput() {
	clearTimeout(tm);
	var userInput = $('#prompt').val();
	ws.send(userInput);
	$("#status").text('');
}
