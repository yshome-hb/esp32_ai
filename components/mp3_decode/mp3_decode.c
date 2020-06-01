#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "esp_log.h"
#include "audio.h"
#include "mp3_decode.h"

#ifdef CONFIG_AUDIO_MAD
	#include "mad.h"
#endif

#ifdef CONFIG_AUDIO_HELIX
	#include "mp3dec.h"
#endif

#define TAG "MP3_DECODE:"

// The theoretical maximum frame size is 2881 bytes,
// MPEG 2.5 Layer II, 8000 Hz @ 160 kbps, with a padding slot plus 8 byte MAD_BUFFER_GUARD.
#ifdef CONFIG_AUDIO_MAD
	#define MAINBUF_SIZE 2889
#endif

#define RING_BUF_SIZE 	1024*6

#define DECODE_PLAY_BIT 	BIT0
#define DECODE_SUSPEND_BIT  BIT1
#define DECODE_DESTROY_BIT  BIT2

static TaskHandle_t xdecodeTask = NULL;
static EventGroupHandle_t decode_event_group = NULL;
static RingbufHandle_t rb;

static int prevRate = 0;

#ifdef CONFIG_AUDIO_MAD
static int input_mad(struct mad_stream *stream, char* read_buf, size_t read_size)
{
	size_t len = 0;
	char *buf;
	//Shift remaining contents of buf to the front
    size_t remain = stream->bufend - stream->next_frame;
	memmove(read_buf, stream->next_frame, remain);

	buf = xRingbufferReceiveUpTo(rb, &len, 0, read_size - remain);
	if (buf){
		memcpy(read_buf + remain, buf, len);
		vRingbufferReturnItem(rb, buf);
	}

	return (remain + len);
}


//Routine to print out an error
static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame) {
	DECODE_DEBUGE(TAG, "decode err 0x%04x (%s)", stream->error, mad_stream_errorstr(stream));
    return MAD_FLOW_CONTINUE;
}
#endif


#ifdef CONFIG_AUDIO_HELIX
typedef struct  {  
	unsigned char* buf;  
	unsigned char* ptr;
	short *out; 
	int left;  
}Mp3Buf_t; 

static esp_err_t input_helix(Mp3Buf_t* read_buf, size_t read_size)
{
	size_t len = 0;
	char *buf;

	memmove(read_buf->buf, read_buf->ptr, read_buf->left);

	buf = xRingbufferReceiveUpTo(rb, &len, 0, read_size - read_buf->left);
	if (buf) {
		memcpy(read_buf->buf + read_buf->left, buf, len);
		vRingbufferReturnItem(rb, buf);
		read_buf->left += len;		
	}	

	read_buf->ptr = read_buf->buf;
	return ESP_OK;	
}
#endif


/* Called by the NXP modifications of libmad. Sets the needed output sample rate. */
void set_dac_sample_rate(int rate)
{
    if(rate != prevRate){
        prevRate = rate;
		Audio_setTxclk(rate);
		Audio_setTxChannel(I2S_CHANNEL_FMT_RIGHT_LEFT);
    }
}


/* render callback for the libmad synth */
void render_sample_block(short *sample_buff_ch0, short *sample_buff_ch1, int num_samples, unsigned int num_channels)
{
	char buf[4];
	for(int i=0; i<num_samples; i++){
		memcpy(buf, sample_buff_ch0+i, 2);
		memcpy(buf+2, sample_buff_ch1+i, 2);
		Audio_i2sWrite(buf, 4, 1000 / portTICK_RATE_MS);
	}
}


