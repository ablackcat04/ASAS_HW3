import pathlib,wave
import numpy as np
root=pathlib.Path(__file__).resolve().parent
with wave.open(str(root/'reference_mono.wav'),'rb') as w:
    sr=w.getframerate();a=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float)/32768

def envelope(ta,tb,f):
    start=int(ta*sr);seg=a[start:int(tb*sr)]
    z=seg*np.exp(-2j*np.pi*f*np.arange(len(seg))/sr)
    n=int(.015*sr);pre=np.r_[0j,np.cumsum(z)];sm=(pre[n:]-pre[:-n])/n
    n2=int(.01*sr);pre=np.r_[0j,np.cumsum(sm)];sm=(pre[n2:]-pre[:-n2])/n2
    times=(np.arange(len(sm))+(n+n2)/2+start)/sr
    return times,2*abs(sm)

for ta,tb,f in [(28.8,29.7,1046.5),(119,123,1048),(141,142.7,1480.4),(35.6,36.5,1976.3)]:
    t,e=envelope(ta,tb,f)
    print('SEGMENT',ta,tb,'carrier',f)
    for tt in np.arange(ta+.05,tb,.05):
        ix=np.argmin(abs(t-tt));print(f'{t[ix]:.3f} {20*np.log10(max(e[ix],1e-12)):.2f}')
