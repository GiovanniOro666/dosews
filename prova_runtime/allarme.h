#ifndef ALLARME_H
#define ALLARME_H

#define REGRESSIONE_INTERCETTA -1.01
#define REGRESSIONE_PENDENZA    0.59
#define SIGMA_INCERTEZZA        0.15

typedef struct {
    double mds, eds, cds;       
    double p_mds, p_eds, p_cds; 
} ConfigurazioneAllarme;

extern const ConfigurazioneAllarme RC_BASSO;
extern const ConfigurazioneAllarme RC_MEDIO;
extern const ConfigurazioneAllarme URM_REG_BASSO;
extern const ConfigurazioneAllarme URM_REG_MEDIO;
extern const ConfigurazioneAllarme URM_STONE_BASSO;
extern const ConfigurazioneAllarme URM_STONE_MEDIO;

double calcola_probabilita_previsiva(double pgd_misurato, double soglia_fisica_drift);

int valuta_allarme_istantaneo(double pgd_istantaneo, const char *tipologia, int n_piani, const char *soglia_target);

const ConfigurazioneAllarme *get_configurazione(const char *tipologia, int n_piani);

#endif
