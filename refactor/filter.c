#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void calcola_coeff_highpass(double fs, double fc,
                            double *a0, double *a1, double *a2,
                            double *b1, double *b2) {
    
    double omega_c = 2.0 * M_PI * fc / fs;
    double K = tan(omega_c / 2.0);
    double norm = 1.0 / (1.0 + sqrt(2.0) * K + K * K);
    
    *a0 = 1.0 * norm;
    *a1 = -2.0 * norm;
    *a2 = 1.0 * norm;
    *b1 = 2.0 * (K * K - 1.0) * norm;
    *b2 = (1.0 - sqrt(2.0) * K + K * K) * norm;
    
    printf("High-pass: fc=%.3f Hz\n", fc);
    printf("a0=%.5e, a1=%.5e, a2=%.5e\n", *a0, *a1, *a2);
    printf("b1=%.5e, b2=%.5e\n", *b1, *b2);
    printf("---------------------------\n");
}

void calcola_coeff_lowpass(double fs, double fc,
                           double *a0, double *a1, double *a2,
                           double *b1, double *b2) {
    
    double omega_c = 2.0 * M_PI * fc / fs;
    double K = tan(omega_c / 2.0);
    double norm = 1.0 / (1.0 + sqrt(2.0) * K + K * K);
    
    *a0 = K * K * norm;
    *a1 = 2.0 * K * K * norm;
    *a2 = K * K * norm;
    *b1 = 2.0 * (K * K - 1.0) * norm;
    *b2 = (1.0 - sqrt(2.0) * K + K * K) * norm;
    
    printf("Low-pass: fc=%.3f Hz\n", fc);
    printf("a0=%.5e, a1=%.5e, a2=%.5e\n", *a0, *a1, *a2);
    printf("b1=%.5e, b2=%.5e\n", *b1, *b2);
    printf("---------------------------\n");
}

void filtro_highpass(const double *in, double *out, int n,
                     double a0, double a1, double a2,
                     double b1, double b2) {
    
    double x1 = 0.0;
    double x2 = 0.0;
    double y1 = 0.0;
    double y2 = 0.0;
    
    for (int i = 0; i < n; i++) {
        double x0 = in[i];
        out[i] = a0 * x0 + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = out[i];
    }
}

void filtro_lowpass(const double *in, double *out, int n,
                    double a0, double a1, double a2,
                    double b1, double b2) {
    
    double x1 = 0.0;
    double x2 = 0.0;
    double y1 = 0.0;
    double y2 = 0.0;
    
    for (int i = 0; i < n; i++) {
        double x0 = in[i];
        out[i] = a0 * x0 + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;
        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = out[i];
    }
}

double applica_filtro_singolo(double x0, double a0, double a1, double a2, 
                              double b1, double b2, double *x1, double *x2, 
                              double *y1, double *y2) {
    double y0 = a0 * x0 + a1 * (*x1) + a2 * (*x2) - b1 * (*y1) - b2 * (*y2);
    *x2 = *x1;
    *x1 = x0;
    *y2 = *y1;
    *y1 = y0;
    return y0;
}