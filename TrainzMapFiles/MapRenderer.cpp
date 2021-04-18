#include "MapRenderer.h"

SDL_Window* window;

MapRenderer::MapRenderer(const GNDFile& _map)
	: map(_map)
{
    k_cameraLeft = k_cameraRight = k_cameraUp = k_cameraDown = false;
    k_moveLeft = k_moveRight = k_moveUp = k_moveDown = false;
    m_view = glm::translate(m_view, glm::vec3(0.0f, 0.0f, -10.0f));
    m_view = glm::rotate(m_view, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        printf("Unable to initialize SDL2!\n");
    }

    window = SDL_CreateWindow("Map tests", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 1024, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_GL_SetSwapInterval(1);

	InitGL();
    UpdateProjection(1024, 1024);

    for (GroundSection& section : map.sections)
        sections.push_back(new BaseboardSection(section));
}

MapRenderer::~MapRenderer()
{
    for (auto& section : sections)
        delete section;
    sections.clear();

    SDL_DestroyWindow(window);
}

void MapRenderer::Start()
{
	int running = 1;
	while (running != 0)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
            switch (event.type)
            {
            case SDL_MOUSEWHEEL:
            {
                float target = zoomFactor * (1.0 + (event.wheel.y * 0.25));
                if (target > 0.01f && target < 50.0f)
                    zoomFactor = target;

                break;
            }
            case SDL_KEYDOWN:
                ProcessKey(true, event.key.keysym.sym);
                break;
            case SDL_KEYUP:
                ProcessKey(false, event.key.keysym.sym);
                break;
            }
			if (event.type == SDL_WINDOWEVENT)
			{
				switch (event.window.event)
				{
				case SDL_WINDOWEVENT_CLOSE:
					running = 0;
					break;
				case SDL_WINDOWEVENT_RESIZED:
					//OpenTRS::WindowResizeEvent send;
					int w = event.window.data1, h = event.window.data2;
					UpdateProjection(w, h);
					break;
				}
			}
		}

        if (k_cameraLeft || k_cameraRight)
            cameravel.z = glm::mix(cameravel.z, glm::radians(2.5f) * (k_cameraRight ? -1.0f : 1.0f), 0.18);
        if (k_cameraUp || k_cameraDown)
            cameravel.x = glm::mix(cameravel.x, glm::radians(2.5f) * (k_cameraDown ? -1.0f : 1.0f), 0.18);

        camerarot.x += cameravel.x;
        camerarot.x = std::max(camerarot.x, glm::radians(1.0f));
        camerarot.x = std::min(camerarot.x, glm::radians(90.0f));

        camerarot.z += cameravel.z;

        cameravel = glm::mix(cameravel, glm::vec3(0.0f), 0.18);



		Render();
	}
}

void DrawGrid()
{
    glDisable(GL_TEXTURE_2D);
    constexpr int GridSize = 20;
    constexpr float GridScale = 1.0f;
    glPushMatrix();
    glLineWidth(0.1f);

    for (int i = 0; i <= GridSize; i++)
    {
        int gridabsolute = i - (GridSize / 2);

        glLineWidth(0.1f);
        if (gridabsolute == 0) glLineWidth(0.8f);

        glBegin(GL_LINES);
        glColor3f(0.4f, 0.4f, 0.4f);
        if (gridabsolute == 0) glColor3f(0.8f, 0.0f, 0.0f);
        glVertex3f(-(GridSize / 2) * GridScale, gridabsolute * GridScale, 0.0f);
        glVertex3f((GridSize / 2) * GridScale, gridabsolute * GridScale, 0.0f);
        if (gridabsolute == 0) glColor3f(0.0f, 0.8f, 0.0f);
        glVertex3f(gridabsolute * GridScale, -(GridSize / 2) * GridScale, 0.0f);
        glVertex3f(gridabsolute * GridScale, (GridSize / 2) * GridScale, 0.0f);
        glEnd();
    }

    glPopMatrix();
}

