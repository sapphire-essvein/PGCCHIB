#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const GLint WIDTH = 800;
const GLint HEIGHT = 600;

bool load_texture(const char* file_name, GLuint* tex) {
    int x, y, n;
    int force_channels = 4;
    unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);
    if (!image_data) {
        std::cerr << "ERROR: could not load texture: " << file_name << std::endl;
        return false;
    }

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image_data);
    return true;
}

struct Quad {
    glm::vec2 position;
    glm::vec2 dimensions;
};

class Sprite {
public:
    GLuint vao;
    GLuint texture;
    GLuint shader;
    Quad quad;
    glm::vec2 uv_min, uv_max;

    Sprite(GLuint vao, GLuint texture, GLuint shader, Quad quad,
           glm::vec2 uv_min = {0,0}, glm::vec2 uv_max = {1,1}) :
           vao(vao), texture(texture), shader(shader), quad(quad),
           uv_min(uv_min), uv_max(uv_max) {}

    void draw(const glm::mat4& proj, const glm::vec2& offsetUV = glm::vec2(0.0f)) {
        glUseProgram(shader);

        glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(quad.position.x, quad.position.y, 0.0f));
        model = glm::scale(model, glm::vec3(quad.dimensions.x, quad.dimensions.y, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glUniform2fv(glGetUniformLocation(shader, "offset"), 1, glm::value_ptr(offsetUV));

        glUniform2fv(glGetUniformLocation(shader, "uv_min"), 1, glm::value_ptr(uv_min));
        glUniform2fv(glGetUniformLocation(shader, "uv_max"), 1, glm::value_ptr(uv_max));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shader, "texture1"), 0);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
};

struct Layer {
    Sprite sprite;
    glm::vec2 offset;
    float parallax_factor;
};

int main() {
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vivencial M4", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD\n";
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const char* vertex_shader_src =
        "#version 410 core\n"
        "layout(location = 0) in vec3 aPos;\n"
        "layout(location = 1) in vec2 aTexCoord;\n"
        "uniform mat4 proj;\n"
        "uniform mat4 model;\n"
        "uniform vec2 uv_min;\n"
        "uniform vec2 uv_max;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "    TexCoord = mix(uv_min, uv_max, aTexCoord);\n"
        "    gl_Position = proj * model * vec4(aPos, 1.0);\n"
        "}\n";

    const char* fragment_shader_src =
        "#version 410 core\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "uniform vec2 offset;\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
            "vec2 uv = fract(TexCoord + offset);"
        "    FragColor = texture(texture1, TexCoord);\n"
        "}\n";

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
    glCompileShader(fragment_shader);

    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    float vertices[] = {
        -0.5f, -0.5f, 0.f,  0.f, 1.f,
         0.5f, -0.5f, 0.f,  1.f, 1.f,
         0.5f,  0.5f, 0.f,  1.f, 0.f,

        -0.5f, -0.5f, 0.f,  0.f, 1.f,
         0.5f,  0.5f, 0.f,  1.f, 0.f,
        -0.5f,  0.5f, 0.f,  0.f, 0.f
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    float parallax_factors[10] = {0.05f, 0.1f, 0.15f, 0.2f, 0.3f, 0.4f, 0.5f, 0.65f, 0.8f, 1.0f};

    std::vector<Layer> layers;

    for (int i = 0; i < 8; ++i) {
        GLuint tex;
        std::string path = "../src/Modulo4/nature_1/" + std::to_string(i + 1) + ".png";
        if (!load_texture(path.c_str(), &tex)) {
            std::cerr << "Erro ao carregar textura: " << path << std::endl;
            return -1;
        }
        Quad q = {{WIDTH / 2.f, HEIGHT / 2.f}, {WIDTH, HEIGHT}};
        Sprite s(vao, tex, shader_program, q);
        layers.push_back({s, {0.f, 0.f}, parallax_factors[i]});
    }

    GLuint texChar;
    if (!load_texture("../src/Modulo4/Karasu_tengu/Jump.png", &texChar)) {
        std::cerr << "Erro ao carregar textura do personagem!\n";
        return -1;
    }
    Quad charQuad = {{WIDTH / 2.f, HEIGHT / 2.f}, {100.f, 100.f}};
    int cols = 15, rows = 1, col = 14, row = 0;
    glm::vec2 uv_min = { col / float(cols), row / float(rows) };
    glm::vec2 uv_max = { (col + 1) / float(cols), (row + 1) / float(rows) };
    Sprite character(vao, texChar, shader_program, charQuad, uv_min, uv_max);

    glm::vec2 playerPos = {WIDTH / 2.f, HEIGHT / 2.f};

    glm::mat4 proj = glm::ortho(0.0f, float(WIDTH), 0.0f, float(HEIGHT), -1.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glm::vec2 movement(0.f, 0.f);
        const float speed = 1.5f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement.y += speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement.y -= speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement.x -= speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement.x += speed;

        playerPos += movement;

        for (auto& layer : layers) {
        glm::vec2 offsetUV = layer.offset / glm::vec2(WIDTH, HEIGHT);
        layer.sprite.draw(proj, offsetUV);
        }

        glClearColor(0.1f, 0.2f, 0.3f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto& layer : layers) {
            layer.sprite.draw(proj, layer.offset);
        }

        character.quad.position = playerPos;
        character.draw(proj, {0.f, 0.f});

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}