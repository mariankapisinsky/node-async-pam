#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <uv.h>
#include <node_api.h>

#include <security/pam_appl.h>

typedef struct {
  char *prompt;
  char *response;
  int retval;
} AuthData;

typedef struct {
  char *username;
  char *service;
  uv_thread_t thread;
  uv_mutex_t mutex;
  napi_threadsafe_function tsfn;
  napi_ref authDataConstructor;
} AddonData;

int nodepamConv( int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr ) {

    AddonData *data = (AddonData *)appdata_ptr;

    AuthData *item = NULL;
    item = memset(malloc(sizeof(*item)), 0, sizeof(*item));

    item->prompt = memset(malloc((strlen(msg[0]->msg) + 1)), 0, (strlen(msg[0]->msg) + 1));

    strcpy(item->prompt, msg[0]->msg);
    item->retval = -1;
    item->response = NULL;

    // Pass the prompt into JavaScript
    assert(napi_call_threadsafe_function(data->tsfn, item, napi_tsfn_blocking) == napi_ok);

    //Wait for the response
    while(true) {

      uv_mutex_lock(&(data->mutex));
      if(item->response == NULL ) {
        uv_mutex_unlock(&(data->mutex));
        continue;
      } 
      else {
        uv_mutex_unlock(&(data->mutex));
        break;
      }
    }

    struct pam_response *response = NULL;

    response = memset(malloc(sizeof(*response)), 0, sizeof(*response));

    response[0].resp = strdup(item->response);
    response[0].resp_retcode = 0;

    *resp = response;

    free(item->prompt);
    free(item->response);
    free(item);

    return PAM_SUCCESS;
}

static void CallJs(napi_env env, napi_value js_cb, void* context, void* data) {

  AddonData* addonData = (AddonData*)context;
  napi_value constructor;

  if (!(env == NULL || js_cb == NULL)) {
    napi_value undefined, js_thread_item;

    assert(napi_get_undefined(env, &undefined) == napi_ok);

    assert(napi_get_reference_value(env, addonData->authDataConstructor, &constructor) == napi_ok);

    assert(napi_new_instance(env, constructor, 0, NULL, &js_thread_item) == napi_ok);

    assert(napi_wrap(env, js_thread_item, data, NULL, NULL, NULL) == napi_ok);

    assert(napi_call_function(env, undefined, js_cb, 1, &js_thread_item, NULL) == napi_ok);
  }
}

static void ThreadFinished(napi_env env, void* data, void* context) {
  (void) context;
  AddonData* addonData = (AddonData*)data;
  assert(uv_thread_join(&(addonData->thread)) == 0);
  addonData->tsfn = NULL;
}

//The authentication thread
static void AuthThread(void* data) {
  AddonData* addonData = (AddonData*) data;

  pam_handle_t *pamh = NULL;
  int retval = 0;

  struct pam_conv conv = { &nodepamConv, (void *) addonData};

  retval = pam_start(addonData->service, addonData->username, &conv, &pamh);
  
  retval = pam_authenticate(pamh, PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

  pam_end(pamh, 0);

  AuthData *item = NULL;
  item = memset(malloc(sizeof(*item)), 0, sizeof(*item));

  item->retval = retval;

  assert(napi_call_threadsafe_function(addonData->tsfn, item, napi_tsfn_blocking) == napi_ok);

  free(item);

  assert(napi_release_threadsafe_function(addonData->tsfn, napi_tsfn_release) == napi_ok);
}

static napi_value Authenticate(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3];
  char *buf;
  size_t result;
  napi_value tsfn_name;
  AddonData* addonData;

  buf = malloc( sizeof(char) * 256 );
  memset(buf, 0, 256);
  
  assert(napi_get_cb_info(env, info, &argc, argv, NULL, (void*)&addonData) == napi_ok);

  assert(argc == 3 && "Exactly three arguments were received");

  assert(napi_get_value_string_utf8(env, argv[0], buf, sizeof(buf), &result) == napi_ok);

  addonData->service =  malloc( sizeof(char) * result );
  strcpy(addonData->service, buf);

  memset(buf, 0, 256);
  assert(napi_get_value_string_utf8(env, argv[1], buf, sizeof(buf), &result) == napi_ok);

  addonData->username =  malloc( sizeof(char) * result );
  strcpy(addonData->username, buf);

  assert(addonData->tsfn == NULL && "Work already in progress");

  assert(napi_create_string_utf8(env, "TSFN", NAPI_AUTO_LENGTH, &tsfn_name) == napi_ok);

  assert(napi_create_threadsafe_function(env, argv[2], NULL, tsfn_name, 0, 1, addonData, ThreadFinished, addonData, CallJs, &addonData->tsfn) == napi_ok);
  
  assert(uv_thread_create(&(addonData->thread), AuthThread, addonData) == 0);

  return NULL;
}

