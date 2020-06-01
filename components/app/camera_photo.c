#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "camera.h"
#include "bitmap.h"
#include "bt_speaker.h"
#include "wifi.h"
#include "http_server.h"
#include "camera_photo.h"


#define TAG "CAM_PHOTO:"

#define CAMERA_PIXEL_FORMAT CAMERA_PF_GRAYSCALE
#define CAMERA_FRAME_SIZE CAMERA_FS_SVGA

static const char* STREAM_CONTENT_TYPE =
        "multipart/x-mixed-replace; boundary=123456789000000000000987654321";

static const char* STREAM_BOUNDARY = "--123456789000000000000987654321";


static esp_err_t write_frame(http_context_t http_ctx)
{
    http_buffer_t fb_data = {
            .data = camera_get_fb(),
            .size = camera_get_data_size(),
            .data_is_persistent = true
    };
    return http_response_write(http_ctx, &fb_data);
}


static void handle_grayscale_pgm(http_context_t http_ctx, void* ctx)
{
    esp_err_t err = camera_run();
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
        return;
    }
    char* pgm_header_str;
    asprintf(&pgm_header_str, "P5 %d %d %d\n",
            camera_get_fb_width(), camera_get_fb_height(), 255);
    if (pgm_header_str == NULL) {
        return;
    }

    size_t response_size = strlen(pgm_header_str) + camera_get_data_size();
    http_response_begin(http_ctx, 200, "image/x-portable-graymap", response_size);
    http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.pgm");
    http_buffer_t pgm_header = { .data = pgm_header_str };
    http_response_write(http_ctx, &pgm_header);
    free(pgm_header_str);

    write_frame(http_ctx);
    http_response_end(http_ctx);
}

static void handle_rgb_bmp(http_context_t http_ctx, void* ctx)
{
    esp_err_t err = camera_run();
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
        return;
    }

    bitmap_header_t* header = bmp_create_header(camera_get_fb_width(), camera_get_fb_height());
    if (header == NULL) {
        return;
    }

    http_response_begin(http_ctx, 200, "image/bmp", sizeof(*header) + camera_get_data_size());
    http_buffer_t bmp_header = {
            .data = header,
            .size = sizeof(*header)
    };
    http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.bmp");
    http_response_write(http_ctx, &bmp_header);
    free(header);

    write_frame(http_ctx);
    http_response_end(http_ctx);
}

static void handle_jpg(http_context_t http_ctx, void* ctx)
{
    esp_err_t err = camera_run();
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
        return;
    }

    http_response_begin(http_ctx, 200, "image/jpeg", camera_get_data_size());
    http_response_set_header(http_ctx, "Content-disposition", "inline; filename=capture.jpg");
    write_frame(http_ctx);
    http_response_end(http_ctx);
}


static void handle_rgb_bmp_stream(http_context_t http_ctx, void* ctx)
{
    http_response_begin(http_ctx, 200, STREAM_CONTENT_TYPE, HTTP_RESPONSE_SIZE_UNKNOWN);
    bitmap_header_t* header = bmp_create_header(camera_get_fb_width(), camera_get_fb_height());
    if (header == NULL) {
        return;
    }
    http_buffer_t bmp_header = {
            .data = header,
            .size = sizeof(*header)
    };


    while (true) {
        esp_err_t err = camera_run();
        if (err != ESP_OK) {
            ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
            return;
        }

        err = http_response_begin_multipart(http_ctx, "image/bitmap",
                camera_get_data_size() + sizeof(*header));
        if (err != ESP_OK) {
            break;
        }
        err = http_response_write(http_ctx, &bmp_header);
        if (err != ESP_OK) {
            break;
        }
        err = write_frame(http_ctx);
        if (err != ESP_OK) {
            break;
        }
        err = http_response_end_multipart(http_ctx, STREAM_BOUNDARY);
        if (err != ESP_OK) {
            break;
        }
    }

    free(header);
    http_response_end(http_ctx);
}

static void handle_jpg_stream(http_context_t http_ctx, void* ctx)
{
    http_response_begin(http_ctx, 200, STREAM_CONTENT_TYPE, HTTP_RESPONSE_SIZE_UNKNOWN);

    while (true) {
        esp_err_t err = camera_run();
        if (err != ESP_OK) {
            ESP_LOGD(TAG, "Camera capture failed with error = %d", err);
            return;
        }
        err = http_response_begin_multipart(http_ctx, "image/jpg",
                camera_get_data_size());
        if (err != ESP_OK) {
            break;
        }
        err = write_frame(http_ctx);
        if (err != ESP_OK) {
            break;
        }
        err = http_response_end_multipart(http_ctx, STREAM_BOUNDARY);
        if (err != ESP_OK) {
            break;
        }
    }
    http_response_end(http_ctx);
}



