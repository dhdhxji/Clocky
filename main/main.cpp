#include "colproc/colproc.h"
#include "colproc/gen/rainbow.h"
#include "colproc/filter/brigtness_scale.h"
#include "colproc/canvas/canvas_strip.h"

#include "display_loop.h"


#define REFRESH_RATE_HZ 60
#define MATRIX_W        19
#define MATRIX_H        7

static ColProc* build_processor() {
    ColProc* gen = new GeneratorRainbow(110, 2, 5000);
    return new BrightnessScale(gen, 10); 
}


extern "C" {
    void app_main(void);
}

void app_main(void)
{
    ColProc* processor = build_processor();
    display_loop_start(
        processor, 
        new CanvasStrip(
            MATRIX_W, 
            MATRIX_H, 
            CanvasStrip::MODE_ZIGZAG, 
            RMT_CHANNEL_0, 
            GPIO_NUM_13
        ), 
        REFRESH_RATE_HZ
    );
}
