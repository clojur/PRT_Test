#include <glad/glad.h>
#include <GLFW/glfw3.h> 
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <GI/probe.h> 
#include <iostream>
#include <algorithm>  

int bands = 2;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void no_mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char* path);
unsigned int generateTexture3D(std::vector<glm::vec3*>& all_coeffs, int coeffs_offset, int width, int height, int depth, char type);
void generateDepthMap(unsigned int& depthMapFBO, unsigned int& depthCubemap,const unsigned int SHADOW_WIDTH = 1024, const unsigned int SHADOW_HEIGHT = 1024);
void drawProbes(Shader& probeShader, std::vector<Probe>& probes, std::vector<glm::vec3*>& all_coeffs);
void renderSphere();
void renderCube();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const unsigned int SHADOW_WIDTH = 2048;
const unsigned int SHADOW_HEIGHT = 2048;
bool useShadow = true;
bool probe_useShadow = true;
bool shadowsKeyPressed = false;
bool probe_shadowsKeyPressed = false;
bool dynamic = false;
bool dynamicKeyPressed = false;
bool show_directLight = true;
bool directLightKeyPressed = false;
bool show_indirectLight = true;
bool indirectLightKeyPressed = false;
bool showProbe = false;
bool showProbeKeyPressed = false;

bool leftKeyPressed = false;
bool rightKeyPressed = false;
bool forwardKeyPressed = false;
bool backKeyPressed = false;

bool pageUpKeyPressed = false;
bool pageDownKeyPressed = false;