//This is the main mp3 decoding task. It will grab data from the input buffer FIFO in the SPI ram and
//output it to the I2S port.
#ifdef CONFIG_AUDIO_MAD
void mad_decode_task(void *pvParameters)
{
    //Allocate structs needed for mp3 decoding
    struct mad_stream* stream = malloc(sizeof(struct mad_stream));
    struct mad_frame* frame = malloc(sizeof(struct mad_frame));
    struct mad_synth* synth = malloc(sizeof(struct mad_synth));
    char* readBuf = malloc(MAINBUF_SIZE);

    if (stream == NULL) {
    	DECODE_DEBUGE(TAG, "stream malloc failed!");
    	goto exit;
    }

    if (frame == NULL) {
    	DECODE_DEBUGE(TAG, "frame malloc failed!");
    	goto exit;
    }

    if (synth == NULL) {
    	DECODE_DEBUGE(TAG, "synth malloc failed!");
    	goto exit;
    }

    if (readBuf==NULL) {
    	DECODE_DEBUGE(TAG, "readbuf malloc failed!");
    	goto exit;
    }

	DECODE_DEBUGI(TAG, "Decoder start.");

    //Initialize mp3 parts
    mad_stream_init(stream);
    mad_frame_init(frame);
    mad_synth_init(synth);

    rb = xRingbufferCreate(RING_BUF_SIZE, RINGBUF_TYPE_BYTEBUF);
	prevRate = 0;

    while(1) {

		vTaskDelay(5 / portTICK_PERIOD_MS);

        if (xEventGroupGetBits(decode_event_group) & DECODE_DESTROY_BIT)
        	break;

        if (xEventGroupGetBits(decode_event_group) & DECODE_SUSPEND_BIT){
			Audio_i2s_clear();
			vTaskDelay(100 / portTICK_PERIOD_MS);
			continue;
		}	

		// calls mad_stream_buffer internally
		int buf_len = input_mad(stream, readBuf, MAINBUF_SIZE);

    	//Okay, let MAD decode the buffer.
    	mad_stream_buffer(stream, (unsigned char*)readBuf, buf_len);

        // decode frames until MAD complains
        while(1) {

            // returns 0 or -1
            if (mad_frame_decode(frame, stream) == -1) {
                if (!MAD_RECOVERABLE(stream->error)) {
                    //We're most likely out of buffer and need to call input() again
                    break;
                }
                error(NULL, stream, frame);
                continue;
            }
            mad_synth_frame(synth, frame);
        }
    }

exit:
	Audio_i2s_clear();
   	vRingbufferDelete(rb);
   	free(synth);
   	free(frame);
   	free(stream);
   	free(readBuf);
	xEventGroupSetBits(decode_event_group, DECODE_PLAY_BIT);	   
   	vTaskDelete(NULL);
}
#endif


#ifdef CONFIG_AUDIO_HELIX
void helix_decode_task(void *pvParameters)
{
	HMP3Decoder hMP3Decoder;
	MP3FrameInfo mp3FrameInfo;
	Mp3Buf_t mp3Buf;
	mp3Buf.buf = malloc(MAINBUF_SIZE);
	mp3Buf.out = malloc(1153*4);
	mp3Buf.ptr = mp3Buf.buf;
	mp3Buf.left = 0;
	hMP3Decoder = MP3InitDecoder();

	if (mp3Buf.buf==NULL) {
		DECODE_DEBUGE(TAG, "readBuf malloc failed");
		goto exit;
	}

	if (mp3Buf.out==NULL) {
		DECODE_DEBUGE(TAG, "outBuf malloc failed");
		goto exit;
	}
	
	if (hMP3Decoder == 0){
		DECODE_DEBUGE(TAG, "MP3Decoder memory is not enough..");
		goto exit;
	}

	DECODE_DEBUGI(TAG, "Decoder start.");

    rb = xRingbufferCreate(RING_BUF_SIZE, RINGBUF_TYPE_BYTEBUF);
	prevRate = 0;

	while (1){    

		vTaskDelay(5 / portTICK_PERIOD_MS);

        if (xEventGroupGetBits(decode_event_group) & DECODE_DESTROY_BIT)
        	break;

        if (xEventGroupGetBits(decode_event_group) & DECODE_SUSPEND_BIT){
			Audio_i2s_clear();
			vTaskDelay(100 / portTICK_PERIOD_MS);
			continue;
		}

		input_helix(&mp3Buf, MAINBUF_SIZE);
	
		int offset = MP3FindSyncWord(mp3Buf.ptr, mp3Buf.left);

		if (offset < 0){  
		//	DECODE_DEBUGE(TAG, "MP3FindSyncWord not find");
			mp3Buf.left = 0;
			Audio_i2s_clear();
			vTaskDelay(10 / portTICK_PERIOD_MS);
			continue;
		}else{
			mp3Buf.ptr += offset;                  //data start point
			mp3Buf.left -= offset;                 //in buffer

			int errs = MP3Decode(hMP3Decoder, &mp3Buf.ptr, &mp3Buf.left, mp3Buf.out, 0);
			if (errs != 0){
				DECODE_DEBUGE(TAG, "MP3Decode failed ,code is %d ",errs);
				mp3Buf.left = 0;
				continue;
			}

			MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo); 
		//	ESP_LOGI(TAG,"mp3file info---bitrate=%d,layer=%d,nChans=%d,samprate=%d,outputSamps=%d",mp3FrameInfo.bitrate,mp3FrameInfo.layer,mp3FrameInfo.nChans,mp3FrameInfo.samprate,mp3FrameInfo.bitsPerSample);  
			set_dac_sample_rate(mp3FrameInfo.samprate);
			if(mp3FrameInfo.nChans < 2){
				render_sample_block(mp3Buf.out, mp3Buf.out, mp3FrameInfo.outputSamps, 2);
			}else{
				Audio_i2sWrite((const char*)mp3Buf.out, mp3FrameInfo.outputSamps*2, 1000 / portTICK_RATE_MS);
			}	
		}		
	}

