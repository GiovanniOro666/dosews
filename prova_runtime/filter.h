#ifndef FILTER_H
#define FILTER_H

typedef struct {
    double x1, x2;
    double y1, y2;
} StatoFiltro;

typedef struct {
    double a0, a1, a2;
    double b1, b2;
} CoeffFiltro;

void calcola_coeff_highpass(double fs, double fc, CoeffFiltro *coeff);
void calcola_coeff_lowpass(double fs, double fc, CoeffFiltro *coeff);

void filtro_highpass(const double *in, double *out, int n, const CoeffFiltro *coeff);
void filtro_lowpass(const double *in, double *out, int n, const CoeffFiltro *coeff);

double applica_filtro(double x0, const CoeffFiltro *coeff, StatoFiltro *stato);

void reset_stato_filtro(StatoFiltro *stato);

#endif
