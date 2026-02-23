#include "integrazione.h"

void init_integratore(StatoIntegratore *stato) {
    stato->valore_precedente = 0.0;
    stato->integrale = 0.0;
    stato->inizializzato = 0;
}

double aggiorna_integratore(StatoIntegratore *stato, double campione, double dt) {
    if (!stato->inizializzato) {
        stato->valore_precedente = campione;
        stato->integrale = 0.0;
        stato->inizializzato = 1;
        return 0.0;
    }

    double area = 0.5 * dt * (stato->valore_precedente + campione);
    stato->integrale += area;
    stato->valore_precedente = campione;

    return stato->integrale;
}

/* Versione batch */
void integra_trapezi(const double *segnale_input, double *segnale_output, int n_campioni, double dt) {
    if (n_campioni <= 0) return;
    segnale_output[0] = 0.0;
    for (int i = 1; i < n_campioni; i++) {
        segnale_output[i] = segnale_output[i-1] + 0.5 * dt * (segnale_input[i-1] + segnale_input[i]);
    }
}
