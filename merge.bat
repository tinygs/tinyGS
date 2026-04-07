@echo off
REM Extract version number from tinyGS/src/Status.h
setlocal enabledelayedexpansion

REM Set TEST_API value from parameter, default to 0 if not provided
if "%1"=="" (
    set TEST_API=0
) else (
    set TEST_API=%1
)

set VERSION=
for /f "tokens=4 delims==; " %%A in ('findstr /c:"const uint32_t version" tinyGS\src\Status.h') do (
    set "VERSION=%%A"
)
REM Eliminar comentarios y espacios
for /f "tokens=1 delims=;" %%B in ("!VERSION!") do set VERSION=%%B
set "VERSION=!VERSION: =!"
set build_dir=.pio\build
set esp32_dir=ESP32
set esp32s3_dir=ESP32-S3
set esp32c3_dir=ESP32-C3

set "PLATFORMIO_BUILD_FLAGS=-DTEST_API=%TEST_API%"
platformio run -e %esp32_dir% -e %esp32s3_dir% -e %esp32c3_dir%

esptool.py --chip esp32 merge_bin -o esp32_%VERSION%_merged.bin --flash_mode dio --flash_size 4MB ^
  0x1000  %build_dir%\%esp32_dir%\bootloader.bin ^
  0x8000  %build_dir%\%esp32_dir%\partitions.bin ^
  0x10000 %build_dir%\%esp32_dir%\firmware.bin

esptool.py --chip esp32-s3 merge_bin -o esp32_s3_%VERSION%_merged.bin --flash_mode dio --flash_size 4MB ^
  0x0  %build_dir%\%esp32s3_dir%\bootloader.bin ^
  0x8000  %build_dir%\%esp32s3_dir%\partitions.bin ^
  0x10000 %build_dir%\%esp32s3_dir%\firmware.bin

esptool.py --chip esp32-c3 merge_bin -o esp32_c3_%VERSION%_merged.bin --flash_mode dio --flash_size 4MB ^
  0x1000  %build_dir%\%esp32c3_dir%\bootloader.bin ^
  0x8000  %build_dir%\%esp32c3_dir%\partitions.bin ^
  0x10000 %build_dir%\%esp32c3_dir%\firmware.bin

endlocal