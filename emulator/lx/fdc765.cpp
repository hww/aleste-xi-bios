// fdc765.cpp - uPD765 sin DMA (ver fdc765.h)
// Shaeon (Carlos Palmero), para aleste-xi-bios (hww).
#include "fdc765.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static bool fdc_log() { static int v = -1; if (v < 0) v = getenv("ALESTELX_FDC_LOG") != nullptr; return v; }

namespace lx {

static constexpr int TRACKS = 80, HEADS = 2, SPT = 9, SECSZ = 512;

bool Fdc765::mount(int d, const std::string& path) {
    d &= 1;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    std::vector<uint8_t> img(TRACKS * HEADS * SPT * SECSZ, 0xE5);
    size_t n = fread(img.data(), 1, img.size(), f);
    fclose(f);
    if (n < 160 * 1024) return false;
    flush(d);                                           // la anterior, con sus cambios
    drv[d].img.swap(img);
    drv[d].path = path;
    drv[d].dirty = false;
    return true;
}

void Fdc765::eject(int d) {
    d &= 1;
    flush(d);
    drv[d].img.clear();
    drv[d].path.clear();
    drv[d].dirty = false;
}

bool Fdc765::flush(int d) {
    d &= 1;
    if (!drv[d].dirty || drv[d].path.empty()) return true;
    if (!save(d, drv[d].path)) return false;
    drv[d].dirty = false;
    return true;
}

bool Fdc765::save(int d, const std::string& path) const {
    if (drv[d & 1].img.empty()) return false;
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    fwrite(drv[d & 1].img.data(), 1, drv[d & 1].img.size(), f);
    fclose(f);
    return true;
}

void Fdc765::reset() {
    phase = CMD; cmd_len = 0; res_len = res_pos = 0; int_pending = false;
}

int Fdc765::lba(int d, int c, int h, int r) const {
    if (c < 0 || c >= TRACKS || h < 0 || h >= HEADS || r < 1 || r > SPT) return -1;
    int l = (c * HEADS + h) * SPT + (r - 1);
    if ((size_t)(l + 1) * SECSZ > drv[d].img.size()) return -1;
    return l;
}

uint8_t Fdc765::read_status() {
    switch (phase) {
    case CMD:     return cmd_len ? 0x90 : 0x80;     // RQM (+ CB a mitad de orden)
    case EXEC_RD: return 0xF0;                      // RQM DIO EXM CB
    case EXEC_WR: return 0xB0;                      // RQM EXM CB
    case RESULT:  return 0xD0;                      // RQM DIO CB
    }
    return 0x80;
}

static int cmd_bytes(uint8_t c) {
    switch (c & 0x1F) {
    case 0x03: return 3;    // SPECIFY
    case 0x04: return 2;    // SENSE DRIVE STATUS
    case 0x05: return 9;    // WRITE DATA
    case 0x06: return 9;    // READ DATA
    case 0x07: return 2;    // RECALIBRATE
    case 0x08: return 1;    // SENSE INTERRUPT STATUS
    case 0x0A: return 2;    // READ ID
    case 0x0F: return 3;    // SEEK
    default:   return 1;    // invalida
    }
}

void Fdc765::write_data(uint8_t v) {
    if (phase == EXEC_WR) {
        if (buf_pos < buf.size()) buf[buf_pos++] = v;
        if (buf_pos == buf.size()) {
            Drive& d = drv[cur_drive];
            for (int r = sect_r, i = 0; r <= sect_eot; r++, i++) {
                int l = lba(cur_drive, sect_c, sect_h, r);
                if (l >= 0) { memcpy(&d.img[l * SECSZ], &buf[i * SECSZ], SECSZ); d.dirty = true; }
            }
            finish_rw(0x40 | (sect_h << 2) | cur_drive, 0x80, 0x00);
        }
        return;
    }
    if (phase != CMD) return;
    if (cmd_len == 0) cmd_need = cmd_bytes(v);
    cmd[cmd_len++] = v;
    if (cmd_len == cmd_need) { start_command(); cmd_len = 0; }
}

uint8_t Fdc765::read_data() {
    if (phase == EXEC_RD) {
        uint8_t v = buf_pos < buf.size() ? buf[buf_pos++] : 0;
        if (buf_pos >= buf.size()) finish_rw(0x40 | (sect_h << 2) | cur_drive, 0x80, 0x00);
        return v;
    }
    if (phase == RESULT) {
        if (fdc_log() && res_pos == 0) {
            fprintf(stderr, "765 res");
            for (int i = 0; i < res_len; i++) fprintf(stderr, " %02X", res[i]);
            fprintf(stderr, "\n");
        }
        uint8_t v = res[res_pos++];
        if (res_pos >= res_len) { phase = CMD; res_len = res_pos = 0; }
        return v;
    }
    return 0xFF;
}

void Fdc765::finish_rw(uint8_t st0, uint8_t st1, uint8_t st2) {
    res[0] = st0; res[1] = st1; res[2] = st2;
    res[3] = sect_c; res[4] = sect_h; res[5] = sect_eot + 1; res[6] = 2;
    res_len = 7; res_pos = 0; phase = RESULT;
}

void Fdc765::start_command() {
    uint8_t op = cmd[0] & 0x1F;
    if (fdc_log()) {
        fprintf(stderr, "765 cmd");
        for (int i = 0; i < cmd_len; i++) fprintf(stderr, " %02X", cmd[i]);
        fprintf(stderr, "\n");
    }
    auto result = [&](std::initializer_list<uint8_t> r) {
        res_len = 0;
        for (uint8_t b : r) res[res_len++] = b;
        res_pos = 0; phase = RESULT;
    };
    switch (op) {
    case 0x03: phase = CMD; break;                                   // SPECIFY
    case 0x07: {                                                     // RECALIBRATE
        int d = cmd[1] & 1;
        drv[d].pcn = 0;
        st0_int = 0x20 | d; pcn_int = 0; int_pending = true;
        phase = CMD;
        break;
    }
    case 0x0F: {                                                     // SEEK
        int d = cmd[1] & 1;
        drv[d].pcn = cmd[2];
        st0_int = 0x20 | ((cmd[1] & 4)) | d; pcn_int = cmd[2]; int_pending = true;
        phase = CMD;
        break;
    }
    case 0x08:                                                       // SENSE INTERRUPT
        if (int_pending) { int_pending = false; result({(uint8_t)st0_int, (uint8_t)pcn_int}); }
        else result({0x80});
        break;
    case 0x04: {                                                     // SENSE DRIVE STATUS
        int d = cmd[1] & 1;
        uint8_t st3 = (cmd[1] & 7) | (drv[d].pcn == 0 ? 0x10 : 0) | (drv[d].img.empty() ? 0 : 0x20) | 0x08;
        result({st3});
        break;
    }
    case 0x0A: {                                                     // READ ID
        int d = cmd[1] & 1, h = (cmd[1] >> 2) & 1;
        if (drv[d].img.empty()) result({(uint8_t)(0x48 | (h << 2) | d), 0x01, 0x00, 0, 0, 0, 0});
        else result({(uint8_t)((h << 2) | d), 0, 0, (uint8_t)drv[d].pcn, (uint8_t)h, 1, 2});
        break;
    }
    case 0x05: case 0x06: {                                          // WRITE / READ DATA
        cur_drive = cmd[1] & 1;
        sect_h = (cmd[1] >> 2) & 1;
        sect_c = cmd[2]; sect_r = cmd[4]; sect_eot = cmd[6];
        Drive& d = drv[cur_drive];
        int n = sect_eot - sect_r + 1;
        if (d.img.empty() || n <= 0 || lba(cur_drive, sect_c, sect_h, sect_r) < 0 ||
            lba(cur_drive, sect_c, sect_h, sect_eot) < 0) {
            uint8_t st0 = 0x40 | (sect_h << 2) | cur_drive | (d.img.empty() ? 0x08 : 0);
            sect_eot = sect_r - 1;
            finish_rw(st0, 0x04, 0x00);                              // sin datos
            break;
        }
        buf.assign((size_t)n * SECSZ, 0);
        buf_pos = 0;
        if (op == 0x06) {
            for (int r = sect_r, i = 0; r <= sect_eot; r++, i++)
                memcpy(&buf[i * SECSZ], &d.img[lba(cur_drive, sect_c, sect_h, r) * SECSZ], SECSZ);
            phase = EXEC_RD;
        } else {
            phase = EXEC_WR;
        }
        break;
    }
    default:
        result({0x80});
    }
}

} // namespace lx
