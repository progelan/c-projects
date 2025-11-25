#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

// Cube vertices (8 vertices, each with 3 coordinates)
GLfloat cube_vertices[] = {
    // Front face
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    
    // Back face
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f
};

// Colors for each vertex (8 vertices, each with 3 RGB values)
GLfloat cube_colors[] = {
    // Front face
    1.0f, 0.0f, 0.0f,  // red
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    
    // Back face
    0.0f, 1.0f, 0.0f,  // green
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f
};

// Indices for cube rendering (6 faces with 2 triangles each = 12 triangles)
GLuint cube_indices[] = {
    // Front face
    0, 1, 2, 2, 3, 0,
    // Back face
    4, 5, 6, 6, 7, 4,
    // Top face
    3, 2, 6, 6, 5, 3,
    // Bottom face
    0, 4, 7, 7, 1, 0,
    // Right face
    1, 2, 6, 6, 7, 1,
    // Left face
    0, 3, 5, 5, 4, 0
};

// Shaders
const char* vertex_shader_source =
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "out vec3 ourColor;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "   ourColor = aColor;\n"
    "}\0";

const char* fragment_shader_source =
    "#version 330 core\n"
    "in vec3 ourColor;\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(ourColor, 1.0);\n"
    "}\0";

GLuint shaderProgram;
GLuint VAO, VBO[2], EBO;
float rotation_angle = 0.0f;

// Function to create perspective projection matrix
void perspectiveMatrix(float fov, float aspect, float near, float far, float* matrix) {
    float f = 1.0f / tanf(fov * 0.5f * 3.14159f / 180.0f);
    
    for (int i = 0; i < 16; i++) matrix[i] = 0.0f;
    
    matrix[0] = f / aspect;
    matrix[5] = f;
    matrix[10] = (far + near) / (near - far);
    matrix[11] = -1.0f;
    matrix[14] = (2.0f * far * near) / (near - far);
}

// Function to create rotation matrix
void rotationMatrix(float angle, float x, float y, float z, float* matrix) {
    float c = cosf(angle * 3.14159f / 180.0f);
    float s = sinf(angle * 3.14159f / 180.0f);
    float t = 1.0f - c;
    
    // Normalize rotation axis
    float length = sqrtf(x*x + y*y + z*z);
    x /= length;
    y /= length;
    z /= length;
    
    matrix[0] = t*x*x + c;
    matrix[1] = t*x*y - s*z;
    matrix[2] = t*x*z + s*y;
    matrix[3] = 0.0f;
    
    matrix[4] = t*x*y + s*z;
    matrix[5] = t*y*y + c;
    matrix[6] = t*y*z - s*x;
    matrix[7] = 0.0f;
    
    matrix[8] = t*x*z - s*y;
    matrix[9] = t*y*z + s*x;
    matrix[10] = t*z*z + c;
    matrix[11] = 0.0f;
    
    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

// Function to create translation matrix
void translationMatrix(float x, float y, float z, float* matrix) {
    for (int i = 0; i < 16; i++) matrix[i] = 0.0f;
    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
    matrix[12] = x;
    matrix[13] = y;
    matrix[14] = z;
}

// Identity matrix
void identityMatrix(float* matrix) {
    for (int i = 0; i < 16; i++) matrix[i] = 0.0f;
    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
}

// Multiply 4x4 matrices
void multiplyMatrices(float* a, float* b, float* result) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i*4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                result[i*4 + j] += a[i*4 + k] * b[k*4 + j];
            }
        }
    }
}

// Function to compile shaders
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        printf("Shader compilation error: %s\n", infoLog);
    }
    return shader;
}

void initShaders() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragment_shader_source);
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("Shader program linking error: %s\n", infoLog);
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void initBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(2, VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_colors), cube_colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void printGPUInfo() {
    printf("=== GPU Information ===\n");
    printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
    printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    printf("========================\n");
}

void initGL() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        printf("GLEW initialization error: %s\n", glewGetErrorString(err));
        return;
    }
    
    printGPUInfo();
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    initShaders();
    initBuffers();
    
    printf("OpenGL initialized successfully! You should see a ROTATING CUBE!\n");
}

void renderCube() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(shaderProgram);
    
    rotation_angle += 1.0f;
    if (rotation_angle > 360.0f) rotation_angle -= 360.0f;
    
    // Create transformation matrices
    float model[16], view[16], projection[16], temp[16], transform[16];
    
    // Model matrix (rotation)
    rotationMatrix(rotation_angle, 0.5f, 1.0f, 0.0f, model);
    
    // View matrix (move camera back)
    translationMatrix(0.0f, 0.0f, -3.0f, view);
    
    // Projection matrix
    perspectiveMatrix(45.0f, (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f, projection);
    
    // Combine matrices: transform = projection * view * model
    multiplyMatrices(view, model, temp);
    multiplyMatrices(projection, temp, transform);
    
    // Pass individual matrices to shader
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
    
    // Render cube
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int main(int argc, char* argv[]) {
    SDL_Window* window = NULL;
    SDL_GLContext glContext;
    
    printf("Starting 3D CUBE on GPU...\n");
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization error: %s\n", SDL_GetError());
        return 1;
    }
    
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    
    window = SDL_CreateWindow("3D CUBE on GPU (ROTATING!)",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             SCREEN_WIDTH,
                             SCREEN_HEIGHT,
                             SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    
    if (window == NULL) {
        printf("Window creation error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    glContext = SDL_GL_CreateContext(window);
    if (glContext == NULL) {
        printf("OpenGL context creation error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    SDL_GL_SetSwapInterval(1);
    
    initGL();
    
    printf("CUBE STARTED! You should see a rotating cube with red and green faces!\n");
    printf("Press ESC to exit.\n");
    
    int running = 1;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    int frames = 0;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                }
            }
        }
        
        renderCube();
        SDL_GL_SwapWindow(window);
        
        frames++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTime >= 1000) {
            printf("FPS: %d\n", frames);
            frames = 0;
            lastTime = currentTime;
        }
        
        SDL_Delay(16);
    }
    
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(2, VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    printf("Application terminated.\n");
    
    return 0;
}