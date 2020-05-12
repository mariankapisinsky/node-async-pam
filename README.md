# node-auth-pam
PAM Authentication for Node.js

Contents:
  - integration/ - contains necessary files for a simple integration to a web application
  - node-auth-example/ - contains an example web application for node-auth-pam
  - pam_reversed_login/ - contains an example PAM module for demonstration/testing purposes
  - src/ - contains node-auth-pam addon source files
  - main.js - the WebSocket server for PAM authentication using node-auth-pam

Dependencies:
  - PAM header files - dnf install pam-devel (on Fedora)
  
Install:
  1. npm install (should be enough)
  2. npm run build
  
Run:
  node main.js -p \<port> -s \<service>
  - port - the WebSocket server port (default: 1234)
  - service - the service name as defined in /etc/pam.d/ (default: login)

