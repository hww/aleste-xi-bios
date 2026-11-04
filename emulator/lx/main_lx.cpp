// main_lx.cpp - alestelx: emulador del Aleste LX (modo nativo)
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
//   alestelx [opciones] boot.rom
//     -a imagen        disco en la unidad A (720 KB, MSX-DOS)
//     -b imagen        disco en la unidad B
//     --keyboard us|es teclado de EE. UU. (por defecto) o espanol, como la
//                      opcion Keyboard del OSD (--es = --keyboard es)
//     --scale N        ventana de 320N x 240N (3 por defecto)
//     --boot amstrad   al pasar al 520EX (orden 520EX), SHIFT pulsado 2,5 s:
//                      el SETUP arranca en modo AMSTRAD (BASIC del CPC)
//   Depurador (consola en la terminal, ver debugger.h):
//     --debug          arranca parado y con la consola
//     --sym fichero    simbolos de un mapa de z88dk (se puede repetir)
//     --break dir      punto de ruptura (direccion, simbolo o p:fisica)
//   Sin ventana (pruebas, como el banco de pruebas del core):
//     --headless --ms N          corre N ms de la maquina y sale
//     --type MS:texto            teclea texto a los MS ms ('|' Enter, '~' Esc)
//     --shot fichero.ppm         pantalla al final
//     --pcm fichero              audio (estereo, 16 bits, 48 kHz)
//     --save-a fichero           imagen del disco A al final
// Mandos: el primero va a la fila 9 de la matriz (joystick del 520EX/CPC):
// cruceta o stick izquierdo, A/Cruz fuego 1 y B/Circulo fuego 2.
// Teclas en la ventana: las del PC van a la maquina (tambien a la matriz del
// 520EX/CPC, para los juegos); F12 reset, Mayus+F12 cambia el teclado (se
// aplica en el siguiente reset, como la opcion del OSD), F11 pantalla
// completa, Pausa o Bloq Despl para y sigue (depurador).
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "machine.h"
#include "debugger.h"

using lx::Machine;

