/*
    Authors:
        Marian Kapisinsky

    Copyright (c) 2020 Marian Kapinsky

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Module defines.
 */

#define PAM_SM_AUTH
#include <security/pam_modules.h>

#define AUTH_SUCCESS 0
#define AUTH_FAIL 1

/**
 * Converse with the calling PAM application.
*/

int doPamConv(pam_handle_t *pamh, int num_msg, const struct pam_message **msg, struct pam_response **resp) {

    struct pam_conv *conv;

    int retval = pam_get_item(pamh, PAM_CONV, (void *)&conv);
    if (retval != PAM_SUCCESS)
        return retval;

    return conv->conv(num_msg, msg, resp, conv->appdata_ptr);
}

/**
 * Validate the user's response.
*/

int authenticate( const char *login, const char *reversed_login ) {

    if ( strlen(login) != strlen(reversed_login) ) return AUTH_FAIL;

    for ( int i = 0; i < strlen(login ); i++) {

        if ( login[i] != reversed_login[strlen(login) - i - 1] )
            return AUTH_FAIL;
    }

    return AUTH_SUCCESS;
}

/**
 * Set messages to the pam_message structure.
*/

void setMessages(struct pam_message *msg) {

    msg[0].msg_style = PAM_PROMPT_ECHO_OFF;
    msg[0].msg = "Reversed login (off): ";

    msg[1].msg_style = PAM_PROMPT_ECHO_ON;
    msg[1].msg = "Reversed login (on): ";

    msg[2].msg_style = PAM_ERROR_MSG;
    msg[2].msg = "This is an error message test";

    msg[3].msg_style = PAM_TEXT_INFO;
    msg[3].msg = "This is a text info test";
}

/**
 * Module's implementation of the pam_authenticate() interface.
*/

PAM_EXTERN int pam_sm_authenticate( pam_handle_t *pamh, int flags, int argc, const char **argv ) {

    const char *login = NULL;
    char *reversed_login = NULL;

    if ( ( pam_get_user(pamh, &login, "Login: ") ) != PAM_SUCCESS )
        fprintf(stderr, "Can't get login\n");

    struct pam_message msg[4];
    const struct pam_message **msgp = NULL;
    struct pam_response *resp = NULL;

    setMessages(msg);

    msgp = malloc(4 * sizeof(struct pam_message));

    msgp[0] = &msg[0];
    msgp[1] = &msg[1];
    msgp[2] = &msg[2];
    msgp[3] = &msg[3];

    int retval = doPamConv(pamh, 4, msgp, &resp);

    int status;
    for ( int i = 0; i < 2; i++ ) {
  
        if ( retval != PAM_SUCCESS || resp == NULL || resp->resp == NULL ) {

            fprintf(stderr, "Didn't get reversed login\n");
            return PAM_SYSTEM_ERR;
        }
        else {

            reversed_login = resp->resp;
        }

        status = authenticate(login, reversed_login);
        if (status == AUTH_FAIL)
            return PAM_AUTH_ERR;

        resp++;
    }

    free(msgp);
    return PAM_SUCCESS;
}

/**
 * Module's implementation of the pam_setcred() interface. Unused - always returns PAM_SUCCES.
*/

PAM_EXTERN int pam_sm_setcred( pam_handle_t *pamh, int flags, int argc, const char **argv ) {
	return PAM_SUCCESS;
}
