#include "output.h"
#include <stdio.h>

void salva_dati(const char *filename, const double *data, int n_campioni) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Errore: impossibile creare il file %s\n", filename);
        return;
    }
    
    for (int i = 0; i < n_campioni; i++) {
        fprintf(fp, "%.10e\n", data[i]);
    }
    
    fclose(fp);
}

void stampa_report_allarme(const char *soglia_target, int idx_trigger, double freq,
                          double pgd_max, double drift_mediano, double soglia_fisica,
                          double prob_calcolata, double soglia_probabilita, int allarme_attivo) {
    
    printf("\n====== EARLY WARNING SYSTEM ======\n");
    printf("Soglia Monitorata: %s\n", soglia_target);
    printf("Trigger a: %.3f secondi\n", idx_trigger / freq);
    printf("PGD misurato: %.6e m\n", pgd_max);
    printf("Drift Mediano previsto: %.6e m\n", drift_mediano);
    printf("Soglia fisica: %.6e m\n", soglia_fisica);
    printf("Probabilità calcolata: %.2f %%\n", prob_calcolata);
    printf("Soglia probabilità: %.2f %%\n", soglia_probabilita);
    printf(">>> ALLARME: %s\n", allarme_attivo ? "ATTIVO" : "NON SUPERATO");
    printf("==================================\n\n");
    
    FILE *fp = fopen("allarme_report.txt", "w");
    if (fp) {
        fprintf(fp, "====== EARLY WARNING SYSTEM ======\n");
        fprintf(fp, "Soglia Monitorata: %s\n", soglia_target);
        fprintf(fp, "Trigger a: %.3f secondi\n", idx_trigger / freq);
        fprintf(fp, "PGD misurato: %.6e m\n", pgd_max);
        fprintf(fp, "Drift Mediano previsto: %.6e m\n", drift_mediano);
        fprintf(fp, "Soglia fisica: %.6e m\n", soglia_fisica);
        fprintf(fp, "Probabilità calcolata: %.2f %%\n", prob_calcolata);
        fprintf(fp, "Soglia probabilità: %.2f %%\n", soglia_probabilita);
        fprintf(fp, ">>> ALLARME: %s\n", allarme_attivo ? "ATTIVO" : "NON SUPERATO");
        fclose(fp);
    }
}