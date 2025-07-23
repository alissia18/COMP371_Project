// COMP371 Assignment 1
// By Alissia Bocarro and Shania Filosi

#include <iostream>


#define GLEW_STATIC 1   // This allows linking with Static Library on Windows, without DLL
#include <GL/glew.h>    // Include GLEW - OpenGL Extension Wrangler

#include <GLFW/glfw3.h> // GLFW provides a cross-platform interface for creating a graphical context,
                        // initializing OpenGL and binding inputs

#include <glm/glm.hpp>  // GLM is an optimized math library with syntax to similar to OpenGL Shading Language
#include <glm/gtc/matrix_transform.hpp> // include this to create transformation matrices

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

void mouse_callback (GLFWwindow* window, double xpos, double ypos);

GLuint loadTexture(const char *filename);

const char* getVertexShaderSource();

const char* getFragmentShaderSource();

const char* getTexturedVertexShaderSource();

const char* getTexturedFragmentShaderSource();

int createTexturedVertexArrayObject(const glm::vec3* vertexArray, int arraySize);

int compileAndLinkShaders(const char* vertexShaderSource, const char* fragmentShaderSource);

using namespace glm;
using namespace std;

// camera movement
vec3 cameraPos   = vec3(0.0f, 1.0f,  10.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f; // initialized to -90 because 0 results in pointing to the right, -90 makes it forward
float pitch = 0.0f;
// float fov = 45.0f;

// mouse state 
bool firstMouse = true;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;

// timing
float deltaTime = 0.0f; // time bw current and last frame
float lastFrame = 0.0f;


const char* getVertexShaderSource()
{
    // For now, you use a string for your shader code, in the assignment, shaders will be stored in .glsl files
    return
                "#version 330 core\n"
                "layout (location = 0) in vec3 aPos;"
                "layout (location = 1) in vec3 aColor;"
                ""
                "uniform mat4 worldMatrix;" // expose world matrix
                "uniform mat4 viewMatrix = mat4(1.0);" // expose view matrix
                "uniform mat4 projectionMatrix = mat4(1.0);"
                ""
                "out vec3 vertexColor;"
                "void main()"
                "{"
                "   vertexColor = aColor;"
                "   mat4 modelViewProjection = projectionMatrix * viewMatrix * worldMatrix;"
                "   gl_Position = modelViewProjection * vec4( aPos.x, aPos.y, aPos.z, 1.0);"
                "}";
}


const char* getFragmentShaderSource()
{
    return
                "#version 330 core\n"
                "in vec3 vertexColor;"
                "out vec4 FragColor;"
                ""
                "uniform float alpha;"
                ""
                "void main()"
                "{"
                "   FragColor = vec4(vertexColor, alpha);"
                "}";
}

const char* getTexturedVertexShaderSource()
{
    return
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;"
    "layout (location = 1) in vec3 aColor;"
    "layout (location = 2) in vec2 aUV;"
    ""
    "uniform mat4 worldMatrix;"
    "uniform mat4 viewMatrix = mat4(1.0);"  // default value for view matrix (identity)
    "uniform mat4 projectionMatrix = mat4(1.0);"
    ""
    "out vec3 vertexColor;"
    "out vec2 vertexUV;"
    ""
    "void main()"
    "{"
    "   vertexColor = aColor;"
    "   mat4 modelViewProjection = projectionMatrix * viewMatrix * worldMatrix;"
    "   gl_Position = modelViewProjection * vec4(aPos.x, aPos.y, aPos.z, 1.0);"
    "   vertexUV = aUV;"
    "}";
}

const char* getTexturedFragmentShaderSource()
{
    return 
    "#version 330 core\n"
    "in vec3 vertexColor;"
    "in vec2 vertexUV;"
    "uniform sampler2D textureSampler;"
    "uniform float alpha = 1.0;" // Add alpha uniform with default value
    ""
    "out vec4 FragColor;"
    "void main()"
    "{"
    "   vec4 textureColor = texture( textureSampler, vertexUV );"
    "   FragColor = vec4(textureColor.rgb, textureColor.a * alpha);" // Use texture's alpha multiplied by uniform alpha
    "}";
}

glm::vec3 squareArray[] = {
    // First Triangle
    glm::vec3(-0.5f, -0.5f, 0.0f),
    glm::vec3( 1.0f,  0.0f, 0.0f),
    glm::vec3( 0.5f,  0.5f, 0.0f),
    glm::vec3( 0.0f,  1.0f, 0.0f),
    glm::vec3(-0.5f,  0.5f, 0.0f),
    glm::vec3( 0.0f,  0.0f, 1.0f),

    // Second Triangle
    glm::vec3( 0.5f, -0.5f, 0.0f),
    glm::vec3( 1.0f,  1.0f, 0.0f),
    glm::vec3( 0.5f,  0.5f, 0.0f),
    glm::vec3( 0.0f,  1.0f, 0.0f),
    glm::vec3(-0.5f, -0.5f, 0.0f),
    glm::vec3( 1.0f,  0.0f, 0.0f),
};

// Cube model
vec3 cubeArray[] = {  // position,                            color
    vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 0.0f, 0.0f), //left - red
    vec3(-0.5f,-0.5f, 0.5f), vec3(1.0f, 0.0f, 0.0f),
    vec3(-0.5f, 0.5f, 0.5f), vec3(1.0f, 0.0f, 0.0f),
        
    vec3(-0.5f,-0.5f,-0.5f), vec3(1.0f, 0.0f, 0.0f),
    vec3(-0.5f, 0.5f, 0.5f), vec3(1.0f, 0.0f, 0.0f),
    vec3(-0.5f, 0.5f,-0.5f), vec3(1.0f, 0.0f, 0.0f),
        
    vec3( 0.5f, 0.5f,-0.5f), vec3(0.0f, 0.0f, 1.0f), // far - blue
    vec3(-0.5f,-0.5f,-0.5f), vec3(0.0f, 0.0f, 1.0f),
    vec3(-0.5f, 0.5f,-0.5f), vec3(0.0f, 0.0f, 1.0f),
        
    vec3( 0.5f, 0.5f,-0.5f), vec3(0.0f, 0.0f, 1.0f),
    vec3( 0.5f,-0.5f,-0.5f), vec3(0.0f, 0.0f, 1.0f),
    vec3(-0.5f,-0.5f,-0.5f), vec3(0.0f, 0.0f, 1.0f),
        
    vec3( 0.5f,-0.5f, 0.5f), vec3(0.0f, 1.0f, 1.0f), // bottom - turquoise
    vec3(-0.5f,-0.5f,-0.5f), vec3(0.0f, 1.0f, 1.0f),
    vec3( 0.5f,-0.5f,-0.5f), vec3(0.0f, 1.0f, 1.0f),
        
    vec3( 0.5f,-0.5f, 0.5f), vec3(0.0f, 1.0f, 1.0f),
    vec3(-0.5f,-0.5f, 0.5f), vec3(0.0f, 1.0f, 1.0f),
    vec3(-0.5f,-0.5f,-0.5f), vec3(0.0f, 1.0f, 1.0f),
        
    vec3(-0.5f, 0.5f, 0.5f), vec3(0.0f, 1.0f, 0.0f), // near - green
    vec3(-0.5f,-0.5f, 0.5f), vec3(0.0f, 1.0f, 0.0f),
    vec3( 0.5f,-0.5f, 0.5f), vec3(0.0f, 1.0f, 0.0f),
        
    vec3( 0.5f, 0.5f, 0.5f), vec3(0.0f, 1.0f, 0.0f),
    vec3(-0.5f, 0.5f, 0.5f), vec3(0.0f, 1.0f, 0.0f),
    vec3( 0.5f,-0.5f, 0.5f), vec3(0.0f, 1.0f, 0.0f),
        
    vec3( 0.5f, 0.5f, 0.5f), vec3(1.0f, 0.0f, 1.0f), // right - purple
    vec3( 0.5f,-0.5f,-0.5f), vec3(1.0f, 0.0f, 1.0f),
    vec3( 0.5f, 0.5f,-0.5f), vec3(1.0f, 0.0f, 1.0f),
        
    vec3( 0.5f,-0.5f,-0.5f), vec3(1.0f, 0.0f, 1.0f),
    vec3( 0.5f, 0.5f, 0.5f), vec3(1.0f, 0.0f, 1.0f),
    vec3( 0.5f,-0.5f, 0.5f), vec3(1.0f, 0.0f, 1.0f),
        
    vec3( 0.5f, 0.5f, 0.5f), vec3(0.0f, 0.3f, 0.1f), // top - dark swampy green
    vec3( 0.5f, 0.5f,-0.5f), vec3(0.0f, 0.3f, 0.1f),
    vec3(-0.5f, 0.5f,-0.5f), vec3(0.0f, 0.3f, 0.1f),
        
    vec3( 0.5f, 0.5f, 0.5f), vec3(0.0f, 0.3f, 0.1f),
    vec3(-0.5f, 0.5f,-0.5f), vec3(0.0f, 0.3f, 0.1f),
    vec3(-0.5f, 0.5f, 0.5f), vec3(0.0f, 0.3f, 0.1f)
};