// ---- SDL -> PS/2 (juego 2): codigo y si es extendida (E0)
struct Ps2 { uint8_t code; bool ext; };
static bool sdl_to_ps2(SDL_Scancode s, Ps2& o) {
    static const struct { SDL_Scancode s; uint8_t c; bool e; } t[] = {
        {SDL_SCANCODE_A,0x1C,0},{SDL_SCANCODE_B,0x32,0},{SDL_SCANCODE_C,0x21,0},{SDL_SCANCODE_D,0x23,0},
        {SDL_SCANCODE_E,0x24,0},{SDL_SCANCODE_F,0x2B,0},{SDL_SCANCODE_G,0x34,0},{SDL_SCANCODE_H,0x33,0},
        {SDL_SCANCODE_I,0x43,0},{SDL_SCANCODE_J,0x3B,0},{SDL_SCANCODE_K,0x42,0},{SDL_SCANCODE_L,0x4B,0},
        {SDL_SCANCODE_M,0x3A,0},{SDL_SCANCODE_N,0x31,0},{SDL_SCANCODE_O,0x44,0},{SDL_SCANCODE_P,0x4D,0},
        {SDL_SCANCODE_Q,0x15,0},{SDL_SCANCODE_R,0x2D,0},{SDL_SCANCODE_S,0x1B,0},{SDL_SCANCODE_T,0x2C,0},
        {SDL_SCANCODE_U,0x3C,0},{SDL_SCANCODE_V,0x2A,0},{SDL_SCANCODE_W,0x1D,0},{SDL_SCANCODE_X,0x22,0},
        {SDL_SCANCODE_Y,0x35,0},{SDL_SCANCODE_Z,0x1A,0},
        {SDL_SCANCODE_1,0x16,0},{SDL_SCANCODE_2,0x1E,0},{SDL_SCANCODE_3,0x26,0},{SDL_SCANCODE_4,0x25,0},
        {SDL_SCANCODE_5,0x2E,0},{SDL_SCANCODE_6,0x36,0},{SDL_SCANCODE_7,0x3D,0},{SDL_SCANCODE_8,0x3E,0},
        {SDL_SCANCODE_9,0x46,0},{SDL_SCANCODE_0,0x45,0},
        {SDL_SCANCODE_RETURN,0x5A,0},{SDL_SCANCODE_ESCAPE,0x76,0},{SDL_SCANCODE_BACKSPACE,0x66,0},
        {SDL_SCANCODE_TAB,0x0D,0},{SDL_SCANCODE_SPACE,0x29,0},{SDL_SCANCODE_MINUS,0x4E,0},
        {SDL_SCANCODE_EQUALS,0x55,0},{SDL_SCANCODE_LEFTBRACKET,0x54,0},{SDL_SCANCODE_RIGHTBRACKET,0x5B,0},
        {SDL_SCANCODE_BACKSLASH,0x5D,0},{SDL_SCANCODE_SEMICOLON,0x4C,0},{SDL_SCANCODE_APOSTROPHE,0x52,0},
        {SDL_SCANCODE_GRAVE,0x0E,0},{SDL_SCANCODE_COMMA,0x41,0},{SDL_SCANCODE_PERIOD,0x49,0},
        {SDL_SCANCODE_SLASH,0x4A,0},{SDL_SCANCODE_NONUSBACKSLASH,0x61,0},{SDL_SCANCODE_CAPSLOCK,0x58,0},
        {SDL_SCANCODE_LSHIFT,0x12,0},{SDL_SCANCODE_RSHIFT,0x59,0},{SDL_SCANCODE_LCTRL,0x14,0},
        {SDL_SCANCODE_RCTRL,0x14,1},{SDL_SCANCODE_LALT,0x11,0},{SDL_SCANCODE_RALT,0x11,1},
        {SDL_SCANCODE_F1,0x05,0},{SDL_SCANCODE_F2,0x06,0},{SDL_SCANCODE_F3,0x04,0},{SDL_SCANCODE_F4,0x0C,0},
        {SDL_SCANCODE_F5,0x03,0},{SDL_SCANCODE_F6,0x0B,0},{SDL_SCANCODE_F7,0x83,0},{SDL_SCANCODE_F8,0x0A,0},
        {SDL_SCANCODE_F9,0x01,0},{SDL_SCANCODE_F10,0x09,0},
        {SDL_SCANCODE_UP,0x75,1},{SDL_SCANCODE_DOWN,0x72,1},{SDL_SCANCODE_LEFT,0x6B,1},{SDL_SCANCODE_RIGHT,0x74,1},
        {SDL_SCANCODE_HOME,0x6C,1},{SDL_SCANCODE_END,0x69,1},{SDL_SCANCODE_PAGEUP,0x7D,1},
        {SDL_SCANCODE_PAGEDOWN,0x7A,1},{SDL_SCANCODE_INSERT,0x70,1},{SDL_SCANCODE_DELETE,0x71,1},
        {SDL_SCANCODE_KP_ENTER,0x5A,1},{SDL_SCANCODE_KP_DIVIDE,0x4A,1},{SDL_SCANCODE_KP_MULTIPLY,0x7C,0},
        {SDL_SCANCODE_KP_MINUS,0x7B,0},{SDL_SCANCODE_KP_PLUS,0x79,0},{SDL_SCANCODE_KP_PERIOD,0x71,0},
        {SDL_SCANCODE_KP_0,0x70,0},{SDL_SCANCODE_KP_1,0x69,0},{SDL_SCANCODE_KP_2,0x72,0},{SDL_SCANCODE_KP_3,0x7A,0},
        {SDL_SCANCODE_KP_4,0x6B,0},{SDL_SCANCODE_KP_5,0x73,0},{SDL_SCANCODE_KP_6,0x74,0},{SDL_SCANCODE_KP_7,0x6C,0},
        {SDL_SCANCODE_KP_8,0x75,0},{SDL_SCANCODE_KP_9,0x7D,0},
    };
    for (auto& e : t) if (e.s == s) { o = {e.c, e.e}; return true; }
    return false;
}

