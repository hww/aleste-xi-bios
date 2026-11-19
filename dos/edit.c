/*
 * edit.c - linea de ordenes de LX-DOS: edicion, historial y Tab
 * Shaeon (Carlos Palmero), para el Aleste LX.
 *
 * Teclas: izquierda/derecha, Inicio/Fin, Retroceso, Supr, Esc (borra la
 * linea), arriba/abajo (historial de las ultimas HIST_N ordenes) y Tab:
 * completa la palabra del cursor. En la primera palabra, con las ordenes
 * internas y los .COM del disco; en las demas, con los ficheros (con A: o B:
 * delante, los de esa unidad). Con un solo candidato lo completa entero; con
 * varios, hasta donde coinciden, y un segundo Tab los lista.
 * La linea cabe en una fila (no pasa a la siguiente).
 */
#include <stdint.h>
#include "drivers/kbd.h"
#include "console.h"
#include "fat.h"
#include "bdos.h"
#include "edit.h"

#define HIST_N   8
#define LINE_MAX 77

static char    hist[HIST_N][LINE_MAX + 1];
static uint8_t hist_cnt, hist_head;    /* ordenes guardadas; la siguiente casilla */

static char*   ed_buf;
static uint8_t ed_len, ed_pos, ed_x0, ed_y, ed_max;

static uint8_t lower(uint8_t c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }

/* dibuja desde 'from' hasta el final, borra 'extra' casillas y pone el cursor */
static void redraw(uint8_t from, uint8_t extra) {
    uint8_t i;
    for (i = from; i < ed_len; i++) con_draw(ed_buf[i], ed_x0 + i, ed_y);
    while (extra--) con_draw(' ', ed_x0 + i++, ed_y);
    con_gotoxy(ed_x0 + ed_pos, ed_y);
}

/* sustituye la linea entera */
static void set_line(const char* s) {
    uint8_t old = ed_len;
    ed_len = 0;
    while (*s) ed_buf[ed_len++] = *s++;
    ed_pos = ed_len;
    redraw(0, old > ed_len ? old - ed_len : 0);
}

static void insert(char c) {
    uint8_t i;
    if (ed_len >= ed_max) return;
    for (i = ed_len; i > ed_pos; i--) ed_buf[i] = ed_buf[i - 1];
    ed_buf[ed_pos] = c;
    ed_len++;
    redraw(ed_pos++, 0);
}

static void erase(uint8_t at) {                 /* borra el caracter 'at' */
    uint8_t i;
    for (i = at; i + 1 < ed_len; i++) ed_buf[i] = ed_buf[i + 1];
    ed_len--;
    ed_pos = at;
    redraw(at, 1);
}

/* ---- Tab */
static const char* const cmds[] = {
    "dir", "type", "copy", "del", "erase", "ren", "rename", "cls", "ver",
    "keyb", "help", "monitor", 0
};

static uint8_t tb_ws, tb_wlen, tb_first, tb_list, tb_count, tb_clen;
static char    tb_common[13];

/* compara 'name' con la palabra; cuenta, guarda lo comun o lo lista */
static void tab_try(const char* name) {
    uint8_t i;
    for (i = 0; i < tb_wlen; i++)
        if (lower(name[i]) != lower(ed_buf[tb_ws + i])) return;
    if (tb_list) {
        con_puts(name);
        for (i = 0; name[i]; i++) ;
        do con_putc(' '); while (++i < 13);
        if (con_getx() > CON_COLS - 13) con_putc('\n');
        return;
    }
    if (!tb_count++) {
        for (i = 0; name[i]; i++) tb_common[i] = name[i];
        tb_clen = i;
    } else {
        for (i = 0; i < tb_clen && lower(tb_common[i]) == lower(name[i]); i++) ;
        tb_clen = i;
    }
}

static uint8_t tab_cb(const fat_entry_t* e) {
    char n[13];
    uint8_t i = 0;
    if (e->attr & 0x18) return 0;               /* ni directorios ni volumen */
    while (e->name[i]) { n[i] = e->name[i]; i++; }
    n[i] = 0;
    if (tb_first) {                             /* primera palabra: NOMBRE de NOMBRE.COM */
        if (i < 5 || n[i - 4] != '.' || n[i - 3] != 'C' || n[i - 2] != 'O' || n[i - 1] != 'M')
            return 0;
        n[i - 4] = 0;
    }
    tab_try(n);
    return 0;
}

