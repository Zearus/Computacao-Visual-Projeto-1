//Nome: Francesco Zangrandi Coppola RA: 10403340
//Nome: João Victor Dallapé Madeira RA: 10400725

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

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

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,"Uso: %s caminho_imagem\n", argv[0]);
        return 1;
    }
    const char *path = argv[1];

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Erro ao iniciar a SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    if (!TTF_Init()) {
        SDL_Log("Erro ao iniciar SDL_ttf: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        SDL_Log("Erro ao iniciar SDL_ttf: %s", SDL_GetError());
        TTF_Quit(); SDL_Quit(); return 1;
    }

    GrayImage *orig = load_and_convert_to_gray(path);
    if (!orig) { TTF_Quit(); SDL_Quit(); return 1; }

    // --- JANELAS ---
    int min_w = 800, min_h = 600;
    int win_w = (orig->w < min_w ? min_w : orig->w);
    int win_h = (orig->h < min_h ? min_h : orig->h);

    SDL_Window *win_img = SDL_CreateWindow("Imagem", win_w, win_h, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren_img = SDL_CreateRenderer(win_img, NULL);
    SDL_Texture *tex_img = SDL_CreateTexture(ren_img, SDL_PIXELFORMAT_RGB24,
                                             SDL_TEXTUREACCESS_STREAMING, orig->w, orig->h);

    int hist_w = 600, hist_h = win_h;
    SDL_Window *win_hist = SDL_CreateWindow("Histograma", hist_w, hist_h, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren_hist = SDL_CreateRenderer(win_hist, NULL);

    GrayImage *current = malloc(sizeof(GrayImage));
    current->w = orig->w; current->h = orig->h;
    current->pixels = malloc((size_t)orig->w * orig->h);
    memcpy(current->pixels, orig->pixels, (size_t)orig->w*orig->h);

    GrayImage *eq = NULL;
    bool is_equalized = false;

    // Botão
    SDL_FRect button = {20, 20, 150, 40};
    bool mouse_over_button = false;
    bool mouse_down_button = false;

    // --- LOOP ---
    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;

            else if (e.type == SDL_EVENT_MOUSE_MOTION && e.motion.windowID == SDL_GetWindowID(win_hist)) {
                float mx = e.motion.x, my = e.motion.y;
                mouse_over_button = (mx >= button.x && mx <= button.x + button.w &&
                                     my >= button.y && my <= button.y + button.h);
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.windowID == SDL_GetWindowID(win_hist)) {
                float mx = e.button.x, my = e.button.y;
                if (mx >= button.x && mx <= button.x + button.w &&
                    my >= button.y && my <= button.y + button.h) {
                    mouse_down_button = true;
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.windowID == SDL_GetWindowID(win_hist)) {
                float mx = e.button.x, my = e.button.y;
                if (mouse_down_button &&
                    mx >= button.x && mx <= button.x + button.w &&
                    my >= button.y && my <= button.y + button.h) {
                    if (!is_equalized) {
                        if (!eq) eq = equalize_histogram(orig);
                        GrayImage *tmp = current; current = eq; eq = tmp;
                        is_equalized = true;
                    } else {
                        if (eq) { GrayImage *tmp = current; current = eq; eq = tmp; }
                        is_equalized = false;
                    }
                }
                mouse_down_button = false;
            }
        }

        const bool *kb = SDL_GetKeyboardState(NULL);
        if (kb[SDL_SCANCODE_ESCAPE]) running = false;
        if (kb[SDL_SCANCODE_S]) { save_gray_png(current, "output_image.png"); SDL_Delay(200); }

        // atualizar textura da imagem
        int pitch = current->w * 3;
        Uint8 *buffer = malloc((size_t)current->w * current->h * 3);
        for (int i = 0; i < current->w*current->h; i++) {
            Uint8 v = current->pixels[i];
            buffer[i*3+0] = v;
            buffer[i*3+1] = v;
            buffer[i*3+2] = v;
        }
        SDL_UpdateTexture(tex_img, NULL, buffer, pitch);
        free(buffer);

        // renderizar imagem
        SDL_SetRenderDrawColor(ren_img, 0,0,0,255);
        SDL_RenderClear(ren_img);
        SDL_RenderTexture(ren_img, tex_img, NULL, NULL);
        SDL_RenderPresent(ren_img);

        // renderizar histograma
        int hist[256]; double mean, std;
        compute_histogram(current, hist, &mean, &std);
        int maxbin = 1; for (int i=0;i<256;i++) if(hist[i]>maxbin) maxbin = hist[i];

        SDL_SetRenderDrawColor(ren_hist, 240,240,240,255);
        SDL_RenderClear(ren_hist);
        for (int i=0; i<256; i++) {
            float bar_h = (float)hist[i]/maxbin * (hist_h-100);
            SDL_FRect r = { (float)i*(hist_w/256.0f), hist_h-bar_h-20, (float)(hist_w/256.0f), bar_h };
            SDL_SetRenderDrawColor(ren_hist, 50,50,200,255);
            SDL_RenderFillRect(ren_hist, &r);
        }

        // Botão com estados
        if (mouse_down_button) SDL_SetRenderDrawColor(ren_hist, 0,0,100,255);
        else if (mouse_over_button) SDL_SetRenderDrawColor(ren_hist, 100,100,255,255);
        else SDL_SetRenderDrawColor(ren_hist, 0,0,200,255);
        SDL_RenderFillRect(ren_hist, &button);
        SDL_SetRenderDrawColor(ren_hist, 0,0,0,255);
        SDL_RenderRect(ren_hist, &button);

        // Texto do botão
        const char *label = is_equalized ? "Original" : "Equalizar";
        SDL_Surface *btnsurf = TTF_RenderText_Blended(font, label, strlen(label), (SDL_Color){255,255,255,255});
        if (btnsurf) {
            SDL_Texture *btntex = SDL_CreateTextureFromSurface(ren_hist, btnsurf);
            SDL_FRect dst = {button.x + 10, button.y + 10, (float)btnsurf->w, (float)btnsurf->h};
            SDL_RenderTexture(ren_hist, btntex, NULL, &dst);
            SDL_DestroyTexture(btntex);
            SDL_DestroySurface(btnsurf);
        }

        // Texto de classificação
        const char *class_intens = (mean < 85 ? "Escura" : (mean < 170 ? "Média" : "Clara"));
        const char *class_contr = (std < 40 ? "Baixo" : (std < 80 ? "Médio" : "Alto"));
        char msg[128];
        snprintf(msg, sizeof(msg), "Intensidade: %s | Contraste: %s", class_intens, class_contr);
        SDL_Surface *txtsurf = TTF_RenderText_Blended(font, msg, strlen(msg), (SDL_Color){0,0,0,255});
        if (txtsurf) {
            SDL_Texture *txttex = SDL_CreateTextureFromSurface(ren_hist, txtsurf);
            SDL_FRect dst2 = {20, 70, (float)txtsurf->w, (float)txtsurf->h};
            SDL_RenderTexture(ren_hist, txttex, NULL, &dst2);
            SDL_DestroyTexture(txttex);
            SDL_DestroySurface(txtsurf);
        }

        SDL_RenderPresent(ren_hist);
        SDL_Delay(16);
    }

    // --- LIMPEZA ---
    if (eq) { free_gray(eq); }
    free_gray(current);
    free_gray(orig);
    SDL_DestroyTexture(tex_img);
    SDL_DestroyRenderer(ren_img);
    SDL_DestroyRenderer(ren_hist);
    SDL_DestroyWindow(win_img);
    SDL_DestroyWindow(win_hist);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}


