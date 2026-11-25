/* scenes.c - GPUDEMO: las escenas del guion de hww (H2W Lab)
 * Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
 *
 * Cada escena carga su microprograma, pone sus parametros y o bien corre un
 * tiempo fijo (1 y 10), o bien busca el maximo de objetos que caben en el
 * presupuesto (find_limit), para cada tamano si los tiene.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "demo.h"
#include "hw.h"
#include "pk_s1_fill.h"
#include "pk_s2_lines.h"
#include "pk_s3_rects.h"
#include "pk_s4_text.h"
#include "pk_s5_sprites.h"
#include "pk_s6_tiles.h"
#include "pk_s7_parallax.h"
#include "pk_s8_rotate.h"
#include "pk_s9_scale.h"
#include "pk_s10_shift.h"
#include "pk_conv.h"

static uint8_t cur;
static uint16_t tbl[17];                /* tablas pequenas para la GPU (alineadas) */
static uint16_t big[129];               /* senos / escalas */

static uint16_t* table(void) { return (uint16_t*)(((uint16_t)tbl + 1) & ~1); }
static uint16_t widx(const void* p) { return (uint16_t)(phys(p) & 0xFFFF) >> 1; }

/* color de 'franja' i (0-15) segun el modo, repetido en una palabra */
static uint16_t band_word(uint8_t i) {
    if (vm.bpp == 8) {
        uint8_t c = (uint8_t)(16 + (i & 7) * 32 + 4 + (i & 8));   /* 8 tonos x 2 brillos */
        return (uint16_t)(c * 0x0101);
    }
    return (uint16_t)((i & 15) * 0x1111);
}

void scene_frame(uint16_t f) {
    (void)f;
}

static void result_line(const char* tag, const char* what, const meas_t* m) {
    char s[48], a[10];
    u32dec(a, m->fps10, 1);
    sprintf(s, "%s: %s, %s FPS", tag, what, a);
    add_row(tag, what, m);
    show_result(s);
}

/* parametros comunes: tamano del bufer, fondo (P13), tabla de 16 colores
   de pixel (P14) y semilla (P15) */
static void common(void) {
    uint16_t* t = table();
    uint8_t i;
    P[1] = vm.pitch;
    P[2] = vm.width;
    P[3] = vm.height;
    P[13] = vm.bpp == 8 ? 0x9292 : 0x0000;      /* azul muy oscuro / negro */
    for (i = 0; i < 16; i++) {
        uint8_t c = (uint8_t)band_word(i);
        t[i] = vm.bpp == 8 ? (uint16_t)(c + 6) : (uint16_t)(i ? i : 15);
    }
    P[14] = widx(t);
    P[15] = 12345;
}

/* resultado de una escena que crece */
static void ramp(const char* tag, const char* unit, uint16_t n0, uint16_t nmax) {
    meas_t m;
    uint16_t p2, n;
    char what[24];
    n = find_limit(tag, n0, nmax, &p2, &m);
    if (n == nmax) sprintf(what, ">=%u %s", n, unit);
    else sprintf(what, "%u %s (2^:%u)", n, unit, p2);
    {
        char s[48], a[10];
        u32dec(a, m.fps10, 1);
        sprintf(s, "%s: %s, %s FPS", tag, what, a);
        add_row(tag, what, &m);
        show_result(s);
    }
}

/* ---- escena 2: lineas */
static void scene2(void) {
    load_scene(pk_s2_lines, sizeof(pk_s2_lines) / 2);
    common();
    ramp("S2 LINES", "lines", 2, 4096);
}

/* ---- escena 3: rectangulos de 2x2 a 32x32 */
static void scene3(void) {
    static const uint8_t sizes[] = { 2, 4, 8, 16, 32 };
    uint8_t i;
    char tag[16];
    load_scene(pk_s3_rects, sizeof(pk_s3_rects) / 2);
    common();
    for (i = 0; i < sizeof(sizes); i++) {
        P[7] = sizes[i];
        if (vm.height < 100 && sizes[i] > 16) break;
        sprintf(tag, "S3 %ux%u", sizes[i], sizes[i]);
        ramp(tag, "rects", 2, 4096);
    }
}

/* ---- escena 4: caracteres */
static void scene4(void) {
    load_scene(pk_s4_text, sizeof(pk_s4_text) / 2);
    common();
    ramp("S4 CHARS", "chars", 32, 4096);
}

