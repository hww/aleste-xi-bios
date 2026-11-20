/* gpudemo.c - GPUDEMO.COM: demo y banco de pruebas de la LX-GPU
 * Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
 * Guion: hww (H2W Lab), "GPU Demo and Benchmark Scenario".
 *
 *   GPUDEMO [--m 8|4] [--s 64|32] [--b N] [--t N] [--bench] [--only N]
 *
 * El Z80 solo organiza: pone los parametros de cada cuadro, lanza el
 * microprograma de la escena y el de la linea de estado, y mide con el
 * temporizador de 1 MHz. Los objetos (posiciones, velocidades) y los
 * graficos (sprites, tiles) los crea y los mueve la propia GPU. Doble bufer
 * en C50000h/C60000h, cambiando R19 en el VBlank. Las cifras solo valen en
 * la MiSTer y en la simulacion del core: la GPU de alestelx no cuenta ciclos.
 *
 * Modos (--m, --s; sin ellos, 8 bpp y 4 bpp, 64 KB y 32 KB):
 *   8 bpp 64 KB = 320x200    8 bpp 32 KB = 320x96 (ventana)
 *   4 bpp 32 KB = 320x200    4 bpp 64 KB = 640x200
 *
 * Presupuesto (--b): el tiempo del cuadro entero (Z80 + GPU + linea de
 * estado) tiene que caber en b cuadros de video. En las escenas que crecen,
 * el numero de objetos se dobla mientras quepa y luego se afina entre N y 2N.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hw.h"
#include "demo.h"
#include "pk_clear.h"
#include "pk_overlay.h"

#define BUF0 0xC5
#define BUF1 0xC6

/* ---- memoria compartida con la GPU (palabras en direcciones pares) */
#define POOL_WORDS 1700
static uint16_t pool_raw[POOL_WORDS + 1];
static uint16_t* pool;
uint16_t*        P;                 /* parametros de la escena (16 palabras) */
static uint16_t* PO;                /* parametros de la linea de estado */
static char*     ov_text;           /* texto de la linea de estado */
static uint8_t*  font;              /* la fuente 8x8 de la BIOS (96 caracteres) */
static uint16_t* pk_scene;          /* paquete de la escena (copia parcheada) */
static uint16_t* pk_ov;
static uint16_t* pk_misc;
static uint32_t  pk_scene_phys, pk_ov_phys, pk_misc_phys;

/* ---- opciones */
uint8_t opt_b = 1, opt_bench, opt_t;
static uint8_t opt_m, opt_s, opt_only;

/* ---- video */
vmode_t  vm;
uint32_t t_frame;                   /* us entre VBlanks */
static uint8_t back, front;
static uint8_t crtc_saved[22];

/* ---- resultados */
#define MAX_ROWS 64
typedef struct {
    char     scene[12];
    char     result[22];
    uint8_t  bpp, kb;
    uint16_t fps10;                 /* FPS x 10 */
    uint8_t  cpu, gpu;              /* % */
    uint16_t ms10;                  /* ms por cuadro x 10 */
} row_t;
static row_t rows[MAX_ROWS];
static uint8_t nrows;
static char status_name[24];

/* ------------------------------------------------------------------ util */
void u32dec(char* s, uint32_t v, uint8_t frac) {    /* v / 10^frac */
    char tmp[12];
    uint8_t n = 0, i = 0;
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v || n <= frac);
    while (n) {
        if (n == frac && frac) s[i++] = '.';
        s[i++] = tmp[--n];
    }
    s[i] = 0;
}

static void load_pk(uint16_t* dst, const uint16_t* src, uint16_t n, uint16_t* params) {
    uint32_t pp = phys(params);
    memcpy(dst, src, n * 2);
    dst[2] = (uint16_t)(pp & 0xFFFF) >> 1;      /* LDI R30: indice de palabra */
    dst[4] = (uint16_t)(pp >> 16);              /* LDI R31: pagina */
}

void load_scene(const uint16_t* pk, uint16_t n) {
    load_pk(pk_scene, pk, n, P);
}

static uint8_t run(uint32_t pk) {
    gpu_start(pk);
    return gpu_wait();
}