void MapRenderer::InitGL()
{
    ctx = SDL_GL_CreateContext(window);
    if (glewInit() != GLEW_OK)
        printf("GLEW failed to init.\n");

    //render data
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glEnable(GL_CULL_FACE);

    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    //fixed function shit
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    GLfloat LightAmbient[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
    GLfloat LightDiffuse[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat LightPosition[4] = { 0.0f, 0.0f, 15.0f, 1.0f };
    glm::vec3 LightPos(-25.0f, -45.0f, 50.0f);

    glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
    glEnable(GL_LIGHT1);
}

void MapRenderer::UpdateProjection(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	m_projection = glm::perspective(glm::radians(45.0f), (float)w / (float)h, 0.1f, 1000.0f);

	glMultMatrixf(glm::value_ptr(m_projection));
	glMatrixMode(GL_MODELVIEW);

    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
}

void MapRenderer::Render()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glm::mat4 offset = glm::mat4(1.0f);
    offset = glm::scale(offset, glm::vec3(zoomFactor));
    offset = glm::rotate(offset, camerarot.x, glm::vec3(1.0f, 0.0f, 0.0f));
    offset = glm::rotate(offset, camerarot.z, glm::vec3(0.0f, 0.0f, 1.0f));
    offset = glm::translate(offset, camerapos);

    //m_view = glm::rotate(m_view, glm::radians(-0.5f), glm::vec3(0.0f, 0.0f, 1.0f));

    m_localView = m_view * offset;

    glm::mat4 inverted = glm::inverse(m_localView);
    glm::vec3 forward = glm::normalize(glm::vec3(inverted[2]));
    forward.z = 0.0f;
    forward = glm::normalize(forward);
    glm::vec3 right = normalize(glm::vec3(inverted[0]));

    if (k_moveLeft || k_moveRight)
        camerapos += right * (k_moveLeft ? 1.0f : -1.0f) * 10.0f;

    if (k_moveUp || k_moveDown)
        camerapos += forward * (k_moveUp ? 1.0f : -1.0f) * 10.0f;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrixf(glm::value_ptr(m_localView));

    DrawGrid();

    //for (auto& section : sections)
    //    section->Render();

    for(int i = 0; i < 1; i++)
        sections[i]->Render();

    SDL_GL_SwapWindow(window);
}

void MapRenderer::ProcessKey(bool state, SDL_Keycode k)
{
    switch (k)
    {
    case SDLK_LEFT:
        k_cameraLeft = state;
        break;
    case SDLK_RIGHT:
        k_cameraRight = state;
        break;
    case SDLK_UP:
        k_cameraUp = state;
        break;
    case SDLK_DOWN:
        k_cameraDown = state;
        break;
    case SDLK_w:
        k_moveUp = state;
        break;
    case SDLK_a:
        k_moveLeft = state;
        break;
    case SDLK_s:
        k_moveDown = state;
        break;
    case SDLK_d:
        k_moveRight = state;
        break;
    }
}


float BaseboardSection::GetHeight(uint32_t x, uint32_t y)
{
    return section.data.vertexTable[x + y * 76].height;
}

BaseboardSection::BaseboardSection(GroundSection& _section)
    : section(_section), VAO(0), VBO(0), EBO(0)
{
    GPUVertices = new GPUVertex[76 * 76];
    memset(GPUVertices, 0, sizeof(GPUVertex) * (76 * 76));

    for (int y = 2; y < 75; y++)
    {
        for (int x = 2; x < 75; x++)
        {
            uint32_t idx = y * 76 + x;
            const auto& srcvert = section.data.vertexTable[idx];
            GPUVertex& vert = GPUVertices[idx];

            vert.pos = glm::vec3((float)(x - 2) * 10.0f, (float)(y - 2) * 10.0f, GetHeight(x, y));
            //vert.normal = glm::vec3(0.0f, 0.0f, 1.0f);
            vert.normal = glm::vec3(srcvert.normal[0], srcvert.normal[1], srcvert.normal[2]);
            //vert.normal = glm::normalize(vert.normal);

            //printf("vert normal %d %d %d\n", srcvert.normal[0], srcvert.normal[1], srcvert.normal[2]);
            vert.tex = glm::vec2(0.0f, 0.0f);

            if (x != 75 - 1 && y != 75 - 1)
            {
                uint32_t AIndex = x + y * 76;
                uint32_t BIndex = AIndex + 1;
                uint32_t CIndex = AIndex + 76;
                uint32_t DIndex = CIndex + 1;
                //MeshData.Triangles.AddTriangle(AIndex, CIndex, DIndex);
                //MeshData.Triangles.AddTriangle(AIndex, DIndex, BIndex);
                indices.push_back(DIndex);
                indices.push_back(CIndex);
                indices.push_back(AIndex);

                indices.push_back(BIndex);
                indices.push_back(DIndex);
                indices.push_back(AIndex);
            }
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    // load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GPUVertex) * (76 * 76), GPUVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size(), indices.data(), GL_STATIC_DRAW);

    // set the vertex attribute pointers
    int offset = 0;

    // vertex positions
    glEnableVertexAttribArray(offset);
    glVertexAttribPointer(offset, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)0);
    offset++;

    // vertex normals
    glEnableVertexAttribArray(offset);
    glVertexAttribPointer(offset, 3, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, normal));
    offset++;

    // vertex texture coords
    glEnableVertexAttribArray(offset);
    glVertexAttribPointer(offset, 2, GL_FLOAT, GL_FALSE, sizeof(GPUVertex), (void*)offsetof(GPUVertex, tex));
    offset++;

    glBindVertexArray(0);
}

BaseboardSection::~BaseboardSection()
{
    delete[] GPUVertices;
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void BaseboardSection::Render()
{
    //glBindVertexArray(VAO);
    //glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);

    //glBindVertexArray(0);
    //glShadeModel(GL_SMOOTH);
    //glEnable(GL_LIGHT0);
    ////glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glPushMatrix();
    glm::mat4 offset(1.0f);
    offset = glm::translate(offset, glm::vec3(section.originH * 720.0f, section.originV * 720.0f, 0.0f));
    glMultMatrixf(glm::value_ptr(offset));

    glEnable(GL_LIGHTING);

    glBegin(GL_TRIANGLES);
    for (uint32_t i = 0; i < indices.size(); i += 3)
    {
        auto& A = GPUVertices[indices[i + 0]];
        auto& B = GPUVertices[indices[i + 1]];
        auto& C = GPUVertices[indices[i + 2]];

        glVertex3fv(glm::value_ptr(A.pos));
        glNormal3fv(glm::value_ptr(A.normal));

        glVertex3fv(glm::value_ptr(B.pos));
        glNormal3fv(glm::value_ptr(B.normal));

        glVertex3fv(glm::value_ptr(C.pos));
        glNormal3fv(glm::value_ptr(C.normal));
    }
    glEnd();

    glDisable(GL_LIGHTING);

    glBegin(GL_LINES);
    glColor3f(0.0f, 0.0f, 1.0f);
    for (uint32_t i = 0; i < indices.size(); i++)
    {
        auto& A = GPUVertices[indices[i + 0]];

        glVertex3fv(glm::value_ptr(A.pos));
        glVertex3fv(glm::value_ptr(A.pos + (A.normal * 8.0f)));
    }
    glEnd();

    glPopMatrix();
}
