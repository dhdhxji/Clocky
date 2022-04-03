#include "event_loop.hpp"
#include <stdexcept>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace std;


static void event_loop_handler(
    void *event_handler_arg, 
    esp_event_base_t event_base, 
    int32_t event_id, 
    void *event_data
) {
    auto ctx = static_cast<EventLoop::event_ctx_t*>(event_handler_arg);
    ctx->cb(*ctx->eloop, event_base, event_id, event_data);
}

EventLoop::EventLoop() {
    esp_event_loop_args_t cfg;
    cfg.queue_size = 128;
    cfg.task_name = "Event loop";
    cfg.task_stack_size = 2048;
    cfg.task_priority = 10;
    cfg.task_core_id = 0;
    
    
    esp_err_t st = esp_event_loop_create(&cfg, &handle);
    if(st != ESP_OK) {
        throw runtime_error(
            "Can not create event loop: " + string(esp_err_to_name(st))
        );
    }
}

EventLoop::~EventLoop() {
    esp_event_loop_delete(handle);
}


EventLoop::handle_t EventLoop::handlerRegister(
    event_base_t base, 
    int32_t eventId,
    event_handler_t handler
) {

    event_ctx_t* ctx = new event_ctx_t;
    ctx->cb = handler;
    ctx->eloop = this;

    evt_context.push_back(std::unique_ptr<event_ctx_t>(ctx));

    handle_t instance;
    instance.base = base;
    instance.evtId = eventId;
    esp_err_t st = esp_event_handler_instance_register_with(
        handle,
        base,
        eventId,
        event_loop_handler,
        ctx,
        &instance.instance
    );
    
    if(st != ESP_OK) {
        throw runtime_error("Can not add handle: " + string(esp_err_to_name(st)));
    }
    
    return instance;
}

void EventLoop::handlerUnregister(EventLoop::handle_t handle) {
    if(esp_event_handler_instance_unregister(
        handle.base,
        handle.evtId,
        handle.instance
    ) != ESP_OK) {
        throw runtime_error("Can not unregister event handler");
    }
}

void EventLoop::post(
    event_base_t base, 
    int32_t eventId, 
    void* data, 
    size_t size
) {
    esp_event_post_to(handle, base, eventId, data, size, portMAX_DELAY);
}