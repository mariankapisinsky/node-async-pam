"use strict";

$( document ).ready(function() {

	var ws = new WebSocket('ws://localhost:1234');

	ws.onopen = function(e) {

		if (document.cookie) {
			ws.send('logout:' + getSID());
			document.cookie = 'SID=; Expires=Thu, 01 Jan 1970 00:00:01 GMT; path=/app';
			ws.close();		
		}
		window.location.href = '/app';
	};
});

/*https://www.the-art-of-web.com/javascript/getcookie/*/
function getSID() {

 	var re = new RegExp('SID' + "=([^;]+)");
   	var value = re.exec(document.cookie);
   	return (value != null) ? unescape(value[1]) : null;
};
