#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <SDL3/SDL.h>

typedef struct {
    int w, h;
    Uint8 *pixels; 
} GrayImage;

static void free_gray(GrayImage *g) {
    if (!g) return;
    free(g->pixels);
    free(g);
}

// Conversão para grayscale
GrayImage *load_and_convert_to_gray(const char *path) {
    SDL_Surface *surf = IMG_Load(path);
    if (!surf) {
        fprintf(stderr, "IMG_Load erro: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Surface *conv = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA8888);
    SDL_DestroySurface(surf);
    if (!conv) {
        fprintf(stderr, "SDL_ConvertSurface falhou: %s\n", SDL_GetError());
        return NULL;
    }

    int w = conv->w, h = conv->h;
    GrayImage *g = malloc(sizeof(GrayImage));
    g->w = w; g->h = h;
    g->pixels = malloc((size_t)w*h);

    Uint8 *src = (Uint8*)conv->pixels;
    int pitch = conv->pitch;

    for (int y = 0; y < h; y++) {
        Uint8 *row = src + y * pitch;
        for (int x = 0; x < w; x++) {
            Uint8 r = row[x*4 + 0];
            Uint8 gg = row[x*4 + 1];
            Uint8 b = row[x*4 + 2];
            double Y = 0.2125*r + 0.7154*gg + 0.0721*b;
            int v = (int)round(Y);
            if (v < 0) v = 0; if (v > 255) v = 255;
            g->pixels[y*w + x] = (Uint8)v;
        }
    }

    SDL_DestroySurface(conv);
    return g;
}

// Histograma + estatísticas
void compute_histogram(const GrayImage *g, int hist[256], double *mean, double *std) {
    memset(hist, 0, sizeof(int)*256);
    long long sum = 0, sumsq = 0;
    int n = g->w * g->h;
    for (int i = 0; i < n; i++) {
        int v = g->pixels[i];
        hist[v]++;
        sum += v;
        sumsq += (long long)v * v;
    }
    *mean = (double)sum / n;
    double variance = (double)sumsq / n - (*mean)*(*mean);
    if (variance < 0) variance = 0;
    *std = sqrt(variance);
}

GrayImage *equalize_histogram(const GrayImage *g) {
    int hist[256]; double mean, std;
    compute_histogram(g, hist, &mean, &std);
    int n = g->w * g->h;
    int cdf[256];
    cdf[0] = hist[0];
    for (int i = 1; i < 256; i++) cdf[i] = cdf[i-1] + hist[i];
    int cdf_min = 0; for (int i = 0; i < 256; i++) if (cdf[i]>0) { cdf_min = cdf[i]; break; }

    GrayImage *out = malloc(sizeof(GrayImage));
    out->w = g->w; out->h = g->h;
    out->pixels = malloc((size_t)out->w * out->h);
    for (int i = 0; i < n; i++) {
        int v = g->pixels[i];
        double mapped = round((double)(cdf[v]-cdf_min) / (n-cdf_min) * 255.0);
        if (mapped < 0) mapped = 0; if (mapped > 255) mapped = 255;
        out->pixels[i] = (Uint8)mapped;
    }
    return out;
}

// Salvar PNG
bool save_gray_png(const GrayImage *g, const char *filename) {
    int w = g->w, h = g->h;
    Uint8 *buf = malloc((size_t)w*h*3); // corrigido para RGB24
    if (!buf) return false;

    for (int i = 0; i < w*h; i++) {
        Uint8 v = g->pixels[i];
        buf[i*3 + 0] = v;
        buf[i*3 + 1] = v;
        buf[i*3 + 2] = v;
    }

    SDL_Surface *surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGB24, buf, w*3);
    if (!surf) {
        fprintf(stderr,"SDL_CreateSurfaceFrom falhou: %s\n", SDL_GetError());
        free(buf);
        return false;
    }

    if (!IMG_SavePNG(surf, filename)) {
        fprintf(stderr,"IMG_SavePNG falhou: %s\n", SDL_GetError());
        SDL_DestroySurface(surf);
        free(buf);
        return false;
    }

    SDL_DestroySurface(surf);
    free(buf);
    return true;
}