/* ------------------------------------------------------------------ video */
static void pal_init(void) {
    static const uint16_t base[16] = {
        0x000, 0x00A, 0x0A0, 0x0AA, 0xA00, 0xA0A, 0xA50, 0xAAA,
        0x555, 0x55F, 0x5F5, 0x5FF, 0xF55, 0xF5F, 0xFF5, 0xFFF };
    static const uint8_t hue[15][3] = {
        {15,0,0},{15,8,0},{15,15,0},{8,15,0},{0,15,0},{0,15,8},{0,15,15},{0,8,15},
        {0,0,15},{8,0,15},{15,0,15},{15,0,8},{15,15,15},{15,10,6},{6,10,15} };
    uint16_t i;
    for (i = 0; i < 16; i++) pal_set((uint8_t)i, base[i]);
    /* 16-255: 15 rampas de 16 tonos (oscuro -> claro) */
    for (i = 16; i < 256; i++) {
        uint8_t h = (uint8_t)((i - 16) >> 4), v = (uint8_t)(i & 15);
        uint8_t r = (uint8_t)(hue[h][0] * v / 15), g = (uint8_t)(hue[h][1] * v / 15),
                b = (uint8_t)(hue[h][2] * v / 15);
        pal_set((uint8_t)i, (uint16_t)((r << 8) | (g << 4) | b));
    }
}

void clear_buf(uint8_t page, uint16_t word) {
    P[0] = (uint16_t)(vm.gmode << 8) | page;
    P[13] = word;
    P[14] = (uint16_t)((uint32_t)vm.pitch * vm.height / vm.ppw);
    load_pk(pk_misc, pk_clear, sizeof(pk_clear) / 2, P);
    run(pk_misc_phys);
}

void clear_both(uint16_t word) {
    clear_buf(BUF0, word);
    clear_buf(BUF1, word);
}

static void set_mode(uint8_t bpp, uint8_t kb) {
    vm.bpp = bpp;
    vm.kb = kb;
    vm.gmode = bpp == 8 ? 1 : 2;
    vm.width = (bpp == 4 && kb == 64) ? 640 : 320;
    vm.height = (bpp == 8 && kb == 32) ? 96 : 200;
    vm.pitch = vm.width;
    vm.ppw = bpp == 8 ? 2 : 4;
    back = BUF1;
    front = BUF0;
    clear_both(0);
    vblank_wait();
    crtc_set(12, 0);
    crtc_set(13, 0);
    crtc_set(19, front);
    crtc_set(6, vm.height == 96 ? 12 : 25);
    if (bpp == 8) { crtc_set(20, 0x07); crtc_set(21, 0x02); crtc_set(18, 0x03); }
    else if (kb == 32) { crtc_set(20, 0x03); crtc_set(21, 0x01); crtc_set(18, 0x12); }
    else { crtc_set(20, 0x07); crtc_set(21, 0x02); crtc_set(18, 0x12); }
}

static void video_restore(void) {
    uint8_t i;
    vblank_wait();
    for (i = 0; i < 22; i++) crtc_set(i, crtc_saved[i]);
    pal_set(0, 0x000);
    pal_set(1, 0xFFF);
}

static void flip(void) {
    uint8_t t = back;
    vblank_wait();
    crtc_set(19, back);
    back = front;
    front = t;
}

uint8_t back_page(void) { return back; }

static uint32_t measure_frame(void) {
    uint32_t t0, t1;
    uint8_t i;
    vblank_wait();
    t0 = timer_us();
    for (i = 0; i < 20; i++) vblank_wait();
    t1 = timer_us();
    return (t1 - t0) / 20;
}

/* ------------------------------------------------------------------ medida */
static uint32_t last_flip;

static void ov_draw(uint16_t y, uint8_t fg, uint8_t bg) {
    PO[0] = (uint16_t)(vm.gmode << 8) | back;
    PO[1] = vm.pitch;
    PO[13] = 4;                     /* x */
    PO[14] = y;
    PO[15] = (uint16_t)(fg | (bg << 8));
    run(pk_ov_phys);
}

void set_status(const char* s) {
    strncpy(status_name, s, sizeof(status_name) - 1);
}

static void meas_text(const meas_t* m) {
    char a[10];
    u32dec(a, m->fps10, 1);
    sprintf(ov_text, "%sFPS|C%u%%|G%u%%|%s|B%u", a, m->cpu, m->gpu, status_name, opt_b);
}

/* 'frames' cuadros con los parametros de ahora; P6 bit 0 (iniciar) solo en
   el primero. Medias en m, sin el primer cuadro. */
