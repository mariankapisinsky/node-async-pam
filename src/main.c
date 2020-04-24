#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <node_api.h>

#include <security/pam_appl.h>

typedef struct {
  char *username;
  char *service;
  char *prompt;
  char *response;
  int retval;
  pthread_t thread;
  pthread_mutex_t mutex;
  napi_threadsafe_function tsfn;
} AddonData;

napi_ref authDataConstructor;
//dorobit terminate
// dorobit + timeout
int nodepamConv( int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr ) {

    AddonData *data = (AddonData *)appdata_ptr;

    data->prompt = memset(malloc((strlen(msg[0]->msg) + 1)), 0, (strlen(msg[0]->msg) + 1));

    strcpy(data->prompt, msg[0]->msg);
    data->retval = -1;
    data->response = NULL;

    // Pass the prompt into JavaScript
    assert(napi_call_threadsafe_function(data->tsfn, data, napi_tsfn_blocking) == napi_ok);

    //Wait for the response - najst lepsiu alternativu
    while(true) {

      pthread_mutex_lock(&(data->mutex));
      if(data->response == NULL ) {
        pthread_mutex_unlock(&(data->mutex));
        continue;
      } 
      else {
        pthread_mutex_unlock(&(data->mutex));
        break;
      }
    }

    struct pam_response *response = NULL;

    response = memset(malloc(sizeof(*response)), 0, sizeof(*response));

    response[0].resp = strdup(data->response);
    response[0].resp_retcode = 0;

    *resp = response;

    free(data->prompt);
    free(data->response);

    return PAM_SUCCESS;
}

static void CallJs(napi_env env, napi_value js_cb, void* context, void* data) {

  napi_value constructor;

  if (!(env == NULL || js_cb == NULL)) {
    napi_value undefined, js_thread_item;

    assert(napi_get_undefined(env, &undefined) == napi_ok);

    assert(napi_get_reference_value(env, authDataConstructor, &constructor) == napi_ok);

    assert(napi_new_instance(env, constructor, 0, NULL, &js_thread_item) == napi_ok);

    assert(napi_wrap(env, js_thread_item, data, NULL, NULL, NULL) == napi_ok);

    assert(napi_call_function(env, undefined, js_cb, 1, &js_thread_item, NULL) == napi_ok);
  }
}

static void ThreadFinished(napi_env env, void* data, void* context) {

  (void) context;

  AddonData* addonData = (AddonData*)data;

  assert(pthread_join(addonData->thread, NULL) == 0);
  
  pthread_mutex_destroy(&(addonData->mutex));

  addonData->tsfn = NULL;

  free(addonData->service);
  free(addonData->username);
  free(addonData);
}

//The authentication thread
void *AuthThread(void* data) {
  AddonData* addonData = (AddonData*) data;

  pam_handle_t *pamh = NULL;
  int retval = 0;

  struct pam_conv conv = { &nodepamConv, (void *) addonData };

  retval = pam_start(addonData->service, addonData->username, &conv, &pamh);
  
  retval = pam_authenticate(pamh, PAM_SILENT | PAM_DISALLOW_NULL_AUTHTOK);

  pam_end(pamh, 0);

  addonData->retval = retval;

  assert(napi_call_threadsafe_function(addonData->tsfn, addonData, napi_tsfn_blocking) == napi_ok);

  assert(napi_release_threadsafe_function(addonData->tsfn, napi_tsfn_release) == napi_ok);
}

static napi_value Authenticate(napi_env env, napi_callback_info info) {

  size_t argc = 3;
  napi_value argv[3];
  char *buf;
  size_t result;
  napi_value tsfn_name;

  AddonData* addonData = memset(malloc(sizeof(*addonData)), 0, sizeof(*addonData));

  assert(pthread_mutex_init(&(addonData->mutex), NULL) == 0);

  buf = malloc( sizeof(char) * 256 );
  memset(buf, 0, 256);
  
  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

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

  assert(napi_create_threadsafe_function(env, argv[2], NULL, tsfn_name, 0, 1, addonData, ThreadFinished, NULL, CallJs, &addonData->tsfn) == napi_ok);
  
  assert(pthread_create(&(addonData->thread), NULL, AuthThread, (void *) addonData) == 0);

  free(buf);

  return NULL;
}

static bool is_thread_item(napi_env env, napi_ref constructor_ref, napi_value value) {

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

  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

  assert(argc == 2 && "Exactly two arguments were received");

  assert(is_thread_item(env, authDataConstructor, argv[0]));

  assert(napi_unwrap(env, argv[0], (void**)&addonData) == napi_ok);

  buf = memset(malloc(256), 0, (256));
  
  assert(napi_get_value_string_utf8(env, argv[1], buf, 256, &result) == napi_ok);

  pthread_mutex_lock(&(addonData->mutex));
  addonData->response = memset(malloc(result), 0, result);
  strcpy(addonData->response, buf);
  pthread_mutex_unlock(&(addonData->mutex));

  free(buf);

  return NULL;
}

static napi_value Terminate(napi_env env, napi_callback_info info) {

  size_t argc = 1;
  napi_value argv[1];
  AddonData *addonData;

  assert(napi_get_cb_info(env, info, &argc, argv, NULL, NULL) == napi_ok);

  assert(argc == 1 && "Exactly one arguments was received");

  assert(is_thread_item(env, authDataConstructor, argv[0]));

  assert(napi_unwrap(env, argv[0], (void**)&addonData) == napi_ok);

  kill(addonData->thread, SIGTERM);

  //assert(napi_release_threadsafe_function(addonData->tsfn, napi_tsfn_release) == napi_ok);

  return NULL;
}

static napi_value AuthDataConstructor(napi_env env, napi_callback_info info) {
  return NULL;
}

static napi_value GetUser(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  AddonData* addonData;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(is_thread_item(env, authDataConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&addonData));
  assert(napi_ok == napi_create_string_utf8(env, addonData->username, strlen(addonData->username), &property));

  return property;
}

static napi_value GetPrompt(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  AddonData* addonData;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(is_thread_item(env, authDataConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&addonData));
  assert(napi_ok == napi_create_string_utf8(env, addonData->prompt, strlen(addonData->prompt), &property));

  return property;
}

static napi_value GetRetval(napi_env env, napi_callback_info info) {

  napi_value jsthis, property;
  AddonData* addonData;

  assert(napi_ok == napi_get_cb_info(env, info, 0, 0, &jsthis, NULL));
  assert(is_thread_item(env, authDataConstructor, jsthis));

  assert(napi_ok == napi_unwrap(env, jsthis, (void**)&addonData));
  assert(napi_ok == napi_create_int64(env, addonData->retval, &property));

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
  assert(napi_create_reference(env, authDataClass, 1, &authDataConstructor) == napi_ok);

  napi_property_descriptor export_properties[] = {
    { "authenticate", NULL, Authenticate, NULL, NULL, NULL, napi_default, 0 },
    { "registerResponse", NULL, RegisterResponse, NULL, NULL, NULL, napi_default, 0 },
    { "terminate", NULL, Terminate, NULL, NULL, NULL, napi_default, 0 }
  };

  assert(napi_define_properties(env, exports, sizeof(export_properties) / sizeof(export_properties[0]), export_properties) == napi_ok);

  return exports;
}
