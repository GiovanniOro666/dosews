import numpy as np

# Carica dati
tempo = np.loadtxt('tempo_cittaducale.txt')
acc = np.loadtxt('base_cittaducale.txt')

# Frequenza
dt = tempo[1] - tempo[0]
fs = 1 / dt
print(f"Δt = {dt:.6f} s")
print(f"Frequenza = {fs:.1f} Hz")

# PGA
pga = np.max(np.abs(acc))
print(f"\nPGA = {pga:.6f}")

# Determina unità
if pga < 2.0:
    print(f"Probabile unità: g (equivalente a {pga*9.81:.2f} m/s²)")
elif pga < 20:
    print(f"Probabile unità: m/s² (equivalente a {pga/9.81:.3f} g)")
else:
    print(f"Probabile unità: m/s² (valore alto!) o cm/s²")

# Plot
import matplotlib.pyplot as plt
plt.figure(figsize=(12, 4))
plt.plot(tempo, acc)
plt.xlabel('Tempo (s)')
plt.ylabel('Accelerazione')
plt.title(f'Accelerogramma - PGA={pga:.3f}')
plt.grid(True)
plt.savefig('accelerogramma.png', dpi=150)
print("\n✓ Grafico salvato: accelerogramma.png")