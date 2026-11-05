# Depurar el Aleste LX con GDB o DeZog (servidor GDB de alestelx)

*(English version: `GDB.md`.)*

`alestelx` lleva un servidor del **protocolo remoto de GDB** (RSP, *Remote
Serial Protocol*): el mismo que usan `gdbserver`, OpenOCD o el *gdbstub* de
MAME. Con él, un depurador externo controla la máquina emulada: la para, la
hace avanzar instrucción a instrucción, pone puntos de ruptura y lee o escribe
registros y memoria, mientras el emulador sigue mostrando la pantalla y
sonando como siempre.

Clientes posibles:

- **DeZog** (extensión de VS Code para Z80): depuración visual con el código
  fuente, registros, memoria, pila y desensamblado. Se conecta como a MAME.
- **GDB** compilado con soporte de Z80, desde la línea de órdenes.
- Cualquier herramienta que hable RSP (IDA, scripts propios...). En
  `tests/gdb_rsp_test.py` hay un cliente mínimo en Python que sirve de
  ejemplo y de prueba.

## Arrancar el emulador con el servidor

Windows: `alestelx-gdb.bat`. Linux: `./alestelx-gdb.sh`. Los dos hacen:

    alestelx -a LXDOS.DSK --gdb 2159 boot.rom

- `--gdb PUERTO` abre el servidor en `localhost:PUERTO` (2159 en los
  lanzadores). Solo escucha en la propia máquina (127.0.0.1): no es
  accesible desde la red.
- La máquina **arranca parada**, en el reset (PC = 0000h), esperando a que
  el depurador se conecte y le diga que siga. Es como MAME con `-debug
  -debugger gdbstub`.
- Al conectarse un depurador, la máquina se para; al desconectarse, sigue
  sola. Se puede volver a conectar.
- Se combina con el resto de opciones: discos, `--keyboard`, `--sym`,
  `--debug` (la consola del depurador propio en la terminal, a la vez)...

Sin ventana, para pruebas automáticas:

    alestelx --headless --gdb 2159 -a LXDOS.DSK boot.rom

En este modo el emulador termina cuando el depurador se desconecta o lo
mata (`kill`).

## Qué ofrece el servidor

### Registros

Doce registros de 16 bits, en el orden del *gdbstub* de MAME (que es el que
espera DeZog), y descritos en el `target.xml` que el servidor envía, así que
GDB los entiende sin configurar nada:

| n | registro | n | registro | n | registro |
|---|---|---|---|---|---|
| 0 | AF | 4 | AF' | 8 | IX |
| 1 | BC | 5 | BC' | 9 | IY |
| 2 | DE | 6 | DE' | 10 | SP |
| 3 | HL | 7 | HL' | 11 | PC |

I, R y el modo de interrupción no van en esta lista: se ven con `monitor r`
(ver más abajo).

### Memoria: lógica y física

El LX tiene 16 MB de memoria física y una MMU que pone bancos de 16 KB en las
cuatro ventanas que ve la CPU. El servidor da acceso a las dos cosas:

| Dirección en el depurador | Qué es |
|---|---|
| `0000`-`FFFF` | lo que ve la CPU ahora, a través de la MMU (en el modo 520EX, de su MMU) |
| `10000` en adelante | memoria **física**: dirección − 10000h |

Ejemplos de memoria física:

| En el depurador | Física | Qué hay |
|---|---|---|
| `0xC10000` | C00000 | la ROM del LX (BIOS) |
| `0xC50000` | C40000 | la VRAM del modo de texto |
| `0xC60000` | C50000 | la VRAM de 320×200×256 |
| `0xC90000` | C80000 | LXDOS.SYS |
| `0xCA0000` | C90000 | el programa .COM en ejecución (su 0000h) |

Lectura y escritura funcionan en las dos zonas (lecturas de hasta 2 KB por
petición), para los clientes que envían la dirección completa, como el de
prueba. **GDB para Z80 trabaja con direcciones de 16 bits** y puede recortar
las de la zona física: con GDB, la memoria física se ve con `monitor pm`
(ver más abajo).

### Puntos de ruptura

- De **ejecución**, por dirección de la CPU (los `break` normales de GDB y los
  de DeZog). Saltan con cualquier banco puesto en esa ventana: un punto en
  4000h para tanto en el código de la ROM como en el del LX-DOS si los dos
  pasan por ahí.
- Para parar solo en un banco concreto, con las órdenes del depurador propio
  a través de `monitor`:
  - `monitor b p:C86CED` — en esa dirección **física** exacta;
  - `monitor b pg:322` — al ejecutar cualquier cosa de la página física
    322h (16 KB: dirección física / 4000h; aquí C88000-C8BFFF, el banco 22h
    del slot 3, la tercera ventana del LX-DOS).
  - Cuando saltan, el depurador externo recibe la parada como una más.
- Puntos de vigilancia de memoria (*watchpoints*): todavía no. GDB, si se le
  pide uno, lo simula él solo avanzando paso a paso (lento, pero funciona).

### Control

| | |
|---|---|
| continuar | `continue` (también desde una dirección) |
| un paso | `stepi` (una instrucción, con sus prefijos) |
| parar | Ctrl-C en GDB, o el botón de pausa de DeZog |
| salir | `detach` (la máquina sigue) o `kill` (termina el emulador) |

### `monitor`: las órdenes del depurador de alestelx

`monitor ORDEN` ejecuta cualquier orden del depurador propio. **Su salida sale
en la terminal del emulador**, no en la del depurador. Las más útiles:

