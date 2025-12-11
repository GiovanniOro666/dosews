import numpy as np
import sys

def main():
    if len(sys.argv) != 4:
        print("Uso: {} <file_orig> <file_filt> <freq_Hz>".format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)
    
    file_orig = sys.argv[1]
    file_filt = sys.argv[2]
    fs = float(sys.argv[3])
    
    # Leggi segnali
    sig_orig = np.loadtxt(file_orig)
    sig_filt = np.loadtxt(file_filt)
    
    if sig_orig.ndim == 2:
        sig_orig = sig_orig[:, -1]
    if sig_filt.ndim == 2:
        sig_filt = sig_filt[:, -1]
    
    n = min(len(sig_orig), len(sig_filt))
    sig_orig = sig_orig[:n]
    sig_filt = sig_filt[:n]
    
    # FFT
    fft_orig = np.fft.fft(sig_orig)
    fft_filt = np.fft.fft(sig_filt)
    freqs = np.fft.fftfreq(n, d=1.0/fs)
    
    # Solo frequenze positive 0-5 Hz
    mask = (freqs >= 0) & (freqs <= 5.0)
    freqs = freqs[mask]
    fft_orig = fft_orig[mask]
    fft_filt = fft_filt[mask]
    
    # Rapporto H(f) = |FFT_filt| / |FFT_orig|
    mag_orig = np.abs(fft_orig)
    mag_filt = np.abs(fft_filt)
    
    # Evita divisione per zero
    threshold = 0.01 * np.max(mag_orig)
    H = np.where(mag_orig > threshold, mag_filt / mag_orig, 0)
    
    # Scrivi su file direttamente
    with open('filter_response.txt', 'w', encoding='utf-8') as f:
        for i in range(len(freqs)):
            if H[i] > 0:
                f.write('{:.4f}  {:.6f}\n'.format(freqs[i], H[i]))

if __name__ == "__main__":
    main()