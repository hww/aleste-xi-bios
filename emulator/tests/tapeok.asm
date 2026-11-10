        org 4000h
        ld hl, msg
loop:   ld a, (hl)
        or a
        jr z, done
        call 0BB5Ah             ; TXT OUTPUT del firmware
        inc hl
        jr loop
done:   jr done                 ; RUN" de un binario no vuelve al BASIC
msg:    db "TAPE OK: loaded from CDT", 13, 10, 0