glm::vec3 mushroomPlane[] = {
    // Triangle 1
    glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), // bottom-left (U=0, V=1) - FLIPPED V
    glm::vec3( 1.0f, 2.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), // top-right (U=1, V=0) - FLIPPED V
    glm::vec3(-1.0f, 2.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), // top-left (U=0, V=0) - FLIPPED V

    // Triangle 2
    glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), // bottom-left (U=0, V=1) - FLIPPED V
    glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f), // bottom-right (U=1, V=1) - FLIPPED V
    glm::vec3( 1.0f, 2.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), // top-right (U=1, V=0) - FLIPPED V
};


int compileAndLinkShaders(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    // compile and link shader program
    // return shader program id
    // ------------------------------------

    // vertex shader
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // fragment shader
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // link shaders
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

int createVertexArrayObject(const glm::vec3* vertexArray, int arraySize)
{
    // Create a vertex array
    GLuint vertexArrayObject;
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);

    // Upload Vertex Buffer to the GPU, keep a reference to it (vertexBufferObject)
    GLuint vertexBufferObject;
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, arraySize, vertexArray, GL_STATIC_DRAW);

    glVertexAttribPointer(0,                   // attribute 0 matches aPos in Vertex Shader
                          3,                   // size
                          GL_FLOAT,            // type
                          GL_FALSE,            // normalized?
                          2*sizeof(glm::vec3), // stride - each vertex contain 2 vec3 (position, color)
                          (void*)0             // array buffer offset
                          );
    glEnableVertexAttribArray(0);


    glVertexAttribPointer(1,                            // attribute 1 matches aColor in Vertex Shader
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          2*sizeof(glm::vec3),
                          (void*)sizeof(glm::vec3)      // color is offseted a vec3 (comes after position)
                          );
    glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return vertexArrayObject;
}

