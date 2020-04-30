#include <pthread.h>

#include <node_api.h>

#define NODE_PAM_JS_CONV 50
#define NODE_PAM_ERR 51

typedef struct {
  char *username;
  char *service;
  char *prompt;
  char *response;
  int retval;
  pthread_t thread;
  pthread_mutex_t mutex;
  napi_threadsafe_function tsfn;
} nodepamCtx;

void nodepamAuthenticate(nodepamCtx *ctx);

void nodepamSetResponse(nodepamCtx *ctx, char *response, size_t responseSize);

void nodepamCleanup(nodepamCtx *ctx);

void nodepamTerminate(nodepamCtx *ctx);