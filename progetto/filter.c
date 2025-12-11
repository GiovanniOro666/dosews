#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void calcola_coeff_bandpass(double fs, double f_low, double f_high,
                            double *a0, double *a1, double *a2,
                            double *b1, double *b2) {
    
    double f_center = sqrt(f_low * f_high);
    double omega0 = 2.0 * M_PI * f_center / fs;
    double BW = f_high - f_low;

    double Q = f_center / BW;
    double alpha = sin(omega0) / (2.0 * Q);
    double cos_omega0 = cos(omega0);
    double denom = 1.0 + alpha;
    *b1 = 2.0 * cos_omega0 / denom;
    *b2 = (alpha - 1.0) / denom;

    *a0 = alpha / denom;
    *a1 = 0.0;
    *a2 = -alpha / denom;

    printf("f_center: %.3f Hz, BW: %.3f Hz\n", f_center, BW);
    printf("Q=%.5e, alpha=%.5e\n", Q, alpha);
    printf("a0=%.5e, a1=%.5e, a2=%.5e\n", *a0, *a1, *a2);
    printf("b1=%.5e, b2=%.5e\n", *b1, *b2);
    printf("---------------------------\n");
}

void filtro_bandpass(const double *in, double *out, int n,
                     double a0, double a1, double a2,
                     double b1, double b2) {
    
    double x1 = 0.0, x2 = 0.0;
    double y1 = 0.0, y2 = 0.0;
    
    for (int i = 0; i < n; i++) {
        double x0 = in[i];
        
        out[i] = a0 * x0 + a1 * x1 + a2 * x2 + b1 * y1 + b2 * y2;
        
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = out[i];
    }
}

int salva_accelerazioni_filtrate(const char *filename, const double *acc_filtrate, int n) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Errore: impossibile aprire il file %s\n", filename);
        return -1;
    }
    
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%.8e\n", acc_filtrate[i]);
    }
    
    fclose(fp);
    return 0;
}
