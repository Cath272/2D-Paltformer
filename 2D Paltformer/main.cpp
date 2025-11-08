#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "loadShaders.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "SOIL.h"
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H  


// -------------------- OpenGL Identifiers --------------------
GLuint PlayerVaoId, PlayerVboId, PlayerEboId;
GLuint PlatformVaoId, PlatformVboId, PlatformEboId;
GLuint ProgramId, movePlayerMatrixLocation;
GLuint playerTexture, platformTexture;

// -------------------- Window --------------------
#define FRAME_INTERVAL 16
GLfloat winWidth = 1920, winHeight = 1080;

// -------------------- Transform matrices --------------------
glm::mat4 movePlayerMatrix, resizeMatrix;
glm::mat4 matrTransl, matrRot;

float angle = 0;
float tx = 0, ty = 0;
float xMin = -800.f, xMax = 800.f, yMin = -600.f, yMax = 600.f;

float playerYVelocity = 1.0f;
float gravity = -0.15f;
float lastTime = 0.0f;
bool onGround = false;

GLfloat anglePlayer = 0.0;


bool movingLeft = false;
bool movingRight = false;
float moveSpeed = 5.0f;

// ==== CHANGED ====
// Added a bool to mark if a platform should use a single stretched texture
struct Platform {
    float xMin, xMax, yMin, yMax;
    bool singleTexture; // <--- new flag
};

// ==== CHANGED ====
// Added `true` for normal platforms, `false` for ground
std::vector<Platform> platforms = {
    {200.0f, 400.0f, 200.0f, 300.0f, true},
    {400.0f, 600.0f, 100.0f, 200.0f, true},
    {1000.0f, 1200.0f, 300.0f, 400.0f, true},
    {0.0f, 200.0f, -200.0f, -100.0f, true},
    {-200.0f, 0.0f, 0.0f, 100.0f, true},
    {-4000.0f, 4000.0f, -400.0f, -300.0f, false} 
};

// -------------------- Timer --------------------
void TimerFunction(int value)
{
    glutPostRedisplay();
    glutTimerFunc(FRAME_INTERVAL, TimerFunction, 0);
}

// -------------------- Input --------------------
void ProcessNormalKeys(unsigned char key, int x, int y)
{
    if (key == ' ' && onGround)
    {
        playerYVelocity = 10.0f;
        onGround = false;
    }


}

void ProcessSpecialKeys(int key, int xx, int yy)
{
    if (key == GLUT_KEY_LEFT)  movingLeft = true;
    if (key == GLUT_KEY_RIGHT) movingRight = true;
}

void ProcessSpecialUpKeys(int key, int xx, int yy)
{
    if (key == GLUT_KEY_LEFT)  movingLeft = false;
    if (key == GLUT_KEY_RIGHT) movingRight = false;
}

