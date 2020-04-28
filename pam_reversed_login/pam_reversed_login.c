#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>

#define AUTH_SUCCESS 0
#define AUTH_FAIL 1

int pam_authenticate_conv(pam_handle_t *pamh, int num_msg, const struct pam_message **msg, struct pam_response **resp) {

    struct pam_conv *conv;

    int retval = pam_get_item(pamh, PAM_CONV, (void *)&conv);
    if (retval != PAM_SUCCESS) {
        return retval;
    }

    return conv->conv(num_msg, msg, resp, conv->appdata_ptr);
}

int authenticate( const char *login, const char *reversed_login ) {

    if ( strlen(login) != strlen(reversed_login) ) return AUTH_FAIL;

    for ( int i = 0; i < strlen(login ); i++) {

        if ( login[i] != reversed_login[strlen(login) - i - 1] ) {
            return AUTH_FAIL;
        }
    }

    return AUTH_SUCCESS;
}

PAM_EXTERN int pam_sm_setcred( pam_handle_t *pamh, int flags, int argc, const char **argv ) {
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_authenticate( pam_handle_t *pamh, int flags, int argc, const char **argv ) {

    const char *login = NULL;
    char *reversed_login = NULL;

    if ( ( pam_get_user(pamh, &login, "Login: ") ) != PAM_SUCCESS ) {

        fprintf(stderr, "Can't get login\n");
    }

    struct pam_message msg;
    const struct pam_message *msgp = NULL;
    struct pam_response *resp = NULL;

    msg.msg_style = PAM_PROMPT_ECHO_ON;
    msg.msg = "Reversed login: ";
    msgp = &msg;

    int retval = pam_authenticate_conv(pamh, 1, &msgp, &resp);

    if ( retval != PAM_SUCCESS || resp == NULL || resp->resp == NULL ) {

        fprintf(stderr, "Didn't get reversed login\n");
        return PAM_SYSTEM_ERR;
    }
    else {

        reversed_login = resp->resp;
    }

    if ( ( authenticate(login, reversed_login) ) == AUTH_SUCCESS) {
        return PAM_SUCCESS;
    }
    else {
        return PAM_AUTH_ERR;
    }
}