exit:
	Audio_i2s_clear();
   	vRingbufferDelete(rb);
	free(mp3Buf.buf);
	free(mp3Buf.out);
	MP3FreeDecoder(hMP3Decoder);  
	xEventGroupSetBits(decode_event_group, DECODE_PLAY_BIT);	
    vTaskDelete(NULL);
}
#endif


esp_err_t decode_start()
{
    xEventGroupWaitBits(decode_event_group, DECODE_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
	xEventGroupClearBits(decode_event_group, DECODE_SUSPEND_BIT);
	xEventGroupClearBits(decode_event_group, DECODE_DESTROY_BIT);	

#ifdef CONFIG_AUDIO_MAD
	return xTaskCreatePinnedToCore(mad_decode_task, "mp3_decode_task", 4096, NULL, 5, &xdecodeTask, 1);
#endif
#ifdef CONFIG_AUDIO_HELIX
	return xTaskCreatePinnedToCore(helix_decode_task, "mp3_decode_task", 2048, NULL, 5, &xdecodeTask, 1);
#endif
}


void decode_init()
{
	if(decode_event_group)
        return;

    decode_event_group = xEventGroupCreate();
	xEventGroupSetBits(decode_event_group, DECODE_PLAY_BIT);
}


void decode_deinit()
{
	if(decode_event_group == NULL)
        return;

	xEventGroupSetBits(decode_event_group, DECODE_DESTROY_BIT);
    xEventGroupWaitBits(decode_event_group, DECODE_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

	vEventGroupDelete(decode_event_group);
	decode_event_group = NULL;
}


void decode_suspend(bool state)
{
	if(decode_event_group == NULL)
        return;

	if(state)
		xEventGroupSetBits(decode_event_group, DECODE_SUSPEND_BIT);
	else
		xEventGroupClearBits(decode_event_group, DECODE_SUSPEND_BIT);
}


void decode_destroy()
{
	if(decode_event_group == NULL)
        return;

	xEventGroupSetBits(decode_event_group, DECODE_DESTROY_BIT);	
}


esp_err_t decode_send(const char *send_buf, size_t send_size, TickType_t ticks_to_wait)
{
	if (xRingbufferSend(rb, send_buf, send_size, ticks_to_wait)) {
		return ESP_OK;
	}
	DECODE_DEBUGI(TAG, "Ringbuffer send on timeout!");
	return ESP_FAIL;
}