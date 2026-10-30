@echo off
rem Coruscant - thug boss (arrives by taxi, jumps off onto the platform).
rem Pass: he arrives, is beaten, and the level ends with the results screen.
rem You stay at full health (--invincible).
cd /d "C:\Games\Star Wars Jedi Power Battles"
start "" "C:\Games\Star Wars Jedi Power Battles\OpenJPB.exe" --quickload corus2 --spawn-position 10624 13445 900 --force-enemy-placement 60 --invincible
