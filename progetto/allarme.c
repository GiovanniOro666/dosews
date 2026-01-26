#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SIGMA 0.15  // deviazione standard del modello

typedef struct {
    double mds;  // Moderate Damage State
    double eds;  // Extensive Damage State
    double cds;  // Complete Damage State
} SoglieDrift;

const SoglieDrift SOGLIE_RC_LOW = {0.0184, 0.0301, 0.0451};
const SoglieDrift SOGLIE_RC_MID = {0.0223, 0.0449, 0.0674};
const SoglieDrift SOGLIE_URM_REG_LOW = {0.0028, 0.0138, 0.0236};
const SoglieDrift SOGLIE_URM_REG_MID = {0.0062, 0.0219, 0.0350};
const SoglieDrift SOGLIE_URM_STONE_LOW = {0.0019, 0.0085, 0.0140};
const SoglieDrift SOGLIE_URM_STONE_MID = {0.0042, 0.0135, 0.0210};

const double PROB_THRESH_RC_LOW[3] = {20.86, 15.53, 16.52};
const double PROB_THRESH_RC_MID[3] = {17.20, 16.60, 10.46};
const double PROB_THRESH_URM_REG_LOW[3] = {13.11, 12.63, 19.70};
const double PROB_THRESH_URM_REG_MID[3] = {26.46, 17.28, 13.55};
const double PROB_THRESH_URM_STONE_LOW[3] = {17.75, 20.27, 12.82};
const double PROB_THRESH_URM_STONE_MID[3] = {36.82, 12.36, 18.30};

double calcola_probabilita_eccedenza(double drift_osservato, double soglia_drift) {
    
    double log10_drift_oss = log10(drift_osservato);
    double log10_soglia = log10(soglia_drift);
    
    double z = (log10_soglia - log10_drift_oss) / SIGMA;
    
    // CDF normale standard: Φ(z) = 0.5 * (1 + erf(z/√2))
    double cdf = 0.5 * (1.0 + erf(z / sqrt(2.0)));
    
    return (1.0 - cdf) * 100.0;  // ritorna percentuale
}

// Funzione per valutazione senza I/O (usata nel loop real-time)
int valuta_allarme_istantaneo(double drift_corrente, const char *tipologia, int n_piani) {
    
    SoglieDrift soglie;
    const double *prob_thresh;
    
    if (strcmp(tipologia, "RC") == 0) {
        if (n_piani <= 3) {
            soglie = SOGLIE_RC_LOW;
            prob_thresh = PROB_THRESH_RC_LOW;
        } else {
            soglie = SOGLIE_RC_MID;
            prob_thresh = PROB_THRESH_RC_MID;
        }
    } else if (strcmp(tipologia, "URM_REG") == 0) {
        if (n_piani <= 2) {
            soglie = SOGLIE_URM_REG_LOW;
            prob_thresh = PROB_THRESH_URM_REG_LOW;
        } else {
            soglie = SOGLIE_URM_REG_MID;
            prob_thresh = PROB_THRESH_URM_REG_MID;
        }
    } else {
        if (n_piani <= 2) {
            soglie = SOGLIE_URM_STONE_LOW;
            prob_thresh = PROB_THRESH_URM_STONE_LOW;
        } else {
            soglie = SOGLIE_URM_STONE_MID;
            prob_thresh = PROB_THRESH_URM_STONE_MID;
        }
    }
    
    double prob_mds = calcola_probabilita_eccedenza(drift_corrente, soglie.mds);
    double prob_eds = calcola_probabilita_eccedenza(drift_corrente, soglie.eds);
    double prob_cds = calcola_probabilita_eccedenza(drift_corrente, soglie.cds);
    
    if (prob_cds >= prob_thresh[2]) return 3;
    if (prob_eds >= prob_thresh[1]) return 2;
    if (prob_mds >= prob_thresh[0]) return 1;
    return 0;
}

