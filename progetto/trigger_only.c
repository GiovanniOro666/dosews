#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/**
 * Rileva il trigger sismico (onda P) usando l'algoritmo STA/LTA ottimizzato.
 * * @param acc_filt     Array dell'accelerazione filtrata (m/s^2)
 * @param n_samples    Numero totale di campioni
 * @param fs           Frequenza di campionamento (Hz)
 * @param sta_sec      Durata finestra Short Time Average (es. 0.5s)
 * @param lta_sec      Durata finestra Long Time Average (es. 6.0s)
 * @param threshold    Soglia di attivazione (es. 4.0)
 * @param start_idx    Indice da cui iniziare l'analisi (solitamente >= lta_len)
 * @param trigger_idx  Puntatore dove salvare l'indice del trigger trovato
 * @return             1 se trigger rilevato, 0 altrimenti
 */
int rileva_trigger(const double *acc_filt, int n_samples, double fs,
                   double sta_sec, double lta_sec, double threshold,
                   int start_idx, int *trigger_idx) {
    
    int sta_len = (int)(sta_sec * fs);
    int lta_len = (int)(lta_sec * fs);
    
    // Controllo sicurezza: abbiamo abbastanza campioni per la finestra LTA?
    if (n_samples < lta_len || start_idx < lta_len) {
        return 0;
    }

    double sta_sum = 0.0;
    double lta_sum = 0.0;
    *trigger_idx = -1;

    // 1. Inizializzazione somme per la posizione di partenza
    for (int i = start_idx - sta_len + 1; i <= start_idx; i++) {
        sta_sum += fabs(acc_filt[i]);
    }
    for (int i = start_idx - lta_len + 1; i <= start_idx; i++) {
        lta_sum += fabs(acc_filt[i]);
    }

    // 2. Loop a finestra mobile (Sliding Window)
    for (int i = start_idx + 1; i < n_samples; i++) {
        // Aggiorna STA sum: aggiungi il nuovo valore assoluto, togli il più vecchio
        sta_sum += fabs(acc_filt[i]) - fabs(acc_filt[i - sta_len]);
        
        // Aggiorna LTA sum: aggiungi il nuovo valore assoluto, togli il più vecchio
        lta_sum += fabs(acc_filt[i]) - fabs(acc_filt[i - lta_len]);

        // Calcolo medie
        double sta = sta_sum / sta_len;
        double lta = lta_sum / lta_len;

        // Calcolo Ratio (evitando divisioni per zero o valori troppo piccoli)
        double ratio = (lta > 1e-12) ? (sta / lta) : 0.0;

        // Se il rapporto supera la soglia, abbiamo il trigger
        if (ratio > threshold) {
            *trigger_idx = i;

            printf("\n[TRIGGER] Evento rilevato!\n");
            printf(" > Tempo:   %.3f s\n", i / fs);
            printf(" > Ratio:   %.3f\n", ratio);
            printf(" > Soglia:  %.2f\n", threshold);
            return 1;
        }
    }

    return 0; // Nessun trigger trovato nel segnale
}