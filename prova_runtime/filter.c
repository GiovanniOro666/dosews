#include "filter.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void calcola_coeff_highpass(double fs, double fc, CoeffFiltro *coeff) {
    double omega_c = 2.0 * M_PI * fc / fs;
    double K = tan(omega_c / 2.0);
    double norm = 1.0 / (1.0 + sqrt(2.0) * K + K * K);

    coeff->a0 =  1.0 * norm;
    coeff->a1 = -2.0 * norm;
    coeff->a2 =  1.0 * norm;
    coeff->b1 =  2.0 * (K * K - 1.0) * norm;
    coeff->b2 =  (1.0 - sqrt(2.0) * K + K * K) * norm;
}

void calcola_coeff_lowpass(double fs, double fc, CoeffFiltro *coeff) {
    double omega_c = 2.0 * M_PI * fc / fs;
    double K = tan(omega_c / 2.0);
    double norm = 1.0 / (1.0 + sqrt(2.0) * K + K * K);

    coeff->a0 = K * K * norm;
    coeff->a1 = 2.0 * K * K * norm;
    coeff->a2 = K * K * norm;
    coeff->b1 = 2.0 * (K * K - 1.0) * norm;
    coeff->b2 = (1.0 - sqrt(2.0) * K + K * K) * norm;
}

void reset_stato_filtro(StatoFiltro *stato) {
    stato->x1 = 0.0;
    stato->x2 = 0.0;
    stato->y1 = 0.0;
    stato->y2 = 0.0;
}

double applica_filtro(double x0, const CoeffFiltro *coeff, StatoFiltro *stato) {
    double y0 = coeff->a0 * x0
              + coeff->a1 * stato->x1
              + coeff->a2 * stato->x2
              - coeff->b1 * stato->y1
              - coeff->b2 * stato->y2;

    stato->x2 = stato->x1;
    stato->x1 = x0;
    stato->y2 = stato->y1;
    stato->y1 = y0;

    return y0;
}

/* Versione batch: utile per test e validazione offline */
void filtro_highpass(const double *in, double *out, int n, const CoeffFiltro *coeff) {
    StatoFiltro stato;
    reset_stato_filtro(&stato);
    for (int i = 0; i < n; i++) {
        out[i] = applica_filtro(in[i], coeff, &stato);
    }
}

void filtro_lowpass(const double *in, double *out, int n, const CoeffFiltro *coeff) {
    StatoFiltro stato;
    reset_stato_filtro(&stato);
    for (int i = 0; i < n; i++) {
        out[i] = applica_filtro(in[i], coeff, &stato);
    }
}
