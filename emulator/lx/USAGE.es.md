# alestelx — guía de uso

*(English version: `USAGE.md`.)*

`alestelx` emula el **Aleste LX** tal como está en el core FPGA
[AlesteLX_MiSTer](https://github.com/shaeon/AlesteLX_MiSTer), en Windows y
Linux. Ejecuta la BIOS y el LX-DOS, el software del LX (gráficos, música por
la Magic Sound 2) y también el modo compatible **Aleste 520EX / Amstrad CPC**
(AlesteDOS, MSX-DOS, el BASIC del CPC, el reproductor MAG, cintas). Cada
dispositivo sigue la descripción del hardware del core: lo que funciona en el
emulador funciona en la MiSTer.

## 1. Qué trae el paquete

| Fichero | |
|---|---|
| `alestelx.exe` + `SDL2.dll` | el emulador para Windows |
| `alestelx-linux` | el emulador para Linux |
| `boot.rom` | la ROM: la BIOS del LX seguida de la ROM del 520EX (el mismo fichero que `games/AlesteLX/boot.rom` en la MiSTer) |
| `LXDOS.DSK` | disco de arranque del LX-DOS, con programas de prueba, el visor de imágenes, las demos e imágenes de ejemplo |
| `alestedos-dev.dsk`, `mag-player.dsk` | discos del 520EX: MSX-DOS con herramientas de desarrollo, y el reproductor MAG con sus canciones |
| `TAPEOK.CDT` | una cinta de prueba pequeña para el modo CPC |
| `*.bat`, `*.sh` | lanzadores para cada uso (Windows / Linux) |
| `bios.map`, `lxdos.map` | símbolos para el depurador |
| `img2lx.py` | convierte imágenes al formato de 256 colores del LX |
| `gdb_rsp_test.py` | cliente de prueba del servidor GDB |
| `README.md`, `GDB.md`, `USAGE.md` (y sus `.es.md`) | documentación |

**Requisitos.** Windows: nada más (deja `SDL2.dll` junto al `.exe`). Linux:
SDL2 (`sudo apt install libsdl2-2.0-0` en Debian/Ubuntu). `img2lx.py`
necesita Python 3 con Pillow (`pip install pillow`).

**La ROM del 520EX.** El modo Aleste 520EX necesita la ROM original del
520EX, `AL512.BIN` (64 KB, set `al520ex` de MAME), que no se distribuye con
el emulador. O `boot.rom` ya la trae (LX.ROM seguida de AL512.BIN, como en
la MiSTer), o pon `AL512.BIN` en la misma carpeta que `boot.rom` y el
emulador la carga solo (o `--rom520 fichero`). Sin ella, la orden `520ex`
avisa de que falta la ROM.

Usa `boot.rom` y `LXDOS.DSK` del mismo paquete: la ROM comprueba que el
LX-DOS se compiló con ella y no arranca uno distinto (lo dice en pantalla).

## 2. Lanzadores

Doble clic en el `.bat` (Windows) o ejecuta el `.sh` (Linux). Los argumentos
que añadas pasan al emulador (p. ej. `alestelx.bat --scale 4`).

| Lanzador | Qué hace |
|---|---|
| `alestelx` | LX-DOS de `LXDOS.DSK`, teclado de EE. UU. |
| `alestelx-es` | lo mismo con el teclado español |
| `alestelx-debug` | arranca parado con el depurador en la terminal (escribe `help` allí) |
| `alestelx-gdb` | servidor GDB en el puerto 2159 para GDB o DeZog (ver `GDB.es.md`) |
| `alestelx-520ex-basic` | 520EX en modo AMSTRAD: escribe `520ex` en el monitor del LX y llegas al BASIC del CPC |
| `alestelx-520ex-msxdos` | 520EX arrancando MSX-DOS de `alestedos-dev.dsk` (escribe `520ex` en el monitor del LX) |
| `alestelx-520ex-mag` | 520EX con `mag-player.dsk`: luego `mag axelf` (también sll1, chrush, headache, techno1) |
| `alestelx-520ex-tape` | BASIC del CPC con `TAPEOK.CDT`: `\|TAPE`, luego `RUN"` y una tecla |

## 3. Teclas de la ventana del emulador

El teclado del PC va a la máquina emulada, salvo estas:

| Tecla | |
|---|---|
| F12 | reset |
| Mayús+F12 | cambia el teclado EE. UU. / español (se aplica en el siguiente reset, como la opción del OSD de la MiSTer) |
| F11 | pantalla completa |
| Ctrl+F1 / Ctrl+F2 | elegir un disco para la unidad A / B (diálogo de ficheros) |
| Ctrl+Mayús+F1 / F2 | expulsar el disco de la unidad A / B |
| Ctrl+F3 | elegir una cinta (modo 520EX/CPC); Ctrl+Mayús+F3 la quita |
| Ctrl+F4 | rebobinar la cinta |
| Ctrl+F5 | capturar / soltar el ratón (un clic en la ventana también lo captura) |
| Pausa o Bloq Despl | parar / seguir (depurador) |

El título de la ventana muestra los discos de cada unidad, la cinta y por
dónde va, la distribución del teclado, y si el ratón está capturado o la
máquina en pausa.

## 4. Primeros pasos con el LX-DOS

`alestelx` (o `alestelx-es`) arranca el LX-DOS de `LXDOS.DSK`: la ROM
encuentra `LXDOS.SYS` en la unidad A y lo pone en marcha. El LX-DOS ejecuta
programas de CP/M 2.2 y MSX-DOS 1 y tiene funciones propias de gráficos y
música.

| Orden | |
|---|---|
| `A:`, `B:` | cambiar la unidad actual |
| `DIR` | listar los ficheros |
| `TYPE fichero` | ver un fichero de texto (Esc para) |
| `COPY origen destino` | copiar un fichero (`COPY X.TXT B:` lo copia a la otra unidad) |
| `DEL fichero`, `REN viejo nuevo` | borrar (comodines `*` y `?`), renombrar |
| `KEYB US` / `KEYB ES` | distribución del teclado |
| `CLS`, `VER`, `HELP` | borrar la pantalla, versión, ayuda |
| `MONITOR` | volver al monitor de la ROM |
| `nombre [argumentos]` | ejecutar `NOMBRE.COM` |

Si hay un `AUTOEXEC.BAT`, se ejecuta al arrancar.

Programas de `LXDOS.DSK`:

| Programa | |
|---|---|
| `VIEW` | visor de imágenes: enseña todas las `.LXI` (una tecla, la siguiente; Esc sale); `VIEW B:*.LXI` |
| `TUNNEL` | demo de 256 colores con la paleta girando (una tecla sale) |
| `MODPLAY LXDEMO.MOD` | toca un MOD o un STM de fondo (`MODPLAY /S` lo para) |
| `HELLO`, `FILETEST`, `MSXTEST`, `GFX8TEST` | programas de prueba (consola, ficheros, funciones de MSX-DOS, modo de 8 bpp) |
| `ZEXDOC`, `ZEXALL` | ejercitadores de instrucciones del Z80 (tardan un rato) |

### Tus propias imágenes

```
python3 img2lx.py foto.jpg FOTO.LXI           # la imagen entera, con bandas negras
python3 img2lx.py foto.jpg FOTO.LXI --fill    # recortada para llenar la pantalla
```

El resultado es de 320×200 con 256 colores de la paleta de 4096 del LX, con
tramado. Para meterla en un disco, usa la herramienta de discos MSX-DOS del
repositorio AlesteLX_MiSTer (`tools/make_msxdisk.py`), o cópiala desde otro
disco con `COPY` dentro del emulador.

### El monitor de la ROM

Sin disco de sistema en la unidad A, o tras `MONITOR`, sale el monitor de la
ROM del LX (prompt `LX>`): `HELP`, `DIR`, `MOD fichero`, `PLAY`, `STOP`,
`DEMO`, `KEYS` (matriz del teclado), `DUMP dirección`, `TIME`, `BOOT`
(arrancar otra vez el LX-DOS) y `520EX` (pasar al modo 520EX).

## 5. Disquetes

- **Arrastra un fichero `.dsk` a la ventana** para ponerlo en la unidad A;
  con **Mayús** pulsada al soltarlo, en la B.
- O **Ctrl+F1 / Ctrl+F2** para un diálogo de ficheros (Windows; en Linux
  usa `zenity` si está instalado), y Ctrl+Mayús+F1 / F2 para expulsar.
- Desde la línea de órdenes: `-a disco.dsk`, `-b disco.dsk`.

Las imágenes son discos MSX-DOS de 720 KB (80 pistas, 2 caras, 9 sectores),
como los del core y del 520EX. **Lo que escribe la máquina se guarda en el
fichero de la imagen**: al cambiar o expulsar el disco, al salir y cada pocos
segundos, como hace la MiSTer con la SD. Guarda una copia de los discos que
no quieras que cambien.

## 6. Modo Aleste 520EX / Amstrad CPC

El LX lleva dentro el 520EX entero. Para entrar: ve al monitor de la ROM (sin
disco de sistema, o `MONITOR` en el LX-DOS) y escribe **`520ex`**. F12
(reset) vuelve al LX.

- Con **`alestedos-dev.dsk`** en la unidad A, el 520EX arranca **MSX-DOS
  1.04** (BIOS FOTON, COMMAND 2.2A): `dir`, M80/L80, PW y las demás
  herramientas.
- Con **`mag-player.dsk`**: `mag axelf` toca canciones de Scream Tracker por
  la MagicSound del 520EX (también `sll1`, `chrush`, `headache`, `techno1`).
- Con **`--boot amstrad`** (el lanzador `alestelx-520ex-basic`), se mantiene
  Mayús 2,5 s al entrar y el SETUP del 520EX arranca en modo AMSTRAD: el
  **BASIC 1.1 del CPC**.
- **Cintas** (`.cdt`, `.tzx`): `--tape fichero`, arrastrar el fichero a la
  ventana o Ctrl+F3. En el BASIC, escribe antes `|TAPE` (por defecto se usa
  el disco) y luego `RUN"` o `LOAD"`. La cinta suena mientras el motor está
  encendido; Ctrl+F4 la rebobina.

A tener en cuenta:

- **El teclado del 520EX es posicional**, como en el core: cada tecla da lo
  que tiene serigrafiado el teclado del 520EX en esa posición. Por ejemplo,
  `"` es Mayús+2, `|` es Mayús + la tecla a la derecha de la P, y la tecla
  `=` del PC da `↑`.
- La **RAM del reloj** (la configuración del SETUP del 520EX) se guarda en
  `aleste.cmo` junto a `boot.rom` (`--cmos fichero` para usar otro), como el
  ALESTE.CMO del core. La hora es la del PC.
- **Ratones:** el ratón serie del 520EX (MSX-DOS) siempre está; los de CPC
  con `--cpc-mouse` (Kempston, SYMBiFACE II, Multiplay) y `--amx-mouse` (AMX
  por el puerto del joystick), como las opciones del OSD del core. Un clic en
  la ventana o Ctrl+F5 captura el ratón del PC.

## 7. Joystick

El primer mando o joystick conectado es el joystick de la máquina (fila 9
de la matriz del teclado, como en el core): cruceta o stick izquierdo para
moverse, A/Cruz fuego 1, B/Círculo fuego 2. Se puede conectar y desconectar
con el emulador en marcha.

## 8. Depuración

- **Depurador propio** (`alestelx-debug`, o Pausa / Bloq Despl en cualquier
  momento): una consola en la terminal con puntos de ruptura (lógicos,
  físicos y por página física), pasos, traza, historial, desensamblado con
  símbolos, memoria, registros con el estado de la MMU, cambio de disco...
  Escribe `help`. Detalles en `README.md`.
- **GDB / DeZog** (`alestelx-gdb`): ver `GDB.es.md`.

## 9. Opciones de la línea de órdenes

    alestelx [opciones] boot.rom

| Opción | |
|---|---|
| `-a fichero`, `-b fichero` | disco en la unidad A / B |
| `--keyboard us\|es` | distribución del teclado (EE. UU. por defecto; `--es` = `--keyboard es`) |
| `--scale N` | ventana de 320N × 240N (3 por defecto) |
| `--boot amstrad` | 520EX: Mayús al entrar, modo AMSTRAD (BASIC del CPC) |
| `--tape fichero` | cinta (CDT / TZX) para el modo 520EX/CPC |
| `--rom520 fichero` | la ROM del 520EX (AL512.BIN) si boot.rom no la trae (por defecto, un AL512.BIN junto a boot.rom) |
| `--cmos fichero` | fichero de la RAM del reloj (por defecto `aleste.cmo` junto a `boot.rom`) |
| `--cpc-mouse`, `--amx-mouse` | ratones de CPC / ratón AMX |
| `--debug` | arrancar parado con la consola del depurador |
| `--sym fichero.map` | cargar símbolos para el depurador (se puede repetir) |
| `--break dir` | punto de ruptura (dirección, símbolo, `p:física`, `pg:página`) |
| `--gdb puerto` | servidor GDB (ver `GDB.es.md`) |

Para pruebas automáticas (con ventana o con `--headless`, sin ella):
`--ms N` (corre N ms de la máquina y sale), `--type MS:texto` (teclea texto
a los MS ms; `|` es Intro, `~` Esc), `--shot fichero.ppm` (la pantalla al
final), `--pcm fichero` (audio, estéreo de 16 bits a 48 kHz), `--save-a
fichero` (la imagen de la unidad A al final), `--cmd MS:orden` (orden del
depurador), `--mouse MS:dx,dy,botones`.

## 10. Problemas

- **"LXDOS.SYS was built for another ROM":** `boot.rom` y `LXDOS.DSK` son de
  compilaciones distintas. Usa los dos del mismo paquete.
- **"Aleste 520EX ROM not found" tras `520ex`:** pon `AL512.BIN` junto a
  `boot.rom` (apartado 1).
- **Pantalla negra o ruido en los modos gráficos del LX:** comprueba que el
  emulador es del mismo paquete (un `.exe` viejo no conoce el modo de 256
  colores).
- **No suena:** mira el volumen del sistema y que ningún otro programa tenga
  el dispositivo de audio en exclusiva.
- **Linux: "error while loading shared libraries: libSDL2":** instala SDL2
  (`sudo apt install libsdl2-2.0-0`).
- **Salen caracteres distintos en el 520EX:** su teclado es posicional
  (apartado 6). En el modo LX, elige la distribución de tu teclado
  (`--keyboard es`, `KEYB ES`).

## 11. Compilar desde el código fuente

    cmake -S . -B build && cmake --build build

en la carpeta `emulator` del repositorio (necesita los ficheros de
desarrollo de SDL2; en Windows, vcpkg o las bibliotecas de desarrollo de
SDL2 con `SDL2_DIR`). Ver `README.md`.