// ---- ASCII -> PS/2 con el teclado de EE. UU. (para --type)
static bool ascii_to_ps2(char ch, uint8_t& code, bool& shift) {
    static const char* lower = "abcdefghijklmnopqrstuvwxyz";
    static const uint8_t letters[] = {0x1C,0x32,0x21,0x23,0x24,0x2B,0x34,0x33,0x43,0x3B,0x42,0x4B,0x3A,
                                      0x31,0x44,0x4D,0x15,0x2D,0x1B,0x2C,0x3C,0x2A,0x1D,0x22,0x35,0x1A};
    static const char* digits = "1234567890";
    static const uint8_t dcode[] = {0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46,0x45};
    static const char* shd = "!@#$%^&*()";
    static const struct { char c; uint8_t k; bool s; } other[] = {
        {' ',0x29,0},{'|',0x5A,0},{'~',0x76,0},{'\n',0x5A,0},{'-',0x4E,0},{'_',0x4E,1},{'=',0x55,0},
        {'+',0x55,1},{'[',0x54,0},{']',0x5B,0},{';',0x4C,0},{':',0x4C,1},{'\'',0x52,0},{'"',0x52,1},
        {',',0x41,0},{'<',0x41,1},{'.',0x49,0},{'>',0x49,1},{'/',0x4A,0},{'?',0x4A,1},{'\\',0x5D,0},
    };
    shift = false;
    if (ch >= 'A' && ch <= 'Z') { shift = true; ch = ch - 'A' + 'a'; }
    if (const char* p = strchr(lower, ch); p && ch) { code = letters[p - lower]; return true; }
    if (const char* p = strchr(digits, ch); p && ch) { code = dcode[p - digits]; return true; }
    if (const char* p = strchr(shd, ch); p && ch) { code = dcode[p - shd]; shift = true; return true; }
    for (auto& o : other) if (o.c == ch) { code = o.k; shift = o.s; return true; }
    return false;
}

// ---- Mandos -> fila 9 de la matriz: bit 0 arriba, 1 abajo, 2 izquierda,
// 3 derecha, 4 fuego 2, 5 fuego 1 (como kbd_520ex.sv)
static uint8_t read_joystick(SDL_GameController* gc, SDL_Joystick* js) {
    uint8_t j = 0;
    const int dz = 12000;
    if (gc) {
        int x = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX);
        int y = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY);
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP) || y < -dz)    j |= 0x01;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || y > dz)   j |= 0x02;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || x < -dz)  j |= 0x04;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || x > dz)  j |= 0x08;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B))                     j |= 0x10;
        if (SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A))                     j |= 0x20;
    } else if (js) {
        int x = SDL_JoystickGetAxis(js, 0), y = SDL_JoystickGetAxis(js, 1);
        uint8_t hat = SDL_JoystickNumHats(js) ? SDL_JoystickGetHat(js, 0) : 0;
        if ((hat & SDL_HAT_UP) || y < -dz)    j |= 0x01;
        if ((hat & SDL_HAT_DOWN) || y > dz)   j |= 0x02;
        if ((hat & SDL_HAT_LEFT) || x < -dz)  j |= 0x04;
        if ((hat & SDL_HAT_RIGHT) || x > dz)  j |= 0x08;
        if (SDL_JoystickNumButtons(js) > 1 && SDL_JoystickGetButton(js, 1)) j |= 0x10;
        if (SDL_JoystickNumButtons(js) > 0 && SDL_JoystickGetButton(js, 0)) j |= 0x20;
    }
    return j;
}

struct TypeJob { double ms; std::string text; size_t pos = 0; int phase = 0; double next = 0; };

