#ifndef _CRTC_H_
#define _CRTC_H_

#include <stdint.h>

enum class EScreenMode{
    Cpc_4b, 
    Cpc_2b,
    Cpc_1b,
    Linear16k1b,
    Linear16k2b,
    Linear16k4b,
    Linear32k2b,
    Linear32k4b,
    Linear32k8b,
    Linear64k2b,
    Linear64k4b,
    Linear64k8b
};

class CRTC {
public:
    EScreenMode screenMode;
    uint32_t screenPosition;

    CRTC() : screenMode(EScreenMode::Linear16k1b), screenPosition(0xC08000){

    }

    uint32_t get_screen_address() { return screenPosition; }
};


#endif // _CRTC_H_