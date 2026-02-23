#ifndef TRIGGER_H
#define TRIGGER_H

int rileva_trigger(double *accelerazione_filtrata, int n_campioni, double frequenza,
                   double sta_sec, double lta_sec, double soglia, int indice_inizio);

#endif