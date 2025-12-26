#ifndef BIOS_H
#define BIOS_H

// Пример фиксированных точек входа
#define bios_audio_init ((void (*)(void))0xFD00)
#define bios_audio_play ((void (*)(void))0xFD03)

#endif
