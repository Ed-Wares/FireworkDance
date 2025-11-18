#include <iostream>
#include <cmath>
#include <vector>
#include <numeric>
#include <array>

// Include GLAD before GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_truetype.h" // For font rendering
#include "stb_image.h"  // For image loading


// --- Configuration Constants ---
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;
const int NUM_EXPLOSIONS = 10;
const int PARTICLES_PER_EXPLOSION = 500;
const int NUM_PARTICLES = NUM_EXPLOSIONS * PARTICLES_PER_EXPLOSION;
const int ATTRIB_SIZE = 10; // origin(3), direction(3), color(3), startTime(1)

// --- Simple 4x4 Matrix Implementation (Replacing JS mat4/GLM) ---

using mat4 = std::array<float, 16>;

/**
 * Creates an identity matrix.
 */
mat4 create_identity() {
    mat4 out = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    return out;
}

/**
 * Creates a perspective projection matrix.
 */
void perspective(mat4& out, float fovy_rad, float aspect, float near, float far) {
    float f = 1.0f / std::tan(fovy_rad / 2.0f);
    float nf = 1.0f / (near - far);

    out.fill(0.0f);
    out[0] = f / aspect;
    out[5] = f;
    out[10] = (far + near) * nf;
    out[11] = -1.0f;
    out[14] = (2.0f * far * near) * nf;
}

/**
 * Creates a lookAt view matrix.
 */
void lookAt(mat4& out, const std::array<float, 3>& eye, const std::array<float, 3>& center, const std::array<float, 3>& up) {
    float x0, x1, x2, y0, y1, y2, z0, z1, z2, len;
    float eyex = eye[0], eyey = eye[1], eyez = eye[2];
    float upx = up[0], upy = up[1], upz = up[2];
    float centerx = center[0], centery = center[1], centerz = center[2];

    z0 = eyex - centerx; z1 = eyey - centery; z2 = eyez - centerz;
    len = 1.0f / std::sqrt(z0 * z0 + z1 * z1 + z2 * z2);
    z0 *= len; z1 *= len; z2 *= len;

    x0 = upy * z2 - upz * z1;
    x1 = upz * z0 - upx * z2;
    x2 = upx * z1 - upy * z0;
    len = 1.0f / std::sqrt(x0 * x0 + x1 * x1 + x2 * x2);
    x0 *= len; x1 *= len; x2 *= len;

    y0 = z1 * x2 - z2 * x1;
    y1 = z2 * x0 - z0 * x2;
    y2 = z0 * x1 - z1 * x0;
    len = 1.0f / std::sqrt(y0 * y0 + y1 * y1 + y2 * y2);
    y0 *= len; y1 *= len; y2 *= len;

    // Transposed for OpenGL column-major order:
    out[0] = x0; out[1] = x1; out[2] = x2; out[3] = 0;
    out[4] = y0; out[5] = y1; out[6] = y2; out[7] = 0;
    out[8] = z0; out[9] = z1; out[10] = z2; out[11] = 0;
    out[12] = -(x0 * eyex + x1 * eyey + x2 * eyez);
    out[13] = -(y0 * eyex + y1 * eyey + y2 * eyez);
    out[14] = -(z0 * eyex + z1 * eyey + z2 * eyez);
    out[15] = 1.0f;
}

/**
 * Multiplies two 4x4 matrices (out = a * b).
 */
void multiply(mat4& out, const mat4& a, const mat4& b) {
    float a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3];
    float a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7];
    float a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11];
    float a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];
    
    // Column 0
    float b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
    out[0] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out[1] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out[2] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out[3] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

    // Column 1
    b0 = b[4]; b1 = b[5]; b2 = b[6]; b3 = b[7];
    out[4] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out[5] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out[6] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out[7] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

    // Column 2
    b0 = b[8]; b1 = b[9]; b2 = b[10]; b3 = b[11];
    out[8] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out[9] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out[10] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out[11] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;

    // Column 3
    b0 = b[12]; b1 = b[13]; b2 = b[14]; b3 = b[15];
    out[12] = b0 * a00 + b1 * a10 + b2 * a20 + b3 * a30;
    out[13] = b0 * a01 + b1 * a11 + b2 * a21 + b3 * a31;
    out[14] = b0 * a02 + b1 * a12 + b2 * a22 + b3 * a32;
    out[15] = b0 * a03 + b1 * a13 + b2 * a23 + b3 * a33;
}


// --- GLSL Shaders (Identical to previous request) ---
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 a_origin;    // The 3D origin of this particle's explosion
layout (location = 1) in vec3 a_direction; // The unique, random direction for this particle
layout (location = 2) in vec3 a_color;     // The unique, random color for this particle
layout (location = 3) in float a_startTime; // The global time this particle's explosion starts

// Global uniforms (from CPU)
uniform float u_time;
uniform mat4 u_projViewMatrix; // Combined Projection + View matrix

// Outputs to the Fragment Shader
out vec3 v_color;
out float v_alpha;

