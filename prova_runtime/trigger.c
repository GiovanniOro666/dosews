#include "trigger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int init_trigger(StatoTrigger *stato, double frequenza, double sta_sec, double lta_sec) {
    stato->sta_len = (int)(sta_sec * frequenza);
    stato->lta_len = (int)(lta_sec * frequenza);

    stato->buf_sta = calloc(stato->sta_len, sizeof(double));
    stato->buf_lta = calloc(stato->lta_len, sizeof(double));

    if (!stato->buf_sta || !stato->buf_lta) {
        free(stato->buf_sta);
        free(stato->buf_lta);
        return -1;
    }

    reset_trigger(stato);
    return 0;
}

void free_trigger(StatoTrigger *stato) {
    free(stato->buf_sta);
    free(stato->buf_lta);
    stato->buf_sta = NULL;
    stato->buf_lta = NULL;
}

void reset_trigger(StatoTrigger *stato) {
    if (stato->buf_sta) memset(stato->buf_sta, 0, stato->sta_len * sizeof(double));
    if (stato->buf_lta) memset(stato->buf_lta, 0, stato->lta_len * sizeof(double));
    stato->sta_idx = 0;
    stato->lta_idx = 0;
    stato->sta_somma = 0.0;
    stato->lta_somma = 0.0;
    stato->campioni_caricati = 0;
    stato->triggered = 0;
}

int aggiorna_trigger(StatoTrigger *stato, double campione_filtrato, double soglia) {
    if (stato->triggered) {
        return 0;
    }

    double campione_sq = campione_filtrato * campione_filtrato;

    /* Aggiorna finestra STA: rimuove il campione che esce, inserisce il nuovo */
    stato->sta_somma -= stato->buf_sta[stato->sta_idx];
    stato->sta_somma += campione_sq;
    stato->buf_sta[stato->sta_idx] = campione_sq;
    stato->sta_idx = (stato->sta_idx + 1) % stato->sta_len;

    /* Aggiorna finestra LTA */
    stato->lta_somma -= stato->buf_lta[stato->lta_idx];
    stato->lta_somma += campione_sq;
    stato->buf_lta[stato->lta_idx] = campione_sq;
    stato->lta_idx = (stato->lta_idx + 1) % stato->lta_len;

    stato->campioni_caricati++;

    /* Aspetta che la finestra LTA sia piena prima di valutare */
    if (stato->campioni_caricati < stato->lta_len) {
        return 0;
    }

    double sta_media = stato->sta_somma / stato->sta_len;
    double lta_media = stato->lta_somma / stato->lta_len;

    if (lta_media > 1e-15) {
        double rapporto = sta_media / lta_media;
        if (rapporto >= soglia) {
            stato->triggered = 1;
            return 1;
        }
    }

    return 0;
}
