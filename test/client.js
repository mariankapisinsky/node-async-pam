"use strict";

const PAM_SUCCESS = 0;
const msgStyle = {
	PAM_PROMPT_ECHO_OFF: 1,
	PAM_PROMPT_ECHO_ON: 2,
	PAM_ERROR_MSG: 3,
	PAM_TEXT_INFO: 4
}

var ws = new WebSocket('ws://localhost:1234');

var user, tm, err;

ws.onopen = function (e) {
	$("#promptLabel").text(['Username:']);
	$("#promptForm").show();
};

ws.onmessage = function(e) {
	var message = JSON.parse(e.data);

	if (message.msg === PAM_SUCCESS) {
		ws.close();
		$("#promptForm").hide();
		$("#status").text('Authenticated');
		sendAuthInfo(user, message.cookie);
	}
	else if (typeof message.msg === 'string') {
		
		switch (message.msgStyle) {
			case msgStyle.PAM_PROMPT_ECHO_OFF:
				$("#promptLabel").text(message.msg);
				$('#prompt').prop('type', 'password');
				startTimer();
				break;
			case msgStyle.PAM_PROMPT_ECHO_ON:
				$("#promptLabel").text(message.msg);
				$('#prompt').prop('type', 'text');
				startTimer();
				break;
			case msgStyle.PAM_ERROR_MSG:
			case msgStyle.PAM_TEXT_INFO:

				if (message.msgStyle === msgStyle.PAM_ERROR_MSG) {
					$("#messages").append('<p style="color:red">' + message.msg + '</p>');
				} else {
					$("#messages").append('<p>' + message.msg + '</p>');
				}
				break;
			default:
				break;
		}
	} else {
		err = true;
		user = undefined;
		$("#status").text('Wrong username or password, please try again');
		$('#prompt').prop('type', 'text');
		$("#promptLabel").text('Username:');
	}
	$('#prompt').val('');
}

function startTimer() {

	tm = setTimeout( () => {
		ws.close();
		$("#promptForm").hide();
		$("#status").text('Connection timeout');
		user = undefined;
	}, 60000);
}

function sendUserInput() {

	clearTimeout(tm);
	$("#status").text('');
	if(err) {
		$("#messages").empty();
		err = false;
	}
	if(!user) user = $('#prompt').val();
	var userInput = $('#prompt').val();
	if (!userInput) {
		$("#status").text('Input has to be non empty!');
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
}

