#ifndef _INCLUDE_BT_SPEAKER_H_
#define _INCLUDE_BT_SPEAKER_H_

void bt_speaker_init(char* name);

bool bt_speaker_is_init();

void bt_speaker_deinit();

bool bt_speaker_is_connected();

bool bt_speaker_is_started();

void bt_speaker_play();

void bt_speaker_stop();

void bt_speaker_pause();

void bt_speaker_forward();

void bt_speaker_backward();

#endif /* _INCLUDE_BT_SPEAKER_H_ */