#include <iostream>
#include <exception>
#include <string>
#include "colproc/runtime/lua_runtime.h"
#include "colproc/variable/variable_callback.h"



#define REFRESH_RATE_HZ 30
#define MATRIX_W        19
#define MATRIX_H        7



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



extern "C" void app_main() {
    CanvasConsole canvas(MATRIX_W, MATRIX_H);
    
    try {
        LuaRuntime rt(&canvas, 30);

        rt.getVariableManager().addVariable(
            "hrs",
            new VariableCallback<std::string>([](){
                time_t ttime = time(0);
                struct tm* tm_time = localtime(&ttime);
                return  std::to_string(tm_time->tm_hour);
            })
        );

        rt.getVariableManager().addVariable(
            "mins",
            new VariableCallback<std::string>([](){
                time_t ttime = time(0);
                struct tm* tm_time = localtime(&ttime);
                return  std::to_string(tm_time->tm_min);
            })
        );

        rt.loadScript("../init.lua");

        rt.runRenderLoop();
    } catch(std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    } catch(...) {
        std::cerr << "Unexpected error" << std::endl;
    }
}