void run_frames(uint16_t frames, meas_t* m) {
    uint16_t f, n = 0;
    uint16_t init = P[6];
    uint32_t t0, t1, t2, tf, render = 0, gpu = 0, shown = 0, maxr = 0;
    for (f = 0; f < frames; f++) {
        t0 = timer_us();
        P[0] = (uint16_t)(vm.gmode << 8) | back;
        P[5]++;
        P[6] = f ? (init & ~1) : init;
        scene_frame(f);
        gpu_start(pk_scene_phys);
        gpu_wait();
        t1 = timer_us();
        ov_draw(0, 15, 1);
        t2 = timer_us();
        flip();
        tf = timer_us();
        if (f) {
            render += t2 - t0;
            if (t2 - t0 > maxr) maxr = t2 - t0;
            gpu += t1 - t0;
            shown += tf - last_flip;
            n++;
        }
        last_flip = tf;
    }
    P[6] = init & ~1;
    if (!n) n = 1;
    if (!shown) shown = 1;
    m->frames = n;
    m->render = render / n;
    m->maxr = maxr;
    m->scene = gpu / n;
    m->fps10 = (uint16_t)((uint32_t)n * 10000000UL / shown);
    m->cpu = (uint8_t)((render - gpu) * 100 / shown);
    m->gpu = (uint8_t)(gpu * 100 / shown);
    meas_text(m);
}

uint8_t fits(const meas_t* m) {
    return m->render <= (uint32_t)opt_b * t_frame;
}

uint16_t step_frames(uint8_t kind) {        /* 0 paso, 1 afinado, 2 escena fija */
    uint32_t us;
    if (opt_bench) return kind == 1 ? 30 : 60;
    if (kind == 2) us = opt_t ? (uint32_t)opt_t * 1000000UL : 3000000UL;
    else us = opt_t ? (uint32_t)opt_t * 1000000UL : 1200000UL;
    if (kind == 1) us /= 2;
    return (uint16_t)(us / t_frame) + 2;
}

/* dobla N mientras quepa en el presupuesto; luego afina entre N y 2N.
   Devuelve el maximo afinado; *pow2 = la ultima potencia de 2 que cabia. */
uint16_t find_limit(const char* tag, uint16_t n0, uint16_t nmax, uint16_t* pow2, meas_t* best) {
    meas_t m;
    uint16_t n = n0, last = 0, lo, hi;
    char s[24];
    best->render = 0;
    while (n <= nmax) {
        P[4] = n;
        P[6] |= 1;
        sprintf(s, "%s N%u", tag, n);
        set_status(s);
        run_frames(step_frames(0), &m);
        if (!fits(&m)) break;
        last = n;
        *best = m;
        if (n > nmax / 2) { n = (uint16_t)(nmax + 1); break; }
        n *= 2;
    }
    *pow2 = last;
    if (!last) { *best = m; return 0; }
    lo = last;
    hi = n;
    while (hi - lo > 1 && hi - lo > lo / 16) {
        uint16_t mid = (uint16_t)(lo + (hi - lo) / 2);
        P[4] = mid;
        P[6] |= 1;
        sprintf(s, "%s N%u", tag, mid);
        set_status(s);
        run_frames(step_frames(1), &m);
        if (fits(&m)) { lo = mid; *best = m; }
        else hi = mid;
    }
    return lo;
}

void add_row(const char* scene, const char* result, const meas_t* m) {
    row_t* r;
    if (nrows >= MAX_ROWS) return;
    r = &rows[nrows++];
    strncpy(r->scene, scene, sizeof(r->scene) - 1);
    strncpy(r->result, result, sizeof(r->result) - 1);
    r->bpp = vm.bpp;
    r->kb = vm.kb;
    r->fps10 = m->fps10;
    r->cpu = m->cpu;
    r->gpu = m->gpu;
    r->ms10 = (uint16_t)(m->render / 100);
}

/* el resultado de una escena, en el centro (solo en la demo) */
void show_result(const char* s) {
    uint16_t f, frames;
    if (opt_bench) return;
    strncpy(ov_text, s, 60);
    frames = (uint16_t)(1500000UL / t_frame);
    for (f = 0; f < frames; f++) {
        ov_draw(vm.height / 2 - 4, 15, 4);
        flip();
    }
}

