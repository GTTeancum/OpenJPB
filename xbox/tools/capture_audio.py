"""Capture only XEMU's emulated AC97 output via its native WAV backend.

Uses the isolated Xbox configuration and its process-local smoke input.
No host input, microphones, desktop capture, or OS audio loopback.
The PC game and its assets are not touched.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import time
import tomllib
import wave
from array import array


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, default=Path('C:/Games/Emulators/Xemu/xemu.exe'))
    p.add_argument('--config', type=Path, default=Path('xbox/build/xemu/xemu.toml'))
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--seconds', type=int, default=60)
    p.add_argument('--port', type=int, default=9248)
    p.add_argument('--callback-address', type=lambda value: int(value, 0))
    p.add_argument('--mix-capture-address', type=lambda value: int(value, 0))
    p.add_argument('--mix-frames-address', type=lambda value: int(value, 0))
    a = p.parse_args()
    if a.exe.name.lower() != 'xemu.exe' or not 1 <= a.seconds <= 600:
        p.error('Use xemu.exe and a capture duration from 1 to 600 seconds')
    raw_config = a.config.read_text()
    config = tomllib.loads(raw_config)
    inputs = config.get('input', {})
    if inputs.get('auto_bind', True) or inputs.get('background_input_capture', False):
        p.error('An isolated configuration without host input is required')
    if any(inputs.get('bindings', {}).get(f'port{i}', '') for i in range(1, 5)):
        p.error('Host controller bindings must be empty')
    if config.get('display', {}).get('quality', {}).get('surface_scale') != 2:
        p.error('This review harness requires the requested 2x resolution')
    a.out = a.out.resolve()
    a.out.mkdir(parents=True, exist_ok=True)
    wav_path = a.out / 'analog-output.wav'
    if wav_path.exists():
        p.error('Use a new output directory to preserve previous evidence')
    # Keep even emulator settings/EEPROM writes within this capture directory.
    eeprom = a.out / 'eeprom.bin'
    shutil.copyfile(config['sys']['files']['eeprom_path'], eeprom)
    raw_config = re.sub(r'(?m)^eeprom_path\s*=.*$', f"eeprom_path='{eeprom.as_posix()}'", raw_config)
    if 'background_input_capture' not in inputs:
        raw_config = raw_config.replace('[input]', '[input]\nbackground_input_capture=false')
    capture_config = a.out / 'xemu.toml'
    capture_config.write_text(raw_config)
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    command = [str(a.exe.resolve()), '-config_path', str(capture_config), '-snapshot',
               '-monitor', f'tcp:127.0.0.1:{a.port},server,nowait',
               '-audio', f'wav,id=capture,path={wav_path.as_posix()},out.frequency=48000,out.channels=2,out.format=s16']
    guest_environment = os.environ.copy()
    guest_environment['TEMP'] = str(a.out)
    guest_environment['TMP'] = str(a.out)
    with (a.out/'stdout.log').open('w') as stdout, (a.out/'stderr.log').open('w') as stderr:
        process = subprocess.Popen(command, cwd=a.out, startupinfo=startup,
                                   stdout=stdout, stderr=stderr,
                                   env=guest_environment)
        print(f'Native AC97 capture PID {process.pid}', flush=True)
        try:
            deadline = time.monotonic() + a.seconds
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    raise RuntimeError(f'XEMU exited early ({process.returncode}); inspect stderr.log')
                time.sleep(.5)
        finally:
            if process.poll() is None:
                try:
                    with socket.create_connection(('127.0.0.1', a.port), timeout=2) as monitor:
                        banner=b''
                        while not banner.endswith(b'(qemu) '):
                            block=monitor.recv(8192)
                            if not block:
                                raise OSError('Monitor closed before its prompt')
                            banner+=block
                        if a.callback_address is not None:
                            monitor.sendall(f'x /1wx {a.callback_address:#x}\n'.encode())
                            answer=b''
                            while not answer.endswith(b'(qemu) '):
                                answer+=monitor.recv(8192)
                            match=re.search(rb':\s*0x([0-9a-fA-F]+)', answer)
                            if match:
                                callback_count=int(match.group(1),16)
                        if a.mix_capture_address is not None and a.mix_frames_address is not None:
                            monitor.sendall(f'x /1wx {a.mix_frames_address:#x}\n'.encode())
                            answer=b''
                            while not answer.endswith(b'(qemu) '):
                                answer+=monitor.recv(8192)
                            match=re.search(rb':\s*0x([0-9a-fA-F]+)', answer)
                            if not match:
                                raise RuntimeError('Could not read in-game mix capture length')
                            mix_frames=int(match.group(1),16)
                            if not 0 < mix_frames <= 144000:
                                raise RuntimeError(f'Invalid in-game mix capture length: {mix_frames}')
                            mix_path=a.out/'mix-output.pcm'
                            monitor.sendall(f'memsave {a.mix_capture_address:#x} {mix_frames*4} "{mix_path.as_posix()}"\n'.encode())
                            answer=b''
                            while not answer.endswith(b'(qemu) '):
                                answer+=monitor.recv(8192)
                            if not mix_path.exists() or mix_path.stat().st_size!=mix_frames*4:
                                raise RuntimeError(f'Guest mix capture failed: {answer[-500:]}')
                        monitor.sendall(b'quit\n')  # Finalizes WAV header; emulator-local command.
                        try:
                            while monitor.recv(8192):
                                pass
                        except (TimeoutError, ConnectionResetError):
                            pass
                    process.wait(timeout=10)
                except (OSError, subprocess.TimeoutExpired):
                    process.terminate()
                    process.wait(timeout=10)
    with wave.open(str(wav_path), 'rb') as wav:
        if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate()) != (2, 2, 48000):
            raise RuntimeError('Unexpected native output format')
        pcm = array('h', wav.readframes(wav.getnframes()))
        report = dict(frames=wav.getnframes(), seconds=wav.getnframes()/48000,
                      peak=max(map(abs, pcm), default=0),
                      nonzero_samples=sum(x != 0 for x in pcm),
                      clipped_samples=sum(x in (-32768, 32767) for x in pcm),
                      note='Signal evidence only; does not certify sound identity, timing, or listening quality.')
        if 'callback_count' in locals():
            report['mixed_callback_frames']=callback_count*1024
        if 'mix_frames' in locals():
            report['captured_mix_frames']=mix_frames
    (a.out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report), flush=True)


if __name__ == '__main__':
    main()
