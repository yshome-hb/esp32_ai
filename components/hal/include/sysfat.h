#ifndef _SYS_FAT_H_
#define _SYS_FAT_H_

#include "esp_partition.h"


#define SYSFAT_DEBUG

#ifdef SYSFAT_DEBUG
	#define SYSFAT_DEBUGI( tag, format, ... )		ESP_LOGI( tag, format, ##__VA_ARGS__)
	#define SYSFAT_DEBUGW( tag, format, ... )		ESP_LOGW( tag, format, ##__VA_ARGS__)
	#define SYSFAT_DEBUGE( tag, format, ... )		ESP_LOGE( tag, format, ##__VA_ARGS__)
#else
	#define SYSFAT_DEBUGI( tag, format, ... )
	#define SYSFAT_DEBUGW( tag, format, ... )
	#define SYSFAT_DEBUGE( tag, format, ... )
#endif


#define FLASH_SPACE		"/spiflash"
#define SDCARD_SPACE	"/sdcard"


typedef struct {
	const esp_partition_t* esp_partition;
	size_t src_offset;
	size_t data_size;
	uint8_t* hdr_data;
	size_t hdr_size;
	const char* file_name;
	const char* file_type;

}Fat_Data_st;


esp_err_t sys_fat_init();
esp_err_t sys_data_init(uint8_t *data, size_t size, const char *f_name, const char *f_type);
void sys_data_free();
esp_err_t sys_data_write(size_t dst_offset, const void* src, size_t size);
void sys_data_erase();
void sys_data_set_size(size_t size);
Fat_Data_st* sys_data_get();


#endif





