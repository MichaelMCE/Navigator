
@echo off

SET CPUSPEED=720

REM Arduino location
SET AR_PATH=H:/Compilers/Arduino/Arduino

REM Cache path
SET CACHE_PATH=M:/Temp/arduino_cache

REM Build path
SET BUILD_PATH=M:/Temp/arduino_build

REM Build libary path
SET LIB_PATH=%BUILD_PATH%/libraries

REM Path where sketch is copied to from where it is built
SET SKH_PATH=%BUILD_PATH%/sketch


REM     Lets Go!


REM Removed "-libraries C:\Users\Administrator\Documents\Arduino\libraries"

REM compile 
@%AR_PATH%/arduino-builder -compile -logger=machine -hardware %AR_PATH%\hardware -tools %AR_PATH%\tools-builder -tools %AR_PATH%\hardware\tools\avr -built-in-libraries %AR_PATH%\libraries -fqbn=teensy:avr:teensy41:usb=serial,speed=%CPUSPEED%,opt=o3std,keys=en-us -ide-version=10819 -build-path %BUILD_PATH% -warnings=more -build-cache %CACHE_PATH% F:\Code\Navigator\src\src.ino
