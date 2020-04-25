#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <node_api.h>

#include <security/pam_appl.h>

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

napi_ref nodepamCtxConstructor;

int nodepamConv( int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr ) {

  if ( !msg || !resp || !appdata_ptr ) return PAM_CONV_ERR;

  if ( num_msg <= 0 || num_msg > PAM_MAX_NUM_MSG ) return PAM_CONV_ERR;

  struct pam_response *response = NULL;
  response = memset(malloc(sizeof(*response)), 0, sizeof(*response));
  if ( !response ) return PAM_CONV_ERR;

  nodepamCtx *ctx = (nodepamCtx *)appdata_ptr;

  for ( int i = 0; i < num_msg; i++ ) {

    ctx->prompt = memset(malloc((strlen(msg[i]->msg) + 1)), 0, (strlen(msg[i]->msg) + 1));

    strcpy(ctx->prompt, msg[0]->msg);
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

static void CallJs(napi_env env, napi_value jsCallback, void* context, void* data) {

  napi_value constructor;

  if (!(env == NULL || jsCallback == NULL)) {

    napi_value undefined, jsData;

    assert(napi_get_undefined(env, &undefined) == napi_ok);

    assert(napi_get_reference_value(env, nodepamCtxConstructor, &constructor) == napi_ok);

    assert(napi_new_instance(env, constructor, 0, NULL, &jsData) == napi_ok);

    assert(napi_wrap(env, jsData, data, NULL, NULL, NULL) == napi_ok);

    assert(napi_call_function(env, undefined, jsCallback, 1, &jsData, NULL) == napi_ok);
  }
}

static void ThreadFinished(napi_env env, void* data, void* context) {

  (void) context;

  nodepamCtx* ctx = (nodepamCtx*) data;

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

//The authentication thread
static void *AuthThread(void* data) {

  nodepamCtx* ctx = (nodepamCtx*) data;

  pam_handle_t *pamh = NULL;
  int retval = 0;

  struct pam_conv conv = { &nodepamConv, (void *) ctx };

  retval = pam_start(ctx->service, ctx->username, &conv, &pamh);
  
  if( retval == PAM_SUCCESS )
    retval = pam_authenticate(pamh, PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

  pam_end(pamh, retval);

  ctx->retval = retval;

  assert(napi_call_threadsafe_function(ctx->tsfn, ctx, napi_tsfn_blocking) == napi_ok);

  assert(napi_release_threadsafe_function(ctx->tsfn, napi_tsfn_release) == napi_ok);

  return (NULL);
}

static napi_value Authenticate(napi_env env, napi_callback_info info) {

  size_t argc = 3;
  napi_value argv[3];
  char *buf;
  size_t stringSize;
  napi_value tsfnName;

  nodepamCtx* ctx = memset(malloc(sizeof(*ctx)), 0, sizeof(*ctx));

  assert(pthread_mutex_init(&(ctx->mutex), NULL) == 0);

  buf = memset(malloc(256), 0, (256));
  
  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

  assert(argc == 3 && "Exactly three arguments were received");

  assert(napi_get_value_string_utf8(env, argv[0], buf, sizeof(buf), &stringSize) == napi_ok);

  ctx->service =  malloc( stringSize );
  assert(ctx->service);
  strcpy(ctx->service, buf);

  memset(buf, 0, 256);

  assert(napi_get_value_string_utf8(env, argv[1], buf, sizeof(buf), &stringSize) == napi_ok);

  ctx->username =  malloc( stringSize );
  assert(ctx->username);
  strcpy(ctx->username, buf);

  assert(napi_create_string_utf8(env, "TSFN", NAPI_AUTO_LENGTH, &tsfnName) == napi_ok);

  assert(napi_create_threadsafe_function(env, argv[2], NULL, tsfnName, 0, 1, ctx, ThreadFinished, NULL, CallJs, &ctx->tsfn) == napi_ok);
  
  assert(pthread_create(&(ctx->thread), NULL, AuthThread, (void *) ctx) == 0);

  free(buf);

  return NULL;
}

static bool ispamCtx(napi_env env, napi_ref constructor_ref, napi_value value) {

  bool validate;
  napi_value constructor;

  assert(napi_get_reference_value(env, constructor_ref, &constructor) == napi_ok);
  assert(napi_instanceof(env, value, constructor, &validate) == napi_ok);

  return validate;
}

static napi_value RegisterResponse(napi_env env, napi_callback_info info) {

  size_t argc = 2;
  napi_value argv[2];
  nodepamCtx *ctx;
  char *buf;
  size_t responseSize;

  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

  assert(argc == 2 && "Exactly two arguments were received");

  assert(ispamCtx(env, nodepamCtxConstructor, argv[0]));

  assert(napi_unwrap(env, argv[0], (void**)&ctx) == napi_ok);

  buf = memset(malloc(256), 0, (256));
  
  assert(napi_get_value_string_utf8(env, argv[1], buf, 256, &responseSize) == napi_ok);

  pthread_mutex_lock(&(ctx->mutex));
  ctx->response = memset(malloc(responseSize), 0, responseSize);

  if (!ctx->response) {
    ctx->retval = NODE_PAM_ERR;
    assert(napi_release_threadsafe_function(ctx->tsfn, napi_tsfn_release) == napi_ok);
    return NULL;
  }

  strcpy(ctx->response, buf);
  pthread_mutex_unlock(&(ctx->mutex));

  free(buf);

  return NULL;
}

static napi_value Terminate(napi_env env, napi_callback_info info) {

  size_t argc = 1;
  napi_value argv[1];
  nodepamCtx *ctx;

  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

  assert(argc == 1 && "Exactly one arguments was received");

  assert(ispamCtx(env, nodepamCtxConstructor, argv[0]));

  assert(napi_unwrap(env, argv[0], (void**)&ctx) == napi_ok);

  ctx->retval = NODE_PAM_ERR;

  assert(napi_release_threadsafe_function(ctx->tsfn, napi_tsfn_release) == napi_ok);

  return NULL;
}

static napi_value AuthDataConstructor(napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value GetUser(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  nodepamCtx* ctx;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(ispamCtx(env, nodepamCtxConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&ctx));
  assert(napi_ok == napi_create_string_utf8(env, ctx->username, strlen(ctx->username), &property));

  return property;
}

static napi_value GetPrompt(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  nodepamCtx* ctx;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(ispamCtx(env, nodepamCtxConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&ctx));
  assert(napi_ok == napi_create_string_utf8(env, ctx->prompt, strlen(ctx->prompt), &property));

  return property;
}

static napi_value GetRetval(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  nodepamCtx* ctx;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(ispamCtx(env, nodepamCtxConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&ctx));
  assert(napi_ok == napi_create_int64(env, ctx->retval, &property));

  return property;
}

NAPI_MODULE_INIT() {

  napi_value authDataClass;

  napi_property_descriptor thread_item_properties[] = {
    { "user", 0, 0, GetUser, 0, 0, napi_writable, 0 },
    { "prompt", 0, 0, GetPrompt, 0, 0, napi_writable, 0 },
    { "retval", 0, 0, GetRetval, 0, 0, napi_enumerable, 0 }
  };

  assert(napi_define_class(env, "AuthData",  NAPI_AUTO_LENGTH, AuthDataConstructor, NULL, 3, thread_item_properties, &authDataClass) == napi_ok);
  assert(napi_create_reference(env, authDataClass, 1, &nodepamCtxConstructor) == napi_ok);

  napi_property_descriptor export_properties[] = {
    { "authenticate", NULL, Authenticate, NULL, NULL, NULL, napi_default, 0 },
    { "registerResponse", NULL, RegisterResponse, NULL, NULL, NULL, napi_default, 0 },
    { "terminate", NULL, Terminate, NULL, NULL, NULL, napi_default, 0 }
  };

  assert(napi_define_properties(env, exports, sizeof(export_properties) / sizeof(export_properties[0]), export_properties) == napi_ok);

  return exports;
}
