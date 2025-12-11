#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_SAMPLES 500000
#define STA_SEC 0.5
#define LTA_SEC 6.0
#define THRESHOLD 4.0

void calcola_coeff_bandpass(double fs, double f_low, double f_high,
                            double *a0, double *a1, double *a2,
                            double *b1, double *b2);

void filtro_bandpass(const double *in, double *out, int n,
                     double a0, double a1, double a2,
                     double b1, double b2);

int rileva_trigger(const double *acc_fir, int n_samples, double fs,
                   double sta_sec, double lta_sec, double threshold,
                   int start_idx, int *trigger_idx);

int salva_accelerazioni_filtrate(const char *filename, const double *acc_filtrate, int n);

void integra_trapezi(const double *in, double *out, int n, double dt);

void rimuovi_trend_lineare(double *data, int n);

int salva_integrazioni(const char *filename, const double *vel, 
                       const double *disp, int n);

int valuta_allarme(double drift_max, const char *tipologia, int n_piani);

double trova_max_assoluto(const double *data, int n);

int main(int argc, char *argv[]) {
    
    double *acc;
    double *acc_filt;
    double *vel;
    double *disp;
    
    int n_samples = 0;
    double fs, dt;
    int lta_len;
    
    acc = (double*)malloc(MAX_SAMPLES * sizeof(double));
    acc_filt = (double*)malloc(MAX_SAMPLES * sizeof(double));
    vel = (double*)malloc(MAX_SAMPLES * sizeof(double));
    disp = (double*)malloc(MAX_SAMPLES * sizeof(double));
    
    if (!acc || !acc_filt || !vel || !disp) {
        printf("Errore: impossibile allocare memoria\n");
        return 1;
    }
    
    if (argc != 5) {
        printf("Utilizzo: %s <file_input> <frequenza> <tipologia> <n_piani>\n", argv[0]);
        printf("Esempio: %s dati.txt 100 RC 3\n", argv[0]);
        printf("Tipologie: RC, URM_REG, URM_STONE\n");
        return 1;
    }
    
    char *filename = argv[1];
    fs = atof(argv[2]);
    char *tipologia = argv[3];
    int n_piani = atoi(argv[4]);
    dt = 1.0 / fs;
    
    int sta_len = (int)(STA_SEC * fs);
    lta_len = (int)(LTA_SEC * fs);
    
    printf("=== CONFIGURAZIONE SISTEMA ===\n");
    printf("Edificio: %s, %d piani\n", tipologia, n_piani);
    printf("Frequenza campionamento: %.1f Hz (dt=%.4f s)\n", fs, dt);
    printf("STA: %d campioni (%.1f s)\n", sta_len, STA_SEC);
    printf("LTA: %d campioni (%.1f s)\n", lta_len, LTA_SEC);
    printf("Soglia STA/LTA: %.1f\n\n", THRESHOLD);
    
    printf("Lettura dati da %s...\n", filename);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Errore: impossibile aprire il file\n");
        return 1;
    }
    
    while (fscanf(fp, "%lf", &acc[n_samples]) == 1 && n_samples < MAX_SAMPLES) {
        acc[n_samples] *= 9.80665;
        n_samples++;
    }
    fclose(fp);
    printf("Letti %d campioni (%.2f s)\n", n_samples, n_samples * dt);
    
    printf("Applicazione filtro passa-banda...\n");
    double f_low = 0.075;
    double f_high = 1.0;
    double a0, a1, a2, b1, b2;
    
    calcola_coeff_bandpass(fs, f_low, f_high, &a0, &a1, &a2, &b1, &b2);
    filtro_bandpass(acc, acc_filt, n_samples, a0, a1, a2, b1, b2);

    printf("Salvataggio accelerazioni filtrate...\n");
    if (salva_accelerazioni_filtrate("acc_filtrata.txt", acc_filt, n_samples) == 0) {
        printf("Accelerazioni filtrate salvate in 'acc_filtrata.txt'\n");
    } else {
        printf("ATTENZIONE: errore nel salvataggio delle accelerazioni filtrate\n");
    }
    
    printf("Calcolo STA/LTA...\n");
    
    int start_idx = lta_len - 1;
    int trigger_idx;
    int trigger_found = rileva_trigger(acc_filt, n_samples, fs,
                                       STA_SEC, LTA_SEC, THRESHOLD,
                                       start_idx, &trigger_idx);
    
    if (trigger_found) {
        printf("\nIntegrazione a partire dal trigger (indice %d)...\n", trigger_idx);
        
        int n_post_trigger = n_samples - trigger_idx;
        
        printf("Prima integrazione: acc -> vel...\n");
        integra_trapezi(&acc_filt[trigger_idx], vel, n_post_trigger, dt);
        rimuovi_trend_lineare(vel, n_post_trigger);

        printf("Applicazione filtro passa-banda su velocità...\n");
        double *vel_filt = (double*)malloc(n_post_trigger * sizeof(double));
        if (!vel_filt) {
            printf("Errore: impossibile allocare memoria per vel_filt\n");
            free(acc);
            free(acc_filt);
            free(vel);
            free(disp);
            return 1;
        }
        filtro_bandpass(vel, vel_filt, n_post_trigger, a0, a1, a2, b1, b2);
        rimuovi_trend_lineare(vel_filt, n_post_trigger);

        printf("Seconda integrazione: vel_filt -> disp...\n");
        integra_trapezi(vel_filt, disp, n_post_trigger, dt);
        rimuovi_trend_lineare(disp, n_post_trigger);
        
        printf("Salvataggio velocità e spostamenti...\n");
        if (salva_integrazioni("integrazioni.txt", vel_filt, disp, n_post_trigger) == 0) {
            printf("Integrazioni salvate in 'integrazioni.txt' (%d campioni)\n", n_post_trigger);
        } else {
            printf("ATTENZIONE: errore nel salvataggio delle integrazioni\n");
        }
        
        // Calcola drift massimo
        double drift_max = trova_max_assoluto(disp, n_post_trigger);
        printf("\nDrift massimo: %.6e m\n", drift_max);
        
        // Valuta allarme
        int livello_allarme = valuta_allarme(drift_max, tipologia, n_piani);
        
        free(vel_filt);
    }
    
    free(acc);
    free(acc_filt);
    free(vel);
    free(disp);
    
    return trigger_found ? 0 : 1;
}

double trova_max_assoluto(const double *data, int n) {
    double max_val = 0.0;
    for (int i = 0; i < n; i++) {
        double abs_val = fabs(data[i]);
        if (abs_val > max_val) {
            max_val = abs_val;
        }
    }
    return max_val;
}