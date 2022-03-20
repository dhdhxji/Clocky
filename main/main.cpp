#include "colproc/colproc.h"
#include "colproc/gen/rainbow.h"
#include "colproc/gen/text.h"
#include "colproc/mod/move.h"
#include "colproc/mod/mixer.h"

#include "colproc/filter/brigtness_scale.h"
#include "colproc/variable/variable_constant.h"
#include "colproc/variable/variable_callback.h"
#include "colproc/switch/switch.h"
#include "colproc/switch/transition_simple.h"
#include "colproc/mod/concat.h"
#include "colproc/runtime/runtime.h"

#include <chrono>


#define REFRESH_RATE_HZ 30
#define MATRIX_W        19
#define MATRIX_H        7


using namespace std::chrono;


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



static ColProc* build_processor(IVariableStorage& storage) {
    storage.addVariable(
        "hrs",
        new VariableCallback<std::string>([](){
            time_t ttime = time(0);
            struct tm* tm_time = localtime(&ttime);
            return  std::to_string(tm_time->tm_hour);
        })
    );

    storage.addVariable(
        "mins",
        new VariableCallback<std::string>([](){
            time_t ttime = time(0);
            struct tm* tm_time = localtime(&ttime);
            return  std::to_string(tm_time->tm_min);
        })
    );

    ColProc* hrs = new GeneratorText(
        storage.getVariable("hrs")->castToVariable<std::string>(),
        new VariableConstant<std::string>("3_by_57")
    );

    ColProc* dash = new Move(
        new GeneratorText(
            new VariableConstant<std::string>(":"),
            new VariableConstant<std::string>("3_by_57")
        ),
        new VariableConstant<int32_t>(7),
        new VariableConstant<int32_t>(0)
    );

    ColProc* min = new Move(
        new GeneratorText(
            storage.getVariable("mins")->castToVariable<std::string>(),
            new VariableConstant<std::string>("3_by_57")
        ),
        new VariableConstant<int32_t>(10),
        new VariableConstant<int32_t>(0)
    );

    ColProc* text = new Concat({hrs, dash, min});

    ColProc* moved_text = new Move(
        text,
        new VariableConstant<int32_t>(1),
        new VariableConstant<int32_t>(1)
    );

    ColProc* rainbow = new GeneratorRainbow(
        new VariableConstant<int32_t>(50),
        new VariableConstant<int32_t>(0),
        new VariableConstant<int32_t>(5000)
    );
    
    ColProc* mixed_text = new Mixer(rainbow, moved_text);

    ColProc* switched = new Switch(
        {mixed_text, rainbow},
        storage.getVariable("rainbow_text_select")->castToVariable<int32_t>(),
        new TransitionSimple()
    );

    return new BrightnessScale(
        switched, 
        new VariableConstant<double>(1)
    );
}


extern "C" void app_main()
{
    CanvasConsole canvas(MATRIX_W, MATRIX_H);

    Runtime rt;
    rt.setCanvas(&canvas);
    rt.setFrameRate(REFRESH_RATE_HZ);
    
    rt.getVariableManager().addVariable(
        "rainbow_text_select",
        new VariableCallback<int32_t>(
            []() {
                int32_t time_ms = 
                    duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                return (time_ms/1000) % 2;
            }
        )
    );

    std::unique_ptr<ColProc> node( build_processor(rt.getVariableManager()) );
    rt.setRenderNode(node.get());

    rt.runRenderLoop();
}
