# node-async-pam
Asynchronous PAM Authentication for Node.js

Install:
  1. npm install
  2. npm run build

Dependencies:
  - PAM header files - dnf install pam-devel (on Fedora)

TO DO:
  - error handling
  - extend conv() function and replace while() if possible
  - integrate it to a test web application (functional, but weird behavior on login page)
  - session management in main.js
