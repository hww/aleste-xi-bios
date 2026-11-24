/* demo.h - GPUDEMO: lo que comparten el programa y las escenas
 * Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
 *
 * Parametros de los microprogramas (16 palabras en P; cada microprograma
 * empieza con LDI R30/R31 = indice de palabra y pagina de P, que GPUDEMO
 * parchea, y los copia a R16-R31):
 *   P0  destino {modo, pagina}: el bufer de atras (8 bpp = 1, 4 bpp = 2)
 *   P1  ancho de linea del bufer (pixeles)   P2 ancho visible   P3 alto
 *   P4  N (objetos)   P5 numero de cuadro    P6 bit 0: iniciar los objetos
 *   P7  tamano / fase de la escena
 *   P8  pagina de los objetos (200000h)      P9 pagina de los graficos
 *   P10 pagina de los datos del Z80 (la del programa)
 *   P11 pagina de la fuente 8x8 (1 bpp, desde el pixel 0)   P12 libre
 *   P13-P15 libres para cada escena
 */
#ifndef DEMO_H
#define DEMO_H

#include <stdint.h>

/* Paginas de trabajo. Con el controlador de SDRAM con filas abiertas, el banco
 * son los dos bits bajos de la pagina, y los buferes de pantalla C5/C6 estan en
 * los bancos 1 y 2. Los graficos y la fuente van en los bancos 0 y 3, para que
 * leerlos no cierre la fila de la pantalla en la que se dibuja. */
#define OBJ_PAGE   0x20            /* banco 0 */
#define ASSET_PAGE 0x28            /* 28h-2Bh bolas (S5), 28h tiles, 2Ch sprite de S8/S9 */
#define FONT_PAGE  0x27            /* banco 3 */
#define NSCENES    10

typedef struct {
    uint8_t  bpp, kb;
    uint16_t width, height, pitch;  /* pixeles */
    uint8_t  gmode;                 /* formato de la GPU: 1 = 8 bpp, 2 = 4 bpp */
    uint8_t  ppw;                   /* pixeles por palabra de 16 bits */
} vmode_t;

typedef struct {
    uint16_t frames;
    uint32_t render;                /* us por cuadro: Z80 + GPU + linea de estado */
    uint32_t maxr;
    uint32_t scene;                 /* us por cuadro de la escena (sin la linea) */
    uint16_t fps10;
    uint8_t  cpu, gpu;              /* % */
} meas_t;

extern uint16_t* P;
extern vmode_t   vm;
extern uint32_t  t_frame;
extern uint8_t   opt_b, opt_bench, opt_t;

void     u32dec(char* s, uint32_t v, uint8_t frac);
void     load_scene(const uint16_t* pk, uint16_t n);
void     clear_buf(uint8_t page, uint16_t word);
void     clear_both(uint16_t word);
uint8_t  back_page(void);
void     set_status(const char* s);
void     run_frames(uint16_t frames, meas_t* m);
uint8_t  fits(const meas_t* m);
uint16_t step_frames(uint8_t kind);
uint16_t find_limit(const char* tag, uint16_t n0, uint16_t nmax, uint16_t* pow2, meas_t* best);
void     add_row(const char* scene, const char* result, const meas_t* m);
void     show_result(const char* s);
uint8_t  gpu_run_scene(void);
void     gpu_copy(uint8_t src_page, uint16_t src_w, uint8_t dst_page, uint16_t dst_w, uint16_t words);
void     pal_restore(void);

/* scenes.c */
void     scene_run(uint8_t n);
void     scene_frame(uint16_t f);

#endif
