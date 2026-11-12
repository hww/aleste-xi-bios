; fdcint.asm - FDCINT.COM: la INT de la disquetera en la linea 0 del PIC
; Shaeon (Carlos Palmero), para el core AlesteLX_MiSTer y el emulador alestelx.
;
; La INT del uPD765 sin DMA sube al terminar una busqueda (hasta SENSE
; INTERRUPT STATUS), en la fase de ejecucion y en la de resultado. Con las
; interrupciones desactivadas (la linea 0 enmascarada no llega a la CPU, pero
; el PIC anota el flanco igual), se borra la marca de la linea 0, se hace
; cada paso y se mira si el PIC la ha vuelto a ver (PEND_LO, bit 0):
;   idle    en reposo                                   0
;   seek    tras RECALIBRATE                            1
;   (sense) SENSE INTERRUPT STATUS y despues            0
;   readid  READ ID, antes de leer el resultado         1
;   (res)   leido el resultado                          0
;   read    READ DATA, en la fase de ejecucion          1
;   (res)   leidos los 512 bytes y el resultado         0
; Esperado: 0101010. Necesita un disco en A: (el del LX-DOS vale).
; pasmo fdcint.asm FDCINT.COM
BDOS     equ 5
PAGE     equ 0F1h              ; pagina MMIO
PIC_PEND equ 05h               ; pagina 0
PIC_CLR  equ 07h
FDC      equ 9                 ; pagina 9
FDC_MSR  equ 00h
FDC_DATA equ 01h
FDC_MOT  equ 02h

        org 100h
        ld de, m_hello
        ld c, 9
        call BDOS
        di
        in a, (PAGE)
        ld (sv_page), a
        ld a, FDC
        out (PAGE), a
        ld a, 1
        out (FDC_MOT), a       ; motor
        ld hl, resbuf           ; resultados ('0' o '1')

        call clear             ; ---- en reposo
        call look

        call clear             ; ---- RECALIBRATE unidad 0
        ld a, 07h
        call fdc_out
        xor a
        call fdc_out
        ld bc, 0               ; espera a que termine (el PIC la ve) o se acaba
w_seek: call pend
        jr nz, w_done
        dec bc
        ld a, b
        or c
        jr nz, w_seek
w_done: call look
        ld a, 08h              ; SENSE INTERRUPT STATUS: 2 bytes
        call fdc_out
        call fdc_in
        call fdc_in
        call clear
        call look

        call clear             ; ---- READ ID, cara 0
        ld a, 4Ah
        call fdc_out
        xor a
        call fdc_out
        call wait_rd           ; el primer byte del resultado esta listo
        call look
        ld b, 7
r_id:   call fdc_in
        djnz r_id
        call clear
        call look

        call clear             ; ---- READ DATA C0 H0 R1 N2 (512 bytes)
        ld de, cmd_rd
        ld b, 9
r_cmd:  ld a, (de)
        call fdc_out
        inc de
        djnz r_cmd
        call wait_rd           ; el primer byte del sector esta listo
        call look
        ld bc, 512             ; los datos
r_dat:  call fdc_in
        dec bc
        ld a, b
        or c
        jr nz, r_dat
        ld b, 7                ; el resultado
r_res:  call fdc_in
        djnz r_res
        call clear
        call look

        ld a, (sv_page)
        out (PAGE), a
        ei
        ld de, m_res
        ld c, 9
        call BDOS
        ld de, resbuf
        ld c, 9
        call BDOS
        ld hl, resbuf           ; comparar con lo esperado
        ld de, expect
        ld b, 7
cmp_l:  ld a, (de)
        cp (hl)
        jr nz, bad
        inc hl
        inc de
        djnz cmp_l
        ld de, m_ok
        jr pr
bad:    ld de, m_bad
pr:     ld c, 9
        jp BDOS

; ---- PIC (pagina 0)
clear:  xor a                  ; borra la marca de la linea 0
        out (PAGE), a
        ld a, 1
        out (PIC_CLR), a
        ld a, FDC
        out (PAGE), a
        ret
pend:   xor a                  ; Z = sin marca en la linea 0
        out (PAGE), a
        in a, (PIC_PEND)
        push af
        ld a, FDC
        out (PAGE), a
        pop af
        and 1
        ret
look:   call pend              ; anota '0' o '1'
        ld a, '0'
        jr z, look1
        inc a
look1:  ld (hl), a
        inc hl
        ret

; ---- uPD765 (pagina 9), sondeando el registro de estado
fdc_out: push af               ; RQM = 1, DIO = 0
fo1:    in a, (FDC_MSR)
        and 0C0h
        cp 80h
        jr nz, fo1
        pop af
        out (FDC_DATA), a
        ret
wait_rd: in a, (FDC_MSR)       ; RQM = 1, DIO = 1
        and 0C0h
        cp 0C0h
        jr nz, wait_rd
        ret
fdc_in: call wait_rd
        in a, (FDC_DATA)
        ret

cmd_rd:  db 46h, 00h, 0, 0, 1, 2, 1, 2Ah, 0FFh
m_hello: db "FDCINT: floppy INT on PIC line 0", 13, 10, "$"
m_res:   db "idle seek sense readid res read res: $"
m_ok:    db 13, 10, "OK (0101010)", 13, 10, "$"
m_bad:   db 13, 10, "different from 0101010", 13, 10, "$"
expect:  db "0101010"
resbuf:  db "0000000$"
sv_page: db 0
