#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

typedef struct AppState {
    float window_width;
    float window_height;

    SDL_Window *window;
    SDL_GLContext context;
    GLuint tri_program_id;
} AppState;
static AppState state = {
    .window_width = 800.0f,
    .window_height = 600.0f,

    .window = NULL,
    .context = NULL,
    .tri_program_id = 0,
};

typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

const GLchar *TRI_VERT_SRC =
    "#version 140\n"
    "in vec2 position;"
    "in vec4 color;"
    "out vec4 v_color;"
    "void main() {"
    "  v_color = color;"
    "  gl_Position = vec4(position.x, position.y, 0, 1);"
    "}";

const GLchar* TRI_FRAG_SRC =
    "#version 140\n"
    "in vec4 v_color;"
    "out vec4 LFragment;"
    "void main() {"
    "  LFragment = v_color;"
    "}";


static GLuint gl_create_shader(GLenum type, const GLchar *src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);
    GLint success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        int len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        char buffer[len]; // TODO do we have to zero this?
        glGetShaderInfoLog(id, len, NULL, buffer);
        printf("Error creating shader: %s\n", buffer);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static GLuint gl_create_program(const char *vert_src, const char *frag_src) {
    GLuint vert = gl_create_shader(GL_VERTEX_SHADER, vert_src);
    GLuint frag = gl_create_shader(GL_FRAGMENT_SHADER, frag_src);
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        int len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        char buffer[len];
        glGetProgramInfoLog(program, len, NULL, buffer);
        printf("Error creating program: %s\n", buffer);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

bool app_init() {

    // Window
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Error initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);

    state.window = SDL_CreateWindow("App", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (int)state.window_width, (int)state.window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (state.window == NULL) {
        printf("Error creating window: %s\n", SDL_GetError());
        return false;
    }

    state.context = SDL_GL_CreateContext(state.window);
    if (state.context == NULL) {
        printf("Error creating OpenGL context: %s\n", SDL_GetError());
        return false;
    }
    glewExperimental = GL_TRUE; 
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        printf("Error initializing GLEW: %s\n", glewGetErrorString(glewError));
        return false;
    }

    if (SDL_GL_SetSwapInterval(1) < 0) {
        printf("Warning: Unable to set VSync: %s\n", SDL_GetError());
    }


    glEnable(GL_MULTISAMPLE);

    // GL Programs
    state.tri_program_id = gl_create_program(TRI_VERT_SRC, TRI_FRAG_SRC);
    if (!state.tri_program_id) {
        return false;
    }

    return true;
}

void app_update() {
    int w, h;
    SDL_GetWindowSize(state.window, &w, &h);
    state.window_width = (float)w;
    state.window_height = (float)h;
}

void app_quit() {
    glDeleteProgram(state.tri_program_id);
    SDL_DestroyWindow(state.window);
    state.window = NULL;
    SDL_Quit();
}

float gl_x(float x) {
    return -1.0f + 2.0f * x / state.window_width;
}

float gl_y(float y) {
    return 1.0f - 2.0f * y / state.window_height;
}

void gl_draw_triangles(GLfloat vertex_data[], GLuint index_data[], int vertex_count, int triangle_count) {
    GLuint vbo, ibo;
    GLint vertex_pos_location = -1, vertex_color_location = -1;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * vertex_count * sizeof(GLfloat), vertex_data, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * triangle_count * sizeof(GLuint), index_data, GL_STATIC_DRAW);

    vertex_pos_location = glGetAttribLocation(state.tri_program_id, "position");
    vertex_color_location = glGetAttribLocation(state.tri_program_id, "color");
    if (vertex_pos_location == -1 || vertex_color_location == -1) {
        return;
    }

    glUseProgram(state.tri_program_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(vertex_pos_location);
    glVertexAttribPointer(vertex_pos_location, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), NULL);
    glEnableVertexAttribArray(vertex_color_location);
    glVertexAttribPointer(vertex_color_location, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glDrawElements(GL_TRIANGLES, 3 * triangle_count, GL_UNSIGNED_INT, NULL);
    glDisableVertexAttribArray(vertex_pos_location);
    glUseProgram(0);
}

