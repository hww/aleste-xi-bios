#include "minibuffer.h"
#include <string.h>
#include <stdio.h>

Minibuffer minibuf;

void minibuffer_init(Minibuffer *mb) {
    memset(mb, 0, sizeof(Minibuffer));
    strcpy(mb->prompt, ">> ");
}

void minibuffer_show(Minibuffer *mb, const char *prompt, const char *initial_text) {
    mb->visible = true;
    mb->cursor_pos = 0;
    mb->length = 0;
    mb->buffer[0] = '\0';
    
    if (prompt) {
        strncpy(mb->prompt, prompt, sizeof(mb->prompt) - 1);
    }
    
    if (initial_text) {
        strncpy(mb->buffer, initial_text, sizeof(mb->buffer) - 1);
        mb->length = strlen(initial_text);
        mb->cursor_pos = mb->length;
    }
    
    SDL_StartTextInput();
}

void minibuffer_hide(Minibuffer *mb) {
    mb->visible = false;
    SDL_StopTextInput();
}

void minibuffer_clear(Minibuffer *mb) {
    mb->buffer[0] = '\0';
    mb->cursor_pos = 0;
    mb->length = 0;
}

bool minibuffer_handle_event(Minibuffer *mb, SDL_Event *event) {
    if (!mb->visible) return false;
    
    bool command_complete = false;
    bool alt_pressed = (SDL_GetModState() & (KMOD_LALT | KMOD_RALT)) != 0;
    bool ctrl_pressed = (SDL_GetModState() & KMOD_CTRL) != 0;
    
    switch (event->type) {
        case SDL_KEYDOWN:
            switch (event->key.keysym.sym) {
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    command_complete = true;
                    break;
                    
                case SDLK_ESCAPE:
                    minibuffer_hide(mb);
                    minibuffer_clear(mb);
                    break;
                    
                case SDLK_BACKSPACE:
                    if (mb->cursor_pos > 0) {
                        for (int i = mb->cursor_pos - 1; i < mb->length; i++) {
                            mb->buffer[i] = mb->buffer[i + 1];
                        }
                        mb->cursor_pos--;
                        mb->length--;
                        mb->buffer[mb->length] = '\0';
                    }
                    break;
                    
                case SDLK_DELETE:
                    if (mb->cursor_pos < mb->length) {
                        for (int i = mb->cursor_pos; i < mb->length; i++) {
                            mb->buffer[i] = mb->buffer[i + 1];
                        }
                        mb->length--;
                        mb->buffer[mb->length] = '\0';
                    }
                    break;
                    
                case SDLK_LEFT:
                    if (mb->cursor_pos > 0) mb->cursor_pos--;
                    break;
                    
                case SDLK_RIGHT:
                    if (mb->cursor_pos < mb->length) mb->cursor_pos++;
                    break;
                    
                case SDLK_HOME:
                    mb->cursor_pos = 0;
                    break;
                    
                case SDLK_END:
                    mb->cursor_pos = mb->length;
                    break;
                    
                case SDLK_INSERT:
                    mb->overwrite_mode = !mb->overwrite_mode;
                    break;
                    
                case SDLK_a:
                    if (alt_pressed) mb->cursor_pos = 0;
                    break;
                    
                case SDLK_e:
                    if (alt_pressed) mb->cursor_pos = mb->length;
                    break;
                    
                case SDLK_d:
                    if (alt_pressed && mb->cursor_pos < mb->length) {
                        for (int i = mb->cursor_pos; i < mb->length; i++) {
                            mb->buffer[i] = mb->buffer[i + 1];
                        }
                        mb->length--;
                        mb->buffer[mb->length] = '\0';
                    }
                    break;
                    
                case SDLK_k:
                    if (alt_pressed) {
                        mb->length = mb->cursor_pos;
                        mb->buffer[mb->length] = '\0';
                    }
                    break;
                    
                case SDLK_u:
                    if (alt_pressed) {
                        int chars_to_move = mb->cursor_pos;
                        for (int i = 0; i <= mb->length - chars_to_move; i++) {
                            mb->buffer[i] = mb->buffer[i + chars_to_move];
                        }
                        mb->length -= chars_to_move;
                        mb->cursor_pos = 0;
                    }
                    break;
                    
                case SDLK_x:
                    if (alt_pressed) {
                        minibuffer_hide(mb);
                        minibuffer_clear(mb);
                    }
                    break;
            }
            break;
            
        case SDL_TEXTINPUT:
            if (mb->length < sizeof(mb->buffer) - 1) {
                if (mb->overwrite_mode && mb->cursor_pos < mb->length) {
                    mb->buffer[mb->cursor_pos] = event->text.text[0];
                } else {
                    for (int i = mb->length; i > mb->cursor_pos; i--) {
                        mb->buffer[i] = mb->buffer[i - 1];
                    }
                    mb->buffer[mb->cursor_pos] = event->text.text[0];
                    mb->length++;
                }
                mb->cursor_pos++;
                if (!mb->overwrite_mode) {
                    mb->buffer[mb->length] = '\0';
                }
            }
            break;
    }
    
    return command_complete;
}

const char* minibuffer_get_text(Minibuffer *mb) {
    return mb->buffer;
}

bool minibuffer_is_visible(Minibuffer *mb) {
    return mb->visible;
}