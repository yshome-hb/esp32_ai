#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sysfat.h"

#define TAG "SYSFAT:"

#define USE_SPI_MODE

#ifdef USE_SPI_MODE
#define PIN_NUM_MISO 	19
#define PIN_NUM_MOSI 	23
#define PIN_NUM_CLK  	18
#define PIN_NUM_CS   	5
#endif //USE_SPI_MODE

#define PARTITION_NAME      "record"
#define FLASH_SECTOR_SIZE	(0x1000)
#define FLASH_ERASE_SIZE	(FLASH_SECTOR_SIZE * 200)

static Fat_Data_st* fat_data = NULL;


esp_err_t sys_fat_init()
{
#ifndef USE_SPI_MODE
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();    
#else
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();

	slot_config.gpio_miso = PIN_NUM_MISO;
	slot_config.gpio_mosi = PIN_NUM_MOSI;
	slot_config.gpio_sck = PIN_NUM_CLK;
	slot_config.gpio_cs = PIN_NUM_CS;
#endif

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_card_t* card;
    esp_err_t err = esp_vfs_fat_sdmmc_mount(SDCARD_SPACE, &host, &slot_config, &mount_config, &card);

    if (err == ESP_FAIL) {
        SYSFAT_DEBUGE(TAG, "Failed to mount filesystem. "
        	          "If you want the card to be formatted, set format_if_mount_failed = true.");
    } else if (err != ESP_OK) {
        SYSFAT_DEBUGE(TAG, "Failed to initialize the card (%d). "
        	          "Make sure SD card lines have pull-up resistors in place.", err);
    } else{

#ifdef SYSFAT_DEBUG
    sdmmc_card_print_info(stdout, card);
#endif

    }

    return err;
}


esp_err_t sys_data_init(uint8_t *data, size_t size, const char *f_name, const char *f_type)
{
	fat_data = (Fat_Data_st*) malloc(sizeof(Fat_Data_st));
	if(fat_data == NULL){
		SYSFAT_DEBUGI(TAG, "fat_data malloc failed!");
        return ESP_FAIL;
	}

    fat_data->esp_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
            								  ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);
    if (fat_data->esp_partition == NULL) {
        SYSFAT_DEBUGE(TAG, "Partition error: can't find partition name: %s", PARTITION_NAME);
        return ESP_FAIL;
    } else {
        SYSFAT_DEBUGI(TAG, "partiton addr: 0x%08x; size: %d; label: %s", fat_data->esp_partition->address, fat_data->esp_partition->size, fat_data->esp_partition->label);
    }

    SYSFAT_DEBUGI(TAG, "Erase size: %d Bytes", FLASH_ERASE_SIZE);
    ESP_ERROR_CHECK(esp_partition_erase_range(fat_data->esp_partition, 0, FLASH_ERASE_SIZE));
    
	fat_data->src_offset = 0;
	fat_data->data_size = 0;
	fat_data->hdr_data = data;
	fat_data->hdr_size = size;
	fat_data->file_name = f_name;
	fat_data->file_type = f_type;

    return ESP_OK;
}


void sys_data_free()
{
	free(fat_data);
}


esp_err_t sys_data_write(size_t dst_offset, const void* src, size_t size)
{
	return esp_partition_write(fat_data->esp_partition, dst_offset, src, size);
}


void sys_data_erase()
{
	size_t flash_erase_size = ((fat_data->data_size + FLASH_SECTOR_SIZE) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
	ESP_ERROR_CHECK(esp_partition_erase_range(fat_data->esp_partition, 0, flash_erase_size));
}


void sys_data_set_size(size_t size)
{
	fat_data->data_size = size;
}


Fat_Data_st* sys_data_get()
{
	return fat_data;
}

