@echo off
rem Ruins - Gungan security chief. Nothing is forced: walk to the three Gungan
rem guards nearby and beat them; the chief's cutscene then plays and he attacks.
rem Pass: he is beaten and the level ends with the results screen.
rem You stay at full health (--invincible).
cd /d "C:\Games\Star Wars Jedi Power Battles"
start "" "C:\Games\Star Wars Jedi Power Battles\OpenJPB.exe" --quickload ruins --spawn-position -29056 3072 -14976 --invincible
