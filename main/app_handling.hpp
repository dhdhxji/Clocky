#ifndef APP_HANDLING_HPP
#define APP_HANDLING_HPP

#include <memory>
#include <string>
#include "cfg.hpp"
#include "event_loop.hpp"
#include "canvas_impl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "colproc/colproc.h"
#include "colproc/runtime/lua_runtime.h"
#include "colproc/gen/text.h"
#include "colproc/variable/variable_constant.h"

DEFINE_BASE_ID(APP);
#define APP_INIT_RUNTIME_CMD        0   //Init runtime and canvas
#define APP_DEINIT_RUNTIME_CMD      1   //Deinit runtime and canvas
#define APP_REINIT_RUNTIME_CMD      2   //Deinit runtime than init it
#define APP_START_LOOP_CMD          3   //Start user defined rendering to canvas
#define APP_STOP_LOOP_CMD           4   //Stop user defined render loop
#define APP_LOOP_STOPPED_EVT        5   //Event loop stopped event
#define APP_ERROR_EVT               6   //Error of app execution event

#define TAG "APP"

#define STR_VWLEN(str) (void*)(str), (strlen((str))+1)

std::unique_ptr<Canvas> canvas = nullptr;
std::unique_ptr<Runtime> rt = nullptr;

Runtime* build_upload_screen_rt(Canvas* cv, uint32_t frameRare) {
    Runtime* rt = new Runtime();
    rt->getVariableManager().addVariable("text", new VariableConstant<std::string>("Upload"));
    rt->getVariableManager().addVariable("text_font", new VariableConstant<std::string>("3_by_57"));

    ColProc* upload = new GeneratorText(
        rt->getVariableManager().getVariable("text")->castToVariable<std::string>(),
        rt->getVariableManager().getVariable("text_font")->castToVariable<std::string>()
    );

    rt->setCanvas(cv);
    rt->setFrameRate(frameRare);
    rt->setRenderNode(upload);

    return rt;
}

extern "C" void render_loop_task_fn(void* parameters) {
    EventLoop* ev = (EventLoop*)parameters;

    if(rt.get() != nullptr) {
        rt->runRenderLoop();
    } 
    else {
        ev->post(APP, APP_ERROR_EVT, STR_VWLEN("Can not start render loop: runtime was not initialised"));
    }
    ev->post(APP, APP_LOOP_STOPPED_EVT, nullptr, 0);

    vTaskDelete(nullptr);
}

void init_app_handler(EventLoop& ev, Cfg& cfg) {
    ev.handlerRegister(APP, APP_INIT_RUNTIME_CMD, [&cfg](EventLoop& ev, event_base_t, int32_t, void*){
        //TODO: Check is render loop running
        //TODO: Check is app init
        // ^ post errors for these events
        
        size_t w = cfg.get<int32_t>("render.screen.w", MATRIX_W_DEFAULT);
        size_t h = cfg.get<int32_t>("render.screen.h", MATRIX_H_DEFAULT);
        uint32_t frameRate = cfg.get<int32_t>("render.frameRate", REFRESH_RATE_DEFAULT);
        
        canvas.reset(new CanvasConsole(w, h));

        std::string initPath = cfg.get<std::string>("render.initScriptPath", "");
        if(initPath != "") {
            try {
                rt.reset(new LuaRuntime(canvas.get(), frameRate, initPath));
            } catch (std::exception& e) {
                ev.post(APP, APP_ERROR_EVT, STR_VWLEN(e.what()));
            }
        } else {
            ev.post(APP, APP_ERROR_EVT, STR_VWLEN("Error: render.initScriptPats not set"));
            rt.reset(build_upload_screen_rt(canvas.get(), frameRate));
        }
    });

    ev.handlerRegister(APP, APP_DEINIT_RUNTIME_CMD, [](EventLoop& ev, event_base_t, int32_t, void*){
        //Check is rl running
        // ^ post errors for these events
        
        rt.release();
        canvas.release();
    });

    ev.handlerRegister(APP, APP_REINIT_RUNTIME_CMD, [](EventLoop& ev, event_base_t, int32_t, void*){
        ev.post(APP, APP_DEINIT_RUNTIME_CMD, nullptr, 0);
        ev.post(APP, APP_INIT_RUNTIME_CMD, nullptr, 0);
    });

    ev.handlerRegister(APP, APP_START_LOOP_CMD, [](EventLoop& ev, event_base_t, int32_t, void*){
        //Check is runtime init
        // ^ post errors for these events
        
        //Check is not running
        // ^ post errors for these events

        xTaskCreate(render_loop_task_fn, "rloop", 4096, &ev, 1, nullptr);
    });

    ev.handlerRegister(APP, APP_STOP_LOOP_CMD, [](EventLoop& ev, event_base_t, int32_t, void*){
        //Check is rloop running
        // ^ post errors for these events

        rt->interrupt();
    });
}

#undef TAG

#endif // APP_HANDLING_HPP
