#ifndef EVENT_LOOP_HPP
#define EVENT_LOOP_HPP

#include <functional>
#include <vector>
#include <memory>
#include <cstring>
#include "esp_event.h"

#define DEFINE_BASE_ID(id) ESP_EVENT_DEFINE_BASE(id)
#define DECLARE_BASE_ID(id) ESP_EVENT_DECLARE_BASE(id)

#define ARG_STR(str)    ((void*)(str)), (strlen(str))
#define ARG_SIMPLE(val) ((void*)(&val)), (sizeof(val))
#define ARG_NONE        (nullptr), (0)

typedef esp_event_base_t event_base_t;

class EventLoop {
public:
    typedef std::function<void(EventLoop&, event_base_t, int32_t, void*)> event_handler_t;

    struct handle_t
    {
        event_base_t base;
        int32_t evtId;
        esp_event_handler_instance_t instance;
    };
  
    struct event_ctx_t {
        EventLoop* eloop;
        event_handler_t cb;
    };
    
    EventLoop();
    ~EventLoop();

    handle_t handlerRegister(
        event_base_t base, 
        int32_t eventId,
        event_handler_t handler
    );

    void handlerUnregister(handle_t handle);

    void post(event_base_t base, int32_t event, void* data, size_t size);

protected:
    esp_event_loop_handle_t handle; 
    std::vector<std::unique_ptr<event_ctx_t>> evt_context;
};

#endif // EVENT_LOOP_HPP
