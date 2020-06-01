deps_config := \
	/mnt/d/esp32_idf/esp-idf/components/app_trace/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/aws_iot/Kconfig \
	/mnt/d/esp32_idf/esp32_ai/components/bsp/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/bt/Kconfig \
	/mnt/d/esp32_idf/esp32_ai/components/camera/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/driver/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/esp32/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/esp_adc_cal/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/esp_http_client/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/espmqtt/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/ethernet/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/fatfs/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/freertos/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/heap/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/libsodium/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/log/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/lwip/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/mbedtls/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/mdns/Kconfig \
	/mnt/d/esp32_idf/esp32_ai/components/mp3_decode/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/openssl/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/pthread/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/spi_flash/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/spiffs/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/tcpip_adapter/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/vfs/Kconfig \
	/mnt/d/esp32_idf/esp-idf/components/wear_levelling/Kconfig \
	/mnt/d/esp32_idf/esp-idf/Kconfig.compiler \
	/mnt/d/esp32_idf/esp-idf/components/bootloader/Kconfig.projbuild \
	/mnt/d/esp32_idf/esp-idf/components/esptool_py/Kconfig.projbuild \
	/mnt/d/esp32_idf/esp32_ai/main/Kconfig.projbuild \
	/mnt/d/esp32_idf/esp-idf/components/partition_table/Kconfig.projbuild \
	/mnt/d/esp32_idf/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
