#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_SAMPLES 500000
#define STA_SEC 0.5
#define LTA_SEC 6.0
#define THRESHOLD 4.0
#define EEW_WINDOW_SEC 3.0 

// --- PROTOTIPI FUNZIONI ESTERNE ---
int rileva_trigger(const double *acc_filt, int n_samples, double fs, double sta_sec, double lta_sec, double threshold, int start_idx, int *trigger_idx);
void integra_trapezi(const double *in, double *out, int n, double dt);
void rimuovi_trend_lineare(double *data, int n);
int salva_accelerazioni_filtrate(const char *filename, const double *acc_filtrate, int n);
int salva_integrazioni(const char *filename, const double *vel, const double *disp, int n);
int valuta_allarme(double pgd_3s, const char *tipologia, int n_piani, int trigger_idx, double fs, double drift_reale_max);
int valuta_allarme_istantaneo(double pgd_3s, const char *tipologia, int n_piani);
void calcola_coeff_highpass(double fs, double fc, double *a0, double *a1, double *a2, double *b1, double *b2);
void calcola_coeff_lowpass(double fs, double fc, double *a0, double *a1, double *a2, double *b1, double *b2);
void filtro_highpass(const double *in, double *out, int n, double a0, double a1, double a2, double b1, double b2);
void filtro_lowpass(const double *in, double *out, int n, double a0, double a1, double a2, double b1, double b2);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Utilizzo: %s <file_input> <frequenza> <tipologia: RC|URM_REG|URM_STONE> <n_piani>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    double fs = atof(argv[2]);
    char *tipologia = argv[3];
    int n_piani = atoi(argv[4]);
    double dt = 1.0 / fs;

    double *acc = malloc(MAX_SAMPLES * sizeof(double));
    double *acc_filt = malloc(MAX_SAMPLES * sizeof(double));
    double *temp_buf = malloc(MAX_SAMPLES * sizeof(double));

    if (!acc || !acc_filt || !temp_buf) {
        printf("Errore: Memoria insufficiente.\n");
        return 1;
    }

    int n_samples = 0;
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("Errore: impossibile aprire %s\n", filename); return 1; }
    while (fscanf(fp, "%lf", &acc[n_samples]) == 1 && n_samples < MAX_SAMPLES) {
        n_samples++;
    }
    fclose(fp);

    double a0_hp, a1_hp, a2_hp, b1_hp, b2_hp;
    double a0_lp, a1_lp, a2_lp, b1_lp, b2_lp;
    calcola_coeff_highpass(fs, 0.075, &a0_hp, &a1_hp, &a2_hp, &b1_hp, &b2_hp);
    calcola_coeff_lowpass(fs, 1.0, &a0_lp, &a1_lp, &a2_lp, &b1_lp, &b2_lp);

    filtro_highpass(acc, temp_buf, n_samples, a0_hp, a1_hp, a2_hp, b1_hp, b2_hp);
    filtro_lowpass(temp_buf, acc_filt, n_samples, a0_lp, a1_lp, a2_lp, b1_lp, b2_lp);
    for (int i = 0; i < n_samples; i++) acc_filt[i] *= 9.80665;
    if (salva_accelerazioni_filtrate("acc_filtrata.txt", acc_filt, n_samples) == 0) {
        printf("Segnale filtrato salvato correttamente in 'acc_filtrata.txt'\n");
    } else {
        printf("Avviso: Errore durante il salvataggio del segnale filtrato.\n");
    }

    int trigger_idx;
    int offset_iniziale = (int)(LTA_SEC * fs);

    if (rileva_trigger(acc_filt, n_samples, fs, STA_SEC, LTA_SEC, THRESHOLD, offset_iniziale, &trigger_idx)) {
        
        double tempo_trigger = (double)trigger_idx / fs;
        printf("\n>>>> MONITORAGGIO CONTINUO ATTIVATO <<<<\n");
        printf("Trigger rilevato a: %.3f s (indice %d)\n", tempo_trigger, trigger_idx);
        
        // --- INTEGRAZIONE COMPLETA POST-TRIGGER ---
        int n_post_trigger = n_samples - trigger_idx;
        double *vel = calloc(n_post_trigger, sizeof(double));
        double *disp = calloc(n_post_trigger, sizeof(double));
        double *temp = calloc(n_post_trigger, sizeof(double));

        if (!vel || !disp || !temp) {
            printf("Errore: memoria insufficiente\n");
            free(acc); free(acc_filt); free(temp_buf);
            return 1;
        }

        // Prima integrazione: acc -> vel
        integra_trapezi(&acc_filt[trigger_idx], vel, n_post_trigger, dt);
        filtro_highpass(vel, temp, n_post_trigger, a0_hp, a1_hp, a2_hp, b1_hp, b2_hp);
        memcpy(vel, temp, n_post_trigger * sizeof(double));

        // Seconda integrazione: vel -> disp
        integra_trapezi(vel, disp, n_post_trigger, dt);
        filtro_highpass(disp, temp, n_post_trigger, a0_hp, a1_hp, a2_hp, b1_hp, b2_hp);
        memcpy(disp, temp, n_post_trigger * sizeof(double));

        salva_integrazioni("integrazioni.txt", vel, disp, n_post_trigger);

        // --- PRE-CALCOLO: TROVA IL PICCO MASSIMO DEL DRIFT ---
        double drift_max = 0.0;
        int idx_drift_max = 0;
        for (int i = 0; i < n_post_trigger; i++) {
            if (fabs(disp[i]) > drift_max) {
                drift_max = fabs(disp[i]);
                idx_drift_max = i;
            }
        }
        double tempo_drift_max = (trigger_idx + idx_drift_max) / fs;
        
        printf("Pre-analisi: Drift massimo = %.6e m al tempo %.3f s\n", drift_max, tempo_drift_max);

        // --- MONITORAGGIO CONTINUO CON FINESTRA MOBILE 3s ---
        // CERCA IL LIVELLO MASSIMO, NON SI FERMA AL PRIMO ALLARME
        int n_eew = (int)(EEW_WINDOW_SEC * fs);
        int livello_massimo = 0;
        double pgd_massimo = 0.0;
        int idx_allarme_max = -1;
        int primo_allarme_emesso = 0;
        int idx_primo_allarme = -1;
        double pgd_primo_allarme = 0.0;
        
        printf("Valutazione con finestra mobile di %.1f secondi...\n", EEW_WINDOW_SEC);
        printf("(Continuo fino alla fine per trovare livello massimo)\n\n");
        
        // Scorri TUTTE le posizioni possibili della finestra di 3s
        for (int i = 0; i <= n_post_trigger - n_eew; i++) {
            
            // Calcola PGD nella finestra corrente [i, i+n_eew]
            double pgd_window = 0.0;
            for (int j = i; j < i + n_eew; j++) {
                if (fabs(disp[j]) > pgd_window) {
                    pgd_window = fabs(disp[j]);
                }
            }
            
            // Valuta livello di allarme per questa finestra
            int livello = valuta_allarme_istantaneo(pgd_window, tipologia, n_piani);
            
            // Registra il PRIMO allarme emesso (qualsiasi livello > 0)
            if (livello > 0 && !primo_allarme_emesso) {
                primo_allarme_emesso = 1;
                idx_primo_allarme = i + n_eew;
                pgd_primo_allarme = pgd_window;
                
                double tempo_allarme = (trigger_idx + idx_primo_allarme) / fs;
                double lead_time = tempo_drift_max - tempo_allarme;
                
                const char* nomi[] = {"NESSUNO", "GIALLO (Moderato)", "ARANCIO (Esteso)", "ROSSO (Completo)"};
                printf("*** PRIMO ALLARME EMESSO! ***\n");
                printf("Tempo: %.3f s (%.3f s dopo trigger)\n", 
                       tempo_allarme, idx_primo_allarme/fs);
                printf("PGD finestra: %.6e m\n", pgd_window);
                printf("Livello: %d - %s\n", livello, nomi[livello]);
                if (lead_time > 0) {
                    printf("LEAD TIME disponibile: %.3f s ✓ (tempo prima del picco)\n\n", lead_time);
                } else {
                    printf("LEAD TIME disponibile: %.3f s (picco già passato)\n\n", lead_time);
                }
            }
            
            // Aggiorna il livello MASSIMO raggiunto
            if (livello > livello_massimo) {
                livello_massimo = livello;
                pgd_massimo = pgd_window;
                idx_allarme_max = i + n_eew;
                
                double tempo_allarme = (trigger_idx + idx_allarme_max) / fs;
                double lead_time = tempo_drift_max - tempo_allarme;
                
                const char* nomi[] = {"NESSUNO", "GIALLO (Moderato)", "ARANCIO (Esteso)", "ROSSO (Completo)"};
                printf(">>> ALLARME AGGIORNATO! <<<\n");
                printf("Tempo: %.3f s (%.3f s dopo trigger)\n", 
                       tempo_allarme, idx_allarme_max/fs);
                printf("PGD finestra: %.6e m\n", pgd_window);
                printf("Livello: %d - %s\n", livello, nomi[livello]);
                if (lead_time > 0) {
                    printf("LEAD TIME disponibile: %.3f s ✓ (tempo prima del picco)\n\n", lead_time);
                } else {
                    printf("LEAD TIME disponibile: %.3f s (picco già passato)\n\n", lead_time);
                }
            }
            
            // Debug ogni secondo (senza sovrascrivere gli aggiornamenti)
            if (i % (int)fs == 0 && livello == livello_massimo && livello > 0) {
                printf("  [t=%.1fs: PGD=%.6e m, livello=%d]\n", i/fs, pgd_window, livello);
            }
        }
        
        // Se nessun allarme è stato mai emesso
        if (livello_massimo == 0) {
            printf("\nNessuna soglia superata durante il monitoraggio.\n");
            // Usa il PGD massimo trovato comunque per il report
            for (int i = 0; i < n_post_trigger; i++) {
                if (fabs(disp[i]) > pgd_massimo) {
                    pgd_massimo = fabs(disp[i]);
                }
            }
            idx_allarme_max = n_post_trigger;
        }

        printf("\n>>>> STATISTICHE TEMPORALI <<<<\n");
        printf("Tempo Trigger:           %.3f s\n", tempo_trigger);
        
        if (primo_allarme_emesso) {
            double tempo_primo_allarme = (trigger_idx + idx_primo_allarme) / fs;
            printf("Tempo PRIMO Allarme:     %.3f s\n", tempo_primo_allarme);
        }
        
        if (livello_massimo > 0) {
            double tempo_allarme_max = (trigger_idx + idx_allarme_max) / fs;
            double tempo_drift_max = (trigger_idx + idx_drift_max) / fs;
            double lead_time = tempo_drift_max - tempo_allarme_max;
            
            printf("Tempo Allarme MASSIMO:   %.3f s\n", tempo_allarme_max);
            printf("Tempo Picco Drift:       %.3f s\n", tempo_drift_max);
            
            if (lead_time > 0) {
                printf("LEAD TIME:               %.3f s ✓\n", lead_time);
            } else {
                printf("LEAD TIME:               %.3f s (BLIND ZONE)\n", lead_time);
            }
        } else {
            printf("Nessun allarme emesso.\n");
        }
        
        printf("\nRISULTATO FINALE:\n");
        printf("Livello massimo: %d\n", livello_massimo);
        printf("PGD al livello massimo: %.6e m\n", pgd_massimo);
        printf("Drift massimo osservato: %.6e m\n", drift_max);
        printf("--------------------------------\n");

        // Report finale con il PGD del livello MASSIMO
        valuta_allarme(pgd_massimo, tipologia, n_piani, trigger_idx, fs, drift_max);

        free(vel); 
        free(disp); 
        free(temp);
    } else {
        printf("Nessun evento rilevato.\n");
    }

    free(acc); free(acc_filt); free(temp_buf);
    return 0;
}