// Constants for the simulation
const float SPEED = 1.1;
const float MAX_LIFETIME = 5.0;     // Particle "explodes" and fades over 3 seconds
const float TOTAL_LIFETIME = 7.0;   // 3 sec life + 2 sec wait = 5 sec total loop
const float GRAVITY_ACCEL = 0.8;  // Acceleration due to gravity
const float GRAVITY_DELAY = 1.5;
const float CAMERA_Z = 5.0;       // Must match the "lookAt" z-position in the JS

// Pseudo-random hash function (converts a vec3 seed to a vec3 result [0,1])
// This is used to generate new origins and colors for each loop.
vec3 hash3(vec3 v) {
return fract(sin(vec3(
    dot(v, vec3(127.1, 311.7, 74.7)),
    dot(v, vec3(269.5, 183.3, 246.1)),
    dot(v, vec3(113.5, 271.9, 124.6))
    )) * 43758.5453123);
}

void main() {
    // 1. Calculate the total time elapsed for this particle
    float time_since_start = u_time - a_startTime;

    // 2. Calculate which loop iteration we are on
    float loop_count = floor(time_since_start / TOTAL_LIFETIME);

    // 3. Calculate the particle's "local" age within the current loop
    float localTime = mod(time_since_start, TOTAL_LIFETIME);

    // 4. Check if particle is "dead" (in its wait period)
    if (localTime > MAX_LIFETIME) {
        gl_Position = vec4(0.0, 0.0, 0.0, -1.0); // Hide it off-screen
        return;
    }

    // We use the loop_count as a "seed" modifier.
    // All 50 particles in this group will generate the *same* new origin and color.

    // 5. Create a new seed from the original origin + loop_count
    vec3 loop_seed_origin = a_origin + loop_count;
    vec3 hash_origin = hash3(loop_seed_origin);

    // Map the [0,1] hash to our world space
    vec3 new_origin = vec3(
        (hash_origin.x * 6.0) - 3.0,  // X: -3 to 3
        (hash_origin.y * 4.0) - 2.0,  // Y: -2 to 2
        (hash_origin.z * -3.0) - 2.0 // Z: -2 to -5
    );

    // 6. Create a new seed from the original color + loop_count
    vec3 loop_seed_color = a_color + loop_count;
    vec3 new_color = hash3(loop_seed_color); // New color is [0,1]

    // 7. Calculate radial distance (linear)
    float explosion_distance = localTime * SPEED;

    // 8. Calculate 3D spherical explosion position
    vec3 explosion_pos = a_direction * explosion_distance;

    // 9. Calculate downward "gravity" offset (accelerating)
    // float gravity_offset = 0.5 * GRAVITY_ACCEL * (localTime * localTime); // original

    float gravity_offset = 0.0;
    // Use the formula for distance under constant acceleration: d = 0.5 * a * t^2
    // This makes the particle fall faster and faster over its lifetime.
    if (localTime > GRAVITY_DELAY) { // only start after the first second
        float gravity_time = (localTime - GRAVITY_DELAY); // offset time
        gravity_offset = 0.5 * GRAVITY_ACCEL * (gravity_time * gravity_time);
    }

    // 10. Combine origin, explosion, and gravity
    //     Use generated new_origin instead of a_origin
    vec3 position = new_origin + explosion_pos - vec3(0.0, gravity_offset, 0.0);

    // 11. Set the final projected position
    gl_Position = u_projViewMatrix * vec4(position, 1.0);

    // 12. Calculate the alpha (transparency)
    v_alpha = 1.0 - (localTime / MAX_LIFETIME);

    // 13. Set the point size based on its Z-distance from the camera
    float camera_dist = CAMERA_Z - position.z;
    gl_PointSize = (50.0 * (localTime * 0.2)) / camera_dist;

    // 14. Pass the color to the fragment shader
    //     Use generated new_color instead of a_color
    v_color = new_color;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
precision highp float;

in vec3 v_color;
in float v_alpha;

out vec4 FragColor;

void main() {
    // Use gl_PointCoord to make the square point a circle
    float dist = distance(gl_PointCoord, vec2(0.5));
    float circle_alpha = 1.0 - smoothstep(0.45, 0.5, dist); 
    
    // Combine the circle's alpha with the particle's lifetime alpha
    float final_alpha = v_alpha * circle_alpha;

    // Discard fragments that are fully transparent
    if (final_alpha < 0.01) {
        discard;
    }

    FragColor = vec4(v_color, final_alpha);
}
)";

// --- Function Declarations ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int compileShader(GLenum type, const char* source);
unsigned int createProgram(const char* vsSource, const char* fsSource);
void setupMatrices(int width, int height, GLint uniformLoc);


// --- Main Application Code ---

