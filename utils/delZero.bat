@echo off

for /r %%F in (*) do if %%~zF==0 del "%%F"