static void tab_scan(uint8_t drv) {
    if (tb_first) {
        uint8_t i;
        for (i = 0; cmds[i]; i++) tab_try(cmds[i]);
    }
    if (!fat_use(drv)) fat_dir(tab_cb);
}

static void complete(const char* prompt, uint8_t again) {
    uint8_t drv = dos_drive, low, i;
    tb_ws = ed_pos;
    while (tb_ws && ed_buf[tb_ws - 1] != ' ') tb_ws--;
    tb_first = 1;
    for (i = 0; i < tb_ws; i++) if (ed_buf[i] != ' ') tb_first = 0;
    if (ed_pos - tb_ws >= 2 && ed_buf[tb_ws + 1] == ':') {   /* A: o B: delante */
        uint8_t d = lower(ed_buf[tb_ws]) - 'a';
        if (d > 1) return;
        drv = d;
        tb_ws += 2;
        tb_first = 0;
    }
    tb_wlen = ed_pos - tb_ws;
    tb_count = 0;
    tb_list = 0;
    tab_scan(drv);
    if (!tb_count) return;
    if (tb_clen > tb_wlen) {                    /* completa hasta donde coinciden */
        low = tb_wlen ? (ed_buf[tb_ws] >= 'a') : 1;
        for (i = tb_wlen; i < tb_clen; i++)
            insert(low ? lower(tb_common[i]) : tb_common[i]);
        if (tb_count == 1) insert(' ');
        return;
    }
    if (tb_count == 1 || !again) return;
    tb_list = 1;                                /* segundo Tab: la lista */
    con_gotoxy(ed_x0 + ed_len, ed_y);
    con_putc('\n');
    tab_scan(drv);
    if (con_getx()) con_putc('\n');
    con_puts(prompt);
    ed_y = con_gety();
    redraw(0, 0);
}

/* ---- historial */
static void hist_add(void) {
    char* h;
    uint8_t i;
    if (!ed_len) return;
    if (hist_cnt) {                             /* no repite la ultima */
        h = hist[(hist_head + HIST_N - 1) % HIST_N];
        for (i = 0; i < ed_len && h[i] == ed_buf[i]; i++) ;
        if (i == ed_len && !h[i]) return;
    }
    h = hist[hist_head];
    for (i = 0; i < ed_len; i++) h[i] = ed_buf[i];
    h[i] = 0;
    hist_head = (hist_head + 1) % HIST_N;
    if (hist_cnt < HIST_N) hist_cnt++;
}

uint8_t le_gets(const char* prompt, char* buf, uint8_t max) {
    uint8_t h = 0, last_tab = 0;
    con_puts(prompt);
    ed_buf = buf;
    ed_len = ed_pos = 0;
    ed_x0 = con_getx();
    ed_y = con_gety();
    if (max > LINE_MAX + 1) max = LINE_MAX + 1;
    if (ed_x0 + max > CON_COLS) max = CON_COLS - ed_x0;
    ed_max = max - 1;
    while (1) {
        uint8_t c = (uint8_t)con_getc();
        uint8_t tab = 0;
        if (ed_pos < ed_len) con_draw(ed_buf[ed_pos], ed_x0 + ed_pos, ed_y);   /* el cursor lo borra */
        switch (c) {
        case KBD_ENTER:
            hist_add();
            buf[ed_len] = 0;
            con_gotoxy(ed_x0 + ed_len, ed_y);
            con_putc('\n');
            return ed_len;
        case KBD_LEFT:  if (ed_pos) ed_pos--; break;
        case KBD_RIGHT: if (ed_pos < ed_len) ed_pos++; break;
        case KBD_HOME:  ed_pos = 0; break;
        case KBD_END:   ed_pos = ed_len; break;
        case KBD_BS:    if (ed_pos) erase(ed_pos - 1); break;
        case KBD_DELETE: if (ed_pos < ed_len) erase(ed_pos); break;
        case KBD_ESC:   set_line(""); h = 0; break;
        case KBD_UP:
            if (h < hist_cnt) { h++; set_line(hist[(hist_head + HIST_N - h) % HIST_N]); }
            break;
        case KBD_DOWN:
            if (h) { h--; set_line(h ? hist[(hist_head + HIST_N - h) % HIST_N] : ""); }
            break;
        case '\t':
            complete(prompt, last_tab);
            tab = 1;
            break;
        default:
            if (c >= 0x20 && c < 0x7F) insert(c);
        }
        last_tab = tab;
        con_gotoxy(ed_x0 + ed_pos, ed_y);
    }
}
