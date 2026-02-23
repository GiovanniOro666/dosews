#include "integrazione.h"
#include <stdio.h>

void integra_trapezi(const double *segnale_input, double *segnale_output, int n_campioni, double dt) {
    segnale_output[0] = 0.0;
    
    for (int i = 1; i < n_campioni; i++) {
        double area_trapezio = 0.5 * dt * (segnale_input[i-1] + segnale_input[i]);
        segnale_output[i] = segnale_output[i-1] + area_trapezio;
    }
}
