#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <iostream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

enum class TileType
{
	Walkable,
	Deadly,
	Blocked,
	Unknown,
	Coin,
};

struct Sprite
{
	GLuint VAO;
	GLuint texID;
	vec3 position;
	vec3 dimensions;
	float ds, dt;
	int iAnimation, iFrame;
	int nAnimations, nFrames;
};

struct Tile
{
	GLuint VAO;
	GLuint texID;
	int iTile;
	vec3 position;
	vec3 dimensions;
	float ds, dt;
	TileType type = TileType::Unknown;
};

struct MapConfig
{
	string tilesetFile;
	int nTiles;
	int tileW, tileH;
	int rows, cols;
	vector<int> matrix;
	int playerInicialRow, playerInicialCol;
};

struct Coin
{
	int i, j;		// Posição no tile
	bool collected; // Verifica se foi coletada ou não
	GLuint texID;
};

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

bool loadMapConfig(const string &path, MapConfig &cfg, string &err);
bool loadTileProps(const string &filename, vector<TileType> &tileTypes);
int setupShader();
int setupSprite(int nAnimations, int nFrames, float &ds, float &dt);
int setupTile(int nTiles, float &ds, float &dt);
int loadTexture(string filePath, int &width, int &height);
void desenharMapa(GLuint shaderID, float x0, float y0);
void inicializarMoedas(GLuint texCoin);
void desenharMoedas(GLuint shaderID, float x0, float y0, const vector<Coin> &moedas);

MapConfig cfg;
vector<Tile> tileset;
int player_i = 0, player_j = 0;
GLuint WIDTH = 800, HEIGHT = 600;

vector<Coin> moedas;
int pontuacao = 0;
int totalMoedas = 0;
int vida = 3;

const GLchar *vertexShaderSource = R"(
 #version 400
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec2 texc;
 out vec2 tex_coord;
 uniform mat4 model;
 uniform mat4 projection;
 void main()
 {
	tex_coord = vec2(texc.s, 1.0 - texc.t);
	gl_Position = projection * model * vec4(position, 1.0);
 }
 )";

const GLchar *fragmentShaderSource = R"(
 #version 400
 in vec2 tex_coord;
 out vec4 color;
 uniform sampler2D tex_buff;
 uniform vec2 offsetTex;

 void main()
 {
	 color = texture(tex_buff,tex_coord + offsetTex);
 }
 )";

