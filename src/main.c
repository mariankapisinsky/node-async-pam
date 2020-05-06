#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include <node_api.h>

#include "auth-pam.h"

napi_ref nodepamCtxConstructor;

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

  nodepamCtx *ctx = (nodepamCtx*) data;

  nodepamCleanup(ctx);
}

static void *AuthThread(void* data) {

  nodepamCtx* ctx = (nodepamCtx*) data;

  nodepamAuthenticate(ctx);
    
  assert(napi_call_threadsafe_function(ctx->tsfn, ctx, napi_tsfn_blocking) == napi_ok);

  assert(napi_release_threadsafe_function(ctx->tsfn, napi_tsfn_release) == napi_ok);

  return (NULL);
};

static napi_value Authenticate(napi_env env, napi_callback_info info) {

  size_t argc = 3;
  napi_value argv[3];
  char *buf;
  size_t stringSize;
  napi_value tsfnName;

  nodepamCtx* ctx = memset(malloc(sizeof(*ctx)), 0, sizeof(*ctx));

  assert(pthread_mutex_init(&(ctx->mutex), NULL) == 0);

  buf = memset(malloc(BUFFERSIZE), 0, BUFFERSIZE);
  
  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

  assert(argc == 3 && "Exactly three arguments were received");

  assert(napi_get_value_string_utf8(env, argv[0], buf, sizeof(buf), &stringSize) == napi_ok);

  ctx->service =  malloc( stringSize );
  assert(ctx->service);
  strcpy(ctx->service, buf);

  memset(buf, 0, BUFFERSIZE);

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

static bool isnodepamCtx(napi_env env, napi_ref constructor_ref, napi_value value) {

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

  assert(isnodepamCtx(env, nodepamCtxConstructor, argv[0]));

  assert(napi_unwrap(env, argv[0], (void**)&ctx) == napi_ok);

  buf = memset(malloc(BUFFERSIZE), 0, BUFFERSIZE);
  
  assert(napi_get_value_string_utf8(env, argv[1], buf, BUFFERSIZE, &responseSize) == napi_ok);

  nodepamSetResponse(ctx, buf, responseSize);

  free(buf);

  return NULL;
}

static napi_value Terminate(napi_env env, napi_callback_info info) {

  size_t argc = 1;
  napi_value argv[1];
  nodepamCtx *ctx;

  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

  assert(argc == 1 && "Exactly one arguments was received");

  assert(isnodepamCtx(env, nodepamCtxConstructor, argv[0]));

  assert(napi_unwrap(env, argv[0], (void**)&ctx) == napi_ok);

  nodepamTerminate(ctx);

  assert(napi_release_threadsafe_function(ctx->tsfn, napi_tsfn_release) == napi_ok);

  return NULL;
}

static napi_value CleanUp(napi_env env, napi_callback_info info) {

  assert(napi_delete_reference(env, nodepamCtxConstructor) == napi_ok);

  return NULL;
}

static napi_value AuthDataConstructor(napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value GetUser(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  nodepamCtx* ctx;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(isnodepamCtx(env, nodepamCtxConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&ctx));
  assert(napi_ok == napi_create_string_utf8(env, ctx->username, strlen(ctx->username), &property));

  return property;
}

static napi_value GetMsg(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  nodepamCtx* ctx;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(isnodepamCtx(env, nodepamCtxConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&ctx));
  assert(napi_ok == napi_create_string_utf8(env, ctx->message, strlen(ctx->message), &property));

  return property;
}

static napi_value GetMsgStyle(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  nodepamCtx* ctx;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(isnodepamCtx(env, nodepamCtxConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&ctx));
  assert(napi_ok == napi_create_int32(env, ctx->msgStyle, &property));

  return property;
}

static napi_value GetRetval(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  nodepamCtx* ctx;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(isnodepamCtx(env, nodepamCtxConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&ctx));
  assert(napi_ok == napi_create_int32(env, ctx->retval, &property));

  return property;
}

NAPI_MODULE_INIT() {

  napi_value authDataClass;

  napi_property_descriptor thread_item_properties[] = {
    { "user", 0, 0, GetUser, 0, 0, napi_writable, 0 },
    { "msg", 0, 0, GetMsg, 0, 0, napi_writable, 0 },
    { "msgStyle", 0, 0, GetMsgStyle, 0, 0, napi_enumerable, 0 },
    { "retval", 0, 0, GetRetval, 0, 0, napi_enumerable, 0 }
  };

  assert(napi_define_class(env, "AuthData",  NAPI_AUTO_LENGTH, AuthDataConstructor, NULL, 4, thread_item_properties, &authDataClass) == napi_ok);
  assert(napi_create_reference(env, authDataClass, 1, &nodepamCtxConstructor) == napi_ok);

  napi_property_descriptor export_properties[] = {
    { "authenticate", NULL, Authenticate, NULL, NULL, NULL, napi_default, 0 },
    { "registerResponse", NULL, RegisterResponse, NULL, NULL, NULL, napi_default, 0 },
    { "terminate", NULL, Terminate, NULL, NULL, NULL, napi_default, 0 },
    { "cleanUp", NULL, CleanUp, NULL, NULL, NULL, napi_default, 0 }
  };

  assert(napi_define_properties(env, exports, sizeof(export_properties) / sizeof(export_properties[0]), export_properties) == napi_ok);

  return exports;
}