| | |
|---|---|
| `monitor r` | registros, I, R, IM, IFF, la MMU (supervisor, slots, bancos) y el PIC |
| `monitor hist 40` | las 40 últimas instrucciones ejecutadas, desensambladas desde donde se ejecutaron |
| `monitor d 4000 20` | desensamblar con símbolos |
| `monitor pm C50000 64` | memoria física |
| `monitor sym lxdos.map` | cargar símbolos de un mapa de z88dk |
| `monitor b p:C86CED`, `monitor b pg:322` | puntos de ruptura físicos o por página física de 16 KB |
| `monitor bl`, `monitor bc all` | listarlos y borrarlos |
| `monitor disk a otro.dsk` | cambiar el disco |

Ver `README.md` para la lista completa.

## Con DeZog (VS Code)

DeZog se conecta al emulador igual que a MAME: tipo de conexión `mame`, en
`localhost` y el puerto del emulador. Un `launch.json` de partida:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "dezog",
            "request": "launch",
            "name": "Aleste LX (alestelx)",
            "remoteType": "mame",
            "mame": { "hostname": "localhost", "port": 2159 },
            "rootFolder": "${workspaceFolder}",
            "startAutomatically": false
        }
    ]
}
```

1. Arranca el emulador con `alestelx-gdb.bat` / `alestelx-gdb.sh`.
2. En VS Code, lanza la configuración "Aleste LX (alestelx)".
3. Registros, memoria, desensamblado, puntos de ruptura (clic en el margen)
   y pasos funcionan desde la interfaz de DeZog.

Para depurar con el código fuente, DeZog lee los ficheros de listado de
z88dk: añade a la configuración el apartado de listados de z88dk que indica
la documentación de DeZog para tu versión, apuntando a los `.lis` y al `.map`
de la compilación (`lx/build/src/build/aleste.map` para la ROM,
`lx/build/dos/lxdos.map` para el LX-DOS). Recuerda que la ROM y el LX-DOS
usan las mismas direcciones lógicas en bancos distintos: carga los listados
del que vayas a depurar.

> El servidor se ha probado con un cliente RSP propio (`tests/gdb_rsp_test.py`)
> que sigue los pasos de GDB y DeZog; con DeZog en sí todavía no. Si algo no
> encaja, la terminal del emulador muestra cada conexión y las órdenes
> `monitor`.

## Con GDB

Hace falta un GDB con soporte de Z80 (existe desde GDB 11). El
`gdb-multiarch` de Ubuntu y Debian **no** lo trae compilado ("Undefined item:
z80"). Para compilarlo en Linux (o en WSL en Windows):

```sh
wget https://ftp.gnu.org/gnu/gdb/gdb-15.2.tar.xz
tar xf gdb-15.2.tar.xz && cd gdb-15.2
./configure --target=z80-unknown-elf --prefix=$HOME/z80gdb --disable-sim
make -j$(nproc) && make install
export PATH=$HOME/z80gdb/bin:$PATH        # queda como z80-unknown-elf-gdb
```

Una sesión:

```
$ z80-unknown-elf-gdb
(gdb) target remote :2159
(gdb) info registers
(gdb) x/8xb 0                     # memoria de la CPU: DI, JP 0100h tras el reset
(gdb) monitor pm C00000 32        # memoria fisica (sale en la terminal del emulador)
(gdb) break *0x4B4E               # un punto de ruptura (main de la ROM: grep _main bios.map)
(gdb) continue
(gdb) x/5i $pc                    # desensamblar
(gdb) stepi
(gdb) monitor hist 20             # salida en la terminal del emulador
(gdb) set $hl = 0x1234            # cambiar un registro
(gdb) set *(unsigned char*)0x0100 = 0     # escribir en la memoria de la CPU
(gdb) continue                    # Ctrl-C para parar
(gdb) detach                      # o kill
```

Las direcciones de las funciones salen de los mapas de z88dk (`aleste.map`,
`lxdos.map`, que van en el paquete como `bios.map` y `lxdos.map`): `grep
_main bios.map`, o `monitor sym bios.map` y luego `monitor d main`.

## Problemas frecuentes

- **No conecta:** el puerto está ocupado por otro programa (prueba otro con
  `--gdb 2160`), o el emulador no se arrancó con `--gdb`. Solo acepta
  conexiones de la misma máquina.
- **Se para "solo" al arrancar:** es lo esperado, espera al depurador. Pulsa
  continuar.
- **Un punto de ruptura salta donde no esperabas:** son por dirección
  lógica; usa `monitor b p:...` para una dirección física concreta.
- **La máquina se cuelga en C000h (`_undefined_method`):** la ROM y el
  LXDOS.SYS no son de la misma compilación; las ROM nuevas lo detectan y
  avisan al arrancar.

## Probar el servidor

```sh
./alestelx-linux --headless --gdb 2159 -a LXDOS.DSK boot.rom &
python3 gdb_rsp_test.py 2159 bios.map
```

Recorre `qSupported`, `target.xml`, registros, memoria lógica y física, un
punto de ruptura en `main` con `continue`, un paso, `monitor`, Ctrl-C y
`kill`.

## Paquetes que entiende el servidor (para quien escriba un cliente)

`?`, `g`, `G`, `p`, `P`, `m`, `M`, `c [dir]`, `s [dir]`, `vCont?`,
`vCont;c|C|s|S|t`, `Z0`/`Z1` y `z0`/`z1`, `D`, `k`, `vKill`, `H`, `T`,
`qSupported`, `qXfer:features:read:target.xml`, `qAttached`, `qC`,
`qfThreadInfo`, `qsThreadInfo`, `qOffsets`, `qSymbol`, `qRcmd` (monitor),
`QStartNoAckMode` y el byte 03h (Ctrl-C). Paradas: `S05` (punto de ruptura o
paso) y `S02` (interrupción).
