#ifndef DOSEWS_H
#define DOSEWS_H

#include "filter.h"
#include "trigger.h"
#include "integrazione.h"
#include "allarme.h"

typedef enum {
    STATO_ATTESA_TRIGGER = 0, 
    STATO_TRIGGERED,           
    STATO_ALLARME              
} StatoSistema;

typedef struct {
    double frequenza;          /* Hz */
    double dt;                 /* 1/frequenza */
    double sta_sec;            /* finestra STA [s] */
    double lta_sec;            /* finestra LTA [s] */
    double soglia_sta_lta;     /* rapporto STA/LTA per il trigger */
    double fc_hp;              /* frequenza di taglio high-pass [Hz] */
    char tipologia[16];        /* "RC", "URM_REG", "URM_STONE" */
    int n_piani;               /* numero piani edificio */
    char soglia_target[8];     /* "MDS", "EDS", "CDS" */
} ConfigSistema;

typedef struct {
    StatoSistema fase;

    CoeffFiltro coeff_hp;
    StatoFiltro filtro_acc;  
    StatoFiltro filtro_vel;    
    StatoFiltro filtro_spost;  

    StatoTrigger trigger;

    StatoIntegratore int_vel;   /* acc → vel */
    StatoIntegratore int_spost; /* vel_filt → spost */

    double pgd_max;
    double pgd_allarme;
    long long indice_campione;  /* campioni totali processati */
    long long indice_trigger;   /* campione in cui è scattato il trigger */
    long long indice_allarme;   /* campione in cui è scattato l'allarme */

    ConfigSistema config;
} StatoDOSEWS;

/* Ritorna 0 in caso di successo, -1 se errore. */
int init_dosews(StatoDOSEWS *sys, const ConfigSistema *config);

void free_dosews(StatoDOSEWS *sys);

StatoSistema processa_campione(StatoDOSEWS *sys, double acc_g);

void stampa_risultati(const StatoDOSEWS *sys);

#endif
