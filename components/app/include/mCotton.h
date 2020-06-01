#ifndef _MCOTTON_H_
#define _MCOTTON_H_


int mCotton_available();
char* mCotton_read();
esp_err_t mCotton_init(const char *uri, const char *cid, const char *usr, const char *pwd);
bool mCotton_isConnected();
void mCotton_subscribe(const char *topic);
void mCotton_unsubscribe(const char *topic);
void mCotton_publish(const char *topic, const char *msg);


#endif
