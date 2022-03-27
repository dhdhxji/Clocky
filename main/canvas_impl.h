#ifndef CANVAS_IMPL_H
#define CANVAS_IMPL_H

#include "colproc/canvas/canvas.h"

#define MATRIX_W_DEFAULT        19
#define MATRIX_H_DEFAULT        7
#define REFRESH_RATE_DEFAULT    30


class CanvasConsole: public Canvas
{
public:
    CanvasConsole(size_t w, size_t h): Canvas(w, h) {
        _canvas = new ColRGB[w*h];

        for(size_t i = 0; i < w*h; ++i) {
            _canvas[i].r = 0;
            _canvas[i].g = 0;
            _canvas[i].b = 0;
        } 
    }

    ~CanvasConsole() override {
        delete[] _canvas;
    }

    void setPix(size_t x, size_t y, ColRGB col) override {
        if(x >= getW() || y >= getH()) {
            throw std::runtime_error(
                std::string("Out of bounds in setPix(") + 
                std::to_string(x) + 
                ";" + 
                std::to_string(y) + 
                ")"
            );
            return;
        }
        size_t index = y*getW() + x;

        _canvas[index] = col;
    }

    ColRGB getPix(size_t x, size_t y) const override {
        if(x >= getW() || y >= getH()) { 
            throw std::runtime_error(
                std::string("Out of bounds in getPix(") + 
                std::to_string(x) + 
                ";" + 
                std::to_string(y) + 
                ")"
            );
            return ColRGB(0,0,0);
        }
        size_t index = y*getW() + x;

        return _canvas[index];
    }

    void display() const override {
        printf("\033[0;0H");
        for(size_t y = 0; y < getH(); ++y) {
            for(size_t x = 0; x < getW(); ++x) {
                ColRGB col = getPix(x, y);
                printf("\033[48;2;%d;%d;%dm ", col.r, col.g, col.b);
                printf("\033[48;2;%d;%d;%dm ", col.r, col.g, col.b);
            }
            printf("\n");
        }
    }

private:
    ColRGB* _canvas;
};

#endif // CANVAS_IMPL_H
