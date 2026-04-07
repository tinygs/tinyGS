
#!/bin/sh

# Obtener TEST_API del primer parámetro, por defecto 0
if [ -z "$1" ]; then
  TEST_API=0
else
  TEST_API=$1
fi

# Extraer el número de versión de Status.h
VERSION=$(grep 'const uint32_t version' tinyGS/src/Status.h | sed -E 's/.*= ([0-9]+);.*/\1/')

build_dir=".pio/build"

esp32_dir="ESP32"
esp32s3_dir="ESP32-S3"
esp32c3_dir="ESP32-C3"

export PLATFORMIO_BUILD_FLAGS="-DTEST_API=${TEST_API}"
platformio run -e ${esp32_dir} -e ${esp32s3_dir} -e ${esp32c3_dir}

esptool.py --chip esp32 merge_bin -o esp32_${VERSION}_merged.bin --flash_mode dio --flash_size 4MB \
  0x1000  ${build_dir}/${esp32_dir}/bootloader.bin \
  0x8000  ${build_dir}/${esp32_dir}/partitions.bin \
  0x10000 ${build_dir}/${esp32_dir}/firmware.bin

esptool.py --chip esp32-s3 merge_bin -o esp32_s3_${VERSION}_merged.bin --flash_mode dio --flash_size 4MB \
  0x0  ${build_dir}/${esp32s3_dir}/bootloader.bin \
  0x8000  ${build_dir}/${esp32s3_dir}/partitions.bin \
  0x10000 ${build_dir}/${esp32s3_dir}/firmware.bin

esptool.py --chip esp32-c3 merge_bin -o esp32_c3_${VERSION}_merged.bin --flash_mode dio --flash_size 4MB \
  0x1000  ${build_dir}/${esp32c3_dir}/bootloader.bin \
  0x8000  ${build_dir}/${esp32c3_dir}/partitions.bin \
  0x10000 ${build_dir}/${esp32c3_dir}/firmware.bin
  