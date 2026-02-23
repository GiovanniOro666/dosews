#include "allarme.h"
#include <math.h>
#include <string.h>

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

const ConfigurazioneAllarme *get_configurazione(const char *tipologia, int n_piani) {
    if (strcmp(tipologia, "RC") == 0) {
        return (n_piani <= 3) ? &RC_BASSO : &RC_MEDIO;
    } else if (strcmp(tipologia, "URM_REG") == 0) {
        return (n_piani <= 2) ? &URM_REG_BASSO : &URM_REG_MEDIO;
    } else {
        return (n_piani <= 2) ? &URM_STONE_BASSO : &URM_STONE_MEDIO;
    }
}

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
    const ConfigurazioneAllarme *config = get_configurazione(tipologia, n_piani);

    double soglia_fisica, soglia_prob;

    if (strcmp(soglia_target, "MDS") == 0) {
        soglia_fisica = config->mds;
        soglia_prob   = config->p_mds;
    } else if (strcmp(soglia_target, "EDS") == 0) {
        soglia_fisica = config->eds;
        soglia_prob   = config->p_eds;
    } else if (strcmp(soglia_target, "CDS") == 0) {
        soglia_fisica = config->cds;
        soglia_prob   = config->p_cds;
    } else {
        return 0;
    }

    double prob = calcola_probabilita_previsiva(pgd_istantaneo, soglia_fisica);
    return (prob >= soglia_prob) ? 1 : 0;
}
