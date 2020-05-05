"use strict";

$( document ).ready(function() {

	var ws = new WebSocket('ws://localhost:1234');

	ws.onopen = function(e) {

		if (document.cookie) {
			ws.send('logout:'+document.cookie);
			document.cookie = 'SID=; Expires=Thu, 01 Jan 1970 00:00:01 GMT;path=/app';
			ws.close();		
		}
		window.location.href = '/app';
	};
});
