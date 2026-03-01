
@echo off

SET CPUSPEED=720

REM Arduino location
SET AR_PATH=H:/Compilers/Arduino/Arduino

REM Build path
SET BUILD_PATH=M:/Temp/arduino_build

REM Build libary path
SET LIB_PATH=%BUILD_PATH%/libraries

REM Path where sketch is copied to from where it is built
SET SKH_PATH=%BUILD_PATH%/sketch


REM     Lets Go!

REM precompile
@%AR_PATH%/arduino-builder -dump-prefs -logger=machine -hardware %AR_PATH%\hardware -tools %AR_PATH%\tools-builder -tools %AR_PATH%\hardware\tools\avr -built-in-libraries %AR_PATH%\libraries -libraries C:\Users\Administrator\Documents\Arduino\libraries -fqbn=teensy:avr:teensy41:usb=serial,speed=%CPUSPEED%,opt=o3std,keys=en-us -ide-version=10819 -build-path %BUILD_PATH% -warnings=more -build-cache M:\Temp\arduino_cache -verbose F:\Code\Navigator\src\src.ino

REM compile
@%AR_PATH%/arduino-builder -compile -logger=machine -hardware %AR_PATH%\hardware -tools %AR_PATH%\tools-builder -tools %AR_PATH%\hardware\tools\avr -built-in-libraries %AR_PATH%\libraries -libraries C:\Users\Administrator\Documents\Arduino\libraries -fqbn=teensy:avr:teensy41:usb=serial,speed=%CPUSPEED%,opt=o3std,keys=en-us -ide-version=10819 -build-path %BUILD_PATH% -warnings=more -build-cache M:\Temp\arduino_cache -verbose F:\Code\Navigator\src\src.ino

