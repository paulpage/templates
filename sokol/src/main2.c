#define SOKOL_IMPL

#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__linux) || defined(__unix__)
#define SOKOL_GLCORE33
#endif

#include "sokol_app.h"

static void init(void) {
}

void frame(void) {
}

void cleanup(void) {
}

void event(const sapp_event *e) {
}

sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .width = 800,
        .height = 600,
        .window_title = "App",
    };
}