int main()
{
	srand(glfwGetTime());

	string err;
	if (!loadMapConfig("../src/Modulo6/config/tileMap.txt", cfg, err))
	{
		cerr << "Erro: " << err << '\n';
		return -1;
	}

	WIDTH = ((cfg.rows + cfg.cols) / 2) * cfg.tileW + 20;
	HEIGHT = ((cfg.rows + cfg.cols) / 2) * cfg.tileH + 20;
	float x0 = (cfg.rows - 1) * cfg.tileW * 0.5f;
	float y0 = 10.0f;

	player_i = cfg.playerInicialRow;
	player_j = cfg.playerInicialCol;

	vector<TileType> tileTypes(cfg.nTiles, TileType::Unknown);
	if (!loadTileProps("../src/Modulo6/config/tileProps.txt", tileTypes))
	{
		cerr << "Erro ao carregar propriedades dos tiles!" << endl;
		return -1;
	}

	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 8);

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Vivencial 3", nullptr, nullptr);

	if (!window)
	{
		std::cerr << "Falha ao criar a janela GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << "Falha ao inicializar GLAD" << std::endl;
		return -1;
	}

	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	GLuint shaderID = setupShader();

	int imgWidth, imgHeight;

	GLuint texID = loadTexture("../assets/tilesets/tilesetIso.png", imgWidth, imgHeight);
	GLuint texCoin = loadTexture("../assets/sprites/coin.png", imgWidth, imgHeight);

	for (int i = 0; i < cfg.nTiles; ++i)
	{
		Tile tile;
		tile.dimensions = vec3(cfg.tileW, cfg.tileH, 1.0);
		tile.iTile = i;
		tile.type = tileTypes[i];
		tile.texID = texID;
		tile.VAO = setupTile(cfg.nTiles, tile.ds, tile.dt);
		tileset.push_back(tile);
	}

	glUseProgram(shaderID);

	double prev_s = glfwGetTime();
	double title_countdown_s = 0.1;

	float colorValue = 0.0;

	glActiveTexture(GL_TEXTURE0);

	glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

	mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Sprite jogador;
	jogador.dimensions = vec3(cfg.tileW, cfg.tileW, 1.0); // altura da sprite, ajustável
	jogador.texID = loadTexture("../assets/sprites/Jump.png", imgWidth, imgHeight);
	jogador.VAO = setupSprite(1, 15, jogador.ds, jogador.dt); // 1 linha, 15 sprites
	jogador.nAnimations = 1;
	jogador.nFrames = 15;
	jogador.iAnimation = 0;
	jogador.iFrame = 6; // começando do sprite 6 ao 12

	double tempo_animacao = 0;
	int frame_atual = 6;

	double lastTime = glfwGetTime();
	double deltaT = 0.0;
	inicializarMoedas(texCoin);

	while (!glfwWindowShouldClose(window))
	{
		{
			double curr_s = glfwGetTime();
			double elapsed_s = curr_s - prev_s;
			prev_s = curr_s;

			title_countdown_s -= elapsed_s;
			if (title_countdown_s <= 0.0 && elapsed_s > 0.0)
			{
				double fps = 1.0 / elapsed_s;

				char tmp[256];
				// Adicione a pontuação ao formato da string
				sprintf(tmp, "PG - Grau B - Amanda Vidal, Lucas Essvein e Marcos Krol\tVida: %d\tScore: %d\t", vida, pontuacao);
				glfwSetWindowTitle(window, tmp);
				title_countdown_s = 0.1;
			}
		}

		glfwPollEvents();

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		double currTime = glfwGetTime();
		deltaT = currTime - lastTime;
		lastTime = currTime;

		tempo_animacao += deltaT;
		if (tempo_animacao > 0.1) // troca de frame a cada 0.1s
		{
			frame_atual++;
			if (frame_atual > 12)
				frame_atual = 6; // loop do 6 ao 12
			jogador.iFrame = frame_atual;
			tempo_animacao = 0;
		}

		desenharMapa(shaderID, x0, y0);
		desenharMoedas(shaderID, x0, y0, moedas);

		Tile curr_tile = tileset[cfg.matrix[player_i * cfg.cols + player_j]];

		float x = x0 + (player_j - player_i) * curr_tile.dimensions.x / 2.0f;
		float y = y0 + (player_j + player_i) * curr_tile.dimensions.y / 2.0f;

		mat4 model = mat4(1.0);
		model = translate(model, vec3(x + curr_tile.dimensions.x / 2.0, y + curr_tile.dimensions.y / 2.0 - jogador.dimensions.y / 2.0, 0));
		model = scale(model, jogador.dimensions);

		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

		vec2 offsetTex;
		offsetTex.s = jogador.iFrame * jogador.ds;
		offsetTex.t = 1.0 - jogador.dt;
		glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);

		glBindVertexArray(jogador.VAO);
		glBindTexture(GL_TEXTURE_2D, jogador.texID);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Verifica se a moeda foi coletada e adiciona pontuação
		for (auto &moeda : moedas)
		{
			if (!moeda.collected && moeda.i == player_i && moeda.j == player_j)
			{
				moeda.collected = true;
				pontuacao++;
				std::cout << "Moeda coletada! Pontuação: " << pontuacao << std::endl;
			}
		}

		// Verifica a pontuação com o total de moedas coledas e emite mensagem de ganhador
		if (pontuacao == totalMoedas && totalMoedas > 0)
		{
			std::cout << "Parabéns! Você conseguiu coletar todas as moedas e ganhou o jogo." << std::endl;
			std::cout << "Moedas coletadas: " << totalMoedas << std::endl;
			glfwSetWindowShouldClose(window, GL_TRUE);
		}

		

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

inline int tileAt(int i, int j) { return cfg.matrix[i * cfg.cols + j]; }

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (action == GLFW_PRESS)
	{
		int di = 0, dj = 0;

		switch (key)
		{
		case GLFW_KEY_Z:
			di = +1;
			dj = -1;
			break; // SW
		case GLFW_KEY_X:
			di = +1;
			dj = 0;
			break; // S
		case GLFW_KEY_C:
			di = +1;
			dj = +1;
			break; // SE
		case GLFW_KEY_A:
			di = 0;
			dj = -1;
			break; // W
		case GLFW_KEY_D:
			di = 0;
			dj = +1;
			break; // E
		case GLFW_KEY_Q:
			di = -1;
			dj = -1;
			break; // NW
		case GLFW_KEY_W:
			di = -1;
			dj = 0;
			break; // N
		case GLFW_KEY_E:
			di = -1;
			dj = +1;
			break; // NE
		}

		int new_i = player_i + di;
		int new_j = player_j + dj;

		if (new_i >= 0 && new_j >= 0 && new_i < cfg.rows && new_j < cfg.cols)
		{
			int tileID = tileAt(new_i, new_j);
			TileType tileType = tileset[tileID].type;

			switch (tileType)
			{
			case TileType::Walkable:
				player_i = new_i;
				player_j = new_j;
				break;
			case TileType::Deadly:
				// Como visitou um tile perigoso, ele perde uma vida.
				cout << "Você pisou em um tile perigoso! Perdeu 1 vida." << endl;
				vida--;
				cout << "Vidas: " << vida << endl;
				break;
			case TileType::Blocked:
				cout << "Tile bloqueado! Não é possível se mover para cá." << endl;
				break;
			case TileType::Unknown:
			default:
				cout << "Tile desconhecido! Não é possível se mover para cá." << endl;
				break;
			}
		}

		// Se a vida for menor ou igual a 0, o jogo encerra.
		if (vida <= 0)
		{
			cout << "Game over! Sua vida chegou a zero." << endl;
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}
}

int setupShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
				  << infoLog << std::endl;
	}
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
				  << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

int setupSprite(int nAnimations, int nFrames, float &ds, float &dt)
{

	ds = 1.0 / (float)nFrames;
	dt = 1.0 / (float)nAnimations;

	GLfloat vertices[] = {
		-0.5, 0.5, 0.0, 0.0, 0.0,
		-0.5, -0.5, 0.0, 0.0, dt,
		0.5, 0.5, 0.0, ds, 0.0,
		0.5, -0.5, 0.0, ds, dt};

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return VAO;
}

int setupTile(int nTiles, float &ds, float &dt)
{

	ds = 1.0 / (float)nTiles;
	dt = 1.0;

	float th = 1.0, tw = 1.0;

	GLfloat vertices[] = {
		0.0, th / 2.0f, 0.0, 0.0, dt / 2.0f,
		tw / 2.0f, th, 0.0, ds / 2.0f, dt,
		tw / 2.0f, 0.0, 0.0, ds / 2.0f, 0.0,
		tw, th / 2.0f, 0.0, ds, dt / 2.0f};

	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return VAO;
}

int loadTexture(string filePath, int &width, int &height)
{
	GLuint texID;

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

void desenharMapa(GLuint shaderID, float x0, float y0)
{
	for (int i = 0; i < cfg.rows; i++)
	{
		for (int j = 0; j < cfg.cols; j++)
		{
			mat4 model = mat4(1);

			Tile curr_tile = tileset[tileAt(i, j)];
			if (i == player_i && j == player_j)
				curr_tile = tileset[6];

			float x = x0 + (j - i) * curr_tile.dimensions.x / 2.0f;
			float y = y0 + (j + i) * curr_tile.dimensions.y / 2.0f;

			model = translate(model, vec3(x, y, 0.0));
			model = scale(model, curr_tile.dimensions);
			glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

			vec2 offsetTex;

			offsetTex.s = curr_tile.iTile * curr_tile.ds;
			offsetTex.t = 0.0;
			glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), offsetTex.s, offsetTex.t);

			glBindVertexArray(curr_tile.VAO);
			glBindTexture(GL_TEXTURE_2D, curr_tile.texID);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}
}

// Função para inicializar as moedas no jogo
void inicializarMoedas(GLuint texCoin)
{
	moedas.clear();

	const int MAX_MOEDAS = 15;
	int moedasCriadas = 0;

	float chanceMoedaPorTile = 0.1f;

	for (int i = 0; i < cfg.rows; ++i)
	{
		for (int j = 0; j < cfg.cols; ++j)
		{

			if (moedasCriadas >= MAX_MOEDAS)
			{
				break;
			}

			int tileID = tileAt(i, j);

			if (tileset[tileID].type == TileType::Walkable &&
				!(i == cfg.playerInicialRow && j == cfg.playerInicialCol))
			{
				float randomValue = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

				if (randomValue < chanceMoedaPorTile)
				{
					moedas.push_back({i, j, false, texCoin});
					moedasCriadas++;
				}
			}
		}
		if (moedasCriadas >= MAX_MOEDAS)
		{
			break;
		}
	}
	totalMoedas = moedas.size();
}

