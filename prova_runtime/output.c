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

void stampa_report_allarme(const char *soglia_target, double t_trigger, double t_allarme,
                           double pgd_allarme, double pgd_max, double drift_mediano,
                           double soglia_fisica, double prob_calcolata,
                           double soglia_probabilita, double lead_time, int allarme_attivo) {

    printf("\n====== EARLY WARNING SYSTEM ======\n");
    printf("Soglia Monitorata  : %s\n", soglia_target);
    printf("Trigger a          : %.3f s\n", t_trigger);

    if (allarme_attivo) {
        printf("Allarme a          : %.3f s\n", t_allarme);
        printf("PGD all'allarme    : %.6e m\n", pgd_allarme);
        printf("Drift mediano prev.: %.6e m\n", drift_mediano);
        printf("Soglia fisica      : %.6e m\n", soglia_fisica);
        printf("Probabilita calc.  : %.2f %%\n", prob_calcolata);
        printf("Soglia probabilita : %.2f %%\n", soglia_probabilita);
        printf("PGD massimo reale  : %.6e m\n", pgd_max);
        printf("Lead time          : %.3f s\n", lead_time);
        printf(">>> ALLARME: ATTIVO\n");
    } else {
        printf("PGD massimo        : %.6e m\n", pgd_max);
        printf("Soglia fisica      : %.6e m\n", soglia_fisica);
        printf("Soglia probabilita : %.2f %%\n", soglia_probabilita);
        printf(">>> ALLARME: NON SUPERATO\n");
    }
    printf("==================================\n\n");

    FILE *fp = fopen("allarme_report.txt", "w");
    if (fp) {
        fprintf(fp, "====== EARLY WARNING SYSTEM ======\n");
        fprintf(fp, "Soglia Monitorata  : %s\n", soglia_target);
        fprintf(fp, "Trigger a          : %.3f s\n", t_trigger);
        if (allarme_attivo) {
            fprintf(fp, "Allarme a          : %.3f s\n", t_allarme);
            fprintf(fp, "PGD all'allarme    : %.6e m\n", pgd_allarme);
            fprintf(fp, "Drift mediano prev.: %.6e m\n", drift_mediano);
            fprintf(fp, "Soglia fisica      : %.6e m\n", soglia_fisica);
            fprintf(fp, "Probabilita calc.  : %.2f %%\n", prob_calcolata);
            fprintf(fp, "Soglia probabilita : %.2f %%\n", soglia_probabilita);
            fprintf(fp, "PGD massimo reale  : %.6e m\n", pgd_max);
            fprintf(fp, "Lead time          : %.3f s\n", lead_time);
            fprintf(fp, ">>> ALLARME: ATTIVO\n");
        } else {
            fprintf(fp, "PGD massimo        : %.6e m\n", pgd_max);
            fprintf(fp, "Soglia fisica      : %.6e m\n", soglia_fisica);
            fprintf(fp, "Soglia probabilita : %.2f %%\n", soglia_probabilita);
            fprintf(fp, ">>> ALLARME: NON SUPERATO\n");
        }
        fclose(fp);
    }
}
