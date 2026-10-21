"""Local transport fixtures, never substitute for actual microphone recordings."""
import pathlib, socket, struct, subprocess, sys, tempfile, threading, wave, zlib

exe = pathlib.Path(sys.argv[1]).resolve()
pcm = struct.pack('<30000h', *([1000, -1000] * 15000))
def header(seq, crc=None):
    return b'HHW1' + struct.pack('<7I', 123, seq, 30000, 1000,
                                zlib.crc32(pcm) if crc is None else crc, 0, 30000)

with tempfile.TemporaryDirectory() as d, socket.socket() as listener:
    listener.bind(('127.0.0.1', 0)); listener.listen(); listener.settimeout(20)
    errors=[]
    def server():
        try:
            # Truncated transfer is retried, never acknowledged.
            c,_=listener.accept()
            with c: c.sendall(header(1)+pcm[:99])
            # CRC mismatch is discarded and retried.
            c,_=listener.accept()
            with c:
                c.sendall(header(1,0)+pcm)
                assert c.recv(12)==b''
            c,_=listener.accept()
            with c:
                for seq in (1,2):
                    h=header(seq)
                    for part in (h[:5],h[5:],pcm[:123],pcm[123:]): c.sendall(part)
                    ack=b''
                    while len(ack)<12: ack+=c.recv(12-len(ack))
                    assert ack == b'ACK1'+struct.pack('<2I',123,seq)
                    file=pathlib.Path(d)/f'clip_0000007b_{seq:06d}_gaps0.wav'
                    assert file.exists(), 'Acknowledged before final file was present'
        except Exception as e: errors.append(e)
    t=threading.Thread(target=server);t.start()
    result=subprocess.run([str(exe),'--host','127.0.0.1','--port',str(listener.getsockname()[1]),
                           '--output-dir',d,'--max-clips','2'],capture_output=True,timeout=25)
    t.join(2)
    assert result.returncode==0,(result.stdout,result.stderr)
    assert not errors,errors
    assert not t.is_alive()
    files=list(pathlib.Path(d).glob('*.wav'));assert len(files)==2
    assert not list(pathlib.Path(d).glob('*.part'))
    for file in files:
        with wave.open(str(file),'rb') as w:
            assert (w.getnchannels(),w.getsampwidth(),w.getframerate(),w.getnframes())==(1,2,1000,30000)
            assert w.readframes(30000)==pcm
print('PASS: fragmented WAV transfer, truncated/CRC retry, persistent-file-before-ACK, PCM identity')