/* ------------------------------------------------------------------ resumen */
static void print_rows(FILE* f) {
    uint8_t i;
    char a[10], b[10];
    fprintf(f, "Scene      |bpp|KB|B|Result               |  FPS| ms/fr|CPU%%|GPU%%\n");
    for (i = 0; i < nrows; i++) {
        row_t* r = &rows[i];
        u32dec(a, r->fps10, 1);
        u32dec(b, r->ms10, 1);
        fprintf(f, "%-11s|%3u|%2u|%u|%-21s|%5s|%6s|%4u|%4u\n", r->scene, r->bpp, r->kb, opt_b,
                r->result, a, b, r->cpu, r->gpu);
    }
}

static void parse(int argc, char** argv) {
    int i;
    for (i = 1; i < argc; i++) {
        char* o = argv[i];
        char* v = i + 1 < argc ? argv[i + 1] : "0";
        if (!strcmp(o, "--M")) { opt_m = (uint8_t)atoi(v); i++; }
        else if (!strcmp(o, "--S")) { opt_s = (uint8_t)atoi(v); i++; }
        else if (!strcmp(o, "--B")) { opt_b = (uint8_t)atoi(v); i++; }
        else if (!strcmp(o, "--T")) { opt_t = (uint8_t)atoi(v); i++; }
        else if (!strcmp(o, "--ONLY")) { opt_only = (uint8_t)atoi(v); i++; }
        else if (!strcmp(o, "--BENCH")) opt_bench = 1;
    }
    if (!opt_b) opt_b = 1;
    if (opt_m != 4 && opt_m != 8) opt_m = 0;
    if (opt_s != 32 && opt_s != 64) opt_s = 0;
}

int main(int argc, char** argv) {
    uint16_t* p;
    uint8_t mi, si, sc, i;
    char a[10];
    FILE* f;
    parse(argc, argv);
    pool = (uint16_t*)(((uint16_t)pool_raw + 1) & ~1);
    p = pool;
    P = p; p += 16;
    PO = p; p += 16;
    ov_text = (char*)p; p += 32;
    font = (uint8_t*)p; p += 384;
    pk_scene = p; p += 1000;
    pk_ov = p; p += 160;
    pk_misc = p; p += 80;
    pk_scene_phys = phys(pk_scene);
    pk_ov_phys = phys(pk_ov);
    pk_misc_phys = phys(pk_misc);
    memset(P, 0, 64);
    memset(PO, 0, 64);
    memcpy(font, (void*)0xE000, 768);           /* C0E000h: ventana 3, banco 3 */
    {
        uint32_t fp = (phys(font) & 0xFFFF) * 8;
        P[11] = PO[11] = (uint16_t)fp;
        P[12] = PO[12] = (uint16_t)(fp >> 16);
    }
    P[8] = OBJ_PAGE;
    P[9] = ASSET_PAGE;
    P[10] = PO[10] = (uint16_t)(phys(pool) >> 16);
    PO[2] = (uint16_t)phys(ov_text);
    load_pk(pk_ov, pk_overlay, sizeof(pk_overlay) / 2, PO);

    printf("GPUDEMO: LX-GPU demo and benchmark (scenario by hww, H2W Lab)\n");
    for (i = 0; i < 22; i++) crtc_saved[i] = crtc_get(i);
    t_frame = measure_frame();
    u32dec(a, 10000000UL / t_frame, 1);
    printf("Video: %s Hz (%lu us per frame), budget %u frame(s)\n", a, t_frame, opt_b);
    if (opt_bench) printf("Bench mode: 60 frames per step\n");

    for (mi = 0; mi < 2; mi++) {
        uint8_t bpp = opt_m ? opt_m : (mi ? 4 : 8);
        if (opt_m && mi) break;
        for (si = 0; si < 2; si++) {
            uint8_t kb = opt_s ? opt_s : (si ? 32 : 64);
            if (opt_s && si) break;
            set_mode(bpp, kb);
            pal_init();
            for (sc = 1; sc <= NSCENES; sc++) {
                if (opt_only && sc != opt_only) continue;
                P[5] = 0;
                P[6] = 0;
                scene_run(sc);
            }
        }
    }
    video_restore();
    printf("\n");
    print_rows(stdout);
    f = fopen("BENCH.TXT", "w");
    if (f) {
        fprintf(f, "GPUDEMO results (video %s Hz, %lu us per frame, budget %u)\n", a, t_frame, opt_b);
        print_rows(f);
        fclose(f);
        printf("Saved to BENCH.TXT\n");
    }
    return 0;
}
