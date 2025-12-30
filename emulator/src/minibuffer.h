#ifndef MINIBUFFER_H
#define MINIBUFFER_H

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct {
    char buffer[256];
    int cursor_pos;
    int length;
    int overwrite_mode; // 0=insert, 1=overwrite
    bool visible;
    char prompt[32];
    int prompt_offset;
} Minibuffer;

void minibuffer_init(Minibuffer *mb);
void minibuffer_show(Minibuffer *mb, const char *prompt, const char *initial_text);
void minibuffer_hide(Minibuffer *mb);
void minibuffer_clear(Minibuffer *mb);
bool minibuffer_handle_event(Minibuffer *mb, SDL_Event *event);

const char* minibuffer_get_text(Minibuffer *mb);
bool minibuffer_is_visible(Minibuffer *mb);

extern Minibuffer minibuf;
#endif