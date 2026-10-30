@echo off
rem City of Theed - the big droid. Starts with the 7 rescues it requires already credited.
rem You start beside the droid's courtyard; its own intro plays, then the fight starts.
rem Pass: the droid is destroyed and the level ends with the results screen.
rem You stay at full health (--invincible).
cd /d "C:\Games\Star Wars Jedi Power Battles"
start "" "C:\Games\Star Wars Jedi Power Battles\OpenJPB.exe" --quickload theed --spawn-position -26880 6400 -10816 --force-enemy-placement 51 --game-counter 7 --invincible
