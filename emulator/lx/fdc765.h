// fdc765.h - uPD765 sin DMA para el emulador del Aleste LX
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
//
// Lo que ve el programa: registro de estado (RQM, DIO, EXM, CB), fase de
// orden, ejecucion byte a byte por el registro de datos y fase de resultado.
// Ordenes: SPECIFY, RECALIBRATE, SEEK, SENSE INTERRUPT STATUS, SENSE DRIVE
// STATUS, READ DATA, WRITE DATA y READ ID. Imagenes en bruto de 720 KB (80
// pistas, 2 caras, 9 sectores de 512), como las de MSX-DOS del 520EX, en dos
// unidades. Sin tiempos de giro ni de busqueda.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lx {

class Fdc765 {
public:
    bool mount(int drive, const std::string& path);
    bool save(int drive, const std::string& path) const;
    void reset();

    uint8_t read_status();
    uint8_t read_data();
    void    write_data(uint8_t v);
    void    motor(bool on) { motor_on = on; }
    // INT del 765: fin de busqueda pendiente (hasta SENSE INTERRUPT), y en
    // ejecucion sin DMA y en la fase de resultado (el 520EX la lee en PB1)
    bool    irq() const { return int_pending || phase != CMD; }

private:
    struct Drive {
        std::vector<uint8_t> img;
        int  pcn = 0;
        bool seek_end = false;
    } drv[2];

    enum Phase { CMD, EXEC_RD, EXEC_WR, RESULT } phase = CMD;
    uint8_t cmd[9] = {};
    int     cmd_len = 0, cmd_need = 0;
    uint8_t res[7] = {};
    int     res_len = 0, res_pos = 0;
    std::vector<uint8_t> buf;
    size_t  buf_pos = 0;
    int     cur_drive = 0, st0_int = 0x80, pcn_int = 0;
    bool    int_pending = false, motor_on = false;
    int     sect_c = 0, sect_h = 0, sect_r = 0, sect_eot = 0;

    void    start_command();
    void    finish_rw(uint8_t st0, uint8_t st1, uint8_t st2);
    int     lba(int d, int c, int h, int r) const;
};

} // namespace lx
