

@echo off

del auto.ubx
serial1_console.exe auto fdelete auto.ubx
serial1_console.exe auto ufetch token:your_22_digit_token:auto.ubx
serial1_console.exe auto fsend auto.ubx
serial1_console.exe auto uload auto.ubx


