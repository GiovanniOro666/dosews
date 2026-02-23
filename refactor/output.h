#ifndef OUTPUT_H
#define OUTPUT_H

void salva_dati(const char *filename, const double *data, int n_campioni);
void stampa_report_allarme(const char *soglia_target, int idx_trigger, double freq, 
                          double pgd_max, double drift_mediano, double soglia_fisica,
                          double prob_calcolata, double soglia_probabilita, int allarme_attivo);

#endif