void mouse_callback (GLFWwindow* window, double xpos, double ypos) {
   if(firstMouse){
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
   }

   float xoffset = xpos - lastX;
   float yoffset = lastY - ypos;
   lastX = xpos;
   lastY = ypos;

   float sensitivity = 0.1f; // can be adjusted
   xoffset *= sensitivity;
   yoffset *= sensitivity;

   yaw += xoffset;
   pitch += yoffset;

   // make sure the pitch doesnt go out of bounds, screen does not get flipped
   if(pitch > 89.0f){
    pitch = 89.0f;
   }
   if(pitch < -89.0f){
    pitch = -89.0f;
   }

   vec3 front;
   front.x = cos(radians(yaw)) * cos(radians(pitch));
   front.y = sin(radians(pitch));
   front.z = sin(radians(yaw)) * cos(radians(pitch));
   cameraFront = normalize(front);
}


int main(int argc, char*argv[])
{
    // Initialize GLFW and OpenGL version
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create Window and rendering context using GLFW, resolution is 800x600
    GLFWwindow* window = glfwCreateWindow(800, 600, "Comp371 - Lab 02", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // tell glfw to retrieve the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    // set mouse pos to center
    glfwSetCursorPos(window, 400.0, 300.0); // Center of your 800x600 window

    glfwSetCursorPosCallback(window, mouse_callback);
    // glfwSetScrollCallback(window, scroll_callback);
    

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to create GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Enable alpha blending (for transparency)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Other OpenGL states to set once
    // Enable Backface culling
   // glEnable(GL_CULL_FACE);
    
    // Enable Depth Test
    //glEnable(GL_DEPTH_TEST); 

    // Add the GL_DEPTH_BUFFER_BIT to glClear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Load Textures
      GLuint mushroom1TextureID = loadTexture("Textures/mushroom.png");
      GLuint flowerTextureID = loadTexture("Textures/flower.png");

    // Black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Compile and link shaders here ...
    int colorShaderProgram = compileAndLinkShaders(getVertexShaderSource(), getFragmentShaderSource());
    int texturedShaderProgram = compileAndLinkShaders(getTexturedVertexShaderSource(), getTexturedFragmentShaderSource());

    // alpha location
    GLuint alphaLocation = glGetUniformLocation(colorShaderProgram, "alpha");
    
    // Define and upload geometry to the GPU here ...
    int squareAO = createVertexArrayObject(squareArray, sizeof(squareArray));
    int groundVAO = createVertexArrayObject(cubeArray, sizeof(cubeArray));
    int mushroomPlaneVAO = createTexturedVertexArrayObject(mushroomPlane, sizeof(mushroomPlane));

    // Mushroom positions
    glm::vec3 mushroomPositions[] = {
        glm::vec3(-5.0f, 0.0f, -5.0f),  // left front
        glm::vec3( 0.0f, 0.0f, -6.0f),  // center front
        glm::vec3( 5.0f, 0.0f, -5.0f),  // right front
        glm::vec3(-4.0f, 0.0f, -10.0f), // left middle
        glm::vec3(-3.0f, 0.0f, -11.0f), // left back
        glm::vec3( 4.0f, 0.0f, -10.0f), // right back
    };

    // Variables to be used later in tutorial
    float angle = 0;
    float rotationSpeed = 180.0f;  // 180 degrees per second

    // ADD THIS HERE - BEFORE the while loop:
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // Entering Main Loop
    while(!glfwWindowShouldClose(window))
    {
        // Each frame, reset color of each pixel to glClearColor
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw color geometry
        glUseProgram(colorShaderProgram);

        //Set projection matrix for color shader:
        GLuint projectionMatrixLocation = glGetUniformLocation(colorShaderProgram, "projectionMatrix");
        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

        glm::mat4 viewMatrix;
        viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
        GLuint viewMatrixLocation = glGetUniformLocation(colorShaderProgram, "viewMatrix");
        glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
        //Draw Rectangle
        //glBindVertexArray(squareAO); // commented out

        // determining the timestep (frame duration) 
        
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // increase rotation angle based on rotation speed and timestep
        angle = (angle + rotationSpeed * deltaTime); // note: angle is in deg, but glm expects rad (conversion below)
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f)); // move square half a unit up
        
        // create rotation matrix around y axis and bind to vertex shader
        GLuint worldMatrixLocation = glGetUniformLocation(colorShaderProgram, "worldMatrix");
        //glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &translationMatrix[0][0]); // commented out

        // draw the model
        //glDrawArrays(GL_TRIANGLES, 0, 6); // 6 vertices, starting at index 0 // commented out

        // Draw ground
        glUniform1f(alphaLocation, 1.0f);
        glm::mat4 groundWorldMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.01f, 0.0f)) *
                                      glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f, 0.02f, 1000.0f));

        glBindVertexArray(groundVAO);
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &groundWorldMatrix[0][0]);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw textured geometry
        glUseProgram(texturedShaderProgram);

        // Get alpha location from textured shader program
        //GLuint texturedAlphaLocation = glGetUniformLocation(texturedShaderProgram, "alpha");

        //Set projection matrix for textured shader:
        GLuint texturedProjectionMatrixLocation = glGetUniformLocation(texturedShaderProgram, "projectionMatrix");
        glUniformMatrix4fv(texturedProjectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);

        // Set view matrix for textured shader too
        GLuint texturedViewMatrixLocation = glGetUniformLocation(texturedShaderProgram, "viewMatrix");
        glUniformMatrix4fv(texturedViewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);

        // Get world matrix location for textured shader
        worldMatrixLocation = glGetUniformLocation(texturedShaderProgram, "worldMatrix");

        glActiveTexture(GL_TEXTURE0);
        GLuint textureLocation = glGetUniformLocation(texturedShaderProgram, "textureSampler");
        glBindTexture(GL_TEXTURE_2D, mushroom1TextureID);
        glUniform1i(textureLocation, 0);                // Set our Texture sampler to user Texture Unit 0

        // Draw mushroom planes
        //glUniform1f(texturedAlphaLocation, 1.0f); // Now use the correct alpha location

        glBindVertexArray(mushroomPlaneVAO);
        for (int i = 0; i < 6; ++i) {
            glm::mat4 mushroomMatrix = glm::translate(glm::mat4(1.0f), mushroomPositions[i]);

            glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &mushroomMatrix[0][0]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Handle inputs

        // escape
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
   
        float cameraSpeed = 2.5 * deltaTime; // adjust accordingly

        // camera movement
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    }

    
    // Shutdown GLFW
    glfwTerminate();
    
    return 0;
}

GLuint loadTexture(const char *filename)
{
    // load textures with dimension data
    int width, height, nrChannels;
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    if(!data){
        cerr << "Error: texture could not load texture file: " << filename << endl;
        return 0;
    }

    // create and bind texturess
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    assert(textureId != 0);

    glBindTexture(GL_TEXTURE_2D, textureId);

    // set filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // upload texture to the PU
    GLenum format = 0;
    if(nrChannels == 1) format = GL_RED;
    else if(nrChannels == 3) format = GL_RGB;
    else if(nrChannels == 4) format = GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // free resources
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureId;
    
}

int createTexturedVertexArrayObject(const glm::vec3* vertexArray, int arraySize)
{
    // Create a vertex array
    GLuint vertexArrayObject;
    glGenVertexArrays(1, &vertexArrayObject);
    glBindVertexArray(vertexArrayObject);

    // Upload Vertex Buffer to the GPU
    GLuint vertexBufferObject;
    glGenBuffers(1, &vertexBufferObject);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, arraySize, vertexArray, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(glm::vec3), (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(1);

    // UV coordinate attribute (location 2) - Note: using only X and Y components
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 3*sizeof(glm::vec3), (void*)(2*sizeof(glm::vec3)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vertexArrayObject;
}

