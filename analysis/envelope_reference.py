import wave, pathlib, json
import numpy as np
root=pathlib.Path(__file__).resolve().parent
z=np.load(root/'spectra.npz')
spec=z['spectra'];freq=z['freq'];hop=float(z['hop']);sr=int(z['sr'])
rows=json.loads((root/'peaks.json').read_text())

print('Stationary pitch classes: relative octave amplitudes, nearest peaks to MIDI notes')
notes=[]
for i,row in enumerate(rows):
    t=row['time']
    if not ((36<t<65) or (227<t<253)) or row['rms']<.03 or not row['peaks']: continue
    ps=row['peaks'];p=ps[0];m=p[2];mc=round(m)%12
    if abs(m-round(m))>.10: continue
    # Single note: at least 95% of energy from strong peaks agrees with octave series.
    strong=[p for p in ps if p[1]>ps[0][1]-15]
    if any(abs(((p[2]-mc+6)%12)-6)>.20 for p in strong): continue
    heights=[]
    for note in range(mc+24,mc+133,12):
        f=440*2**((note-69)/12)
        k=round(f*16384/sr)
        if k<=1 or k>=len(freq)-2: continue
        # Hann FFT magnitude with true frequency interpolation correction.
        ii=np.argmax(spec[i,k-1:k+2])+k-1
        y=np.log(np.maximum(spec[i,ii-1:ii+2],1e-12))
        shift=.5*(y[0]-y[2])/(y[0]-2*y[1]+y[2])
        peak=y[1]-.25*(y[0]-y[2])*shift
        h=20/np.log(10)*peak
        heights.append([note,f,h])
    maxh=max(h[2] for h in heights)
    notes.append([t,mc,[[n,f,h-maxh] for n,f,h in heights]])
for mc in range(12):
    vals=[v for t,m,v in notes if m==mc and 36<t<65]
    if not vals: continue
    arr=np.array(vals)
    print('C C# D D# E F F# G G# A A# B'.split()[mc],len(vals),' '.join(f'{f:.1f}:{h:.1f}dB' for n,f,h in zip(arr[0,:,0],arr[0,:,1],np.median(arr[:,:,2],axis=0)) if h>-50))
allpoints=[]
for t,mc,vals in notes:
    if not 36<t<65: continue
    for n,f,h in vals:
        if h>-35 and f>60 and f<10000: allpoints.append([np.log2(f/440),h,mc])
pts=np.array(allpoints)
# Include one nuisance intercept per pitch class (normalization differs by class), shared curvature/centre.
X=np.column_stack([pts[:,0]**2,pts[:,0],np.eye(12)[pts[:,2].astype(int)]])
coef=np.linalg.lstsq(X,pts[:,1],rcond=None)[0]
err=pts[:,1]-X@coef
center=-coef[1]/(2*coef[0]);sigma=np.sqrt(-20/(2*np.log(10)*coef[0]))
print('Pooled quadratic log-frequency fit:',coef[:2],'centre',440*2**center,'sigma octaves',sigma,'RMS dB error',np.sqrt(np.mean(err**2)))
(root/'octave_relative_levels.json').write_text(json.dumps(notes))

from PIL import Image,ImageDraw
width=1700;height=720
im=Image.new('RGB',(width,height),'white');draw=ImageDraw.Draw(im)
ta=28;tb=65;fmin=50;fmax=12000
x0=70;x1=width-20;y0=40;y1=height-60
times=np.array([r['time'] for r in rows])
logfs=np.geomspace(fmax,fmin,y1-y0)
out=np.zeros((y1-y0,x1-x0,3),np.uint8)
for xx in range(x1-x0):
    t=ta+(tb-ta)*xx/(x1-x0)
    idx=np.argmin(abs(times-t))
    db=20*np.log10(np.maximum(spec[idx],1e-9))
    v=np.interp(logfs,freq,db)
    q=np.clip((v+65)/50,0,1)
    out[:,xx,0]=(255*q).astype(np.uint8)
    out[:,xx,1]=(255*q**2).astype(np.uint8)
    out[:,xx,2]=(255*q**5).astype(np.uint8)
im.paste(Image.fromarray(out),(x0,y0))
for f in [55,110,220,440,880,1760,3520,7040]:
    yy=y0+(np.log2(fmax/f)/np.log2(fmax/fmin))*(y1-y0)
    draw.line((x0,yy,x1,yy),fill=(80,80,100))
    draw.text((3,yy-5),str(f))
for t in range(28,66):
    xx=x0+(t-ta)/(tb-ta)*(x1-x0)
    draw.text((xx-5,y1+10),str(t))
draw.text((70,10),'Reference spectrogram: 28-65 s, 50 Hz-12 kHz, dB floor -65; no playback')
im.save(root/'spectrogram_28_65.png')
