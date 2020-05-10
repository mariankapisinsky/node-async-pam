/*!
 * login.js
 * Copyright(c) 2020 Marian Kapisinsky
 * MIT Licensed
 */

"use strict";

/**
 * PAM related defines
 */

const PAM_SUCCESS = 0;
const msgStyle = {
	PAM_PROMPT_ECHO_OFF: 1,
	PAM_PROMPT_ECHO_ON: 2,
	PAM_ERROR_MSG: 3,
	PAM_TEXT_INFO: 4
};

/**
 * username, timeout, error
 */

var user, tm, err;

/**
 * If the browser does not have a cookie
 * do the authentication process,
 * else hide the form, print 'Already authenticated' status
 * and redirect
 */

if (!document.cookie.includes('SID')) {

	// Connect to the WebSocket server
	var ws = new WebSocket('ws://localhost:1234');

	// If an error occured (authentication server is down)
	// print the 'WebSocket Error' status
	ws.onerror = function (e) {

		$("#status").text('WebSocket Error');
	}

	// Set the initial prompt and display the form
	ws.onopen = function (e) {

		$("#promptLabel").text('Username:');
		$("#promptForm").show();
	};

	// The logic for the authentication process
	ws.onmessage = function(e) {

		// Parse the incoming JSON data
		var message = JSON.parse(e.data);

		// The authentication finished successfully,
		// close the connection, hide the form, display
		// the 'Authenticated' success message, set the received cookie
		// and issue a redirect
		if (message.msg === PAM_SUCCESS) {

			ws.close();

			$("#promptForm").hide();
			$("#status").text('Authenticated');

			document.cookie = message.cookie;

			setTimeout( () => {
				window.location.href = '/';
			}, 3000);

		} else if (typeof message.msg === 'string') { // A string with PAM message is received
				
			switch (message.msgStyle) {
				// Don't show the input field's value while obtaining the response, start inactivity timeout
				case msgStyle.PAM_PROMPT_ECHO_OFF:
					$("#promptLabel").text(message.msg);
					$('#prompt').prop('type', 'password');
					startTimer();
					break;
				// Show the input field's value while obtaining the response, start inactivity timeout
				case msgStyle.PAM_PROMPT_ECHO_ON:
					$("#promptLabel").text(message.msg);
					$('#prompt').prop('type', 'text');
					startTimer();
					break;
				// Print the message - error (red), info (default)
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
		} else { // Authentication error - clear the inactivity timeout, print the error, set the initial prompt

			clearTimeout(tm);

			err = true;
			user = undefined;

			$("#status").text('Wrong username or password, please try again');
			$('#prompt').prop('type', 'text');
			$("#promptLabel").text('Username:');
		}
		$('#prompt').val('');
	};
} else { // If the user is already logged in, print the 'Already logged in' status and and issue a redirect

	$(document).ready( () => {
		$("#status").text('Already logged in');
		setTimeout( () => {
			window.location.href = '/';
		}, 3000);
	});
};

/**
 * Start the one-minute inactivity timeout
 */

function startTimer() {

	tm = setTimeout( () => {
		ws.close();
		$("#promptForm").hide();
		$("#status").text('Connection timeout');
		user = undefined;
	}, 60000);
};

/**
 * Collect and send user's input
 */

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
};