void desenharMoedas(GLuint shaderID, float x0, float y0, const vector<Coin> &moedas)
{
	// Define tamanho e outras propriedades da moeda
	vec3 dimensoesMoeda = vec3(cfg.tileW * 0.4f, cfg.tileH * 0.9f, 1.0f);
	float ds = 1.0;
	float dt = 1.0;

	static GLuint coinVAO = setupSprite(1, 1, ds, dt); // Uma sprite estática (1x1)

	glBindVertexArray(coinVAO);

	for (const auto &moeda : moedas)
	{
		if (moeda.collected)
			continue;

		Tile curr_tile = tileset[tileAt(moeda.i, moeda.j)];

		float x = x0 + (moeda.j - moeda.i) * curr_tile.dimensions.x / 2.0f;
		float y = y0 + (moeda.j + moeda.i) * curr_tile.dimensions.y / 2.0f;

		mat4 model = mat4(1.0);
		model = translate(model, vec3(x + curr_tile.dimensions.x / 2.0f, y + curr_tile.dimensions.y / 2.0f - dimensoesMoeda.y / 2.0f, 0.1f));
		model = scale(model, dimensoesMoeda);

		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

		// Usando o canto superior esquerdo da textura da moeda
		glUniform2f(glGetUniformLocation(shaderID, "offsetTex"), 0.0f, 1.0f - dt);

		// IMPORTANTE: Vincula a textura da moeda!
		glBindTexture(GL_TEXTURE_2D, moeda.texID);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glBindVertexArray(0);
}

bool loadMapConfig(const string &path, MapConfig &cfg, string &err)
{
	ifstream in(path);
	if (!in)
	{
		err = "Não foi possível abrir o arquivo de configuração: " + path;
		return false;
	}

	string line, section;
	unordered_set<string> seen;
	vector<int> matrixVals;

	auto need = [&](const string &s)
	{
		if (!seen.count(s))
		{
			err = "Seção [" + s + "] ausente";
			return false;
		}
		return true;
	};

	while (getline(in, line))
	{
		// remove espaços laterais
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (line.empty())
			continue;
		if (line.front() == '[' && line.back() == ']')
		{ // nova seção
			section = line.substr(1, line.size() - 2);
			seen.insert(section);
			continue;
		}

		istringstream iss(line);
		if (section == "file")
			getline(iss, cfg.tilesetFile);
		else if (section == "nTiles")
			iss >> cfg.nTiles;
		else if (section == "width")
			iss >> cfg.tileW;
		else if (section == "height")
			iss >> cfg.tileH;
		else if (section == "rows")
			iss >> cfg.rows;
		else if (section == "columns")
			iss >> cfg.cols;
		else if (section == "rowInicialPosition")
			iss >> cfg.playerInicialRow;
		else if (section == "columnInicialPosition")
			iss >> cfg.playerInicialCol;
		else if (section == "matrix")
		{
			int v;
			while (iss >> v)
				matrixVals.push_back(v);
		}
		else
		{
			err = "Seção [" + section + "] desconhecida";
			return false;
		}
	}

	if (!need("file") || !need("nTiles") || !need("width") || !need("height") ||
		!need("rows") || !need("columns") || !need("matrix") ||
		!need("rowInicialPosition") || !need("columnInicialPosition"))
		return false;

	if (cfg.rows <= 0 || cfg.cols <= 0)
	{
		err = "Rows/Columns inválidos";
		return false;
	}

	if ((int)matrixVals.size() != cfg.rows * cfg.cols)
	{
		err = "Tamanho da matriz (" + to_string(matrixVals.size()) +
			  ") difere de rows*columns (" +
			  to_string(cfg.rows * cfg.cols) + ")";
		return false;
	}

	cfg.matrix.swap(matrixVals);

	if (cfg.playerInicialRow < 0 || cfg.playerInicialRow >= cfg.rows ||
		cfg.playerInicialCol < 0 || cfg.playerInicialCol >= cfg.cols)
	{
		err = "Posição inicial fora da matriz";
		return false;
	}

	return true;
}

bool loadTileProps(const string &filename, vector<TileType> &tileTypes)
{
	ifstream file(filename);
	if (!file.is_open())
	{
		cerr << "Erro ao abrir o arquivo de propriedades dos tiles: " << filename << endl;
		return false;
	}
	string line, currentSection;
	unordered_map<std::string, TileType> sectionMap = {
		{"walkable", TileType::Walkable},
		{"deadly", TileType::Deadly},
		{"blocked", TileType::Blocked},
		{"coin", TileType::Coin}};

	while (getline(file, line))
	{
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (line.empty() || line[0] == '#')
			continue;
		if (line.front() == '[' && line.back() == ']')
		{
			currentSection = line.substr(1, line.size() - 2);
			for (auto &c : currentSection)
				c = tolower(c);
			continue;
		}
		if (sectionMap.count(currentSection))
		{
			TileType tipo = sectionMap[currentSection];
			istringstream iss(line);
			int tileID;
			while (iss >> tileID)
			{
				if (tileID >= 0 && tileID < (int)tileTypes.size())
					tileTypes[tileID] = tipo;
				else
					cerr << "ID de tile inválido no arquivo tileProps.txt: " << tileID << endl;
			}
		}
	}

	return true;
}