int main() {
    // 1. Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // 2. Create Window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Firework Dance", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Load GLAD (OpenGL function pointers)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 4. Global GL Settings
    glEnable(GL_PROGRAM_POINT_SIZE); // Necessary for gl_PointSize in core profile
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    // 5. Compile and Link Shaders
    unsigned int shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);

    // 6. Get Uniform Locations
    GLint timeUniformLoc = glGetUniformLocation(shaderProgram, "u_time");
    GLint projViewMatrixUniformLoc = glGetUniformLocation(shaderProgram, "u_projViewMatrix");

    // 7. Generate Particle Data (VBO data)
    // The data stores the INITIAL seeds for the random generation in the vertex shader.
    std::vector<float> particleData(NUM_PARTICLES * ATTRIB_SIZE); 
    srand(time(0)); // Seed random number generator

    for (int i = 0; i < NUM_EXPLOSIONS; i++) {
        // Initial seeds for the entire group
        const float originX = (static_cast<float>(rand()) / RAND_MAX * 6.0f) - 3.0f;
        const float originY = (static_cast<float>(rand()) / RAND_MAX * 4.0f) - 2.0f;
        const float originZ = (static_cast<float>(rand()) / RAND_MAX * -3.0f) - 2.0f; // Z: -2 to -5
        const float startTime = static_cast<float>(rand()) / RAND_MAX * 5.0f;

        const float r_seed = static_cast<float>(rand()) / RAND_MAX;
        const float g_seed = static_cast<float>(rand()) / RAND_MAX;
        const float b_seed = static_cast<float>(rand()) / RAND_MAX;


        for (int j = 0; j < PARTICLES_PER_EXPLOSION; j++) {
            size_t offset = (i * PARTICLES_PER_EXPLOSION + j) * ATTRIB_SIZE;
            
            // Random direction
            float x_dir = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
            float y_dir = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
            float z_dir = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
            float len = std::sqrt(x_dir*x_dir + y_dir*y_dir + z_dir*z_dir);

            // a_origin (Location 0)
            particleData[offset + 0] = originX;
            particleData[offset + 1] = originY;
            particleData[offset + 2] = originZ;
            
            // a_direction (Location 1)
            particleData[offset + 3] = x_dir / len;
            particleData[offset + 4] = y_dir / len;
            particleData[offset + 5] = z_dir / len;
            
            // a_color (Location 2)
            particleData[offset + 6] = r_seed;
            particleData[offset + 7] = g_seed;
            particleData[offset + 8] = b_seed;

            // a_startTime (Location 3)
            particleData[offset + 9] = startTime;
        }
    }

    // 8. Setup VAO and VBO
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, particleData.size() * sizeof(float), particleData.data(), GL_STATIC_DRAW);

    const GLsizei stride = ATTRIB_SIZE * sizeof(float);

    // a_origin (Location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // a_direction (Location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // a_color (Location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // a_startTime (Location 3)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);

    // 9. Initial Matrix Setup
    setupMatrices(SCR_WIDTH, SCR_HEIGHT, projViewMatrixUniformLoc);

    // 10. Render Loop
    while (!glfwWindowShouldClose(window)) {
        // Input processing
        processInput(window);

        // Calculate time
        float currentTime = (float)glfwGetTime();

        // Rendering commands
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT); 

        // Activate shader and set uniforms
        glUseProgram(shaderProgram);
        glUniform1f(timeUniformLoc, currentTime);
        
        // Draw the particles
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
        glBindVertexArray(0);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 11. Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}


// --- Utility Implementations ---

/**
 * Recalculates the projection and view matrices when the window is resized.
 */
void setupMatrices(int width, int height, GLint uniformLoc) {
    mat4 projectionMatrix = create_identity();
    mat4 viewMatrix = create_identity();
    mat4 projViewMatrix = create_identity();
    
    float aspect = (float)width / (float)height;
    // FOV: 45 degrees in radians
    perspective(projectionMatrix, 45.0f * (M_PI / 180.0f), aspect, 0.1f, 100.0f);
    
    // Camera position (Z=5.0), looking at origin (0, 0, 0), Y-up
    lookAt(viewMatrix, {0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    
    multiply(projViewMatrix, projectionMatrix, viewMatrix);
    
    //glUseProgram(glGetCurrentProgram()); // Ensure the program is active
    glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, projViewMatrix.data());
}

/**
 * Handles window resizing.
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    GLint program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    GLint uniformLoc = glGetUniformLocation(program, "u_projViewMatrix");
    setupMatrices(width, height, uniformLoc);
}

/**
 * Handles basic input (ESC to close window).
 */
void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

/**
 * Compiles a single shader (Vertex or Fragment).
 */
unsigned int compileShader(GLenum type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

/**
 * Links the vertex and fragment shaders into a program.
 */
unsigned int createProgram(const char* vsSource, const char* fsSource) {
    unsigned int vertex, fragment;
    vertex = compileShader(GL_VERTEX_SHADER, vsSource);
    fragment = compileShader(GL_FRAGMENT_SHADER, fsSource);

    if (vertex == 0 || fragment == 0) return 0;

    unsigned int ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    int success;
    char infoLog[512];
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        glDeleteProgram(ID);
        return 0;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    return ID;
}