/* ---- escena 5: sprites con color clave, de 16x16 a 128x128 */
static void scene5(void) {
    static const uint8_t sizes[] = { 16, 32, 64, 128 };
    static const uint8_t shift[] = { 5, 7, 9, 11 };
    static const uint8_t c4[16] = { 1, 9, 15, 0, 2, 10, 15, 0, 4, 12, 15, 0, 5, 13, 15, 0 };
    static const uint8_t c8[4] = { 16, 80, 144, 48 };
    uint16_t* t;
    uint8_t i;
    char tag[16];
    meas_t m;
    load_scene(pk_s5_sprites, sizeof(pk_s5_sprites) / 2);
    common();
    t = table();
    for (i = 0; i < 16; i++) t[i] = vm.bpp == 8 ? (uint16_t)c8[i / 4] : c4[i];
    for (i = 0; i < sizeof(sizes); i++) {
        if (sizes[i] >= vm.height) break;
        P[7] = (uint16_t)(sizes[i] | (shift[i] << 8));
        P[4] = 1;
        P[6] = 3;                               /* crear las bolas */
        set_status("S5 SPRITES");
        run_frames(1, &m);
        P[6] = 0;
        sprintf(tag, "S5 %ux%u", sizes[i], sizes[i]);
        ramp(tag, "sprites", 2, 2048);
    }
}

static void tiles_table(void) {
    uint16_t* t = table();
    uint8_t i;
    for (i = 0; i < 16; i++) {
        if (vm.bpp == 8) t[i] = (uint16_t)(16 + (i & 7) * 32 + (i < 8 ? 7 : 13));
        else t[i] = (uint16_t)(i < 8 ? 1 + (i % 6) : 9 + (i % 6));
    }
    P[13] = vm.bpp == 8 ? 16 + 12 * 16 + 4 : 8;            /* mortero */
}

/* crea los tiles (escena 6) en la pagina P9 */
static void make_tiles(void) {
    meas_t m;
    load_scene(pk_s6_tiles, sizeof(pk_s6_tiles) / 2);
    common();
    tiles_table();
    P[4] = 1;
    P[6] = 3;
    run_frames(1, &m);
    P[6] = 0;
}

/* ---- escena 6: tiles: fondo opaco y gemas con color clave que se desplazan */
static void scene6(void) {
    meas_t m;
    uint16_t cells, n, p2;
    char what[24], s[48], a[10];
    set_status("S6 TILES");
    make_tiles();
    cells = (vm.width / 8) * (vm.height / 8);
    n = find_limit("S6 TILES", 8, cells, &p2, &m);
    sprintf(what, "%u tiles (%u%%)", n, (uint16_t)((uint32_t)n * 100 / cells));
    u32dec(a, m.fps10, 1);
    sprintf(s, "S6 TILES: %s, %s FPS", what, a);
    add_row("S6 TILES", what, &m);
    show_result(s);
}

/* ---- escena 7: parallax: fondo y 1-3 capas (1, 2 y 4 pixeles por cuadro) */
static void scene7(void) {
    meas_t m;
    uint32_t prev = 0;
    uint16_t* t;
    uint8_t l, i;
    char tag[12], what[24], s[48], a[10], b[10];
    set_status("S7 PARALLAX");
    make_tiles();
    load_scene(pk_s7_parallax, sizeof(pk_s7_parallax) / 2);
    t = table();
    for (i = 0; i < 16; i++) {                      /* cielo: azul, de oscuro a claro y vuelta */
        uint8_t v = (uint8_t)(i < 8 ? i : 15 - i);
        uint8_t c = vm.bpp == 8 ? (uint8_t)(16 + 8 * 16 + 2 + v) : (uint8_t)(v < 4 ? 1 : 9);
        t[i] = vm.bpp == 8 ? (uint16_t)(c * 0x0101) : (uint16_t)(c * 0x1111);
    }
    for (l = 0; l <= 3; l++) {
        P[7] = l;
        sprintf(tag, "S7 L%u", l);
        set_status(tag);
        run_frames(step_frames(0), &m);
        if (!l) sprintf(what, "background");
        else {
            uint32_t d = m.render > prev ? m.render - prev : 0;
            u32dec(a, d / 100, 1);
            u32dec(b, d * 1000 / ((uint32_t)opt_b * t_frame), 1);
            sprintf(what, "+%s ms (%s%%)", a, b);
        }
        prev = m.render;
        u32dec(a, m.fps10, 1);
        sprintf(s, "%s: %s, %s FPS", tag, what, a);
        add_row(tag, what, &m);
        show_result(s);
    }
}

