#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "filter.h"
#include "trigger.h"
#include "output.h"
#include "integrazione.h"
#include "allarme.h"

#define G 9.81
#define FREQUENZA 200.0
#define SOGLIA_DANNO "EDS"
#define N_PIANI 3

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <file_accelerometrico>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Errore: impossibile aprire il file\n");
        return 1;
    }

    int n_campioni = 0;
    double value;
    while (fscanf(fp, "%lf", &value) == 1) {
        n_campioni++;
    }
    rewind(fp);

    double *acc_data = malloc(n_campioni * sizeof(double));
    double *acc_hp = malloc(n_campioni * sizeof(double));
    double *acc_filtrata = malloc(n_campioni * sizeof(double));

    int i = 0;
    while (fscanf(fp, "%lf", &value) == 1) {
        acc_data[i++] = value * G;
    }
    fclose(fp);

    salva_dati("acc_convertita.txt", acc_data, n_campioni);

    double a0_hp, a1_hp, a2_hp, b1_hp, b2_hp;
    double a0_lp, a1_lp, a2_lp, b1_lp, b2_lp;
    
    calcola_coeff_highpass(FREQUENZA, 0.075, &a0_hp, &a1_hp, &a2_hp, &b1_hp, &b2_hp);
    calcola_coeff_lowpass(FREQUENZA, 1.0, &a0_lp, &a1_lp, &a2_lp, &b1_lp, &b2_lp);

    filtro_highpass(acc_data, acc_filtrata, n_campioni, a0_hp, a1_hp, a2_hp, b1_hp, b2_hp);
    salva_dati("acc_hp.txt", acc_hp, n_campioni);

    //filtro_lowpass(acc_hp, acc_filtrata, n_campioni, a0_lp, a1_lp, a2_lp, b1_lp, b2_lp);
    //salva_dati("acc_lp.txt", acc_filtrata, n_campioni);

    int indice_trigger = rileva_trigger(acc_filtrata, n_campioni, FREQUENZA, 0.5, 6.0, 4, 1200);
    
    if (indice_trigger >= 0) {
        double vel = 0.0;
        double vel_filt = 0.0;
        double vel_filt_prev = 0.0;
        double spost = 0.0;
        double dt = 1.0 / FREQUENZA;
        double pgd_max = 0.0;
        int indice_pgd_max = indice_trigger;
        int allarme_attivo = 0;
        int indice_allarme = -1;
        double pgd_allarme = 0.0;
        
        double x1_vel = 0.0, x2_vel = 0.0, y1_vel = 0.0, y2_vel = 0.0;
        double x1_spost = 0.0, x2_spost = 0.0, y1_spost = 0.0, y2_spost = 0.0;
        
        for (int i = indice_trigger + 1; i < n_campioni; i++) {
            vel += 0.5 * dt * (acc_filtrata[i-1] + acc_filtrata[i]);
            vel_filt = applica_filtro_singolo(vel, a0_hp, a1_hp, a2_hp, b1_hp, b2_hp, 
                                             &x1_vel, &x2_vel, &y1_vel, &y2_vel);
            
            spost += 0.5 * dt * (vel_filt_prev + vel_filt);
            double spost_filt = applica_filtro_singolo(spost, a0_hp, a1_hp, a2_hp, b1_hp, b2_hp,
                                                       &x1_spost, &x2_spost, &y1_spost, &y2_spost);
            
            double pgd = fabs(spost_filt);
            
            if (!allarme_attivo) {
                int allarme = valuta_allarme_istantaneo(pgd, "RC", N_PIANI, SOGLIA_DANNO);
                if (allarme) {
                    allarme_attivo = 1;
                    indice_allarme = i;
                    pgd_allarme = pgd;
                }
            }
            
            if (pgd > pgd_max) {
                pgd_max = pgd;
                indice_pgd_max = i;
            }
            
            vel_filt_prev = vel_filt;
        }
        
        ConfigurazioneAllarme config = RC_BASSO;
        double soglia_fisica = (strcmp(SOGLIA_DANNO, "MDS") == 0) ? config.mds :
                            (strcmp(SOGLIA_DANNO, "EDS") == 0) ? config.eds : config.cds;
        double soglia_prob = (strcmp(SOGLIA_DANNO, "MDS") == 0) ? config.p_mds :
                            (strcmp(SOGLIA_DANNO, "EDS") == 0) ? config.p_eds : config.p_cds;
        
        double prob_calcolata = calcola_probabilita_previsiva(pgd_allarme, soglia_fisica);
        double log10_drift = REGRESSIONE_INTERCETTA + REGRESSIONE_PENDENZA * log10(pgd_allarme);
        double drift_mediano = pow(10.0, log10_drift);
        
        double lead_time = allarme_attivo ? (indice_pgd_max - indice_allarme) / FREQUENZA : 0.0;
        
        printf("\n====== EARLY WARNING SYSTEM ======\n");
        printf("Soglia Monitorata: %s\n", SOGLIA_DANNO);
        printf("Trigger a: %.3f secondi\n", indice_trigger / FREQUENZA);
        if (allarme_attivo) {
            printf("Allarme a: %.3f secondi\n", indice_allarme / FREQUENZA);
            printf("PGD all'allarme: %.6e m\n", pgd_allarme);
            printf("Drift Mediano previsto: %.6e m\n", drift_mediano);
            printf("Soglia fisica: %.6e m\n", soglia_fisica);
            printf("Probabilità calcolata: %.2f %%\n", prob_calcolata);
            printf("Soglia probabilità: %.2f %%\n", soglia_prob);
            printf("PGD massimo reale: %.6e m\n", pgd_max);
            printf("Lead time: %.3f secondi\n", lead_time);
            printf(">>> ALLARME: ATTIVO\n");
        } else {
            printf("PGD massimo: %.6e m\n", pgd_max);
            printf("Soglia fisica: %.6e m\n", soglia_fisica);
            printf("Soglia probabilità: %.2f %%\n", soglia_prob);
            printf(">>> ALLARME: NON SUPERATO\n");
        }
        printf("==================================\n\n");
    } else {
        printf("Nessun trigger rilevato\n");
    }

    free(acc_data);
    free(acc_hp);
    free(acc_filtrata);
    return 0;
}