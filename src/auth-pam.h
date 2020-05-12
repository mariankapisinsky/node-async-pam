/**
 * node-auth-pam - auth-pam.h
 * Copyright(c) 2020 Marian Kapisinsky
 * MIT Licensed
 */

#include <stdbool.h>
#include <pthread.h>

#include <node_api.h>

/**
 * Addon's defines.
*/

#define BUFFERSIZE 256

#define NODE_PAM_JS_CONV 50
#define NODE_PAM_ERR 51

/**
 * Authentication context
*/

typedef struct {
  char *service;  /* service name */
  char *username; /* username */
  char *message;  /* current PAM message */
  int msgStyle;   /* message style of the message */
  char *response; /* user's response */
  bool respFlag;  /* response flag - true, if the response is set */
  int retval;     /* return value, also used as flag for NODE_PAM_JS_CONV and NODE_PAM_ERR */
  pthread_t thread; /* authnetication thread */
  pthread_mutex_t mutex;  /* mutex protecting response and respFlag */
  napi_threadsafe_function tsfn; /* N-API thread-safe function */
} nodepamCtx;

/**
 * The addon's authentication function:
 * Sets the pam_conv structure - sets the application's data pointer to nodepamCtx.
 * Start the PAM transaction (service and username are specified in the nodepamCtx).
 * Authenticates the user.
 * Ends the PAM transaction and sets the return value to nodepamCtx.
*/

void nodepamAuthenticate(nodepamCtx *ctx);

/**
 * Sets the received response and respFlag in nodepamCtx.
*/

void nodepamSetResponse(nodepamCtx *ctx, const char *response, size_t responseSize);

/**
 * Cleanup function.
 * If retval is NODE_PAM_ERR, kill the running thread and free ctx->message,
 * otherwise join the thread.
 * Free nodepamCtx.
*/

void nodepamCleanup(nodepamCtx *ctx);

/**
 * Set the retval in nodepamCtx to NODE_PAM_ERR.
*/

void nodepamTerminate(nodepamCtx *ctx);
