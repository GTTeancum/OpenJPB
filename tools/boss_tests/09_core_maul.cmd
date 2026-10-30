@echo off
rem Theed generator core - Darth Maul, final fight. Nothing is forced: you start
rem a short way from him and his own fight starts when you are close.
rem Pass: Maul is beaten and the level ends with the results screen.
rem You stay at full health (--invincible).
cd /d "C:\Games\Star Wars Jedi Power Battles"
start "" "C:\Games\Star Wars Jedi Power Battles\OpenJPB.exe" --quickload core --spawn-position 31104 2560 -14848 --invincible
