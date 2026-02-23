#ifndef FILTER_H
#define FILTER_H

void calcola_coeff_highpass(double fs, double fc,
                            double *a0, double *a1, double *a2,
                            double *b1, double *b2);

void calcola_coeff_lowpass(double fs, double fc,
                           double *a0, double *a1, double *a2,
                           double *b1, double *b2);

void filtro_highpass(const double *in, double *out, int n,
                     double a0, double a1, double a2,
                     double b1, double b2);

void filtro_lowpass(const double *in, double *out, int n,
                    double a0, double a1, double a2,
                    double b1, double b2);

double applica_filtro_singolo(double x0, double a0, double a1, double a2,
                              double b1, double b2, double *x1, double *x2,
                              double *y1, double *y2);

#endif