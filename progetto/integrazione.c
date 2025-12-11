#include <stdio.h>

void integra_trapezi(const double *in, double *out, int n, double dt) {
    
    out[0] = 0.0;
    
    for (int i = 1; i < n; i++) {
        out[i] = out[i-1] + 0.5 * dt * (in[i-1] + in[i]);
    }
}

void rimuovi_trend_lineare(double *data, int n) {
    // Calcola coefficienti fit lineare: y = a + b*x
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    
    for (int i = 0; i < n; i++) {
        sum_x += i;
        sum_y += data[i];
        sum_xy += i * data[i];
        sum_x2 += i * i;
    }
    
    double b = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    double a = (sum_y - b * sum_x) / n;
    
    // Rimuovi trend
    for (int i = 0; i < n; i++) {
        data[i] -= (a + b * i);
    }
}

int salva_integrazioni(const char *filename, const double *vel, 
                       const double *disp, int n) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Errore: impossibile aprire %s\n", filename);
        return -1;
    }
    
    fprintf(fp, "#vel disp\n");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "%.8e %.8e\n", vel[i], disp[i]);
    }

    fclose(fp);
    return 0;
}