#ifndef MIXER_H
#define MIXER_H

#include "colproc/colproc.h"
#include "colproc/canvas/canvas_pixmap.h"

class Mixer: public ColProc {
public: 
    Mixer(
        ColProc* src,
        ColProc* mask
    ) {
        _src = src;
        _mask = mask;
    }

    virtual void render(
        int16_t off_x,
        int16_t off_y,
        uint32_t time,
        Canvas* canvas
    ) {
        CanvasPixMap mask_cv(canvas->getW(), canvas->getH());
        CanvasPixMap src_cv(canvas->getW(), canvas->getH());
        
        _mask->render(off_x, off_y, time, &mask_cv);
        _src->render(off_x, off_y, time, &src_cv);

        // Fit the canvas with result
        mask_cv.traverseSetPixels(
            [canvas, mask_cv, src_cv](uint16_t x, uint16_t y) {
                ColRGB new_color = 
                    src_cv.getPix(x, y) * mask_cv.getPix(x, y);

                canvas->setPix(x, y, new_color);
            }
        );
    }

protected:
    ColProc* _src;
    ColProc* _mask;
};

#endif // MIXER_H
