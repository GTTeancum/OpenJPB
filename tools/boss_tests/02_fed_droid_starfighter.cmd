@echo off
rem Federation ship - Droid Starfighter (final boss).
rem Pass: after its last phase the level ends and the results screen shows.
rem You stay at full health (--invincible).
cd /d "C:\Games\Star Wars Jedi Power Battles"
start "" "C:\Games\Star Wars Jedi Power Battles\OpenJPB.exe" --quickload fed --spawn-position -22656 5376 -9856 --force-enemy-placement 166 --invincible
