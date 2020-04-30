#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <security/pam_appl.h>

#include "auth-pam.h"

int nodepamConv( int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr ) {

  if ( !msg || !resp || !appdata_ptr ) return PAM_CONV_ERR;

  if ( num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG ) return PAM_CONV_ERR;

  struct pam_response *response = NULL;
  response = memset(malloc(sizeof(*response)), 0, sizeof(*response));
  if ( !response ) return PAM_CONV_ERR;

  nodepamCtx *ctx = (nodepamCtx *)appdata_ptr;

  for ( int i = 0; i < num_msg; i++ ) {

    ctx->prompt = memset(malloc((strlen(msg[i]->msg) + 1)), 0, (strlen(msg[i]->msg) + 1));

    strcpy(ctx->prompt, msg[i]->msg);
    ctx->retval = NODE_PAM_JS_CONV;
    ctx->response = NULL;

    // Pass the prompt into JavaScript
    assert(napi_call_threadsafe_function(ctx->tsfn, ctx, napi_tsfn_blocking) == napi_ok);

    //Wait for the response - najst lepsiu alternativu
    while(true) {

      pthread_mutex_lock(&(ctx->mutex));
      if(ctx->response == NULL ) {
        pthread_mutex_unlock(&(ctx->mutex));
        continue;
      } 
      else {
        pthread_mutex_unlock(&(ctx->mutex));
        break;
      }
    }

    response[i].resp = strdup(ctx->response);
    response[i].resp_retcode = 0;

    free(ctx->prompt);
    free(ctx->response);
  }

  *resp = response;
  return PAM_SUCCESS;
}

//The authentication thread
void nodepamAuthenticate(nodepamCtx *ctx) {

  pam_handle_t *pamh = NULL;
  int retval = 0;

  struct pam_conv conv = { &nodepamConv, (void *) ctx };

  retval = pam_start(ctx->service, ctx->username, &conv, &pamh);
  
  if( retval == PAM_SUCCESS )
    retval = pam_authenticate(pamh, PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

  pam_end(pamh, retval);

  ctx->retval = retval;
}

void nodepamSetResponse(nodepamCtx *ctx, char *response, size_t responseSize) {

  pthread_mutex_lock(&(ctx->mutex));
  ctx->response = memset(malloc(responseSize), 0, responseSize);

  if (!ctx->response) {
    ctx->retval = NODE_PAM_ERR;
    assert(napi_release_threadsafe_function(ctx->tsfn, napi_tsfn_release) == napi_ok);
    return;
  }

  strcpy(ctx->response, response);
  pthread_mutex_unlock(&(ctx->mutex));
}

void nodepamCleanup(nodepamCtx *ctx) {

  if (ctx->retval == NODE_PAM_ERR) {
    kill(ctx->thread, SIGTERM);
    free(ctx->prompt);
  } else {
    assert(pthread_join(ctx->thread, NULL) == 0);
  }
  
  pthread_mutex_destroy(&(ctx->mutex));

  ctx->tsfn = NULL;

  free(ctx->service);
  free(ctx->username);
  free(ctx);
}

void nodepamTerminate(nodepamCtx *ctx) {

  ctx->retval = NODE_PAM_ERR;
  assert(napi_release_threadsafe_function(ctx->tsfn, napi_tsfn_release) == napi_ok);
}