bool Key4Pressed = false;
bool Key2Pressed = false;
void actionKeyPressed(glm::vec3& lightPos, Light& light);
// camera 
Camera camera(glm::vec3(0.0f, 1.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
 
// lighting
 
glm::vec3 lightAmbient(0.05f, 0.05f, 0.05f); 
glm::vec3 lightDiffuse(2.0f, 2.0f, 2.0f);
glm::vec3 lightSpecular(0.6f, 0.6f, 0.6f);
float lightConstant(1.0f);
float lightLinear(0.045f);
float lightQuadratic(0.016f);

glm::vec3 lightPos(0.0f, 5.0f, 0.0f);
Light light(lightPos, lightAmbient, lightDiffuse, lightSpecular, lightConstant, lightLinear, lightQuadratic);
// esc
bool esc_is_pressed = false;
   
int main() 
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
     
    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Phong_PRTdemo", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state 
    // -----------------------------
    glEnable(GL_DEPTH_TEST); 
    //glEnable(GL_CULL_FACE);

    // glCullFace(GL_FRONT);
    // build and compile our shader zprogram
    // ------------------------------------
    // Shader ourShader("camera.vs", "camera.fs");
    Shader lightingShader("light_casters.vs", "light_casters.fs");
    Shader CubeShader("light_casters.vs", "light_casters.fs");
    //Shader CubeShader("cube.vs", "cube.fs");
    Shader lightSourceShader("light_source.vs", "light_source.fs");
    Shader probeShader("probe.vs", "probe.fs");
    Shader simpleDepthShader("point_shadows_depth.vs", "point_shadows_depth.fs", "point_shadows_depth.gs"); 

    // configure depth map FBO
    // -----------------------
    unsigned int depthMapFBO;
    unsigned int depthCubemap;
    generateDepthMap(depthMapFBO, depthCubemap, SHADOW_WIDTH, SHADOW_HEIGHT);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float wallVertices[] = {
        // positions          // normals           // texture coords
        -10.0f, -10.0f, -0.1f,  0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
         10.0f,  10.0f, -0.1f,  0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
         10.0f, -10.0f, -0.1f,  0.0f,  0.0f, -1.0f,   1.0f, 0.0f,

         10.0f,  10.0f, -0.1f,  0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
        -10.0f, -10.0f, -0.1f,  0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
        -10.0f,  10.0f, -0.1f,  0.0f,  0.0f, -1.0f,   0.0f, 1.0f,

        -10.0f, -10.0f,  0.1f,  0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
         10.0f, -10.0f,  0.1f,  0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
         10.0f,  10.0f,  0.1f,  0.0f,  0.0f,  1.0f,   1.0f, 1.0f,

         10.0f,  10.0f,  0.1f,  0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
        -10.0f,  10.0f,  0.1f,  0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
        -10.0f, -10.0f,  0.1f,  0.0f,  0.0f,  1.0f,   0.0f, 0.0f,

        -10.0f,  10.0f,  0.1f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
        -10.0f,  10.0f, -0.1f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
        -10.0f, -10.0f, -0.1f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        -10.0f, -10.0f, -0.1f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
        -10.0f, -10.0f,  0.1f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
        -10.0f,  10.0f,  0.1f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,

         10.0f,  10.0f,  0.1f,  1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         10.0f, -10.0f, -0.1f,  1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         10.0f,  10.0f, -0.1f,  1.0f,  0.0f,  0.0f,   1.0f, 1.0f,

         10.0f, -10.0f, -0.1f,  1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         10.0f,  10.0f,  0.1f,  1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
         10.0f, -10.0f,  0.1f,  1.0f,  0.0f,  0.0f,   0.0f, 0.0f,

        -10.0f, -10.0f, -0.1f,  0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
         10.0f, -10.0f, -0.1f,  0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
         10.0f, -10.0f,  0.1f,  0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
         10.0f, -10.0f,  0.1f,  0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
        -10.0f, -10.0f,  0.1f,  0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
        -10.0f, -10.0f, -0.1f,  0.0f, -1.0f,  0.0f,   0.0f, 1.0f,

        -10.0f,  10.0f, -0.1f,  0.0f, 1.0f,  0.0f,   0.0f, 1.0f,
         10.0f,  10.0f,  0.1f,  0.0f, 1.0f,  0.0f,   1.0f, 0.0f,
         10.0f,  10.0f, -0.1f,  0.0f, 1.0f,  0.0f,   1.0f, 1.0f,

         10.0f,  10.0f,  0.1f,  0.0f, 1.0f,  0.0f,   1.0f, 0.0f,
        -10.0f,  10.0f, -0.1f,  0.0f, 1.0f,  0.0f,   0.0f, 1.0f,
        -10.0f,  10.0f,  0.1f,  0.0f, 1.0f,  0.0f,   0.0f, 0.0f
    };
    glm::vec3 wall_material_objectColor[] = {
            glm::vec3(1.0f,0.0f,0.0f) / PI,glm::vec3(1.0f,1.0f,1.0f) / PI,glm::vec3(0.0f,1.0f,0.0f) / PI,
            glm::vec3(1.0f,1.0f,1.0f) / PI,glm::vec3(1.0f,1.0f,1.0f) / PI,glm::vec3(0.0f,1.0f,1.0f) / PI

            //glm::vec3(1.0f,0.0f,0.0f),glm::vec3(1.0f,1.0f,1.0f),glm::vec3(0.0f,1.0f,0.0f),
            //glm::vec3(1.0f,1.0f,1.0f),glm::vec3(1.0f,1.0f,1.0f),glm::vec3(1.0f,1.0f,1.0f)

            //glm::vec3(0.0f,1.0f,0.0f),glm::vec3(1.0f,1.0f,1.0f),glm::vec3(0.0f,0.0f,1.0f), 
            //glm::vec3(1.0f,1.0f,1.0f),glm::vec3(1.0f,1.0f,1.0f),glm::vec3(1.0f,1.0f,1.0f),

    }; 
     
    glm::vec3 wallPositions[] = {
        glm::vec3(0.0f,  0.0f,  10.0f),
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(0.0f,  0.0f, -10.0f),
        glm::vec3(0.0f,  10.0f,  0.0f),
        glm::vec3(10.0f,  0.0f,  0.0f),
        glm::vec3(-10.0f, 0.0f,  0.0f),
    };
     
    float wallAngle[] = {
        0.0f,90.0f,0.0f,-90.0f,
        90.0f,-90.0f
    };
    
    glm::vec3 wallAxis[] = {
        glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(1.0f,  0.0f,  0.0f),
        glm::vec3(0.0f,  1.0f,  0.0f),
        glm::vec3(0.0f,  1.0f,  0.0f),
    };
    float cubeVertices[] = {
        // positions          // normals           // texture coords
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,    
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,

         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        
       -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
        1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,

        1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
       -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
       -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

       -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
       -1.0f,  1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
       -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
       -1.0f, -1.0f, -1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
       -1.0f, -1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
       -1.0f,  1.0f,  1.0f,  -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

        1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

        -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
         1.0f,  1.0f,  1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 1.0f,

         1.0f,  1.0f,  1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
        -1.0f,  1.0f,  1.0f,  0.0f, 1.0f,  0.0f, 0.0f, 0.0f
    };
    glm::vec3 cube_material_objectColor[] = {
            glm::vec3(1.0f,1.0f,1.0f) / PI,glm::vec3(1.0f,1.0f,1.0f) / PI,glm::vec3(1.0f,1.0f,1.0f) / PI,
            glm::vec3(1.0f,1.0f,1.0f) / PI,glm::vec3(1.0f,1.0f,1.0f) / PI,glm::vec3(1.0f,1.0f,1.0f) / PI,
    };
    // world space positions of our cubes
    glm::vec3 cubePositions[] = {
        glm::vec3(7.0f, 1.0f,  6.0f),
        glm::vec3(7.0f, 1.0f,  0.0f),
        glm::vec3(7.0f, 1.0f, -6.0f),
        glm::vec3(-7.0f, 1.0f,  6.0f),
        glm::vec3(-7.0f, 1.0f,  0.0f),
        glm::vec3(-7.0f, 1.0f, -6.0f),
    };
   
    int wall_num = 6;
    int cube_num = 6;
    std::vector<TrianglePlane> trianglePlanes =
        GetTrianglePlanes(wall_num,wallVertices,wall_material_objectColor, wallPositions,wallAngle, wallAxis, 
            cube_num, cubeVertices, cube_material_objectColor, cubePositions);

    std::vector<glm::vec3> probePositions;
    int width=0, height=0, depth=0;
    for (float z = -9.5f; z <= 9.5f+0.1f; z=z+(9.5f+9.5f)/6.0f)
    {
        for (float y = 0.5f; y <= 9.5f+0.1f; y=y+(9.5f-0.5f)/3.0f) 
        {
            for (float x = -9.5f; x <= 9.5f+0.1f; x=x+(9.5f+9.5f)/6.0f)
            {
                probePositions.push_back(glm::vec3(x,y,z));
                width++; 
            }
            height++;
        }
        depth++;
    }
    width /= height;
    height /= depth;
     
    std::vector<Probe> probes = GenerateProbes(probePositions, trianglePlanes, light);
    improve_probesInCubes(probes, cube_num, cubePositions, trianglePlanes, wall_num, light);
    std::vector<glm::vec3*> all_coeffs; 
    for (auto& probe : probes)
    {
        glm::vec3* coeffs = ProjectIrradianceFunctionSH(probe, bands);
        all_coeffs.push_back(coeffs);
    } 

    unsigned int coeff3DMapr0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'r');
    unsigned int coeff3DMapg0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'g');
    unsigned int coeff3DMapb0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'b');
    unsigned int coeff3DMapr4;unsigned int coeff3DMapg4;unsigned int coeff3DMapb4;
    unsigned int coeff3DMapr8;unsigned int coeff3DMapg8;unsigned int coeff3DMapb8;
    unsigned int coeff3DMapr12;unsigned int coeff3DMapg12;unsigned int coeff3DMapb12;
    if (bands == 4)
    {
        coeff3DMapr4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'r');
        coeff3DMapg4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'g');
        coeff3DMapb4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'b');
        coeff3DMapr8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'r');
        coeff3DMapg8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'g');
        coeff3DMapb8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'b');
        coeff3DMapr12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'r');
        coeff3DMapg12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'g');
        coeff3DMapb12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'b');
    }
    
    unsigned int VAOs[2], VBOs[2];
    glGenVertexArrays(2, VAOs);
    glGenBuffers(2, VBOs);
    // first wall's triangle setup
    // -------------------
    glBindVertexArray(VAOs[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second cube's triangle setup
    // -------------------
    glBindVertexArray(VAOs[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // third, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    // note that we update the lamp's position attribute's stride to reflect the updated buffer data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    if (bands == 4)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapr0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapr4);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapr8);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapr12);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapg0);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapg4);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapg8);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapg12);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapb0);
        glActiveTexture(GL_TEXTURE9); 
        glBindTexture(GL_TEXTURE_3D, coeff3DMapb4);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapb8);
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapb12);
    }
    else if (bands == 2)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapr0);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapg0);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_3D, coeff3DMapb0);
    }

    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    lightingShader.use();

    // light properties 
    lightingShader.setVec3("light.ambient", light.ambient);
    lightingShader.setFloat("light.constant", light.constant);
    lightingShader.setFloat("light.linear", light.linear);
    lightingShader.setFloat("light.quadratic", light.quadratic);

    // material properties
    lightingShader.setFloat("material.shininess", 16.0f);
    lightingShader.setVec3("material.objectColor", 1.0f/PI, 1.0f/PI, 1.0f/PI);
    
    //lightingShader.setVec3fv("coeffs", bands* bands, coeffs);
    //lightingShader.setVec3("probePos", probes[3].position);
    //-------------------------------------------
    for (int i = 0; i < 4; i++)
    {
        lightingShader.setInt("coeff3DMapr[" + std::to_string(i) + "]", i);
        lightingShader.setInt("coeff3DMapg[" + std::to_string(i) + "]", i + 4);
        lightingShader.setInt("coeff3DMapb[" + std::to_string(i) + "]", i + 8);
    }
    lightingShader.setInt("depthMap", 12);
    lightingShader.setInt("bands", bands);
    //-------------------------------------------
    CubeShader.use();
    CubeShader.setFloat("material.shininess", 16.0f);
    CubeShader.setVec3("material.objectColor", 1.0f/PI, 1.0f/PI, 1.0f/PI);
    CubeShader.setVec3("light.ambient", light.ambient);
    CubeShader.setFloat("light.constant", light.constant);
    CubeShader.setFloat("light.linear", light.linear);
    CubeShader.setFloat("light.quadratic", light.quadratic);
    //-------------------------------------------
    for (int i = 0; i < 4; i++)
    {
        CubeShader.setInt("coeff3DMapr[" + std::to_string(i) + "]", i);
        CubeShader.setInt("coeff3DMapg[" + std::to_string(i) + "]", i + 4);
        CubeShader.setInt("coeff3DMapb[" + std::to_string(i) + "]", i + 8);
    }
    
    CubeShader.setInt("depthMap", 12);
    CubeShader.setInt("bands", bands);
    //-------------------------------------------
    //CubeShader.setVec3fv("coeffs", bands* bands, all_coeffs[3]);
    //CubeShader.setVec3("probePos", probes[3].position);

    // render loop 
    // -----------  
    float lightPosBias = PI;
    bool lastUseShadow = useShadow;
    bool probe_lastUseShadow = probe_useShadow;
    int lastbands = bands;

    int frameCount = 0;
    lastFrame = 0.0f;
    float deltaTime_p30f = 0.0f;

    int probe_compute_offset = 0;
    std::vector<glm::vec3*> partial_coeffs;
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        deltaTime_p30f += deltaTime;
        if (frameCount == 30)
        {
            std::cout << deltaTime_p30f << std::endl;
            frameCount = 0;
            deltaTime_p30f = 0.0f;
        }
        frameCount++;
        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

        // be sure to activate shader when setting uniforms/drawing objects
        if(probe_compute_offset==0)
        actionKeyPressed(lightPos,light);
        
        //shadow----------------------------------------------------------------------------------------------------------------------------
        // 0. create depth cubemap transformation matrices
        // -----------------------------------------------
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        // 1. render scene to depth cubemap
        // --------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        simpleDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        simpleDepthShader.setFloat("far_plane", far_plane);
        simpleDepthShader.setVec3("lightPos", lightPos);
        // renderScene(simpleDepthShader)----------------------
        // render walls
        glBindVertexArray(VAOs[0]);
        for (unsigned int i = 0; i < wall_num; i++) 
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            model = glm::translate(model, wallPositions[i]);
            model = glm::rotate(model, glm::radians(wallAngle[i]), wallAxis[i]);
            simpleDepthShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        // render cubes
        glBindVertexArray(VAOs[1]);
        for (unsigned int i = 0; i < cube_num; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            simpleDepthShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        // end renderScene(simpleDepthShader)----------------------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. render scene as normal 
        // -------------------------
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightingShader.use();
        lightingShader.setVec3("light.position", light.position);
        lightingShader.setVec3("viewPos", camera.Position);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setInt("useShadow", useShadow); // enable/disable shadows by pressing 'SPACE'
        lightingShader.setFloat("far_plane", far_plane);
        lightingShader.setBool("show_directLight", show_directLight);
        lightingShader.setBool("show_indirectLight", show_indirectLight);

        // render walls
        glBindVertexArray(VAOs[0]);
        for (unsigned int i = 0; i < wall_num; i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            model = glm::translate(model, wallPositions[i]);

            model = glm::rotate(model, glm::radians(wallAngle[i]), wallAxis[i]);
            lightingShader.setMat4("model", model);
            lightingShader.setVec3("material.objectColor", wall_material_objectColor[i]);
            lightingShader.setVec3("light.diffuse", light.diffuse);
            lightingShader.setVec3("light.specular", light.specular);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        //----------------------------------------------------------------------------------------
        if (bands != lastbands)
        {
            lightingShader.use();
            lightingShader.setInt("bands", bands);
            CubeShader.use();
            CubeShader.setInt("bands", bands);
            all_coeffs.clear();
            for (auto& probe : probes)
            {
                for (int i = 0; i < probe.surfels.size(); i++)
                {
                    probe.surfels[i].SetColor(light, trianglePlanes);
                }
                probe.CalculateIrradiance();
                glm::vec3* coeffs = ProjectIrradianceFunctionSH(probe, bands);
                all_coeffs.push_back(coeffs);
            }
            coeff3DMapr0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'r');
            coeff3DMapg0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'g');
            coeff3DMapb0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'b');
            if (bands == 4)
            {
                coeff3DMapr4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'r');
                coeff3DMapg4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'g');
                coeff3DMapb4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'b');
                coeff3DMapr8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'r');
                coeff3DMapg8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'g');
                coeff3DMapb8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'b');
                coeff3DMapr12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'r');
                coeff3DMapg12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'g');
                coeff3DMapb12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'b');
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapr0);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapr4);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapr8);
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapr12);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapg0);
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapg4);
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapg8);
                glActiveTexture(GL_TEXTURE7);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapg12);
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapb0);
                glActiveTexture(GL_TEXTURE9);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapb4);
                glActiveTexture(GL_TEXTURE10);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapb8);
                glActiveTexture(GL_TEXTURE11);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapb12);
            }
            else if (bands == 2)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapr0);
                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapg0);
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_3D, coeff3DMapb0);

            }
        }
        else if (probe_compute_offset != 0||dynamic/*|| (useShadow != lastUseShadow)*/||(probe_useShadow != probe_lastUseShadow)||
            rightKeyPressed|| leftKeyPressed||forwardKeyPressed||backKeyPressed||pageUpKeyPressed || pageDownKeyPressed)
        {
            int step_length = 100;//每帧计算多少个probe
            //all_coeffs.clear();
            for (int count=0; (count<step_length)&&(probe_compute_offset<probes.size()); count++, probe_compute_offset++)
            {
                for (int i = 0; i < probes[probe_compute_offset].surfels.size(); i++)
                {
                    probes[probe_compute_offset].surfels[i].SetColor(light, trianglePlanes);
                }
                probes[probe_compute_offset].CalculateIrradiance();
                glm::vec3* coeffs = ProjectIrradianceFunctionSH(probes[probe_compute_offset], bands);
                //all_coeffs.push_back(coeffs);
                partial_coeffs.push_back(coeffs);
            }
            if (probe_compute_offset == probes.size())
            {
                all_coeffs.clear();
                for (auto coeffs : partial_coeffs)
                {
                    all_coeffs.push_back(coeffs);
                }
                partial_coeffs.clear();
                probe_compute_offset = 0;

                coeff3DMapr0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'r');
                coeff3DMapg0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'g');
                coeff3DMapb0 = generateTexture3D(all_coeffs, 0, width, height, depth, 'b');
                if (bands == 4)
                {
                    coeff3DMapr4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'r');
                    coeff3DMapg4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'g');
                    coeff3DMapb4 = generateTexture3D(all_coeffs, 4, width, height, depth, 'b');
                    coeff3DMapr8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'r');
                    coeff3DMapg8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'g');
                    coeff3DMapb8 = generateTexture3D(all_coeffs, 8, width, height, depth, 'b');
                    coeff3DMapr12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'r');
                    coeff3DMapg12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'g');
                    coeff3DMapb12 = generateTexture3D(all_coeffs, 12, width, height, depth, 'b');
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapr0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapr4);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapr8);
                    glActiveTexture(GL_TEXTURE3);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapr12);
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapg0);
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapg4);
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapg8);
                    glActiveTexture(GL_TEXTURE7);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapg12);
                    glActiveTexture(GL_TEXTURE8);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapb0);
                    glActiveTexture(GL_TEXTURE9);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapb4);
                    glActiveTexture(GL_TEXTURE10);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapb8);
                    glActiveTexture(GL_TEXTURE11);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapb12);
                }
                else if (bands == 2)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapr0);
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapg0);
                    glActiveTexture(GL_TEXTURE8);
                    glBindTexture(GL_TEXTURE_3D, coeff3DMapb0);

                }
            }
        }
        //////////////////////////////////////////////////////////////////////////////////////////////
        CubeShader.use();
        CubeShader.setVec3("light.position", light.position);
        CubeShader.setVec3("viewPos", camera.Position);

        CubeShader.setMat4("projection", projection);
        CubeShader.setMat4("view", view);
        CubeShader.setInt("useShadow", useShadow); // enable/disable shadows by pressing 'SPACE'
        CubeShader.setFloat("far_plane", far_plane);
        CubeShader.setBool("show_directLight", show_directLight);
        CubeShader.setBool("show_indirectLight", show_indirectLight);

        // render cubes
        glBindVertexArray(VAOs[1]);
        for (unsigned int i = 0; i < cube_num; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            CubeShader.setMat4("model", model);
            CubeShader.setVec3("light.diffuse", light.diffuse);
            CubeShader.setVec3("light.specular", light.specular);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // also draw the lamp object
        lightSourceShader.use();
        lightSourceShader.setMat4("projection", projection);
        lightSourceShader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, light.position);
        model = glm::scale(model, glm::vec3(0.1f)); // a smaller Sphere
        lightSourceShader.setMat4("model", model);
        lightSourceShader.setVec3("lightColor", light.diffuse);
        renderSphere();
        //glBindVertexArray(lightCubeVAO);
        //glDrawArrays(GL_TRIANGLES, 0, 36);
        if(showProbe)
        drawProbes(probeShader, probes, all_coeffs);
        //drawProbes(lightCubeShader, lightCubeVAO, probes);

        lastUseShadow = useShadow;
        probe_lastUseShadow = probe_useShadow;
        lastbands = bands;

        // frame synchronization
        deltaTime = glfwGetTime() - lastFrame;
        float waitTime = 1.0f / 30.0f - deltaTime;
        if (waitTime>0.0f)
        {
            Sleep(waitTime * 1000);
        }
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(2, VAOs);
    glDeleteBuffers(2, VBOs);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        //glfwSetWindowShouldClose(window, true);
        if (esc_is_pressed == true)
        {
            glfwSetCursorPosCallback(window, mouse_callback);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            esc_is_pressed = false;
            firstMouse = true;
            Sleep(500);
        }
        else
        {
            glfwSetCursorPosCallback(window, no_mouse_callback);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            esc_is_pressed = true;
            firstMouse = true;
            Sleep(500);
        }
    }
    if (esc_is_pressed == true) return;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    //------------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        useShadow = !useShadow;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS && !dynamicKeyPressed)
    {
        dynamic = !dynamic;
        dynamicKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_RELEASE)
    {
        dynamicKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS && !probe_shadowsKeyPressed)
    {
        probe_useShadow = !probe_useShadow;
        probe_shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE)
    {
        probe_shadowsKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !directLightKeyPressed)
    {
        show_directLight = !show_directLight;
        directLightKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE)
    {
        directLightKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !showProbeKeyPressed)
    {
        showProbe = !showProbe;
        showProbeKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE)
    {
        showProbeKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !indirectLightKeyPressed)
    {
        show_indirectLight = !show_indirectLight;
        indirectLightKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE)
    {
        indirectLightKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && !rightKeyPressed)
    {
        rightKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE)
    {
        rightKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && !leftKeyPressed)
    {
        leftKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE)
    {
        leftKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && !forwardKeyPressed)
    {
        forwardKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE)
    {
        forwardKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && !backKeyPressed)
    {
        backKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)
    {
        backKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS && !pageUpKeyPressed)
    {
        pageUpKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_RELEASE)
    {
        pageUpKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS && !pageDownKeyPressed)
    {
        pageDownKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_RELEASE)
    {
        pageDownKeyPressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !Key4Pressed)
    {
        bands = 4;
        Key4Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE)
    {
        Key4Pressed = false;
    }
    //-----------------------------------------------------------------------------
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !Key2Pressed)
    {
        bands = 2;
        Key2Pressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE)
    {
        Key2Pressed = false;
    }
}
void actionKeyPressed(glm::vec3& lightPos, Light& light)
{
    if (dynamic)
    {
        lightPos = glm::vec3(0.0f, 5.0f, 0.0f) + glm::vec3(sin(glfwGetTime()) * 6.0f, 0.0f, 0.0f);
        light.position = lightPos;
    }
    else if (rightKeyPressed)
    {
        lightPos += glm::vec3(0.3f, 0.0f, 0.0f);
        light.position = lightPos;
    }
    else if (leftKeyPressed)
    {
        lightPos -= glm::vec3(0.3f, 0.0f, 0.0f);
        light.position = lightPos;
    }
    else if (forwardKeyPressed)
    {
        lightPos -= glm::vec3(0.0f, 0.0f, 0.3f);
        light.position = lightPos;
    }
    else if (backKeyPressed)
    {
        lightPos += glm::vec3(0.0f, 0.0f, 0.3f);
        light.position = lightPos;
    }
    else if (pageUpKeyPressed)
    {
        if (light.diffuse.x < 8.0f)
        {
            light.diffuse += glm::vec3(0.2f);
            light.specular += glm::vec3(0.1f);
        }
    }
    else if (pageDownKeyPressed)
    {
        if (light.diffuse.x > 0.2f)
        {
            light.diffuse -= glm::vec3(0.2f);
            light.specular -= glm::vec3(0.1f);
        }
    }
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height); 
}