static void save_ppm(const Machine& m, const std::string& path) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return;
    int w = m.fb_width(), h = m.fb_height();
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    const uint32_t* p = m.framebuffer();
    for (int i = 0; i < w * h; i++) {
        uint8_t rgb[3] = {(uint8_t)(p[i] >> 16), (uint8_t)(p[i] >> 8), (uint8_t)p[i]};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

int main(int argc, char** argv) {
    std::string rom, dska, dskb, shot, pcm, save_a;
    bool headless = false, es = false, debug = false, boot_amstrad = false;
    std::vector<std::string> syms, breaks;
    double run_ms = 0;
    int scale = 3;
    std::vector<TypeJob> jobs;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "-a") dska = next();
        else if (a == "-b") dskb = next();
        else if (a == "--es") es = true;
        else if (a == "--keyboard") { std::string k = next(); es = (k == "es" || k == "ES"); }
        else if (a == "--debug") debug = true;
        else if (a == "--boot") { std::string b = next(); boot_amstrad = (b == "amstrad"); }
        else if (a == "--sym") syms.push_back(next());
        else if (a == "--break") breaks.push_back(next());
        else if (a == "--scale") scale = std::max(1, atoi(next().c_str()));
        else if (a == "--headless") headless = true;
        else if (a == "--ms") run_ms = atof(next().c_str());
        else if (a == "--shot") shot = next();
        else if (a == "--pcm") pcm = next();
        else if (a == "--save-a") save_a = next();
        else if (a == "--type") {
            std::string v = next();
            size_t c = v.find(':');
            if (c != std::string::npos) jobs.push_back({atof(v.substr(0, c).c_str()), v.substr(c + 1)});
        }
        else if (a[0] != '-') rom = a;
        else { fprintf(stderr, "Unknown option %s\n", a.c_str()); return 1; }
    }
    if (rom.empty()) {
        fprintf(stderr, "Usage: alestelx [-a disk] [-b disk] [--keyboard us|es] [--debug] [--sym file.map]\n"
                        "                [--break addr] [--headless --ms N --type MS:text --shot file.ppm] boot.rom\n");
        return 1;
    }

    static Machine m;
    if (!m.load_boot_rom(rom)) { fprintf(stderr, "Cannot load %s\n", rom.c_str()); return 1; }
    if (!dska.empty() && !m.fdc.mount(0, dska)) fprintf(stderr, "Cannot mount %s\n", dska.c_str());
    if (!dskb.empty() && !m.fdc.mount(1, dskb)) fprintf(stderr, "Cannot mount %s\n", dskb.c_str());
    m.layout_es = es;
    m.boot_shift = boot_amstrad;
    m.reset();
    lx::Debugger dbg(m);
    for (auto& f : syms) dbg.load_symbols(f);
    for (auto& b : breaks) dbg.add_breakpoint(b);

    FILE* pcmf = pcm.empty() ? nullptr : fopen(pcm.c_str(), "wb");

    // teclea los trabajos de --type (6 ms por pulsacion y otros tantos al soltar)
    auto do_typing = [&](double now_ms) {
        for (auto& j : jobs) {
            if (now_ms < j.ms || j.pos >= j.text.size() || now_ms < j.next) continue;
            uint8_t code; bool sh;
            if (!ascii_to_ps2(j.text[j.pos], code, sh)) { j.pos++; continue; }
            if (j.phase == 0) { if (sh) m.key_event(0x12, true, false); m.key_event(code, true, false); j.phase = 1; }
            else { m.key_event(code, false, false); if (sh) m.key_event(0x12, false, false); j.phase = 0; j.pos++; }
            j.next = now_ms + (m.legacy() ? 40 : 6);    // el CPC explora la matriz cada 20 ms
        }
    };

    if (headless) {
        double ms = 0;
        while (ms < run_ms) {
            do_typing(ms);
            if (!m.run_frame()) { fprintf(stderr, "Stopped: %s\n", m.stop_reason.c_str()); break; }
            ms += 1000.0 / 60.0;
            if (pcmf) fwrite(m.audio().data(), 2, m.audio().size(), pcmf);
            m.audio().clear();
        }
        m.render();
        if (!shot.empty()) save_ppm(m, shot);
        if (!save_a.empty()) m.fdc.save(0, save_a);
        if (pcmf) fclose(pcmf);
        printf("cycles %llu, %.3f s\n", (unsigned long long)m.cycles, m.cycles / m.cpu_hz());
        if (m.legacy()) m.dump_legacy();
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) { fprintf(stderr, "SDL: %s\n", SDL_GetError()); return 1; }
    SDL_Window* win = SDL_CreateWindow("Aleste LX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       320 * scale, 240 * scale, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // la imagen siempre a 4:3; su tamano cambia con el modo (LX o 520EX/CPC)
    SDL_Texture* tex = nullptr;
    int tex_w = 0, tex_h = 0;
    auto fit_texture = [&]() {
        if (tex && tex_w == m.fb_width() && tex_h == m.fb_height()) return;
        if (tex) SDL_DestroyTexture(tex);
        tex_w = m.fb_width(); tex_h = m.fb_height();
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, tex_w, tex_h);
        SDL_RenderSetLogicalSize(ren, tex_w, tex_w * 3 / 4);
    };
    fit_texture();
    SDL_AudioSpec want = {}, have = {};
    want.freq = lx::AUDIO_RATE; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024;
    SDL_AudioDeviceID audio = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (audio) SDL_PauseAudioDevice(audio, 0);

    SDL_GameController* gc = nullptr;
    SDL_Joystick* js = nullptr;
    auto open_pad = [&]() {
        if (gc || js) return;
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i) && (gc = SDL_GameControllerOpen(i))) {
                printf("Joystick: %s\n", SDL_GameControllerName(gc)); return;
            }
            if ((js = SDL_JoystickOpen(i))) { printf("Joystick: %s\n", SDL_JoystickName(js)); return; }
        }
    };
    open_pad();
    bool quit = false, full = false, console = false;
    auto set_title = [&]() {
        std::string t = std::string("Aleste LX - keyboard ") + (m.layout_es ? "ES" : "US") +
                        (dbg.paused() ? " - PAUSED (debugger in the terminal)" : "");
        SDL_SetWindowTitle(win, t.c_str());
    };
    if (debug || !breaks.empty()) { dbg.start_console(); console = true; }
    if (debug) dbg.set_paused(true);
    set_title();
    bool was_paused = dbg.paused();
    uint32_t t0 = SDL_GetTicks();
    double frames = 0;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_CONTROLLERDEVICEADDED || e.type == SDL_JOYDEVICEADDED) open_pad();
            else if (e.type == SDL_CONTROLLERDEVICEREMOVED || e.type == SDL_JOYDEVICEREMOVED) {
                if (gc) { SDL_GameControllerClose(gc); gc = nullptr; }
                if (js) { SDL_JoystickClose(js); js = nullptr; }
                open_pad();
            }
            else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                bool down = e.type == SDL_KEYDOWN;
                if (e.key.keysym.scancode == SDL_SCANCODE_F12) {
                    if (down && (SDL_GetModState() & KMOD_SHIFT)) {
                        m.layout_es = !m.layout_es;
                        printf("Keyboard: %s (applies at the next reset, F12)\n", m.layout_es ? "Spanish" : "US");
                        set_title();
                    } else if (down) m.reset();
                    continue;
                }
                if (e.key.keysym.scancode == SDL_SCANCODE_PAUSE || e.key.keysym.scancode == SDL_SCANCODE_SCROLLLOCK) {
                    if (down) {
                        if (!console) { dbg.start_console(); console = true; }
                        dbg.set_paused(!dbg.paused());
                    }
                    continue;
                }
                if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
                    if (down) { full = !full; SDL_SetWindowFullscreen(win, full ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); }
                    continue;
                }
                if (e.key.repeat) continue;
                Ps2 k;
                if (sdl_to_ps2(e.key.keysym.scancode, k)) m.key_event(k.code, down, k.ext);
            }
        }
        m.joy = read_joystick(gc, js);
        if (console) dbg.poll();
        if (dbg.quit_requested()) break;
        if (!dbg.paused() && !m.run_frame()) {
            static bool told = false;
            if (!told) { fprintf(stderr, "Stopped: %s\n", m.stop_reason.c_str()); told = true; }
        }
        if (dbg.paused() != was_paused) { was_paused = dbg.paused(); set_title(); }
        if (audio) {
            auto& a = m.audio();
            if (SDL_GetQueuedAudioSize(audio) < (uint32_t)lx::AUDIO_RATE)   // menos de ~0,25 s en cola
                SDL_QueueAudio(audio, a.data(), a.size() * 2);
            a.clear();
        } else m.audio().clear();
        m.render();
        fit_texture();
        SDL_UpdateTexture(tex, nullptr, m.framebuffer(), tex_w * 4);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        // 60 cuadros por segundo
        frames++;
        double target = t0 + frames * 1000.0 / 60.0;
        int32_t wait = (int32_t)(target - SDL_GetTicks());
        if (wait > 0) SDL_Delay(wait);
        else if (wait < -500) { t0 = SDL_GetTicks(); frames = 0; }
    }
    if (audio) SDL_CloseAudioDevice(audio);
    SDL_Quit();
    return 0;
}
