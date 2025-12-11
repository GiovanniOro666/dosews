/**
 * =================================================================================
 * DOSEWS - Damage-based On-Site Early Warning System
 * Sistema di Allerta Sismica basato su Drift Strutturale
 * 
 * Versione unificata del codice sorgente
 * 
 * Autore: Sistema DOSEWS
 * Data: 2025
 * Licenza: GPL v3
 * =================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

// =================================================================================
// SEZIONE 1: DEFINIZIONI E STRUTTURE DATI
// =================================================================================

#define MAX_SAMPLES 500000
#define MAX_KERNEL 400

// Tipologie edificio
typedef enum {
    RC_LOW_RISE,         // Cemento armato, 1-3 piani
    RC_MID_RISE,         // Cemento armato, 4-7 piani  
    URM_REG_LOW_RISE,    // Muratura regolare, 1-3 piani
    URM_REG_MID_RISE,    // Muratura regolare, 4-7 piani
    URM_SS_LOW_RISE,     // Muratura piano morbido, 1-3 piani
    URM_SS_MID_RISE      // Muratura piano morbido, 4-7 piani
} BuildingType;

// Stati di danno
typedef enum {
    MODERATE,    // Danno moderato
    EXTENSIVE,   // Danno esteso
    COMPLETE     // Collasso completo
} DamageState;

// Soglie allarme
typedef struct {
    BuildingType type;
    DamageState state;
    float drift_limit;       // Soglia drift
    float prob_threshold;    // Soglia probabilità
} AlarmThreshold;

// Dati segnale
typedef struct {
    float *acc;           // Accelerazione originale
    float *acc_hp;        // Accelerazione filtrata high-pass
    float *acc_fir;       // Accelerazione filtrata FIR
    float *vel_unf;       // Velocità non filtrata
    float *vel_filt;      // Velocità filtrata
    float *disp;          // Spostamento
    int n_samples;        // Numero campioni
} SignalData;

// Configurazione filtri
typedef struct {
    int fs;               // Frequenza campionamento
    float dt;             // Intervallo temporale
    float hp_a, hp_b;     // Coefficienti filtro high-pass
    int filter_len;       // Lunghezza kernel FIR
    float *kernel;        // Kernel FIR
} FilterConfig;

// Parametri trigger
typedef struct {
    float STA_len_s;      // Finestra STA (Short Term Average)
    float LTA_len_s;      // Finestra LTA (Long Term Average)
    float threshold;      // Soglia STA/LTA
    int trigger_idx;      // Indice trigger
    int triggered;        // Flag trigger attivato
} TriggerParams;

// Risultati analisi
typedef struct {
    float pgd_base;           // Peak Ground Displacement base
    float max_drift_abs;      // Drift assoluto massimo
    float max_drift_norm;     // Drift normalizzato massimo
    float max_prob;           // Probabilità massima
    int alarm_triggered;      // Flag allarme
    int alarm_idx;            // Indice allarme
} AnalysisResults;

// =================================================================================
// SEZIONE 2: COSTANTI GLOBALI
// =================================================================================

const float BUILDING_HEIGHT_M = 10.0f;
const int INPUT_UNIT_IS_G = 1;
const float G_TO_MS2 = 9.81f;
const float REG_INTERCEPT = -1.01f;
const float REG_SLOPE = 0.59f;
const float PRED_STD_DEV_LOG10 = 0.25f;

// Database soglie per tipologie edificio/danno
const AlarmThreshold thresholds[] = {
    {RC_LOW_RISE, MODERATE,  0.0184f, 20.86f},
    {RC_LOW_RISE, EXTENSIVE, 0.0301f, 15.53f},
    {RC_LOW_RISE, COMPLETE,  0.0451f, 16.52f},
    {RC_MID_RISE, MODERATE,  0.0223f, 17.20f},
    {RC_MID_RISE, EXTENSIVE, 0.0449f, 16.60f},
    {RC_MID_RISE, COMPLETE,  0.0674f, 10.46f},
    {URM_REG_LOW_RISE, MODERATE,  0.0028f, 13.11f},
    {URM_REG_LOW_RISE, EXTENSIVE, 0.0138f, 12.63f},
    {URM_REG_LOW_RISE, COMPLETE,  0.0236f, 19.70f},
    {URM_REG_MID_RISE, MODERATE,  0.0062f, 26.46f},
    {URM_REG_MID_RISE, EXTENSIVE, 0.0219f, 17.28f},
    {URM_REG_MID_RISE, COMPLETE,  0.0350f, 13.55f},
    {URM_SS_LOW_RISE, MODERATE,   0.0019f, 17.75f},
    {URM_SS_LOW_RISE, EXTENSIVE,  0.0085f, 20.27f},
    {URM_SS_LOW_RISE, COMPLETE,   0.0140f, 12.82f},
    {URM_SS_MID_RISE, MODERATE,   0.0042f, 36.82f},
    {URM_SS_MID_RISE, EXTENSIVE,  0.0135f, 12.36f},
    {URM_SS_MID_RISE, COMPLETE,   0.0210f, 18.30f}
};
const int NUM_THRESHOLDS = sizeof(thresholds) / sizeof(AlarmThreshold);

// Nomi descrittivi
const char* building_names[] = {
    "RC Low-Rise (cemento armato, 1-3 piani)",
    "RC Mid-Rise (cemento armato, 4-7 piani)",
    "URM Regular Low-Rise (muratura regolare, 1-3 piani)",
    "URM Regular Mid-Rise (muratura regolare, 4-7 piani)",
    "URM Soft-Story Low-Rise (muratura piano morbido, 1-3 piani)",
    "URM Soft-Story Mid-Rise (muratura piano morbido, 4-7 piani)"
};

const char* damage_names[] = {
    "Moderate (danno moderato)",
    "Extensive (danno esteso)",
    "Complete (collasso completo)"
};

// =================================================================================
// SEZIONE 3: GESTIONE MEMORIA E SEGNALI
// =================================================================================

SignalData* create_signal_data(int n_samples) {
    SignalData *data = (SignalData*)malloc(sizeof(SignalData));
    if (!data) return NULL;
    
    data->acc = (float*)calloc(n_samples, sizeof(float));
    data->acc_hp = (float*)calloc(n_samples, sizeof(float));
    data->acc_fir = (float*)calloc(n_samples, sizeof(float));
    data->vel_unf = (float*)calloc(n_samples, sizeof(float));
    data->vel_filt = (float*)calloc(n_samples, sizeof(float));
    data->disp = (float*)calloc(n_samples, sizeof(float));
    data->n_samples = n_samples;
    
    return data;
}

void free_signal_data(SignalData *data) {
    if (data) {
        free(data->acc);
        free(data->acc_hp);
        free(data->acc_fir);
        free(data->vel_unf);
        free(data->vel_filt);
        free(data->disp);
        free(data);
    }
}

float calculate_pga(float *data, int n) {
    float pga = 0.0f;
    for (int i = 0; i < n; i++) {
        if (fabsf(data[i]) > pga) {
            pga = fabsf(data[i]);
        }
    }
    return pga;
}

// =================================================================================
// SEZIONE 4: FILTRI DIGITALI
// =================================================================================

// Calcola coefficienti filtro passa-alto per qualsiasi frequenza
static void calculate_highpass_coefficients(int fs, float *hp_a, float *hp_b) {
    const float fc = 0.075f;  // Frequenza di taglio in Hz
    const float PI = 3.14159265358979323846f;
    
    float rc = 1.0f / (2.0f * PI * fc);  // Costante di tempo RC
    float dt = 1.0f / (float)fs;          // Periodo di campionamento
    float alpha = rc / (rc + dt);         // Coefficiente alpha
    
    *hp_a = alpha;                        // Coefficiente feedback (y[n-1])
    *hp_b = (1.0f + alpha) / 2.0f;       // Coefficiente feedforward (x[n] e x[n-1])
}

void create_gaussian_kernel(float *kernel, int len, float *sum) {
    float temp_sum = 0.0f;
    
    #pragma omp parallel for reduction(+:temp_sum)
    for (int k = 0; k < len; k++) {
        float gg = expf(-powf((float)(len - 1 - k) / (0.050f * len), 2));
        kernel[k] = gg;
        temp_sum += gg;
    }
    
    *sum = temp_sum;
    
    // Normalizza
    #pragma omp parallel for
    for (int k = 0; k < len; k++) {
        kernel[k] /= temp_sum;
    }
}

void apply_highpass_filter(float *input, float *output, int n, float hp_a, float hp_b) {
    output[0] = 0.0f;
    for (int i = 1; i < n; i++) {
        output[i] = input[i] * hp_b - input[i-1] * hp_b + hp_a * output[i-1];
    }
}

void apply_fir_filter(float *input, float *output, int n, float *kernel, int kernel_len) {
    #pragma omp parallel for
    for (int i = kernel_len; i < n; i++) {
        float accu = 0.0f;
        for (int k = 0; k < kernel_len; k++) {
            accu += input[i-k] * kernel[k];
        }
        output[i] = accu;
    }
}

void init_filter_config(FilterConfig *config, int fs) {
    config->fs = fs;
    config->dt = 1.0f / fs;
    
    if (fs < 10 || fs > 1000) {
        printf("⚠ Frequenza fuori range (10-1000 Hz)\n");
        config->hp_a = config->hp_b = 0.0f;
        return;
    }
    
    // Usa coefficienti predefiniti per frequenze standard
    switch (fs) {
        case 128: 
            config->hp_b = 0.9981626f; 
            config->hp_a = 0.99632521f;
            printf("✔ Usando coefficienti predefiniti per 128 Hz\n");
            break;
        case 100: 
            config->hp_b = 0.99764934f; 
            config->hp_a = 0.99529868f;
            printf("✔ Usando coefficienti predefiniti per 100 Hz\n");
            break;
        case 200: 
            config->hp_b = 0.99882329f; 
            config->hp_a = 0.99764658f;
            printf("✔ Usando coefficienti predefiniti per 200 Hz\n");
            break;
        default:
            calculate_highpass_coefficients(fs, &config->hp_a, &config->hp_b);
            printf("✔ Calcolati coefficienti per %d Hz (custom)\n", fs);
            printf("  hp_a = %.8f\n", config->hp_a);
            printf("  hp_b = %.8f\n", config->hp_b);
            break;
    }
    
    // Alloca e crea kernel
    config->filter_len = 2 * fs;
    config->kernel = (float*)malloc(config->filter_len * sizeof(float));
    
    float sum;
    create_gaussian_kernel(config->kernel, config->filter_len, &sum);
    
    printf("  Dimensione kernel: %d campioni (2 secondi)\n", config->filter_len);
}

void cleanup_filter_config(FilterConfig *config) {
    if (config->kernel) {
        free(config->kernel);
        config->kernel = NULL;
    }
}

// =================================================================================
// SEZIONE 5: TRIGGER STA/LTA
// =================================================================================

void init_trigger_params(TriggerParams *params, float sta_s, float lta_s) {
    params->STA_len_s = sta_s;
    params->LTA_len_s = lta_s;
    params->threshold = 4.0f;
    params->trigger_idx = -1;
    params->triggered = 0;
}

int find_trigger(float *signal, int n, TriggerParams *params, FilterConfig *filter_cfg) {
    int sta_len = (int)(params->STA_len_s * filter_cfg->fs);
    int lta_len = (int)(params->LTA_len_s * filter_cfg->fs);
    int start_idx = filter_cfg->filter_len + lta_len - 1;
    
    float sta_sum = 0.0f;
    float lta_sum = 0.0f;
    
    // Inizializza finestre
    for (int i = start_idx - lta_len + 1; i <= start_idx; i++) {
        lta_sum += fabsf(signal[i]);
        if (i >= start_idx - sta_len + 1) {
            sta_sum += fabsf(signal[i]);
        }
    }
    
    // Cerca trigger
    for (int i = start_idx; i < n; i++) {
        if (i > start_idx) {
            sta_sum += fabsf(signal[i]) - fabsf(signal[i - sta_len]);
            lta_sum += fabsf(signal[i]) - fabsf(signal[i - lta_len]);
        }
        
        float sta_avg = sta_sum / sta_len;
        float lta_avg = lta_sum / lta_len;
        float ratio = (lta_avg > 1e-9f) ? sta_avg / lta_avg : 0.0f;
        
        if (ratio > params->threshold) {
            params->trigger_idx = i;
            params->triggered = 1;
            printf("✔ TRIGGER: indice=%d, t=%.3fs, STA/LTA=%.2f\n\n", 
                   i, i * filter_cfg->dt, ratio);
            return 1;
        }
    }
    
    printf("✗ NESSUN TRIGGER\n");
    return 0;
}

// =================================================================================
// SEZIONE 6: ANALISI DRIFT PROBABILISTICA
// =================================================================================

int get_alarm_thresholds(BuildingType type, DamageState state,
                         float *drift_limit, float *prob_threshold) {
    for (int i = 0; i < NUM_THRESHOLDS; i++) {
        if (thresholds[i].type == type && thresholds[i].state == state) {
            *drift_limit = thresholds[i].drift_limit;
            *prob_threshold = thresholds[i].prob_threshold / 100.0f;
            return 1;
        }
    }
    return 0;
}

// Calcola probabilità di superamento soglia drift usando modello log-normale
float calculate_exceedance_probability(float pgd_base, float drift_limit) {
    if (pgd_base < 1e-9f) return 0.0f;
    
    float log10_pgd = log10f(pgd_base);
    float mean_log10_drift = REG_INTERCEPT + REG_SLOPE * log10_pgd;
    float log10_drift_limit = log10f(drift_limit);
    float z = (log10_drift_limit - mean_log10_drift) / 
              (PRED_STD_DEV_LOG10 * sqrtf(2.0f));
    float prob_not_exceeding = 0.5f * (1.0f + erff(z));
    
    return 1.0f - prob_not_exceeding;
}

void perform_drift_analysis(SignalData *top, SignalData *base,
                            TriggerParams *trigger, FilterConfig *filter,
                            float ptm_len_s, float building_height,
                            AlarmThreshold *threshold,
                            AnalysisResults *results,
                            const char *debug_file) {
    
    int start = trigger->trigger_idx;
    int ptm_len = (int)(ptm_len_s * filter->fs);
    int end = start + ptm_len;
    if (end > top->n_samples) end = top->n_samples;
    
    // Inizializza risultati
    results->pgd_base = 0.0f;
    results->max_drift_abs = 0.0f;
    results->max_drift_norm = 0.0f;
    results->max_prob = 0.0f;
    results->alarm_triggered = 0;
    results->alarm_idx = -1;
    
    // Altezza normalizzazione (2/3 per sensori solo top)
    float norm_height = (2.0f / 3.0f) * building_height;
    
    FILE *fdebug = fopen(debug_file, "w");
    if (fdebug) {
        fprintf(fdebug, "# t(s), PGD_base(m), Drift_abs(m), Drift_norm(mm/m), Prob(%%)\n");
    }
    
    int report_every = filter->fs;
    int next_report = start + report_every;
    
    printf("ANALISI POST-TRIGGER...\n");
    
    // Inizializza al trigger
    top->vel_unf[start] = 0.0f;
    top->vel_filt[start] = 0.0f;
    top->disp[start] = 0.0f;
    base->vel_unf[start] = 0.0f;
    base->vel_filt[start] = 0.0f;
    base->disp[start] = 0.0f;
    
    // Loop di integrazione sequenziale
    for (int i = start + 1; i < end && !results->alarm_triggered; i++) {
        
        // ===== INTEGRAZIONE TOP =====
        // Integrazione trapezoidale acc -> velocità non filtrata
        top->vel_unf[i] = top->vel_unf[i-1] + 
                          (top->acc_hp[i-1] + top->acc_hp[i]) * 0.5f * filter->dt;
        
        // High-pass sulla velocità (rimuove offset)
        top->vel_filt[i] = top->vel_unf[i] * filter->hp_b - 
                           top->vel_unf[i-1] * filter->hp_b + 
                           filter->hp_a * top->vel_filt[i-1];
        
        // Integrazione trapezoidale velocità filtrata -> spostamento
        top->disp[i] = top->disp[i-1] + 
                       (top->vel_filt[i-1] + top->vel_filt[i]) * 0.5f * filter->dt;
        
        // ===== INTEGRAZIONE BASE =====
        base->vel_unf[i] = base->vel_unf[i-1] + 
                           (base->acc_hp[i-1] + base->acc_hp[i]) * 0.5f * filter->dt;
        
        base->vel_filt[i] = base->vel_unf[i] * filter->hp_b - 
                            base->vel_unf[i-1] * filter->hp_b + 
                            filter->hp_a * base->vel_filt[i-1];
        
        base->disp[i] = base->disp[i-1] + 
                        (base->vel_filt[i-1] + base->vel_filt[i]) * 0.5f * filter->dt;
        
        // ===== CALCOLO DRIFT E ANALISI =====
        float drift_abs = top->disp[i] - base->disp[i];
        float drift_norm = drift_abs / norm_height;
        
        // Aggiorna massimi
        float abs_disp_base = fabsf(base->disp[i]);
        if (abs_disp_base > results->pgd_base) {
            results->pgd_base = abs_disp_base;
        }
        
        if (fabsf(drift_abs) > results->max_drift_abs) {
            results->max_drift_abs = fabsf(drift_abs);
        }
        
        if (fabsf(drift_norm) > results->max_drift_norm) {
            results->max_drift_norm = fabsf(drift_norm);
        }
        
        // Calcola probabilità
        float prob = calculate_exceedance_probability(results->pgd_base, 
                                                      threshold->drift_limit);
        
        if (prob > results->max_prob) {
            results->max_prob = prob;
        }
        
        // Log debug
        if (fdebug) {
            fprintf(fdebug, "%.3f, %.6e, %.6e, %.2f, %.2f\n",
                    i * filter->dt, results->pgd_base, 
                    results->max_drift_abs, results->max_drift_norm * 1000,
                    prob * 100.0f);
        }
        
        // Report periodico
        if (i >= next_report) {
            printf("  T+%.1fs: PGD=%.5fm, Drift=%.2f mm/m, P=%.2f%%\n",
                   (i - start) * filter->dt, results->pgd_base,
                   results->max_drift_norm * 1000, prob * 100.0f);
            next_report += report_every;
        }
        
        // Check allarme
        if (prob > threshold->prob_threshold) {
            results->alarm_triggered = 1;
            results->alarm_idx = i;
            
            printf("\n*** ALLARME ROSSO! ***\n");
            printf("Tempo: %.3f s dopo trigger\n", (i - start) * filter->dt);
            printf("PGD base: %.5f m\n", results->pgd_base);
            printf("Drift normalizzato: %.2f mm/m\n", 
                   results->max_drift_norm * 1000);
            printf("Probabilità: %.2f%% > %.2f%%\n",
                   prob * 100.0f, threshold->prob_threshold * 100.0f);
            break;
        }
    }
    
    if (fdebug) fclose(fdebug);
}

// =================================================================================
// SEZIONE 7: INPUT/OUTPUT
// =================================================================================

int read_acceleration_file(const char *filename, float *data, 
                           int max_samples, float unit_conversion) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("ERRORE: Impossibile aprire %s\n", filename);
        return -1;
    }
    
    int count = 0;
    float temp;
    while (count < max_samples && fscanf(fp, "%f", &temp) == 1) {
        data[count] = temp * unit_conversion;
        count++;
    }
    
    fclose(fp);
    return count;
}

void write_results(const char *filename, SignalData *top, SignalData *base,
                   float *drift, float *drift_norm, int n,
                   float dt, int alarm_idx) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "# Indice, Tempo(s), Drift_abs(m), Drift_norm(mm/m), "
                "Disp_TOP(m), Disp_BASE(m), Allarme\n");
    
    for (int i = 0; i < n; i++) {
        char alarm_flag = (i == alarm_idx) ? 'R' : ' ';
        fprintf(fp, "%d, %.6f, %.8e, %.6f, %.8e, %.8e, %c\n",
                i+1, i*dt, drift[i], drift_norm[i] * 1000,
                top->disp[i], base->disp[i], alarm_flag);
    }
    
    fclose(fp);
}

void print_input_statistics(int n_samples, float dt, 
                            float pga_top, float pga_base) {
    printf("\nSTATISTICHE INPUT:\n");
    printf("  Campioni: %d (durata: %.1f s)\n", n_samples, n_samples * dt);
    printf("  PGA TOP:  %.3f g (%.3f m/s²)\n", pga_top / G_TO_MS2, pga_top);
    printf("  PGA BASE: %.3f g (%.3f m/s²)\n", pga_base / G_TO_MS2, pga_base);
}

void print_final_report(AnalysisResults *results, AlarmThreshold *threshold) {
    printf("\n========== REPORT FINALE ==========\n");
    printf("PGD massimo: %.5f m\n", results->pgd_base);
    printf("Drift assoluto: %.5f m\n", results->max_drift_abs);
    printf("Drift normalizzato: %.2f mm/m\n", results->max_drift_norm * 1000);
    printf("Probabilità massima: %.2f%%\n", results->max_prob * 100.0f);
    
    if (results->alarm_triggered) {
        printf("\n✔ ALLARME ATTIVATO\n");
    } else {
        printf("\n✗ NESSUN ALLARME\n");
        if (results->max_prob > threshold->prob_threshold * 0.7) {
            printf("  Probabilità vicina alla soglia\n");
        }
    }
}

// =================================================================================
// SEZIONE 8: FUNZIONI INTERFACCIA UTENTE
// =================================================================================

void print_building_types() {
    printf("\n========== TIPOLOGIE EDIFICIO ==========\n");
    for (int i = 0; i < 6; i++) {
        printf("  %d - %s\n", i, building_names[i]);
    }
}

void print_damage_states() {
    printf("\n========== STATI DI DANNO ==========\n");
    for (int i = 0; i < 3; i++) {
        printf("  %d - %s\n", i, damage_names[i]);
    }
}

BuildingType get_building_type() {
    int choice;
    print_building_types();
    printf("\nSeleziona tipo edificio (0-5): ");
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > 5) {
        printf("⚠ Scelta non valida, uso default: RC Low-Rise\n");
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return RC_LOW_RISE;
    }
    return (BuildingType)choice;
}

DamageState get_damage_state() {
    int choice;
    print_damage_states();
    printf("\nSeleziona stato danno (0-2): ");
    if (scanf("%d", &choice) != 1 || choice < 0 || choice > 2) {
        printf("⚠ Scelta non valida, uso default: Extensive\n");
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return EXTENSIVE;
    }
    return (DamageState)choice;
}

float get_building_height() {
    float height;
    printf("\n========== ALTEZZA EDIFICIO ==========\n");
    printf("Inserisci altezza edificio in metri (es. 10.0): ");
    if (scanf("%f", &height) != 1 || height <= 0 || height > 200) {
        printf("⚠ Valore non valido, uso default: 10.0 m\n");
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return 10.0f;
    }
    return height;
}

int get_input_unit() {
    int choice;
    printf("\n========== UNITÀ DI MISURA INPUT ==========\n");
    printf("  0 - m/s² (metri al secondo quadrato)\n");
    printf("  1 - g (accelerazione di gravità, 1g = 9.81 m/s²)\n");
    printf("\nSeleziona unità di misura (0-1): ");
    if (scanf("%d", &choice) != 1 || (choice != 0 && choice != 1)) {
        printf("⚠ Scelta non valida, uso default: g\n");
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return 1;
    }
    return choice;
}

int get_sampling_frequency() {
    int fs;
    int choice;
    
    printf("\n========== FREQUENZA CAMPIONAMENTO ==========\n");
    printf("  1 - Frequenza predefinita (100, 128, 200 Hz)\n");
    printf("  2 - Frequenza personalizzata\n");
    printf("\nSeleziona opzione (1-2): ");
    
    if (scanf("%d", &choice) != 1 || (choice != 1 && choice != 2)) {
        printf("⚠ Scelta non valida, uso default: 128 Hz\n");
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        return 128;
    }
    
    if (choice == 1) {
        printf("\nFrequenze predefinite:\n");
        printf("  1 - 100 Hz\n");
        printf("  2 - 128 Hz (default)\n");
        printf("  3 - 200 Hz\n");
        printf("\nSeleziona (1-3): ");
        
        int freq_choice;
        if (scanf("%d", &freq_choice) != 1) {
            printf("⚠ Scelta non valida, uso default: 128 Hz\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            return 128;
        }
        
        switch(freq_choice) {
            case 1: return 100;
            case 2: return 128;
            case 3: return 200;
            default:
                printf("⚠ Scelta non valida, uso default: 128 Hz\n");
                return 128;
        }
    } else {
        printf("\n🎯 FREQUENZA PERSONALIZZATA\n");
        printf("Inserisci frequenza in Hz (10-1000, es. 250): ");
        
        if (scanf("%d", &fs) != 1 || fs < 10 || fs > 1000) {
            printf("⚠ Valore non valido (deve essere 10-1000 Hz)\n");
            printf("  Uso default: 128 Hz\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            return 128;
        }
        
        printf("✔ Frequenza impostata: %d Hz\n", fs);
        return fs;
    }
}

void print_configuration_summary(BuildingType building_type, DamageState damage_state,
                                float building_height, int input_is_g, int fs,
                                float drift_limit, float prob_threshold) {
    printf("\n");
    printf("==========================================================\n");
    printf("               RIEPILOGO CONFIGURAZIONE\n");
    printf("==========================================================\n");
    printf("Parametri Edificio:\n");
    printf("  • Tipologia: %s\n", building_names[building_type]);
    printf("  • Altezza: %.1f m\n", building_height);
    printf("  • Stato danno monitorato: %s\n", damage_names[damage_state]);
    printf("\nParametri Acquisizione:\n");
    printf("  • Frequenza campionamento: %d Hz\n", fs);
    printf("  • Unità input: %s\n", input_is_g ? "g (gravità)" : "m/s²");
    printf("\nSoglie Allarme:\n");
    printf("  • Drift limite: %.4f (%.2f mm/m)\n", drift_limit, drift_limit * 1000);
    printf("  • Probabilità soglia: %.2f%%\n", prob_threshold * 100.0f);
    printf("==========================================================\n\n");
}

// =================================================================================
// SEZIONE 9: MAIN - PROGRAMMA PRINCIPALE
// =================================================================================

int main() {
    char filein_top[256], filein_base[256];
    char fileout_csv[256], fileout_debug[256];
    
    printf("==========================================================\n");
    printf("  DOSEWS - Sistema di Allerta Sismica per Edifici\n");
    printf("  Damage-based On-Site Early Warning System\n");
    printf("==========================================================\n\n");
    
    // CONFIGURAZIONE INTERATTIVA
    printf("Inizia la configurazione del sistema...\n");
    
    BuildingType building_type = get_building_type();
    DamageState damage_state = get_damage_state();
    float building_height = get_building_height();
    int input_is_g = get_input_unit();
    int fs = get_sampling_frequency();
    
    // Parametri fissi
    float sta_s = 0.5f;   // Short Term Average window
    float lta_s = 6.0f;   // Long Term Average window
    float ptm_s = 10.0f;  // Post-Trigger Monitoring window
    
    // Trova soglie per la configurazione scelta
    float drift_limit, prob_threshold;
    if (!get_alarm_thresholds(building_type, damage_state,
                              &drift_limit, &prob_threshold)) {
        printf("\n❌ ERRORE: Soglie non trovate per la configurazione selezionata\n");
        return 1;
    }
    
    // Mostra riepilogo configurazione
    print_configuration_summary(building_type, damage_state, building_height,
                               input_is_g, fs, drift_limit, prob_threshold);
    
    printf("Premere INVIO per continuare...");
    getchar(); // Consuma il newline lasciato da scanf
    getchar(); // Aspetta INVIO
    
    // Inizializza filtri
    FilterConfig filter;
    init_filter_config(&filter, fs);
    
    if (filter.hp_a == 0.0f) {
        printf("❌ ERRORE: Frequenza non supportata (%d Hz)\n", fs);
        return 1;
    }
    
    // Alloca dati
    SignalData *top = create_signal_data(MAX_SAMPLES);
    SignalData *base = create_signal_data(MAX_SAMPLES);
    
    if (!top || !base) {
        printf("❌ ERRORE: Impossibile allocare memoria\n");
        return 1;
    }
    
    // Leggi file
    float unit_conv = input_is_g ? G_TO_MS2 : 1.0f;
    
    printf("\n========== CARICAMENTO DATI ==========\n");
    printf("File accelerazioni TOP (tetto/sommità edificio): ");
    if (scanf("%255s", filein_top) != 1) {
        printf("❌ ERRORE di input\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    int n_top = read_acceleration_file(filein_top, top->acc, 
                                       MAX_SAMPLES, unit_conv);
    if (n_top < 0) {
        printf("❌ Impossibile leggere il file TOP\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    printf("File accelerazioni BASE (fondazione/base edificio): ");
    if (scanf("%255s", filein_base) != 1) {
        printf("❌ ERRORE di input\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    int n_base = read_acceleration_file(filein_base, base->acc,
                                        MAX_SAMPLES, unit_conv);
    if (n_base < 0) {
        printf("❌ Impossibile leggere il file BASE\n");
        free_signal_data(top);
        free_signal_data(base);
        cleanup_filter_config(&filter);
        return 1;
    }
    
    if (n_top != n_base) {
        printf("⚠ ATTENZIONE: File con lunghezze diverse (TOP=%d, BASE=%d)\n", 
               n_top, n_base);
    }
    
    int n = (n_top < n_base) ? n_top : n_base;
    top->n_samples = base->n_samples = n;
    
    // Statistiche
    float pga_top = calculate_pga(top->acc, n);
    float pga_base = calculate_pga(base->acc, n);
    print_input_statistics(n, filter.dt, pga_top, pga_base);
    
    // Prepara nomi file output
    snprintf(fileout_csv, sizeof(fileout_csv), "%s_results.csv", filein_top);
    snprintf(fileout_debug, sizeof(fileout_debug), "%s_debug.txt", filein_top);
    
    // Elaborazione segnali
    printf("\n========== ELABORAZIONE SEGNALI ==========\n");
    printf("Applicazione filtri high-pass e FIR...\n");
    
    apply_highpass_filter(top->acc, top->acc_hp, n, filter.hp_a, filter.hp_b);
    apply_fir_filter(top->acc_hp, top->acc_fir, n, filter.kernel, filter.filter_len);
    
    apply_highpass_filter(base->acc, base->acc_hp, n, filter.hp_a, filter.hp_b);
    apply_fir_filter(base->acc_hp, base->acc_fir, n, filter.kernel, filter.filter_len);
    
    printf("✔ Filtri applicati con successo\n");
    
    // Prepara struttura soglie allarme
    AlarmThreshold alarm_threshold;
    alarm_threshold.type = building_type;
    alarm_threshold.state = damage_state;
    alarm_threshold.drift_limit = drift_limit;
    alarm_threshold.prob_threshold = prob_threshold;
    
    // Trigger
    printf("\n========== RICERCA TRIGGER ==========\n");
    printf("Parametri: STA=%.1fs, LTA=%.1fs, Soglia=4.0\n", sta_s, lta_s);
    
    TriggerParams trigger;
    init_trigger_params(&trigger, sta_s, lta_s);
    
    if (!find_trigger(top->acc_fir, n, &trigger, &filter)) {
        printf("\n========== RISULTATO ==========\n");
        printf("⚪ Nessun evento sismico rilevato\n");
        printf("   (Rapporto STA/LTA non supera la soglia di trigger)\n");
        goto cleanup;
    }
    
    // Analisi drift post-trigger
    printf("\n========== ANALISI DRIFT POST-TRIGGER ==========\n");
    printf("Finestra analisi: %.1f secondi\n", ptm_s);
    printf("Altezza normalizzazione: %.2f m (2/3 di %.1f m)\n", 
           (2.0f/3.0f) * building_height, building_height);
    
    AnalysisResults results;
    perform_drift_analysis(top, base, &trigger, &filter, ptm_s,
                          building_height, &alarm_threshold,
                          &results, fileout_debug);
    
    // Report finale
    print_final_report(&results, &alarm_threshold);
    
    // Salva risultati completi
    printf("\n========== SALVATAGGIO RISULTATI ==========\n");
    
    float *drift = (float*)malloc(n * sizeof(float));
    float *drift_norm = (float*)malloc(n * sizeof(float));
    
    if (drift && drift_norm) {
        float norm_height = (2.0f / 3.0f) * building_height;
        for (int i = 0; i < n; i++) {
            drift[i] = top->disp[i] - base->disp[i];
            drift_norm[i] = drift[i] / norm_height;
        }
        
        write_results(fileout_csv, top, base, drift, drift_norm, n,
                      filter.dt, results.alarm_idx);
        
        printf("✔ File risultati: %s\n", fileout_csv);
        printf("✔ File debug: %s\n", fileout_debug);
        
        free(drift);
        free(drift_norm);
    }
    
    printf("\n==========================================================\n");
    if (results.alarm_triggered) {
        printf("  🔴 STATO FINALE: ALLARME ROSSO ATTIVATO\n");
    } else {
        printf("  🟢 STATO FINALE: NESSUN ALLARME\n");
    }
    printf("==========================================================\n");
    
cleanup:
    free_signal_data(top);
    free_signal_data(base);
    cleanup_filter_config(&filter);
    
    return 0;
}