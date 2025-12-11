#include <stdio.h>
#include <math.h>

int rileva_trigger(const double *acc_fir, int n_samples, double fs,
                  double sta_sec, double lta_sec, double threshold,
                  int start_idx, int *trigger_idx) {
    
    double dt = 1.0 / fs;
    int sta_len = (int)(sta_sec * fs);
    int lta_len = (int)(lta_sec * fs);
    
    int trigger_found = 0;
    *trigger_idx = -1;
    
    FILE *fp_debug = fopen("trigger_debug.txt", "w");
    fprintf(fp_debug, "#idx tempo STA LTA ratio\n");
    
    for (int i = start_idx; i < n_samples; i++) {
        
        double sta = 0.0;
        for (int j = i - sta_len + 1; j <= i; j++) {
            sta += fabs(acc_fir[j]);
        }
        sta = sta / sta_len;
        
        double lta = 0.0;
        for (int j = i - lta_len + 1; j <= i; j++) {
            lta += fabs(acc_fir[j]);
        }
        lta = lta / lta_len;
        
        double ratio = 0.0;
        if (lta > 0.0) {
            ratio = sta / lta;
        }
        
        fprintf(fp_debug, "%d %.3f %.6e %.6e %.3f\n", 
                i, i*dt, sta, lta, ratio);
        
        if (!trigger_found && ratio > threshold) {
            trigger_found = 1;
            *trigger_idx = i;
            
            printf("\n*** TRIGGER RILEVATO ***\n");
            printf("Campione: %d\n", i - start_idx + 2);
            printf("Indice: %d\n", i);
            printf("Tempo: %.3f s\n", i * dt);
            printf("STA: %.6e\n", sta);
            printf("LTA: %.6e\n", lta);
            printf("Ratio: %.3f\n", ratio);
            printf("************************\n\n");
        }
    }
    
    fclose(fp_debug);
    
    if (!trigger_found) {
        printf("\nNessun trigger rilevato\n");
    }
    
    return trigger_found;
}