REM link
@"%AR_PATH%/hardware/tools/arm/bin/arm-none-eabi-gcc" -O3 -Wl,--gc-sections,--relax "-T%AR_PATH%\hardware\teensy\avr\cores\teensy4/imxrt1062_t41.ld" -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -o "%BUILD_PATH%/src.ino.elf" "%SKH_PATH%\libvfont.c.o" "%SKH_PATH%\timedate.c.o" "%SKH_PATH%\ubx.c.o" "%SKH_PATH%\FT5216.cpp.o" "%SKH_PATH%\clock.cpp.o" "%SKH_PATH%\cmd.cpp.o" "%SKH_PATH%\displays.cpp.o" "%SKH_PATH%\encoder.cpp.o" "%SKH_PATH%\encoders.cpp.o" "%SKH_PATH%\fileio.cpp.o" "%SKH_PATH%\gps.cpp.o" "%SKH_PATH%\libvfont.cpp.o" "%SKH_PATH%\log.cpp.o" "%SKH_PATH%\map.cpp.o" "%SKH_PATH%\mtp.cpp.o" "%SKH_PATH%\palette.cpp.o" "%SKH_PATH%\poi.cpp.o" "%SKH_PATH%\powersave.cpp.o" "%SKH_PATH%\record.cpp.o" "%SKH_PATH%\scene.cpp.o" "%SKH_PATH%\src.ino.cpp.o" "%SKH_PATH%\tft_display.cpp.o" "%SKH_PATH%\tiles.cpp.o" "%SKH_PATH%\touch.cpp.o" "%SKH_PATH%\tracks.cpp.o" "%SKH_PATH%\ui.cpp.o" "%LIB_PATH%\InternalTemperature\InternalTemperature.cpp.o" "%LIB_PATH%\FlexIO_t4\FlexIOSPI.cpp.o" "%LIB_PATH%\FlexIO_t4\FlexIO_t4.cpp.o" "%LIB_PATH%\FlexIO_t4\FlexSerial.cpp.o" "%LIB_PATH%\SD\SD.cpp.o" "%LIB_PATH%\SdFat\FreeStack.cpp.o" "%LIB_PATH%\SdFat\MinimumSerial.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatDbg.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatFile.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatFilePrint.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatFileWrite.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatFormatter.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatName.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatPartition.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\ExFatVolume.cpp.o" "%LIB_PATH%\SdFat\ExFatLib\upcase.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatDbg.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatFile.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatFileLFN.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatFilePrint.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatFileSFN.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatFormatter.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatName.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatPartition.cpp.o" "%LIB_PATH%\SdFat\FatLib\FatVolume.cpp.o" "%LIB_PATH%\SdFat\FsLib\FsFile.cpp.o" "%LIB_PATH%\SdFat\FsLib\FsNew.cpp.o" "%LIB_PATH%\SdFat\FsLib\FsVolume.cpp.o" "%LIB_PATH%\SdFat\SdCard\SdCardInfo.cpp.o" "%LIB_PATH%\SdFat\SdCard\SdSpiCard.cpp.o" "%LIB_PATH%\SdFat\SdCard\SdioTeensy.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiArtemis.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiChipSelect.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiDue.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiESP.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiParticle.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiSTM32.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiSTM32Core.cpp.o" "%LIB_PATH%\SdFat\SpiDriver\SdSpiTeensy3.cpp.o" "%LIB_PATH%\SdFat\common\FmtNumber.cpp.o" "%LIB_PATH%\SdFat\common\FsCache.cpp.o" "%LIB_PATH%\SdFat\common\FsDateTime.cpp.o" "%LIB_PATH%\SdFat\common\FsName.cpp.o" "%LIB_PATH%\SdFat\common\FsStructs.cpp.o" "%LIB_PATH%\SdFat\common\FsUtf.cpp.o" "%LIB_PATH%\SdFat\common\PrintBasic.cpp.o" "%LIB_PATH%\SdFat\common\upcase.cpp.o" "%LIB_PATH%\SdFat\iostream\StdioStream.cpp.o" "%LIB_PATH%\SdFat\iostream\StreamBaseClass.cpp.o" "%LIB_PATH%\SdFat\iostream\istream.cpp.o" "%LIB_PATH%\SdFat\iostream\ostream.cpp.o" "%LIB_PATH%\SPI\SPI.cpp.o" "%LIB_PATH%\Wire\Wire.cpp.o" "%LIB_PATH%\Wire\WireIMXRT.cpp.o" "%LIB_PATH%\Wire\WireKinetis.cpp.o" "%LIB_PATH%\Wire\utility\twi.c.o" "%BUILD_PATH%/core\core.a" "-L%BUILD_PATH%" -larm_cortexM7lfsp_math -lm -lstdc++

@"%AR_PATH%/hardware/tools/arm/bin/arm-none-eabi-objcopy" -O ihex -j .eeprom --set-section-flags=.eeprom=alloc,load --no-change-warnings --change-section-lma .eeprom=0 "%BUILD_PATH%/src.ino.elf" "%BUILD_PATH%/src.ino.eep"

@"%AR_PATH%/hardware/tools/arm/bin/arm-none-eabi-objcopy" -O ihex -R .eeprom "%BUILD_PATH%/src.ino.elf" "%BUILD_PATH%/src.ino.hex"

@"%AR_PATH%/hardware/tools/teensy_post_compile" -file=src.ino "-path=%BUILD_PATH%" "-tools=%AR_PATH%\hardware/tools/" -board=TEENSY41

@"%AR_PATH%/hardware/tools/stdout_redirect" "%BUILD_PATH%/src.ino.sym" "%AR_PATH%\hardware\teensy/../tools/arm/bin/arm-none-eabi-objdump" -t -C "%BUILD_PATH%/src.ino.elf"

@"%AR_PATH%/hardware/tools/teensy_size" "%BUILD_PATH%/src.ino.elf"

@"%AR_PATH%/hardware/tools/stdout_redirect" "%BUILD_PATH%/src.ino.lst" "%AR_PATH%\hardware\teensy/../tools/arm/bin/arm-none-eabi-objdump" -d -S -C "%BUILD_PATH%/src.ino.elf"

@"%AR_PATH%/hardware/tools/teensy_ports" -L


