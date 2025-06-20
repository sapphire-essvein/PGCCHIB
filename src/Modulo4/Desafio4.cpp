#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const GLint WIDTH = 800, HEIGHT = 600;

struct Quad {
    glm::vec2 position = {0.0f, 0.0f};
    glm::vec2 dimensions = {1.0f, 1.0f};
};

struct Sprite {
    GLuint vao;
    GLuint texture;
    GLuint shader;
    Quad quad;
    glm::vec2 uv_min = {0.0f, 0.0f};
    glm::vec2 uv_max = {1.0f, 1.0f};

    Sprite(GLuint vao, GLuint texture, GLuint shader, const Quad& quad)
        : vao(vao), texture(texture), shader(shader), quad(quad) {}

    Sprite(GLuint vao, GLuint texture, GLuint shader, Quad quad, glm::vec2 uv_min, glm::vec2 uv_max)
        : vao(vao), texture(texture), shader(shader), quad(quad), uv_min(uv_min), uv_max(uv_max) {}

    void draw(glm::mat4 proj) {
        glUseProgram(shader);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(quad.position, 0.0f));
        model = glm::scale(model, glm::vec3(quad.dimensions, 1.0f));

        glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(glGetUniformLocation(shader, "matrix"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform2fv(glGetUniformLocation(shader, "uv_min"), 1, glm::value_ptr(uv_min));
        glUniform2fv(glGetUniformLocation(shader, "uv_max"), 1, glm::value_ptr(uv_max));

        glBindVertexArray(vao);
        glBindTexture(GL_TEXTURE_2D, texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
};

bool load_texture(const char* file_name, GLuint* tex) {
    int x, y, n;
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, 4);
    if (!image_data) {
        std::cerr << "Erro ao carregar imagem: " << file_name << std::endl;
        return false;
    }

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image_data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return true;
}

GLuint createQuadVAO() {
    GLfloat vertices[] = {
        // x, y, z      // cor       // tex coord
        -0.5f, -0.5f, 0.0f, 1, 1, 1, 0.0f, 1.0f,
         0.5f, -0.5f, 0.0f, 1, 1, 1, 1.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1, 1, 1, 1.0f, 0.0f,

        -0.5f, -0.5f, 0.0f, 1, 1, 1, 0.0f, 1.0f,
         0.5f,  0.5f, 0.0f, 1, 1, 1, 1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 1, 1, 1, 0.0f, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // posições
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // cores
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // textura
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return VAO;
}

const char* vertex_shader = R"(
    #version 410
    layout (location = 0) in vec3 vPosition;
    layout (location = 1) in vec3 vColor;
    layout (location = 2) in vec2 vTexture;

    uniform mat4 proj;
    uniform mat4 matrix;
    uniform vec2 uv_min;
    uniform vec2 uv_max;

    out vec2 text_map;
    out vec3 color;

    void main() {
        color = vColor;
        // ajusta UV: (0,0)->uv_min, (1,1)->uv_max
        text_map = mix(uv_min, uv_max, vTexture); 
        gl_Position = proj * matrix * vec4(vPosition, 1.0);
    }
)";

const char* fragment_shader = R"(
    #version 410
    in vec2 text_map;
    uniform sampler2D basic_texture;
    out vec4 frag_color;
    void main() {
        frag_color = texture(basic_texture, text_map);
    }
)";

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio M4", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erro ao criar janela" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Erro ao carregar GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, nullptr);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, nullptr);
    glCompileShader(fs);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs);
    glAttachShader(shader, fs);
    glLinkProgram(shader);

    GLuint vao = createQuadVAO();

    GLuint tex1, tex2, tex3, tex4, tex5;

    if (!load_texture("../src/Modulo4/nature_1/orig.png", &tex1)) return -1;
    if (!load_texture("../src/Modulo4/Knight_1/Protect.png", &tex2)) return -1;
    if (!load_texture("../src/Modulo4/Kunoichi/Attack_2.png", &tex3)) return -1;
    if (!load_texture("../src/Modulo4/Gorgon_1/Attack_1.png", &tex4)) return -1;
    if (!load_texture("../src/Modulo4/Karasu_tengu/Jump.png", &tex5)) return -1;

    glm::mat4 proj = glm::ortho(0.0f, float(WIDTH), 0.0f, float(HEIGHT), -1.0f, 1.0f);

    std::vector<Sprite> sprites;
    sprites.emplace_back(vao, tex1, shader, Quad{{WIDTH / 2, HEIGHT / 2}, {WIDTH, HEIGHT}});
    sprites.emplace_back(vao, tex2, shader, Quad{{200, 150}, {250, 250}});
    int cols = 8, rows = 1;
    int col = 1, row = 0;
    glm::vec2 uv_min = { col / float(cols), row / float(rows) };
    glm::vec2 uv_max = { (col + 1) / float(cols), (row + 1) / float(rows) };
    sprites.emplace_back(vao, tex3, shader, Quad{{400, 150}, {250, 250}}, uv_min, uv_max);
    cols = 16, rows = 1;
    col = 15, row = 0;
    uv_min = { col / float(cols), row / float(rows) };
    uv_max = { (col + 1) / float(cols), (row + 1) / float(rows) };
    sprites.emplace_back(vao, tex4, shader, Quad{{600, 150}, {250, 250}}, uv_min, uv_max);
    cols = 15, rows = 1;
    col = 14, row = 0;
    uv_min = { col / float(cols), row / float(rows) };
    uv_max = { (col + 1) / float(cols), (row + 1) / float(rows) };
    sprites.emplace_back(vao, tex5, shader, Quad{{300, 400}, {250, 250}}, uv_min, uv_max);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto& sprite : sprites) {
            sprite.draw(proj);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
