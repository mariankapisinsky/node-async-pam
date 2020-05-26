# Simple Integration to a Web Application

The login.html file contains the necessary HTML code for a login page.
Edit it to needs and taste, but do *not* delete included scripts, the form,
and #status and #messages elements. If the application uses a templating language,
it can also be slip into several parts.

The login.js file contains the client-side JavaScript code for the login page.
Only edit window.location.href path and the WebSocket server address.

It is expected from the application to trust the sessions file created by the WebSocket server
and validate session cookies received in a request against it. 
When a user logs out, it should delete the appropriate session from the file.

For a test run main.js and open login.html in a browser.

For an example web application check out node-auth-example.