int valuta_allarme(double drift_max, const char *tipologia, int n_piani, 
                   int trigger_idx, int allarme_idx, int picco_idx, double fs) {
    
    double tempo_trigger = trigger_idx / fs;
    double tempo_allarme = (trigger_idx + allarme_idx) / fs;
    double tempo_picco = (trigger_idx + picco_idx) / fs;
    double tempo_lancio_allarme = allarme_idx / fs;
    double lead_time = (picco_idx - allarme_idx) / fs;
    
    printf("\n=== VALUTAZIONE ALLARME ===\n");
    printf("Trigger: campione %d, tempo %.3f s\n", trigger_idx, tempo_trigger);
    printf("Allarme: campione %d, tempo %.3f s\n", trigger_idx + allarme_idx, tempo_allarme);
    printf("Picco:   campione %d, tempo %.3f s\n", trigger_idx + picco_idx, tempo_picco);
    printf("\nTempo lancio allarme: %.3f s (%d campioni)\n", tempo_lancio_allarme, allarme_idx);
    printf("Lead time: %.3f s (%d campioni)\n", lead_time, picco_idx - allarme_idx);
    printf("\nDrift massimo: %.6e m\n", drift_max);
    printf("Tipologia: %s, Piani: %d\n", tipologia, n_piani);
    
    // Apri file per salvare info allarme
    FILE *fp_allarme = fopen("allarme_info.txt", "w");
    if (fp_allarme) {
        fprintf(fp_allarme, "=== INFORMAZIONI ALLARME ===\n");
        fprintf(fp_allarme, "Trigger: campione %d, tempo %.3f s\n", trigger_idx, tempo_trigger);
        fprintf(fp_allarme, "Allarme: campione %d, tempo %.3f s\n", trigger_idx + allarme_idx, tempo_allarme);
        fprintf(fp_allarme, "Picco:   campione %d, tempo %.3f s\n", trigger_idx + picco_idx, tempo_picco);
        fprintf(fp_allarme, "\nTempo lancio allarme: %.3f s (%d campioni)\n", tempo_lancio_allarme, allarme_idx);
        fprintf(fp_allarme, "Lead time: %.3f s (%d campioni)\n", lead_time, picco_idx - allarme_idx);
        fprintf(fp_allarme, "\nDrift massimo: %.6e m\n", drift_max);
        fprintf(fp_allarme, "Tipologia: %s, Piani: %d\n\n", tipologia, n_piani);
    }
    
    // Seleziona soglie appropriate
    SoglieDrift soglie;
    const double *prob_thresh;
    
    if (strcmp(tipologia, "RC") == 0) {
        if (n_piani <= 3) {
            soglie = SOGLIE_RC_LOW;
            prob_thresh = PROB_THRESH_RC_LOW;
        } else {
            soglie = SOGLIE_RC_MID;
            prob_thresh = PROB_THRESH_RC_MID;
        }
    } else if (strcmp(tipologia, "URM_REG") == 0) {
        if (n_piani <= 2) {
            soglie = SOGLIE_URM_REG_LOW;
            prob_thresh = PROB_THRESH_URM_REG_LOW;
        } else {
            soglie = SOGLIE_URM_REG_MID;
            prob_thresh = PROB_THRESH_URM_REG_MID;
        }
    } else {
        if (n_piani <= 2) {
            soglie = SOGLIE_URM_STONE_LOW;
            prob_thresh = PROB_THRESH_URM_STONE_LOW;
        } else {
            soglie = SOGLIE_URM_STONE_MID;
            prob_thresh = PROB_THRESH_URM_STONE_MID;
        }
    }
    
    // Calcola probabilità eccedenza per ogni livello danno
    double prob_mds = calcola_probabilita_eccedenza(drift_max, soglie.mds);
    double prob_eds = calcola_probabilita_eccedenza(drift_max, soglie.eds);
    double prob_cds = calcola_probabilita_eccedenza(drift_max, soglie.cds);
    
    printf("\nProbabilità eccedenza:\n");
    printf("  MDS (%.4f m): %.2f%% (soglia: %.2f%%)\n", 
           soglie.mds, prob_mds, prob_thresh[0]);
    printf("  EDS (%.4f m): %.2f%% (soglia: %.2f%%)\n", 
           soglie.eds, prob_eds, prob_thresh[1]);
    printf("  CDS (%.4f m): %.2f%% (soglia: %.2f%%)\n", 
           soglie.cds, prob_cds, prob_thresh[2]);
    
    if (fp_allarme) {
        fprintf(fp_allarme, "Probabilità eccedenza:\n");
        fprintf(fp_allarme, "  MDS (%.4f m): %.2f%% (soglia: %.2f%%)\n", 
                soglie.mds, prob_mds, prob_thresh[0]);
        fprintf(fp_allarme, "  EDS (%.4f m): %.2f%% (soglia: %.2f%%)\n", 
                soglie.eds, prob_eds, prob_thresh[1]);
        fprintf(fp_allarme, "  CDS (%.4f m): %.2f%% (soglia: %.2f%%)\n\n", 
                soglie.cds, prob_cds, prob_thresh[2]);
    }
    
    int allarme = 0;
    
    if (prob_cds >= prob_thresh[2]) {
        printf("\n*** ALLARME ROSSO ***\n");
        printf("DANNO COMPLETO\n");
        allarme = 3;
        if (fp_allarme) {
            fprintf(fp_allarme, "*** ALLARME ROSSO ***\n");
            fprintf(fp_allarme, "DANNO COMPLETO\n");
        }
    } else if (prob_eds >= prob_thresh[1]) {
        printf("\n*** ALLARME ROSSO ***\n");
        printf("DANNO ESTESO\n");
        allarme = 2;
        if (fp_allarme) {
            fprintf(fp_allarme, "*** ALLARME ROSSO ***\n");
            fprintf(fp_allarme, "DANNO ESTESO\n");
        }
    } else if (prob_mds >= prob_thresh[0]) {
        printf("\n*** ALLARME ROSSO ***\n");
        printf("DANNO MODERATO PROBABILE\n");
        allarme = 1;
        if (fp_allarme) {
            fprintf(fp_allarme, "*** ALLARME ROSSO ***\n");
            fprintf(fp_allarme, "DANNO MODERATO PROBABILE\n");
        }
    } else {
        printf("\n*** NESSUN ALLARME ***\n");
        printf("Probabilità danno sotto soglia\n");
        if (fp_allarme) {
            fprintf(fp_allarme, "*** NESSUN ALLARME ***\n");
            fprintf(fp_allarme, "Probabilità danno sotto soglia\n");
        }
    }
    
    if (fp_allarme) {
        fclose(fp_allarme);
        printf("\nInformazioni allarme salvate in 'allarme_info.txt'\n");
    }
    
    return allarme;
}