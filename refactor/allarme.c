#include "allarme.h"
#include <math.h>
#include <string.h>

#define REGRESSIONE_INTERCETTA -1.01
#define REGRESSIONE_PENDENZA 0.59
#define SIGMA_INCERTEZZA 0.15

const ConfigurazioneAllarme RC_BASSO = {
    .mds = 0.0184, .eds = 0.0301, .cds = 0.0451,
    .p_mds = 13.26, .p_eds = 12.37, .p_cds = 9.06
};

const ConfigurazioneAllarme RC_MEDIO = {
    .mds = 0.0223, .eds = 0.0449, .cds = 0.0674,
    .p_mds = 9.96, .p_eds = 9.13, .p_cds = 4.23
};

const ConfigurazioneAllarme URM_REG_BASSO = {
    .mds = 0.0028, .eds = 0.0138, .cds = 0.0236,
    .p_mds = 6.34, .p_eds = 17.32, .p_cds = 10.33
};

const ConfigurazioneAllarme URM_REG_MEDIO = {
    .mds = 0.0062, .eds = 0.0219, .cds = 0.0350,
    .p_mds = 5.08, .p_eds = 8.27, .p_cds = 5.02
};

const ConfigurazioneAllarme URM_STONE_BASSO = {
    .mds = 0.0019, .eds = 0.0085, .cds = 0.0140,
    .p_mds = 11.66, .p_eds = 12.66, .p_cds = 12.19
};

const ConfigurazioneAllarme URM_STONE_MEDIO = {
    .mds = 0.0042, .eds = 0.0135, .cds = 0.0210,
    .p_mds = 32.19, .p_eds = 6.38, .p_cds = 10.71
};

double calcola_probabilita_previsiva(double pgd_misurato, double soglia_fisica_drift) {
    if (pgd_misurato <= 0.0) {
        return 0.0;
    }
    
    double log10_drift_predetto = REGRESSIONE_INTERCETTA + REGRESSIONE_PENDENZA * log10(pgd_misurato);
    double log10_soglia = log10(soglia_fisica_drift);
    double z_score = (log10_soglia - log10_drift_predetto) / SIGMA_INCERTEZZA;
    double probabilita = 0.5 * (1.0 - erf(z_score / sqrt(2.0)));
    
    return probabilita * 100.0;
}

int valuta_allarme_istantaneo(double pgd_istantaneo, const char *tipologia, int n_piani, const char *soglia_target) {
    ConfigurazioneAllarme config;
    
    if (strcmp(tipologia, "RC") == 0) {
        if (n_piani <= 3) {
            config = RC_BASSO;
        } else {
            config = RC_MEDIO;
        }
    } else if (strcmp(tipologia, "URM_REG") == 0) {
        if (n_piani <= 2) {
            config = URM_REG_BASSO;
        } else {
            config = URM_REG_MEDIO;
        }
    } else {
        if (n_piani <= 2) {
            config = URM_STONE_BASSO;
        } else {
            config = URM_STONE_MEDIO;
        }
    }
    
    double prob_calcolata;
    
    if (strcmp(soglia_target, "MDS") == 0) {
        prob_calcolata = calcola_probabilita_previsiva(pgd_istantaneo, config.mds);
        return (prob_calcolata >= config.p_mds) ? 1 : 0;
    } else if (strcmp(soglia_target, "EDS") == 0) {
        prob_calcolata = calcola_probabilita_previsiva(pgd_istantaneo, config.eds);
        return (prob_calcolata >= config.p_eds) ? 1 : 0;
    } else if (strcmp(soglia_target, "CDS") == 0) {
        prob_calcolata = calcola_probabilita_previsiva(pgd_istantaneo, config.cds);
        return (prob_calcolata >= config.p_cds) ? 1 : 0;
    }
    
    return 0;
}