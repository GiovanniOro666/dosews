#ifndef INTEGRAZIONE_H
#define INTEGRAZIONE_H

typedef struct {
    double valore_precedente;  /* ultimo campione di input (per il trapezio) */
    double integrale;          /* valore accumulato */
    int inizializzato;         /* 0 al primo campione */
} StatoIntegratore;

void init_integratore(StatoIntegratore *stato);

double aggiorna_integratore(StatoIntegratore *stato, double campione, double dt);

/* Versione batch: utile per test e validazione offline */
void integra_trapezi(const double *segnale_input, double *segnale_output, int n_campioni, double dt);

#endif