static bool is_thread_item (napi_env env, napi_ref constructor_ref, napi_value value) {

  bool validate;
  napi_value constructor;

  assert(napi_get_reference_value(env, constructor_ref, &constructor) == napi_ok);
  assert(napi_instanceof(env, value, constructor, &validate) == napi_ok);

  return validate;
}

static napi_value RegisterResponse(napi_env env, napi_callback_info info) {

  size_t argc = 2;
  napi_value argv[2];
  AddonData *addonData;
  char *buf;
  size_t result;
  AuthData *item;

  assert(napi_get_cb_info(env, info, &argc, argv, NULL, (void*)&addonData) == napi_ok);

  assert(argc == 2 && "Exactly two arguments were received");

  assert(is_thread_item(env, addonData->authDataConstructor, argv[0]));

  assert(napi_unwrap(env, argv[0], (void**)&item) == napi_ok);

  buf = malloc( sizeof(char) * 256 );
  memset(buf, 0, 256);
  
  assert(napi_get_value_string_utf8(env, argv[1], buf, sizeof(buf), &result) == napi_ok);

  uv_mutex_lock(&(addonData->mutex));
  item->response = memset(malloc(result), 0, result);
  strcpy(item->response, buf);
  uv_mutex_unlock(&(addonData->mutex));

  return NULL;
}

static napi_value AuthDataConstructor(napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value GetPrompt(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  AddonData* addonData;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&addonData));
  assert(is_thread_item(env, addonData->authDataConstructor, jsthis));

  AuthData* authData;

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&authData));
  assert(napi_ok == napi_create_string_utf8(env, authData->prompt, strlen(authData->prompt), &property));

  return property;
}

static napi_value GetRetval(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  AddonData* addonData;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, (void*)&addonData));
  assert(is_thread_item(env, addonData->authDataConstructor, jsthis));

  AuthData* authData;

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&authData));
  assert(napi_ok == napi_create_int64(env, authData->retval, &property));

  return property;
}

static void addon_is_unloading(napi_env env, void* data, void* hint) {

  AddonData* addonData = (AddonData*)data;

  uv_mutex_destroy(&(addonData->mutex));

  assert(napi_delete_reference(env, addonData->authDataConstructor) == napi_ok);

  free(addonData);
}

// Initialize an instance of this addon. This function may be called multiple
// times if multiple instances of Node.js are running on multiple threads, or if
// there are multiple Node.js contexts running on the same thread.
NAPI_MODULE_INIT() {

  AddonData* addonData = memset(malloc(sizeof(*addonData)), 0, sizeof(*addonData));

  assert(napi_wrap(env, exports, addonData, addon_is_unloading, NULL, NULL) == napi_ok);

  assert(uv_mutex_init(&(addonData->mutex)) == 0);

  napi_value authDataClass;

  napi_property_descriptor thread_item_properties[] = {
    { "prompt", 0, 0, GetPrompt, 0, 0, napi_writable, addonData },
    { "retval", 0, 0, GetRetval, 0, 0, napi_enumerable, addonData }
  };

  assert(napi_define_class(env, "AuthData",  NAPI_AUTO_LENGTH, AuthDataConstructor, addonData, 2, thread_item_properties, &authDataClass) == napi_ok);
  assert(napi_create_reference(env, authDataClass, 1, &(addonData->authDataConstructor)) == napi_ok);

  napi_property_descriptor export_properties[] = {
    { "authenticate", NULL, Authenticate, NULL, NULL, NULL, napi_default, addonData },
    { "registerResponse", NULL, RegisterResponse, NULL, NULL, NULL, napi_default, addonData }
  };

  assert(napi_define_properties(env, exports, sizeof(export_properties) / sizeof(export_properties[0]), export_properties) == napi_ok);

  return exports;
}
