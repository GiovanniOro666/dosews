import numpy as np

fs = 100
durata = 30
n = int(fs * durata)
t = np.linspace(0, durata, n)

acc = np.random.normal(0, 0.0005, n)

# Evento molto debole a t=10s
t_evento = 10
idx_evento = int(t_evento * fs)
for i in range(idx_evento, idx_evento + 150):
    if i < n:
        dt = (i - idx_evento) / fs
        acc[i] += 0.01 * np.exp(-dt**2 / 0.5) * np.sin(2*np.pi*2*dt)

with open('evento_molto_debole.txt', 'w') as f:
    for a in acc:
        f.write(f"{a:.6e}\n")

print("Generato evento_molto_debole.txt")