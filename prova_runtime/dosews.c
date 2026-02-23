#include "dosews.h"
#include "output.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#define G 9.81

int init_dosews(StatoDOSEWS *sys, const ConfigSistema *config) {
    memset(sys, 0, sizeof(StatoDOSEWS));
    sys->config = *config;

    // Calcola coefficienti filtro high-pass
    calcola_coeff_highpass(config->frequenza, config->fc_hp, &sys->coeff_hp);

    // Inizializza stati filtri
    reset_stato_filtro(&sys->filtro_acc);
    reset_stato_filtro(&sys->filtro_vel);
    reset_stato_filtro(&sys->filtro_spost);

    // Inizializza trigger
    if (init_trigger(&sys->trigger, config->frequenza,
                     config->sta_sec, config->lta_sec) != 0) {
        return -1;
    }

    // Inizializza integratori
    init_integratore(&sys->int_vel);
    init_integratore(&sys->int_spost);

    sys->fase = STATO_ATTESA_TRIGGER;
    sys->pgd_max = 0.0;
    sys->pgd_allarme = 0.0;
    sys->indice_campione = 0;
    sys->indice_trigger = -1;
    sys->indice_allarme = -1;

    return 0;
}

void free_dosews(StatoDOSEWS *sys) {
    free_trigger(&sys->trigger);
}

StatoSistema processa_campione(StatoDOSEWS *sys, double acc_g) {
    const ConfigSistema *cfg = &sys->config;

    double acc_ms2 = acc_g * G;
    double acc_filt = applica_filtro(acc_ms2, &sys->coeff_hp, &sys->filtro_acc);

    sys->indice_campione++;

    if (sys->fase == STATO_ATTESA_TRIGGER) {
        int scattato = aggiorna_trigger(&sys->trigger, acc_filt, cfg->soglia_sta_lta);
        if (scattato) {
            sys->fase = STATO_TRIGGERED;
            sys->indice_trigger = sys->indice_campione;
            printf("Trigger rilevato a: %.3f s (campione %lld)\n",
                   sys->indice_campione / cfg->frequenza, sys->indice_campione);
        }
        return sys->fase;
    }

    double vel = aggiorna_integratore(&sys->int_vel, acc_filt, cfg->dt);

    double vel_filt = applica_filtro(vel, &sys->coeff_hp, &sys->filtro_vel);

    double spost = aggiorna_integratore(&sys->int_spost, vel_filt, cfg->dt);

    double spost_filt = applica_filtro(spost, &sys->coeff_hp, &sys->filtro_spost);

    double pgd = fabs(spost_filt);

    if (pgd > sys->pgd_max) {
        sys->pgd_max = pgd;
    }

    if (sys->fase == STATO_TRIGGERED) {
        int allarme = valuta_allarme_istantaneo(pgd, cfg->tipologia,
                                                cfg->n_piani, cfg->soglia_target);
        if (allarme) {
            sys->fase = STATO_ALLARME;
            sys->indice_allarme = sys->indice_campione;
            sys->pgd_allarme = pgd;
            printf(">>> ALLARME a: %.3f s (campione %lld)\n",
                   sys->indice_campione / cfg->frequenza, sys->indice_campione);
        }
    }

    return sys->fase;
}

void stampa_risultati(const StatoDOSEWS *sys) {
    const ConfigSistema *cfg = &sys->config;

    if (sys->indice_trigger < 0) {
        printf("Nessun trigger rilevato.\n");
        return;
    }

    const ConfigurazioneAllarme *config_danno = get_configurazione(cfg->tipologia, cfg->n_piani);

    double soglia_fisica, soglia_prob;
    if (strcmp(cfg->soglia_target, "MDS") == 0) {
        soglia_fisica = config_danno->mds;
        soglia_prob   = config_danno->p_mds;
    } else if (strcmp(cfg->soglia_target, "EDS") == 0) {
        soglia_fisica = config_danno->eds;
        soglia_prob   = config_danno->p_eds;
    } else {
        soglia_fisica = config_danno->cds;
        soglia_prob   = config_danno->p_cds;
    }

    double t_trigger = sys->indice_trigger / cfg->frequenza;
    double t_allarme = (sys->indice_allarme >= 0) ? sys->indice_allarme / cfg->frequenza : 0.0;

    double drift_mediano = 0.0;
    double prob_calcolata = 0.0;
    double lead_time = 0.0;

    if (sys->fase == STATO_ALLARME) {
        double log10_drift = REGRESSIONE_INTERCETTA + REGRESSIONE_PENDENZA * log10(sys->pgd_allarme);
        drift_mediano = pow(10.0, log10_drift);
        prob_calcolata = calcola_probabilita_previsiva(sys->pgd_allarme, soglia_fisica);

        /* Lead time = tempo tra allarme e fine del segnale (proxy del picco sismico) */
        lead_time = (sys->pgd_max > sys->pgd_allarme)
                    ? (sys->indice_campione - sys->indice_allarme) / cfg->frequenza
                    : 0.0;
    }

    stampa_report_allarme(cfg->soglia_target, t_trigger, t_allarme,
                          sys->pgd_allarme, sys->pgd_max, drift_mediano,
                          soglia_fisica, prob_calcolata, soglia_prob,
                          lead_time, (sys->fase == STATO_ALLARME));
}
