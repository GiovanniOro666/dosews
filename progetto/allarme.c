#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- COSTANTI DI REGRESSIONE (Tabella 1 del Paper) ---
// Queste costanti definiscono la relazione: log10(Drift) = a + b * log10(PGD)
#define SIGMA 0.15          // Deviazione standard dell'errore di previsione
#define REG_INTERCEPT -1.01 // Parametro 'a'
#define REG_SLOPE 0.59      // Parametro 'b'

// --- STRUTTURE DATI ---
// Struttura per le soglie di danno e probabilità ottimali (Tabella 2 del paper)
typedef struct {
    double mds, eds, cds;           // Soglie fisiche di drift (metri)
    double p_mds, p_eds, p_cds;     // Soglie di probabilità ottimali (%) per minimizzare falsi allarmi
} ConfigurazioneAllarme;

// --- DATABASE TIPOLOGIE EDILIZIE (Valori ottimali dal Paper) ---
const ConfigurazioneAllarme CONF_RC_LOW = {0.0184, 0.0301, 0.0451, 20.86, 15.53, 16.52};
const ConfigurazioneAllarme CONF_RC_MID = {0.0223, 0.0449, 0.0674, 17.20, 16.60, 10.46};
const ConfigurazioneAllarme CONF_URM_REG_LOW = {0.0028, 0.0138, 0.0236, 13.11, 12.63, 19.70};
const ConfigurazioneAllarme CONF_URM_REG_MID = {0.0062, 0.0219, 0.0350, 26.46, 17.28, 13.55};
const ConfigurazioneAllarme CONF_URM_STONE_LOW = {0.0019, 0.0085, 0.0140, 17.75, 20.27, 12.82};
const ConfigurazioneAllarme CONF_URM_STONE_MID = {0.0042, 0.0135, 0.0210, 36.82, 12.36, 18.30};

/**
 * Calcola la probabilità di superamento di una soglia di danno.
 * Utilizza la distribuzione log-normale dei dati di drift.
 */
double calcola_probabilita_previsiva(double pgd_3s, double soglia_fisica) {
    if (pgd_3s <= 0) return 0.0;

    // 1. Calcolo del Drift mediano atteso in scala logaritmica
    double log10_drift_predetto = REG_INTERCEPT + REG_SLOPE * log10(pgd_3s);
    
    // 2. Calcolo dello Z-score rispetto alla soglia dell'edificio
    double log10_soglia = log10(soglia_fisica);
    double z = (log10_soglia - log10_drift_predetto) / SIGMA;

    // 3. Calcolo della probabilità di eccedenza (Complemento della CDF normale)
    // Usiamo la funzione di errore erf() per calcolare l'area della coda
    double prob = 0.5 * (1.0 - erf(z / sqrt(2.0)));
    
    return prob * 100.0; // Restituisce valore in percentuale (0-100)
}

/**
 * Valuta istantaneamente il livello di allarme confrontando le probabilità
 * calcolate con le soglie ottimali derivate dal paper.
 */
int valuta_allarme_istantaneo(double pgd_3s, const char *tipologia, int n_piani) {
    ConfigurazioneAllarme c;

    // Selezione del profilo di vulnerabilità dell'edificio
    if (strcmp(tipologia, "RC") == 0) {
        c = (n_piani <= 3) ? CONF_RC_LOW : CONF_RC_MID;
    } else if (strcmp(tipologia, "URM_REG") == 0) {
        c = (n_piani <= 2) ? CONF_URM_REG_LOW : CONF_URM_REG_MID;
    } else { // URM_STONE
        c = (n_piani <= 2) ? CONF_URM_STONE_LOW : CONF_URM_STONE_MID;
    }

    // Calcolo delle probabilità previsive per i tre stati di danno (MDS, EDS, CDS)
    double p_mds_calc = calcola_probabilita_previsiva(pgd_3s, c.mds);
    double p_eds_calc = calcola_probabilita_previsiva(pgd_3s, c.eds);
    double p_cds_calc = calcola_probabilita_previsiva(pgd_3s, c.cds);

    // Decisione bayesiana: si parte dal danno più grave
    if (p_cds_calc >= c.p_cds) return 3; // ROSSO: Danno Completo previsto
    if (p_eds_calc >= c.p_eds) return 2; // ARANCIO: Danno Esteso previsto
    if (p_mds_calc >= c.p_mds) return 1; // GIALLO: Danno Moderato previsto

    return 0; // NESSUN ALLARME
}

/**
 * Funzione di reportistica chiamata dal main.
 * Stampa a video i dettagli della decisione e salva i risultati su file.
 */
int valuta_allarme(double pgd_3s, const char *tipologia, int n_piani, 
                   int trigger_idx, double fs, double drift_reale_max) {
    
    ConfigurazioneAllarme c;
    if (strcmp(tipologia, "RC") == 0) {
        c = (n_piani <= 3) ? CONF_RC_LOW : CONF_RC_MID;
    } else if (strcmp(tipologia, "URM_REG") == 0) {
        c = (n_piani <= 2) ? CONF_URM_REG_LOW : CONF_URM_REG_MID;
    } else {
        c = (n_piani <= 2) ? CONF_URM_STONE_LOW : CONF_URM_STONE_MID;
    }

    double p_mds = calcola_probabilita_previsiva(pgd_3s, c.mds);
    double p_eds = calcola_probabilita_previsiva(pgd_3s, c.eds);
    double p_cds = calcola_probabilita_previsiva(pgd_3s, c.cds);

    int livello = valuta_allarme_istantaneo(pgd_3s, tipologia, n_piani);
    const char* colori[] = {"NESSUNO", "GIALLO (Moderato)", "ARANCIO (Esteso)", "ROSSO (Completo)"};

    // Output a console per l'operatore
    printf("\n===============================================\n");
    printf("   EARLY WARNING SYSTEM - ANALISI BAYESIANA    \n");
    printf("===============================================\n");
    printf("Trigger rilevato a: %.3f secondi\n", trigger_idx / fs);
    printf("PGD (primi 3s):     %.6e m\n", pgd_3s);
    printf("Drift Mediano previsto: %.6e m\n", pow(10.0, REG_INTERCEPT + REG_SLOPE * log10(pgd_3s)));
    
    printf("\nPROBABILITA' DI ECCEDENZA VS SOGLIE:\n");
    printf("  Soglia MDS: %5.2f%% (Target: %5.2f%%)\n", p_mds, c.p_mds);
    printf("  Soglia EDS: %5.2f%% (Target: %5.2f%%)\n", p_eds, c.p_eds);
    printf("  Soglia CDS: %5.2f%% (Target: %5.2f%%)\n", p_cds, c.p_cds);

    printf("\n>>> LIVELLO ALLARME: %s <<<\n", colori[livello]);
    
    if (drift_reale_max > 0) {
        printf("Validazione post-evento (Drift Max): %.6e m\n", drift_reale_max);
    }
    printf("===============================================\n");

    // Salvataggio persistente per analisi post-evento
    FILE *fp = fopen("allarme_report.txt", "w");
    if (fp) {
        fprintf(fp, "PGD_3s: %.6e\n", pgd_3s);
        fprintf(fp, "Livello_Allarme: %d\n", livello);
        fprintf(fp, "Prob_MDS: %.2f\nProb_EDS: %.2f\nProb_CDS: %.2f\n", p_mds, p_eds, p_cds);
        fclose(fp);
    }

    return livello;
}