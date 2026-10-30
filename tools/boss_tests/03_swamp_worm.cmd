@echo off
rem Swamps of Naboo - giant worm.
rem Pass: the worm dies; if the level ends here the results screen shows.
rem You stay at full health (--invincible).
cd /d "C:\Games\Star Wars Jedi Power Battles"
start "" "C:\Games\Star Wars Jedi Power Battles\OpenJPB.exe" --quickload marsh --spawn-position -30800 4096 7744 --force-enemy-placement 79 --invincible
