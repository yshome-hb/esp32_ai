#ifndef _MCOTTON_REST_H_
#define _MCOTTON_REST_H_

typedef void (*restDone_callback_t)    (int code, const char* msg);

void mcotton_rest_start(restDone_callback_t resDone_cb);
void mcotton_rest_conn();
void mcotton_rest_send();


#endif
