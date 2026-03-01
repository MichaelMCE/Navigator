
@echo off

SET AR_PATH=H:/Compilers/Arduino/Arduino

@echo %1
"%AR_PATH%/hardware/tools/teensy_post_compile" -v -board=TEENSY41 -path=M:\temp\arduino_build -file=src.ino -tools=H:\Compilers\Arduino\Arduino\hardware\tools -portlabel="%1 (Teensy 4.1) Serial" -reboot




