@echo off

for /r %%F in (*) do if %%~zF==128 del "%%F"

