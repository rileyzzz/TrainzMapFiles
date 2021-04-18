#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "GNDFile.h"

struct GPUVertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 tex;
};

class BaseboardSection
{
	GroundSection& section;
	GLuint VAO, VBO, EBO;
	GPUVertex* GPUVertices = nullptr;
	std::vector<uint16_t> indices;

	float GetHeight(uint32_t x, uint32_t y);
public:
	BaseboardSection(GroundSection& _section);
	~BaseboardSection();

	void Render();
};

class MapRenderer
{
private:
	GNDFile map;

	bool k_cameraLeft, k_cameraRight, k_cameraUp, k_cameraDown;
	bool k_moveLeft, k_moveRight, k_moveUp, k_moveDown;

	glm::vec3 camerapos = glm::vec3(0.0f);
	glm::vec3 camerarot = glm::vec3(0.0f);
	glm::vec3 cameravel = glm::vec3(0.0f);

	glm::mat4 m_view = glm::mat4(1.0f);
	glm::mat4 m_localView = glm::mat4(1.0f);
	glm::mat4 m_projection = glm::mat4(1.0f);
	float zoomFactor = 1.0f;

	SDL_GLContext ctx;

	void InitGL();
	void UpdateProjection(int w, int h);
	void Render();
	void ProcessKey(bool state, SDL_Keycode k);

	std::vector<BaseboardSection*> sections;

public:
	MapRenderer(const GNDFile& _map);
	~MapRenderer();

	void Start();
};

