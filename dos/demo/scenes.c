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

static uint8_t cur;
static uint16_t tbl[17];                /* tablas pequenas para la GPU (alineadas) */

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
    default: break;
    }
}