static void camera_photo_task(void* pvParameters)
{
    ip4_addr_t* s_ip_addr = (ip4_addr_t *)pvParameters;

    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    camera_config_t camera_config = {
       .ledc_channel = LEDC_CHANNEL_0,
       .ledc_timer = LEDC_TIMER_0,
       .pin_d0 = CONFIG_CAM_D0,
       .pin_d1 = CONFIG_CAM_D1,
       .pin_d2 = CONFIG_CAM_D2,
       .pin_d3 = CONFIG_CAM_D3,
       .pin_d4 = CONFIG_CAM_D4,
       .pin_d5 = CONFIG_CAM_D5,
       .pin_d6 = CONFIG_CAM_D6,
       .pin_d7 = CONFIG_CAM_D7,
       .pin_xclk = CONFIG_CAM_XCLK,
       .pin_pclk = CONFIG_CAM_PCLK,
       .pin_vsync = CONFIG_CAM_VSYNC,
       .pin_href = CONFIG_CAM_HREF,
       .pin_reset = CONFIG_CAM_RESET,
       .xclk_freq_hz = CONFIG_XCLK_FREQ,
    };

    camera_model_t camera_model;
    esp_err_t err = camera_probe(&camera_config, &camera_model);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera probe failed with error 0x%x", err);
        return;
    }

    if (camera_model == CAMERA_OV7725) {
        camera_config.pixel_format = CAMERA_PIXEL_FORMAT;
        camera_config.frame_size = CAMERA_FRAME_SIZE;
        ESP_LOGI(TAG, "Detected OV7725 camera, using %s bitmap format",
                CAMERA_PIXEL_FORMAT == CAMERA_PF_GRAYSCALE ?
                        "grayscale" : "RGB565");
    } else if (camera_model == CAMERA_OV2640) {
        ESP_LOGI(TAG, "Detected OV2640 camera, using JPEG format");
        camera_config.pixel_format = CAMERA_PF_JPEG;
        camera_config.frame_size = CAMERA_FRAME_SIZE;
        camera_config.jpeg_quality = 15;
    } else {
        ESP_LOGE(TAG, "Camera not supported");
        return;
    }

    err = camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }


    http_server_t server;
    http_server_options_t http_options = HTTP_SERVER_OPTIONS_DEFAULT();
    ESP_ERROR_CHECK( http_server_start(&http_options, &server) );

    if (camera_config.pixel_format == CAMERA_PF_GRAYSCALE) {
        ESP_ERROR_CHECK( http_register_handler(server, "/pgm", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_grayscale_pgm, NULL) );
        ESP_LOGI(TAG, "Open http://" IPSTR "/pgm for a single image/x-portable-graymap image", IP2STR(s_ip_addr));
    }
    if (camera_config.pixel_format == CAMERA_PF_RGB565) {
        ESP_ERROR_CHECK( http_register_handler(server, "/bmp", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_rgb_bmp, NULL) );
        ESP_LOGI(TAG, "Open http://" IPSTR "/bmp for single image/bitmap image", IP2STR(s_ip_addr));
        ESP_ERROR_CHECK( http_register_handler(server, "/bmp_stream", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_rgb_bmp_stream, NULL) );
        ESP_LOGI(TAG, "Open http://" IPSTR "/bmp_stream for multipart/x-mixed-replace stream of bitmaps", IP2STR(s_ip_addr));
    }
    if (camera_config.pixel_format == CAMERA_PF_JPEG) {
        ESP_ERROR_CHECK( http_register_handler(server, "/jpg", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_jpg, NULL) );
        ESP_LOGI(TAG, "Open http://" IPSTR "/jpg for single image/jpg image", IP2STR(s_ip_addr));
        ESP_ERROR_CHECK( http_register_handler(server, "/jpg_stream", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_jpg_stream, NULL) );
        ESP_LOGI(TAG, "Open http://" IPSTR "/jpg_stream for multipart/x-mixed-replace stream of JPEGs", IP2STR(s_ip_addr));
        ESP_ERROR_CHECK( http_register_handler(server, "/", HTTP_GET, HTTP_HANDLE_RESPONSE, &handle_jpg_stream, NULL) );
    }
    ESP_LOGI(TAG, "Free heap: %u", xPortGetFreeHeapSize());
    ESP_LOGI(TAG, "Camera demo ready");
}


void camera_photo_init()
{
    bt_speaker_deinit();
    wifi_sta_init();
    wifi_set_config("xiaomi_Wrt", "xiaomiwrt");
    tcpip_adapter_ip_info_t ip = wifi_waitConnect();
		
    xTaskCreate(camera_photo_task, "camera_photo_task", 4096, (void*)&ip.ip, 5, NULL);
}