static uint16_t* bigt(void) { return (uint16_t*)(((uint16_t)big + 1) & ~1); }

/* 16 colores vivos para el sprite de 32x32 (escenas 8 y 9) */
static void sprite_table(void) {
    uint16_t* t = table();
    uint8_t i;
    for (i = 0; i < 16; i++) t[i] = vm.bpp == 8 ? (uint16_t)(16 + (i % 12) * 16 + 12 + (i / 12) * 3) : (uint16_t)(1 + (i % 14));
}

/* ---- escena 8: sprites que giran */
static void scene8(void) {
    static const int16_t s16[17] = { 0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255, 256 };
    uint16_t* t = bigt();
    uint8_t i;
    meas_t m;
    load_scene(pk_s8_rotate, sizeof(pk_s8_rotate) / 2);
    common();
    sprite_table();
    for (i = 0; i < 64; i++) {                  /* seno de 64 pasos, 8.8 */
        uint8_t q = (uint8_t)(i & 15);
        int16_t v = (i & 16) ? s16[16 - q] : s16[q];
        t[i] = (uint16_t)((i & 32) ? -v : v);
    }
    P[7] = widx(t);
    P[4] = 1;
    P[6] = 3;
    set_status("S8 ROTATE");
    run_frames(1, &m);
    P[6] = 0;
    ramp("S8 ROTATE", "sprites", 1, 1024);
}

/* ---- escena 9: sprites a escala */
static void scene9(void) {
    uint16_t* t = bigt();
    uint8_t i;
    meas_t m;
    load_scene(pk_s9_scale, sizeof(pk_s9_scale) / 2);
    common();
    sprite_table();
    for (i = 0; i < 32; i++) {                  /* lado 16..96..16, paso = 32 * 256 / lado */
        uint8_t k = (uint8_t)(i < 16 ? i : 31 - i);
        uint16_t side = (uint16_t)(16 + k * 16 / 3);
        if (vm.height < 100 && side > 64) side = 64;
        t[i * 2] = side;
        t[i * 2 + 1] = (uint16_t)(8192 / side);
    }
    P[7] = widx(t);
    P[4] = 1;
    P[6] = 3;
    set_status("S9 SCALE");
    run_frames(1, &m);
    P[6] = 0;
    ramp("S9 SCALE", "sprites", 1, 1024);
}

/* ---- escena 10: una imagen desplazada linea a linea */
#define IMG8 0x23                           /* la imagen en 8 bpp */
#define IMG4 0x24                           /* y en 4 bpp */
#define CHUNK 2048
static uint8_t chunk_raw[CHUNK + 2];
static uint8_t lut[256];
static uint16_t img_pal[256];
static uint16_t img_w, img_h, img_n;
static uint8_t img_ok;

