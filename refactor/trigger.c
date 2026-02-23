#include "trigger.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int rileva_trigger(double *accelerazione_filtrata, int n_campioni, double frequenza,
                   double sta_sec, double lta_sec, double soglia, int indice_inizio) {
    
    int sta_lunghezza = (int)(sta_sec * frequenza);
    int lta_lunghezza = (int)(lta_sec * frequenza);
    
    if (n_campioni < lta_lunghezza || indice_inizio < lta_lunghezza) {
        return -1;
    }
    
    // Inizializzazione somme con il quadrato
    double sta_somma = 0.0;
    for (int i = indice_inizio - sta_lunghezza; i < indice_inizio; i++) {
        double val = accelerazione_filtrata[i];
        sta_somma += val * val; // <--- MODIFICATO
    }
    
    double lta_somma = 0.0;
    for (int i = indice_inizio - lta_lunghezza; i < indice_inizio; i++) {
        double val = accelerazione_filtrata[i];
        lta_somma += val * val; // <--- MODIFICATO
    }
    
    for (int i = indice_inizio; i < n_campioni; i++) {
        // Calcolo i quadrati per il campione corrente-1 e quelli che escono dalle finestre
        double attuale_sq = accelerazione_filtrata[i-1] * accelerazione_filtrata[i-1];
        double out_sta_sq = accelerazione_filtrata[i - sta_lunghezza - 1] * accelerazione_filtrata[i - sta_lunghezza - 1];
        double out_lta_sq = accelerazione_filtrata[i - lta_lunghezza - 1] * accelerazione_filtrata[i - lta_lunghezza - 1];

        // Aggiornamento somme mobili (sliding window)
        sta_somma += attuale_sq - out_sta_sq; // <--- MODIFICATO
        lta_somma += attuale_sq - out_lta_sq; // <--- MODIFICATO
        
        double sta_media = sta_somma / sta_lunghezza;
        double lta_media = lta_somma / lta_lunghezza;
        
        double rapporto;
        // La soglia di tolleranza può essere leggermente più alta per i quadrati (es. 1e-15)
        if (lta_media > 1e-15) {
            rapporto = sta_media / lta_media;
        } else {
            rapporto = 0.0;
        }
        
        if (rapporto >= soglia) {
            printf("Trigger rilevato all'indice %d (Rapporto Quadrati: %.2f)\n", i, rapporto);
            return i;
        }
    }
    
    return -1;
}