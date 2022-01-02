#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>

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

void gl_draw_triangles(GLfloat vertex_data[], GLuint index_data[]) {
    GLuint vbo, ibo;
    GLint vertex_pos_location = -1, vertex_color_location = -1;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(GLfloat), vertex_data, GL_STATIC_DRAW);

    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), index_data, GL_STATIC_DRAW);

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
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
    glDisableVertexAttribArray(vertex_pos_location);
    glUseProgram(0);
}

void gl_draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    float x1 = -1 + 2 * x / state.window_width;
    float y1 = 1 - 2 * y / state.window_height;
    float x2 = -1 + 2 * (x + w) / state.window_width;
    float y2 = 1 - 2 * (y + h) / state.window_height;

    GLfloat vertex_data[24] = {
        x1, y1, r, g, b, a,
        x2, y1, r, g, b, a,
        x2, y2, r, g, b, a,
        x1, y2, r, g, b, a,
    };
    GLuint index_data[] = { 0, 1, 2, 3, 0, 2 };

    gl_draw_triangles(vertex_data, index_data);
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
            }
        }

        // Draw
        glClear(GL_COLOR_BUFFER_BIT);
        gl_draw_rect(50.0f, 100.0f, 300.0f, 400.0f, 0.5f, 0.5f, 1.0f, 1.0f);
        gl_draw_rect(100.0f, 50.0f, 300.0f, 400.0f, 1.0f, 0.5f, 1.0f, 1.0f);
        SDL_GL_SwapWindow(state.window);
    }

    app_quit();
    return 0;
}
