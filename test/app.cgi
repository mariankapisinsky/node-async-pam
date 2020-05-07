#!/usr/bin/perl

#  Copyright 2014 Jan Pazdziora
#  Copyright 2020 Marian Kapisinsky
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

use strict;
use warnings FATAL => 'all';
use CGI ();
use CGI::Carp qw(fatalsToBrowser);

my $LOGIN = '/login';
my $LOGOUT = '/logout';
my $AUTH_COOKIE = 'SID';

my $sessionsFile = "/opt/node-auth-pam/sessions/webapp";

my $q = new CGI;
my $cookie = $q->cookie($AUTH_COOKIE);
my $user;
 
if ($cookie) { 
	open(FD, '<', $sessionsFile);
	while (<FD>) {
		if (rindex($_, $cookie, 0) == 0) {
			my @cred = split('::', $_);
			$user = $cred[1];
			last;
		}	
	}
	close(FD);
}

my @nav;

print "Content-Type: text/html; charset=UTF-8\n";
print "Pragma: no-cache\n";

my $title = "Application";
my $script = '';
my $body = "<p>This is a test application; public view, not much to see.<p>";
if (defined $user) {
	$title .= " authenticated ($user)";
	$body = "<p>Test application; logged in as user $user."
		 . " There is much more content for authenticated users.</p>";
}

sub logout {
	print "Set-Cookie: $AUTH_COOKIE=; Expires=Thu, 01 Jan 1970 00:00:01 GMT; path=$ENV{SCRIPT_NAME}\n";
	print "Refresh: 3; URL=$ENV{SCRIPT_NAME}\n";
	$title = "Logged out";
	$body = '<p>Successfully logged out. You will be redirected to the '
		. qq!<a href="$ENV{SCRIPT_NAME}">home page</a></p>!;
}
sub login {
	if (defined $user) {
		print "Refresh: 3; URL=$ENV{SCRIPT_NAME}\n";
		$title = "Already logged in";
		$body = "You are already logged in as user $user.\n";
		return;
	}
	$script = '<script src="../login.js"></script>';
	$title = "Log in to application";

	no warnings 'uninitialized';
	$body = <<"EOS";
	<form hidden id="promptForm" onsubmit="sendUserInput(); return false;">
		<label id="promptLabel" for="prompt"></label>
		<input id="prompt" type="text" />
		<button type="button" onclick="sendUserInput();">Next</button>
	</form>
	<h2 id="status"></h2>
	<div id="messages">
	</div>
EOS
}

if (defined $ENV{PATH_INFO}) {
	if (substr($ENV{PATH_INFO}, 0, length($LOGIN)) eq $LOGIN) {
		login();
		push @nav, qq!<a href="$ENV{SCRIPT_NAME}">Back to application</a>!;
	} elsif ($ENV{PATH_INFO} eq $LOGOUT) {
		logout();
		push @nav, qq!<a href="$ENV{SCRIPT_NAME}">Back to application</a>!;
	}
}

if (not @nav) {
	push @nav, (defined $user
		? qq!<a href="$ENV{SCRIPT_NAME}$LOGOUT">Log out</a>!
		: qq!<a href="$ENV{SCRIPT_NAME}$LOGIN">Log in</a>!);
}

print <<"EOS";

<html>
  <head>
    <title>$title</title>
    <script src="https://code.jquery.com/jquery-3.4.1.min.js"></script>
    $script
  </head>
  <body>
    <h1>$title</h1>
    $body
    <hr/>
    <p>@nav</p>
    <!--
    <hr/>
    <pre>@{[ join "\n", map CGI::escapeHTML("$_=$ENV{$_}"), sort keys %ENV ]}
    </pre>
    -->
  </body>
</html>
EOS