static uint8_t load_image(const char* name) {
    FILE* f = fopen(name, "rb");
    uint8_t* buf = (uint8_t*)(((uint16_t)chunk_raw + 1) & ~1);
    uint8_t hdr[10];
    uint32_t left, off = 0;
    uint16_t i;
    if (!f) return 0;
    if (fread(hdr, 1, 10, f) != 10 || hdr[0] != 'L' || hdr[1] != 'X' || hdr[2] != 'I' || hdr[3] != '1') { fclose(f); return 0; }
    img_w = hdr[4] | (hdr[5] << 8);
    img_h = hdr[6] | (hdr[7] << 8);
    img_n = hdr[8] | (hdr[9] << 8);
    if (img_w != 320 || img_h > 200 || !img_n || img_n > 256) { fclose(f); return 0; }
    fread(img_pal, 2, img_n, f);
    left = (uint32_t)img_w * img_h;
    while (left) {
        uint16_t n = left > CHUNK ? CHUNK : (uint16_t)left;
        fread(buf, 1, n, f);
        gpu_copy((uint8_t)(phys(buf) >> 16), (uint16_t)(phys(buf) & 0xFFFF) >> 1, IMG8, (uint16_t)(off >> 1), (n + 1) / 2);
        off += n;
        left -= n;
    }
    fclose(f);
    /* 4 bpp: el mas parecido de los 16 colores base */
    for (i = 0; i < img_n; i++) {
        static const uint16_t base[16] = {
            0x000, 0x00A, 0x0A0, 0x0AA, 0xA00, 0xA0A, 0xA50, 0xAAA,
            0x555, 0x55F, 0x5F5, 0x5FF, 0xF55, 0xF5F, 0xFF5, 0xFFF };
        uint8_t k, best = 0;
        uint16_t bd = 0xFFFF;
        int8_t r = (int8_t)((img_pal[i] >> 8) & 15), g = (int8_t)((img_pal[i] >> 4) & 15), b = (int8_t)(img_pal[i] & 15);
        for (k = 0; k < 16; k++) {
            int8_t dr = r - (int8_t)((base[k] >> 8) & 15), dg = g - (int8_t)((base[k] >> 4) & 15), db = b - (int8_t)(base[k] & 15);
            uint16_t d = (uint16_t)(dr * dr * 3 + dg * dg * 4 + db * db * 2);
            if (d < bd) { bd = d; best = k; }
        }
        lut[i] = best;
    }
    load_scene(pk_conv, sizeof(pk_conv) / 2);
    P[13] = IMG8;
    P[3] = IMG4;
    P[2] = img_w * img_h;
    P[14] = (uint16_t)phys(lut);
    gpu_run_scene();
    return 1;
}

static void scene10(void) {
    static const char* const names[] = { "RISE.LXI", "NEBULA.LXI", "MANDEL.LXI" };
    meas_t m;
    uint8_t i, ph;
    char tag[12], s[48], a[10];
    if (!img_ok) {
        set_status("S10 LOADING");
        show_message("S10: LOADING IMAGE...");
        for (i = 0; i < 3 && !img_ok; i++) img_ok = load_image(names[i]);
        if (!img_ok) { add_row("S10 SHIFT", "no .LXI image", &m); return; }
    }
    if (vm.bpp == 8) { uint16_t k; for (k = 0; k < img_n; k++) pal_set((uint8_t)k, img_pal[k]); }
    load_scene(pk_s10_shift, sizeof(pk_s10_shift) / 2);
    common();
    P[13] = vm.bpp == 8 ? IMG8 : IMG4;
    P[14] = img_w / vm.ppw;
    P[15] = img_h;
    for (ph = 0; ph < 2; ph++) {
        P[7] = ph;
        strcpy(tag, ph ? "S10 SHIFT V" : "S10 SHIFT H");
        set_status(tag);
        run_frames(step_frames(2) / 2 + 1, &m);
        u32dec(a, m.fps10, 1);
        sprintf(s, "%s: %s FPS", tag, a);
        add_row(tag, ph ? "lines up" : "lines left/right", &m);
        show_result(s);
    }
    if (vm.bpp == 8) pal_restore();
}

/* ---- escena 1: relleno */
static void scene1(void) {
    meas_t m;
    char what[24], a[10];
    uint32_t mpix100;
    load_scene(pk_s1_fill, sizeof(pk_s1_fill) / 2);
    P[1] = vm.pitch;
    P[2] = vm.width;
    P[3] = vm.height;
    {
        uint16_t* t = table();
        uint8_t i;
        for (i = 0; i < 16; i++) t[i] = band_word(i);
        P[15] = widx(t);
    }
    P[13] = vm.pitch / vm.ppw;
    P[14] = vm.width / vm.ppw;
    set_status("S1 FILL");
    run_frames(3, &m);
    run_frames(step_frames(2), &m);
    /* Mpix/s con el tiempo de la escena (sin la linea de estado) */
    mpix100 = m.scene ? (uint32_t)vm.width * vm.height * 100 / m.scene : 0;
    u32dec(a, mpix100, 2);
    sprintf(what, "%s Mpix/s", a);
    result_line("S1 FILL", what, &m);
}

void scene_run(uint8_t n) {
    cur = n;
    switch (n) {
    case 1: scene1(); break;
    case 2: scene2(); break;
    case 3: scene3(); break;
    case 4: scene4(); break;
    case 5: scene5(); break;
    case 6: scene6(); break;
    case 7: scene7(); break;
    case 8: scene8(); break;
    case 9: scene9(); break;
    case 10: scene10(); break;
    default: break;
    }
}
