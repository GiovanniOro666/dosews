#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dosews.h"
#include "output.h"


#define FREQUENZA        200.0
#define FC_HIGHPASS      0.075
#define STA_SEC          0.5
#define LTA_SEC          6.0
#define SOGLIA_STA_LTA   4.0
#define TIPOLOGIA        "RC"
#define N_PIANI          3
#define SOGLIA_DANNO     "EDS"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <file_accelerometrico>\n", argv[0]);
        return 1;
    }


    ConfigSistema config = {
        .frequenza      = FREQUENZA,
        .dt             = 1.0 / FREQUENZA,
        .sta_sec        = STA_SEC,
        .lta_sec        = LTA_SEC,
        .soglia_sta_lta = SOGLIA_STA_LTA,
        .fc_hp          = FC_HIGHPASS,
        .n_piani        = N_PIANI,
    };
    strncpy(config.tipologia,     TIPOLOGIA,    sizeof(config.tipologia) - 1);
    strncpy(config.soglia_target, SOGLIA_DANNO, sizeof(config.soglia_target) - 1);

    StatoDOSEWS sys;
    if (init_dosews(&sys, &config) != 0) {
        fprintf(stderr, "Errore: inizializzazione sistema fallita\n");
        return 1;
    }


    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Errore: impossibile aprire il file %s\n", argv[1]);
        free_dosews(&sys);
        return 1;
    }

    printf("DOSEWS avviato — file: %s\n", argv[1]);
    printf("Configurazione: %s %d piani, soglia %s, fs=%.0f Hz, HP=%.3f Hz\n\n",
           config.tipologia, config.n_piani, config.soglia_target,
           config.frequenza, config.fc_hp);


    double valore;
    while (fscanf(fp, "%lf", &valore) == 1) {
        processa_campione(&sys, valore);

        /* In produzione: qui ci sarebbe la ricezione dal sensore, non fscanf */
    }

    fclose(fp);


    stampa_risultati(&sys);

    free_dosews(&sys);
    return 0;
}
