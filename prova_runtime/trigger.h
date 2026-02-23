#ifndef TRIGGER_H
#define TRIGGER_H

typedef struct {
    double *buf_sta;      
    double *buf_lta;     
    int sta_len;
    int lta_len; 
    int sta_idx; 
    int lta_idx;     
    double sta_somma;  
    double lta_somma;
    int campioni_caricati; 
    int triggered;   
} StatoTrigger;

int init_trigger(StatoTrigger *stato, double frequenza, double sta_sec, double lta_sec);

void free_trigger(StatoTrigger *stato);

void reset_trigger(StatoTrigger *stato);

int aggiorna_trigger(StatoTrigger *stato, double campione_filtrato, double soglia);

#endif
