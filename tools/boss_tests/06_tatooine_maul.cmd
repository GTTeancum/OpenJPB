@echo off
rem Tatooine - Darth Maul. Nothing is forced: walk forward and the level's own
rem Anakin cutscene plays, then Maul arrives. He cannot be killed here; hold him
rem off for about a minute.
rem Pass: Maul leaves, the ship flies in, then the results screen shows.
rem You stay at full health (--invincible).
cd /d "C:\Games\Star Wars Jedi Power Battles"
start "" "C:\Games\Star Wars Jedi Power Battles\OpenJPB.exe" --quickload tato --spawn-position -20979 8960 6208 --invincible