void gl_draw_rect(float x, float y, float w, float h, Color color) {
    float x1 = gl_x(x);
    float y1 = gl_y(y);
    float x2 = gl_x(x + w);
    float y2 = gl_y(y + h);

    float r = (float)color.r / 255.0f;
    float g = (float)color.g / 255.0f;
    float b = (float)color.b / 255.0f;
    float a = (float)color.a / 255.0f;

    GLfloat vertex_data[24] = {
        x1, y1, r, g, b, a,
        x2, y1, r, g, b, a,
        x2, y2, r, g, b, a,
        x1, y2, r, g, b, a,
    };
    GLuint index_data[] = { 0, 1, 2, 3, 0, 2 };

    gl_draw_triangles(vertex_data, index_data, 4, 2);
}

void gl_draw_circle_arc(float x, float y, float cr, float theta_start, float theta_arc, Color color) {
    int n = (int)(40.0f * theta_arc / (2*M_PI));
    GLfloat vertex_data[(n + 2) * 6];
    GLuint index_data[n * 3];
    int p = 0, i = 0;

    float r = (float)color.r / 255.0f;
    float g = (float)color.g / 255.0f;
    float b = (float)color.b / 255.0f;
    float a = (float)color.a / 255.0f;

    vertex_data[p] = gl_x(x); p++;
    vertex_data[p] = gl_y(y); p++;
    vertex_data[p] = r; p++;
    vertex_data[p] = g; p++;
    vertex_data[p] = b; p++;
    vertex_data[p] = a; p++;
    vertex_data[p] = gl_x(x + cr * cosf(theta_start)); p++;
    vertex_data[p] = gl_y(y - cr * sinf(theta_start)); p++;
    vertex_data[p] = r; p++;
    vertex_data[p] = g; p++;
    vertex_data[p] = b; p++;
    vertex_data[p] = a; p++;
    for (int t = 0; t < n; t++) {
        float theta = theta_start + (float)(t + 1) * theta_arc / (float)n;
        vertex_data[p] = gl_x(x + cr * cosf(theta)); p++;
        vertex_data[p] = gl_y(y - cr * sinf(theta)); p++;
        vertex_data[p] = r; p++;
        vertex_data[p] = g; p++;
        vertex_data[p] = b; p++;
        vertex_data[p] = a; p++;

        index_data[i] = 0; i++;
        index_data[i] = t + 1; i++;
        index_data[i] = t + 2; i++;
    }

    gl_draw_triangles(vertex_data, index_data, n + 2, n);
}

void gl_draw_rounded_rect(float x, float y, float w, float h, float cr, Color color) {
    gl_draw_rect(x + cr, y, w - 2*cr, h, color);
    gl_draw_rect(x, y + cr, w, h - 2*cr, color);

    gl_draw_circle_arc(x + w - cr, y + cr, cr, 0.0f, M_PI_2, color);
    gl_draw_circle_arc(x + cr, y + cr, cr, M_PI_2, M_PI_2, color);
    gl_draw_circle_arc(x + cr, y + h - cr, cr, M_PI, M_PI_2, color);
    gl_draw_circle_arc(x + w - cr, y + h - cr, cr, M_PI + M_PI_2, M_PI_2, color);
}

int main(void) {
    if (!app_init()) {
        printf("Failed to initialize");
        return 1;
    }

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        app_update();
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_q) {
                        quit = true;
                    }
                    break;
            }
        }

        // Draw
        glClear(GL_COLOR_BUFFER_BIT);
        gl_draw_rect(50.0f, 100.0f, 300.0f, 400.0f, {128, 128, 255, 255});
        gl_draw_rect(100.0f, 50.0f, 300.0f, 400.0f, {255, 128, 255, 255});
        gl_draw_rounded_rect(150.0f, 150.0f, 300.0f, 400.0f, 40.0f, {255, 255, 255, 255});
        SDL_GL_SwapWindow(state.window);
    }

    app_quit();
    return 0;
}