// -------------------- Load texture --------------------
void LoadTexture(const char* texturePath, GLuint& tex)
{
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height;
    unsigned char* image = SOIL_load_image(texturePath, &width, &height, 0, SOIL_LOAD_RGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// -------------------- Shaders --------------------
void CreateShaders(void)
{
    ProgramId = LoadShaders("04_04_Shader.vert", "04_04_Shader.frag");
    glUseProgram(ProgramId);
    movePlayerMatrixLocation = glGetUniformLocation(ProgramId, "movePlayerMatrix");
}

// -------------------- Player VBO --------------------
void CreatePlayerVBO(void)
{
    GLfloat Vertices[] = {
        -75.0f, -75.0f, 0.0f, 1.0f,   1,0,0,1,  0.0f,0.0f,
         75.0f, -75.0f, 0.0f, 1.0f,   0,1,0,1,  1.0f,0.0f,
         75.0f,  75.0f, 0.0f, 1.0f,   1,1,0,1,  1.0f,1.0f,
        -75.0f,  75.0f, 0.0f, 1.0f,   0,1,1,1,  0.0f,1.0f
    };

    GLuint Indices[] = { 0,1,2, 0,2,3 };

    glGenVertexArrays(1, &PlayerVaoId);
    glBindVertexArray(PlayerVaoId);

    glGenBuffers(1, &PlayerVboId);
    glBindBuffer(GL_ARRAY_BUFFER, PlayerVboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &PlayerEboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlayerEboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
}

// -------------------- Platform VBO --------------------
void CreatePlatformVBO(void)
{
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    GLuint offset = 0;

    for (const auto& p : platforms)
    {
        // ==== CHANGED ====
        // Decide UV scaling based on singleTexture flag
        GLfloat texRepeatX, texRepeatY;
        if (p.singleTexture)
        {
            texRepeatX = 1.0f;
            texRepeatY = 1.0f;
        }
        else
        {
            texRepeatX = (p.xMax - p.xMin) / 200.0f; // repeat pattern for small platforms
            texRepeatY = (p.yMax - p.yMin) / 100.0f;
        }

        GLfloat verts[] = {
            p.xMin, p.yMin, 0.0f, 1.0f,  1,0,0,1,  0.0f, 0.0f,
            p.xMax, p.yMin, 0.0f, 1.0f,  1,0,0,1,  texRepeatX, 0.0f,
            p.xMax, p.yMax, 0.0f, 1.0f,  1,0,0,1,  texRepeatX, texRepeatY,
            p.xMin, p.yMax, 0.0f, 1.0f,  1,0,0,1,  0.0f, texRepeatY
        };

        vertices.insert(vertices.end(), std::begin(verts), std::end(verts));

        GLuint inds[] = { 0,1,2, 0,2,3 };
        for (int i = 0; i < 6; i++)
            indices.push_back(inds[i] + offset);

        offset += 4;
    }

    glGenVertexArrays(1, &PlatformVaoId);
    glBindVertexArray(PlatformVaoId);

    glGenBuffers(1, &PlatformVboId);
    glBindBuffer(GL_ARRAY_BUFFER, PlatformVboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &PlatformEboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PlatformEboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
}

// -------------------- Collision --------------------
void CheckPlatformCollisions(float& tx, float& ty)
{
    onGround = false;

    float playerLeft = tx - 20.0f;
    float playerRight = tx + 20.0f;
    float playerBottom = ty - 50.0f;
    float playerTop = ty + 75.0f;

    for (const auto& p : platforms)
    {
        bool overlapX = playerRight > p.xMin && playerLeft < p.xMax;
        bool overlapY = playerBottom  <= p.yMax  && playerTop >= p.yMin ;

        if (overlapX && overlapY && playerYVelocity <= 0.0f && playerBottom > p.yMax - 40.0f)
        {
            ty = p.yMax + 40.0f;
            playerYVelocity = 0.0f;
            onGround = true;
            break;
        }
    }
}

void spin() {
    anglePlayer= anglePlayer + 0.3f;
    if (anglePlayer >= 360.0) {
        anglePlayer = 0.0;
    }

}

void spinback() {
    anglePlayer = anglePlayer - 0.3f;
    if (anglePlayer == 0.0) {
        anglePlayer = 360.0;
    }

}

// -------------------- Render Function --------------------
void RenderFunction(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    if (!onGround)
        playerYVelocity += gravity * deltaTime * 60.0f;

    ty += playerYVelocity * deltaTime * 60.0f;

    // Continuous, smooth left/right movement
    if (movingLeft){
        tx -= moveSpeed; // constant step per frame
        spinback();
    }

    if (movingRight) {
        tx += moveSpeed;
        spin();
    }

    CheckPlatformCollisions(tx, ty);

    glm::mat4 cameraMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-tx, -ty, 0.0f));

    // ---------------- Platforms ----------------
    glBindVertexArray(PlatformVaoId);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, platformTexture);

    glUniform1i(glGetUniformLocation(ProgramId, "platformTexture"), 1);
    glUniform1i(glGetUniformLocation(ProgramId, "isPlatform"), 1);

    glm::mat4 platformMatrix = resizeMatrix * cameraMatrix;
    glUniformMatrix4fv(movePlayerMatrixLocation, 1, GL_FALSE, &platformMatrix[0][0]);

    glDrawElements(GL_TRIANGLES, platforms.size() * 6, GL_UNSIGNED_INT, 0);

    // ---------------- Player ----------------
    glm::mat4 playerMatrix = resizeMatrix * cameraMatrix *
        glm::translate(glm::mat4(1.0f), glm::vec3(tx, ty, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), anglePlayer, glm::vec3(0, 0, 1));

    glBindVertexArray(PlayerVaoId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, playerTexture);

    glUniform1i(glGetUniformLocation(ProgramId, "playerTexture"), 0);
    glUniform1i(glGetUniformLocation(ProgramId, "isPlatform"), 0);
    glUniformMatrix4fv(movePlayerMatrixLocation, 1, GL_FALSE, &playerMatrix[0][0]);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glutSwapBuffers();
    glFlush();
}

// -------------------- Initialization --------------------
void Initialize(void)
{
    glClearColor(0.53f, 0.81f, 0.93f, 1.0f);

    CreatePlayerVBO();
    CreatePlatformVBO();

    LoadTexture("prince_2_400x400.png", playerTexture);
    LoadTexture("platform.png", platformTexture);

    CreateShaders();
    resizeMatrix = glm::ortho(xMin, xMax, yMin, yMax);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// -------------------- Cleanup --------------------
void Cleanup(void)
{
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);

    glDeleteBuffers(1, &PlayerVboId);
    glDeleteBuffers(1, &PlayerEboId);
    glDeleteBuffers(1, &PlatformVboId);
    glDeleteBuffers(1, &PlatformEboId);

    glDeleteVertexArrays(1, &PlayerVaoId);
    glDeleteVertexArrays(1, &PlatformVaoId);

    glDeleteProgram(ProgramId);
}

// -------------------- Main --------------------
int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(winWidth, winHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Textured Platforms with Player");

    glewInit();

    Initialize();

    glutDisplayFunc(RenderFunction);
    glutTimerFunc(FRAME_INTERVAL, TimerFunction, 0);
    glutKeyboardFunc(ProcessNormalKeys);
    glutSpecialFunc(ProcessSpecialKeys);
    glutSpecialUpFunc(ProcessSpecialUpKeys);
    glutCloseFunc(Cleanup);

    glutMainLoop();
    return 0;
}
