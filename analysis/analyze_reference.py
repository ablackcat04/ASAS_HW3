import wave, pathlib, json
import numpy as np

root = pathlib.Path(__file__).resolve().parent
with wave.open(str(root/'reference_mono.wav'),'rb') as w:
    sr=w.getframerate()
    a=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(np.float64)/32768

N=16384
hop=2205
win=np.hanning(N)
freq=np.fft.rfftfreq(N,1/sr)
rows=[]
spectra=[]
for start in range(0,len(a)-N,hop):
    seg=a[start:start+N]
    spec=np.abs(np.fft.rfft(seg*win))/(win.sum()/2)
    # Local maxima in audible instrument range.
    ix=np.where((spec[1:-1]>spec[:-2])&(spec[1:-1]>=spec[2:]))[0]+1
    ix=ix[(freq[ix]>40)&(freq[ix]<16000)]
    ix=ix[np.argsort(spec[ix])[-20:][::-1]]
    peaks=[]
    for k in ix[:12]:
        y=np.log(np.maximum(spec[k-1:k+2],1e-12))
        shift=.5*(y[0]-y[2])/(y[0]-2*y[1]+y[2])
        f=(k+shift)*sr/N
        midi=69+12*np.log2(f/440)
        peaks.append([round(f,3),round(float(20*np.log10(max(spec[k],1e-12))),2),round(float(midi),3)])
    rows.append(dict(time=round((start+N/2)/sr,4),rms=round(float(np.sqrt(np.mean(seg*seg))),6),peaks=peaks))
    spectra.append(spec.astype(np.float32))
np.savez_compressed(root/'spectra.npz', spectra=np.asarray(spectra),freq=freq,hop=hop,sr=sr)
(root/'peaks.json').write_text(json.dumps(rows))
for t in np.arange(0,len(a)/sr,2):
    row=rows[min(range(len(rows)),key=lambda i:abs(rows[i]['time']-t))]
    print(f"{row['time']:6.2f} rms {row['rms']:.4f} "+' '.join(f"{p[0]:6.1f}/{p[1]:5.1f}({p[2]:4.1f})" for p in row['peaks'][:6]))
