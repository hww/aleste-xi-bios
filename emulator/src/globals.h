#ifndef _GLOBALS_H_
#define _GLOBALS_H_

    enum class ECycle { IOWR, IORD, MWR, MRD, OPCODE };
    enum class Mode { LEGACY, NATIVE };

    class Minibuffer;
    class IBuffer;
    class CLI;
        
    extern Minibuffer minibuffer;
    extern IBuffer ibuffer;
    
    extern CLI cli;

#endif