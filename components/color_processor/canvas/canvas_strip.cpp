#include "colproc/canvas/canvas_strip.h"

#include <cstring>

CanvasStrip::CanvasStrip(size_t w, size_t h, strip_mode_t mode): 
    Canvas(w, h) 
{
    _color_buffer = new uint8_t[_bufferSize()];
    memset(_color_buffer, 0, _bufferSize());
    _mode = mode;
}

CanvasStrip::~CanvasStrip() {
    delete[] _color_buffer;
}

void CanvasStrip::setPix(size_t x, size_t y, ColRGB col) {
    color_grb_t* pix = _getPixPtr(x, y);
    if(pix != nullptr) {
        *pix = color_grb_t(col);
    }
}

ColRGB CanvasStrip::getPix(size_t x, size_t y) {
    color_grb_t* pix = _getPixPtr(x, y);
    if(pix != nullptr) {
        return *pix;
    }
}

void CanvasStrip::display() {
    /* ESP_ERROR_CHECK(
        ctx->strip->set_pixels(
            ctx->strip, 
            0,
            ctx->led_count, 
            (const uint8_t*)(ctx->buffer)
        )
        );

        ESP_ERROR_CHECK(ctx->strip->refresh(ctx->strip, 100));
    */

    //ctx->buffer = new color_t[led_count];
}

CanvasStrip::color_grb_t* CanvasStrip::_getPixPtr(size_t x, size_t y) {
    // Check bounds
    if(y >= getH() || x >= getW()) {
        return nullptr;
    }

    // According to mode transform coordinates to an index
    size_t index;
    if(_mode == MODE_CONTINUOUS) {
        index = y*getW() + x;
    } else if(_mode == MODE_ZIGZAG) {
        index = y*getW() + ( (y%2 == 0) ? x:(getW()-1-x) );
    } else {
        return nullptr;
    }

    return &((color_grb_t*)_color_buffer)[index];
}

size_t CanvasStrip::_bufferSize() {
    return sizeof(color_grb_t) * getW() * getH();
}