// glfw: whenever the mouse moves, this callback is called
// ------------------------------------------------------- 
void mouse_callback(GLFWwindow* window, double xpos, double ypos)  
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}
void no_mouse_callback(GLFWwindow* window, double xpos, double ypos)
{

}
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
unsigned int generateTexture3D(std::vector<glm::vec3*>& all_coeffs,int coeffs_offset,int width,int height,int depth, char type)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    float* data = new float[width * height * depth * 4];
    if (type == 'r')
    {
        for (int i = 0; i < width * height * depth; i++)
        {
            data[4 * i] = all_coeffs[i][coeffs_offset+0].r;//r
            data[4 * i + 1] = all_coeffs[i][coeffs_offset+1].r;//g
            data[4 * i + 2] = all_coeffs[i][coeffs_offset+2].r;//b
            data[4 * i + 3] = all_coeffs[i][coeffs_offset+3].r;//a
        }
    }
    else if (type == 'g')
    {
        for (int i = 0; i < width * height * depth; i++)
        {
            data[4 * i] = all_coeffs[i][coeffs_offset+0].g;
            data[4 * i + 1] = all_coeffs[i][coeffs_offset+1].g;
            data[4 * i + 2] = all_coeffs[i][coeffs_offset+2].g;
            data[4 * i + 3] = all_coeffs[i][coeffs_offset+3].g;
        }
    }
    else if (type == 'b')
    {
        for (int i = 0; i < width * height * depth; i++)
        {
            data[4 * i] = all_coeffs[i][coeffs_offset+0].b;
            data[4 * i + 1] = all_coeffs[i][coeffs_offset+1].b;
            data[4 * i + 2] = all_coeffs[i][coeffs_offset+2].b;
            data[4 * i + 3] = all_coeffs[i][coeffs_offset+3].b;
        }
    }
    
    glBindTexture(GL_TEXTURE_3D, textureID);
    glTexImage3D(GL_TEXTURE_3D,0,GL_RGBA32F,width,height,depth,0,GL_RGBA, GL_FLOAT,data);

    glGenerateMipmap(GL_TEXTURE_3D);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

// generate DepthMap for shadow
void generateDepthMap(unsigned int& depthMapFBO, unsigned int& depthCubemap, const unsigned int SHADOW_WIDTH, const unsigned int SHADOW_HEIGHT)
{
    glGenFramebuffers(1, &depthMapFBO);
    // create depth cubemap texture
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
// draw probes
void drawProbes(Shader& probeShader, std::vector<Probe>& probes, std::vector<glm::vec3*>& all_coeffs)
{
    //glDisable(GL_CULL_FACE);
    probeShader.use();
    probeShader.setInt("bands", bands);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    probeShader.setMat4("projection", projection);
    probeShader.setMat4("view", view);
    int i=0;
    for (auto& probe : probes)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, probe.position);
        model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        probeShader.setMat4("model", model);
        probeShader.setVec3("probePos", probe.position);
        probeShader.setVec3fv("coeffs", bands * bands, all_coeffs[i]);
        renderSphere();
        i++;
    } 
    //glEnable(GL_CULL_FACE);
}

// renders (and builds at first invocation) a sphere
// -------------------------------------------------
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();
        std::vector<float> data;
        for (std::size_t i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
        }

        std::reverse(indices.begin(), indices.end());
        std::reverse(data.begin(), data.end());

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        float stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
