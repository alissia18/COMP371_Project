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

#include "OBJloaderV2.h"  //For loading .obj files using a polygon list format


void mouse_callback (GLFWwindow* window, double xpos, double ypos);

GLuint loadTexture(const char *filename);

const char* getVertexShaderSource();

const char* getFragmentShaderSource();

const char* getTexturedVertexShaderSource();

const char* getTexturedFragmentShaderSource();

int createTexturedVertexArrayObject(const glm::vec3* vertexArray, int arraySize);

int compileAndLinkShaders(const char* vertexShaderSource, const char* fragmentShaderSource);

void setupShadowMapping();

void renderSceneForShadows(glm::mat4 lightSpaceMatrix, int floorVAO, glm::mat4 groundWorldMatrix,
                           GLuint mushroomVAO, glm::vec3* mushroomPositions, glm::vec3* mushroomScales,
                           int mushroomCount, int mushroomVertices,
                           GLuint plantVAO, glm::vec3* plantPositions, glm::vec3* plantScales,
                           int plantCount, int plantVertices,
                           GLuint specialPlantVAO, glm::vec3 specialPlantPosition1, glm::vec3 specialPlantPosition2,
                           glm::vec3 specialPlantPosition3, int specialPlantVertices,
                           bool specialPlant1Collected, bool specialPlant2Collected, bool specialPlant3Collected);

using namespace glm;
using namespace std;

// flashlight variables
bool flashlightOn = true;
bool fKeyPressed = false;
// Magical light variable
bool magicalLightOn = true;
bool mKeyPressed = false;

// camera movement
vec3 cameraPos   = vec3(0.0f, 1.0f,  10.0f);
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
vec3 cameraRight = normalize(cross(cameraFront, cameraUp));
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

unsigned int depthMapFBO, depthMap;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
int shadowShaderProgram;
int colorShaderProgramWithShadows;  // New shader program with shadows


const char* getVertexShaderSource()
{
    // For now, you use a string for your shader code, in the assignment, shaders will be stored in .glsl files
    return
                "#version 330 core\n"
                "layout (location = 0) in vec3 aPos;"
                "layout (location = 1) in vec3 aColor;"
                "layout (location = 2) in vec3 aNormal;"
                ""
                "uniform mat4 worldMatrix;" // expose world matrix
                "uniform mat4 viewMatrix = mat4(1.0);" // expose view matrix
                "uniform mat4 projectionMatrix = mat4(1.0);"
                ""
                "out vec3 vertexColor;"
                "out vec3 FragPos;"
                "out vec3 Normal;"
                ""
                "void main()"
                "{"
                "   vertexColor = aColor;"
                "   vec4 worldPos = worldMatrix * vec4(aPos, 1.0);"
                "   FragPos = worldPos.xyz;"
                "   Normal = mat3(transpose(inverse(worldMatrix))) * aNormal;"
                "   mat4 modelViewProjection = projectionMatrix * viewMatrix * worldMatrix;"
                "   gl_Position = modelViewProjection * vec4( aPos.x, aPos.y, aPos.z, 1.0);"
                "}";
}


const char* getFragmentShaderSource()
{
    return
    "#version 330 core\n"
    "in vec3 vertexColor;"
    "in vec3 FragPos;"
    "in vec3 Normal;"
    "out vec4 FragColor;"
    ""
    "uniform float alpha;"
    "uniform vec3 cameraPos;"
    "uniform vec3 fogColor;"
    "uniform float fogStart;"
    "uniform float fogEnd;"
    // Flashlight uniforms
    "uniform vec3 lightPos;"
    "uniform vec3 lightDir;"
    "uniform float cutOff;"
    "uniform float outerCutOff;"
    "uniform vec3 lightAmbient;"
    "uniform vec3 lightDiffuse;"
    "uniform vec3 lightSpecular;"
    "uniform float shininess;"
    // Magical light uniforms
    "uniform vec3 magicalLightPos;"
    "uniform vec3 magicalLightColor;"
    "uniform float magicalLightIntensity;"
    "uniform float magicalLightRadius;"
    ""
    "void main()"
    "{"
    "    vec3 norm = normalize(Normal);"
    ""
    "    vec3 globalAmbient = vec3(0.2, 0.2, 0.25);" // Slightly blue ambient light
    "    vec3 baseColor = vertexColor * globalAmbient;"
    ""
    // Flashlight calculations
    "    vec3 lightDirection = normalize(lightPos - FragPos);"
    "    float theta = dot(lightDirection, normalize(-lightDir));"
    "    float epsilon = max(cutOff - outerCutOff, 0.001);" // Prevent division by zero
    "    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);"
    ""
    // Enhanced flashlight lighting with minimum ambient
    "    vec3 ambient = max(lightAmbient, vec3(0.1)) * vertexColor;" // Ensure minimum ambient
    "    float diff = max(dot(norm, lightDirection), 0.0);"
    "    vec3 diffuse = lightDiffuse * diff * vertexColor;"
    "    vec3 viewDir = normalize(cameraPos - FragPos);"
    "    vec3 reflectDir = reflect(-lightDirection, norm);"
    "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(shininess, 1.0));" // Prevent shininess issues
    "    vec3 specular = lightSpecular * spec;"
    "    vec3 flashlightLighting = ambient + (diffuse + specular) * intensity;"
    ""
    // Magical Light Calculations
    "    vec3 magicalLightDir = normalize(magicalLightPos - FragPos);"
    "    float magicalDistance = length(magicalLightPos - FragPos);"
    ""
    // Attenuation
    "    float magicalAttenuation = magicalLightIntensity / max(1.0 + 0.09 * magicalDistance + 0.032 * magicalDistance * magicalDistance, 1.0);"
    "    float magicalFalloff = 1.0 - smoothstep(0.0, max(magicalLightRadius, 1.0), magicalDistance);"
    "    magicalFalloff = pow(max(magicalFalloff, 0.0), 2.0);"
    "    magicalAttenuation *= magicalFalloff;"
    ""
    "    float magicalDiff = max(dot(norm, magicalLightDir), 0.0);"
    "    vec3 magicalDiffuse = magicalLightColor * magicalDiff * vertexColor * magicalAttenuation;"
    ""
    "    vec3 magicalReflectDir = reflect(-magicalLightDir, norm);"
    "    float magicalSpec = pow(max(dot(viewDir, magicalReflectDir), 0.0), max(shininess * 0.5, 1.0));"
    "    vec3 magicalSpecular = magicalLightColor * magicalSpec * magicalAttenuation * 0.3;"
    ""
    "    vec3 magicalLighting = magicalDiffuse + magicalSpecular;"
    ""
    // Combine all lighting
    "    vec3 litColor = baseColor + flashlightLighting + magicalLighting;"
    "    litColor = clamp(litColor, vec3(0.0), vec3(1.0));"
    ""
    // Apply fog
    "    float distance = length(cameraPos - FragPos);"
    "    float fogRange = max(fogEnd - fogStart, 0.1);" // Prevent division by zero
    "    float fogFactor = clamp((fogEnd - distance) / fogRange, 0.0, 1.0);"
    "    vec3 finalColor = mix(fogColor, litColor, fogFactor);"
    "    finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));"
    "    FragColor = vec4(finalColor, clamp(alpha, 0.0, 1.0));"
    "}";
}

const char* getTexturedVertexShaderSource()
{
    return
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;"
    "layout (location = 1) in vec3 aColor;"
    "layout (location = 2) in vec2 aUV;"
    "layout (location = 3) in vec3 aNormal;" // Add normal input
    ""
    "uniform mat4 worldMatrix;"
    "uniform mat4 viewMatrix = mat4(1.0);"
    "uniform mat4 projectionMatrix = mat4(1.0);"
    ""
    "out vec3 vertexColor;"
    "out vec2 vertexUV;"
    "out vec3 FragPos;"   // World-space position for lighting
    "out vec3 Normal;"    // World-space normal for lighting
    ""
    "void main()"
    "{"
    "   vertexColor = aColor;"
    "   vertexUV = aUV;"
    "   vec4 worldPos = worldMatrix * vec4(aPos, 1.0);"
    "   FragPos = worldPos.xyz;"
    "   Normal = mat3(transpose(inverse(worldMatrix))) * aNormal;"
    "   mat4 modelViewProjection = projectionMatrix * viewMatrix * worldMatrix;"
    "   gl_Position = modelViewProjection * vec4(aPos.x, aPos.y, aPos.z, 1.0);"
    "}";
}

const char* getTexturedFragmentShaderSource()
{
    return 
    "#version 330 core\n"
    "in vec3 vertexColor;"
    "in vec2 vertexUV;"
    "in vec3 FragPos;"
    "in vec3 Normal;"
    ""
    "uniform sampler2D textureSampler;"
    "uniform float alpha = 1.0;"
    ""
    // Fog uniforms
    "uniform vec3 cameraPos;"
    "uniform vec3 fogColor;"
    "uniform float fogStart;"
    "uniform float fogEnd;"
    ""
    // Spotlight uniforms
    "uniform vec3 lightPos;"
    "uniform vec3 lightDir;"
    "uniform float cutOff;"
    "uniform float outerCutOff;"
    "uniform vec3 lightAmbient;"
    "uniform vec3 lightDiffuse;"
    "uniform vec3 lightSpecular;"
    "uniform float shininess;"
    ""
    // Magical light uniforms
    "uniform vec3 magicalLightPos;"
    "uniform vec3 magicalLightColor;"
    "uniform float magicalLightIntensity;"
    "uniform float magicalLightRadius;"
    ""
    // Customization parameters with defaults
    "uniform float lightOpacity = 0.8;"
    "uniform vec3 flashlightColor = vec3(1.0, 1.0, 0.6);"
    "uniform float falloffSmoothness = 2.0;"
    ""
    "out vec4 FragColor;"
    ""
    "void main()"
    "{"
    "   vec4 textureColor = texture(textureSampler, vertexUV);"
    "   if(textureColor.a < 0.5) discard;"
    ""
    // ENHANCED BASE AMBIENT for textured objects
    "   vec3 globalAmbient = vec3(0.25, 0.25, 0.3);" // Slightly brighter for textured objects
    "   vec3 baseColor = textureColor.rgb * globalAmbient;"
    ""
    // Lighting calculations
    "   vec3 norm = normalize(Normal);"
    "   vec3 lightDirection = normalize(lightPos - FragPos);"
    "   float theta = dot(lightDirection, normalize(-lightDir));"
    ""
    // Enhanced spotlight falloff with smoothness parameter
    "   float epsilon = max(cutOff - outerCutOff, 0.001);" // Prevent division by zero
    "   float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);"
    "   intensity = pow(intensity, max(falloffSmoothness, 0.1));" // Safeguard smoothness
    ""
    // Ambient lighting - ensure it's never zero
    "   vec3 ambient = max(lightAmbient, vec3(0.15)) * textureColor.rgb;"
    ""
    // Diffuse lighting
    "   float diff = max(dot(norm, lightDirection), 0.0);"
    "   vec3 diffuse = lightDiffuse * diff * textureColor.rgb;"
    ""
    // Specular lighting
    "   vec3 viewDir = normalize(cameraPos - FragPos);"
    "   vec3 reflectDir = reflect(-lightDirection, norm);"
    "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(shininess, 1.0));" // Prevent shininess issues
    "   vec3 specular = lightSpecular * spec;"
    ""
    // Flashlight contribution
    "   vec3 flashlightLighting = ambient + (diffuse + specular) * intensity;"
    "   vec3 lightContribution = intensity * lightDiffuse * flashlightColor * clamp(lightOpacity, 0.0, 1.0);"
    ""
    // IMPROVED MAGICAL LIGHT CALCULATIONS
    "   vec3 magicalLightDir = normalize(magicalLightPos - FragPos);"
    "   float magicalDistance = length(magicalLightPos - FragPos);"
    ""
    // Better attenuation with safeguards
    "   float magicalAttenuation = magicalLightIntensity / max(1.0 + 0.09 * magicalDistance + 0.032 * magicalDistance * magicalDistance, 1.0);"
    "   float magicalFalloff = 1.0 - smoothstep(0.0, max(magicalLightRadius, 1.0), magicalDistance);"
    "   magicalFalloff = pow(max(magicalFalloff, 0.0), 2.0);"
    "   magicalAttenuation *= magicalFalloff;"
    ""
    "   float magicalDiff = max(dot(norm, magicalLightDir), 0.0);"
    "   vec3 magicalDiffuse = magicalLightColor * magicalDiff * textureColor.rgb * magicalAttenuation;"
    ""
    "   vec3 magicalReflectDir = reflect(-magicalLightDir, norm);"
    "   float magicalSpec = pow(max(dot(viewDir, magicalReflectDir), 0.0), max(shininess * 0.3, 1.0));"
    "   vec3 magicalSpecular = magicalLightColor * magicalSpec * magicalAttenuation * 0.2;"
    ""
    "   vec3 magicalLighting = magicalDiffuse + magicalSpecular;"
    ""
    // COMBINE ALL LIGHTING WITH PROPER BASE
    "   vec3 litColor = baseColor + flashlightLighting + lightContribution + magicalLighting;"
    "   litColor = clamp(litColor, vec3(0.0), vec3(1.0));"
    ""
    // Apply fog with safeguards
    "   float distance = length(cameraPos - FragPos);"
    "   float fogRange = max(fogEnd - fogStart, 0.1);" // Prevent division by zero
    "   float fogFactor = clamp((fogEnd - distance) / fogRange, 0.0, 1.0);"
    "   vec3 finalColor = mix(fogColor, litColor, fogFactor);"
    "   finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));"
    "   FragColor = vec4(finalColor, clamp(alpha, 0.0, 1.0));"
    "}";
}

// Shadow Vertex Shader
const char* getShadowVertexShaderSource()
{
    return
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "uniform mat4 worldMatrix;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = lightSpaceMatrix * worldMatrix * vec4(aPos, 1.0);\n"
    "}\n";
}

// Shadow Fragment Shader
const char* getShadowFragmentShaderSource()
{
    return
    "#version 330 core\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // gl_FragDepth is written automatically\n"
    "}\n";
}

// Updated Vertex Shader WITH shadow support
const char* getVertexShaderSourceWithShadows()
{
    return
    "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec3 aColor;\n"
    "layout (location = 2) in vec3 aNormal;\n"
    "\n"
    "uniform mat4 worldMatrix;\n"
    "uniform mat4 viewMatrix = mat4(1.0);\n"
    "uniform mat4 projectionMatrix = mat4(1.0);\n"
    "uniform mat4 lightSpaceMatrix;\n"  // for shadow mapping
    "\n"
    "out vec3 vertexColor;\n"
    "out vec3 FragPos;\n"
    "out vec3 Normal;\n"
    "out vec4 FragPosLightSpace;\n"  // for shadow mapping
    "\n"
    "void main()\n"
    "{\n"
    "    vertexColor = aColor;\n"
    "    vec4 worldPos = worldMatrix * vec4(aPos, 1.0);\n"
    "    FragPos = worldPos.xyz;\n"
    "    Normal = mat3(transpose(inverse(worldMatrix))) * aNormal;\n"
    "    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);\n"
    "    mat4 modelViewProjection = projectionMatrix * viewMatrix * worldMatrix;\n"
    "    gl_Position = modelViewProjection * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\n";
}

// Updated Fragment Shader WITH shadows (based on your exact working version)
const char* getFragmentShaderSourceWithShadows()
{
    return
        "#version 330 core\n"
        "in vec3 vertexColor;\n"
        "in vec3 FragPos;\n"
        "in vec3 Normal;\n"
        "in vec4 FragPosLightSpace;\n"
        "out vec4 FragColor;\n"
        "\n"
        "uniform float alpha;\n"
        "uniform vec3 cameraPos;\n"
        "uniform vec3 fogColor;\n"
        "uniform float fogStart;\n"
        "uniform float fogEnd;\n"
        // Flashlight uniforms
        "uniform vec3 lightPos;\n"
        "uniform vec3 lightDir;\n"
        "uniform float cutOff;\n"
        "uniform float outerCutOff;\n"
        "uniform vec3 lightAmbient;\n"
        "uniform vec3 lightDiffuse;\n"
        "uniform vec3 lightSpecular;\n"
        "uniform float shininess;\n"
        // Magical light uniforms
        "uniform vec3 magicalLightPos;\n"
        "uniform vec3 magicalLightColor;\n"
        "uniform float magicalLightIntensity;\n"
        "uniform float magicalLightRadius;\n"
        // Shadow uniforms
        "uniform sampler2D shadowMap;\n"
        "uniform bool enableShadows;\n"
        "\n"
        // Shadow calculation function
        "float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDirection)\n"
        "{\n"
        "    // Perform perspective divide\n"
        "    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;\n"
        "    // Transform to [0,1] range\n"
        "    projCoords = projCoords * 0.5 + 0.5;\n"
        "\n"
        "    // Keep the shadow at 0.0 when outside the far_plane region\n"
        "    if(projCoords.z > 1.0)\n"
        "        return 0.0;\n"
        "\n"
        "    // Get closest depth value from light's perspective\n"
        "    float closestDepth = texture(shadowMap, projCoords.xy).r;\n"
        "    // Get depth of current fragment from light's perspective\n"
        "    float currentDepth = projCoords.z;\n"
        "\n"
        "    float bias = max(0.05 * (1.0 - dot(normal, lightDirection)), 0.005);\n"
        "\n"
        "    // Check whether current frag pos is in shadow\n"
        "    float shadow = 0.0;\n"
        "    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);\n"
        "    for(int x = -1; x <= 1; ++x)\n"
        "    {\n"
        "        for(int y = -1; y <= 1; ++y)\n"
        "        {\n"
        "            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;\n"
        "            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;\n"
        "        }\n"
        "    }\n"
        "    shadow /= 9.0;\n"
        "\n"
        "    return shadow;\n"
        "}\n"
        "\n"
        "void main()\n"
        "{\n"
        "    vec3 norm = normalize(Normal);\n"
        "\n"
        "    // ENHANCED BASE AMBIENT - This ensures objects are never completely black\n"
        "    vec3 globalAmbient = vec3(0.2, 0.2, 0.25);\n" // Slightly blue ambient light
        "    vec3 baseColor = vertexColor * globalAmbient;\n"
        "\n"
        "    // Flashlight calculations\n"
        "    vec3 lightDirection = normalize(lightPos - FragPos);\n"
        "    float theta = dot(lightDirection, normalize(-lightDir));\n"
        "    float epsilon = max(cutOff - outerCutOff, 0.001);\n" // Prevent division by zero
        "    float intensity = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);\n"
        "\n"
        "    // Enhanced flashlight lighting with minimum ambient\n"
        "    vec3 ambient = max(lightAmbient, vec3(0.1)) * vertexColor;\n" // Ensure minimum ambient
        "    float diff = max(dot(norm, lightDirection), 0.0);\n"
        "    vec3 diffuse = lightDiffuse * diff * vertexColor;\n"
        "    vec3 viewDir = normalize(cameraPos - FragPos);\n"
        "    vec3 reflectDir = reflect(-lightDirection, norm);\n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(shininess, 1.0));\n" // Prevent shininess issues
        "    vec3 specular = lightSpecular * spec;\n"
        "\n"
        "    // Calculate shadow factor and apply to lighting\n"
        "    float shadow = 0.0;\n"
        "    if(enableShadows && intensity > 0.0)\n"
        "    {\n"
        "        shadow = ShadowCalculation(FragPosLightSpace, norm, lightDirection);\n"
        "    }\n"
        "\n"
        "    // Apply shadows to diffuse and specular (but not ambient)\n"
        "    vec3 flashlightLighting = ambient + (diffuse + specular) * intensity * (1.0 - shadow);\n"
        "\n"
        "    // IMPROVED MAGICAL LIGHT CALCULATIONS (unchanged)\n"
        "    vec3 magicalLightDir = normalize(magicalLightPos - FragPos);\n"
        "    float magicalDistance = length(magicalLightPos - FragPos);\n"
        "\n"
        "    // Better attenuation with safeguards\n"
        "    float magicalAttenuation = magicalLightIntensity / max(1.0 + 0.09 * magicalDistance + 0.032 * magicalDistance * magicalDistance, 1.0);\n"
        "    float magicalFalloff = 1.0 - smoothstep(0.0, max(magicalLightRadius, 1.0), magicalDistance);\n"
        "    magicalFalloff = pow(max(magicalFalloff, 0.0), 2.0);\n"
        "    magicalAttenuation *= magicalFalloff;\n"
        "\n"
        "    float magicalDiff = max(dot(norm, magicalLightDir), 0.0);\n"
        "    vec3 magicalDiffuse = magicalLightColor * magicalDiff * vertexColor * magicalAttenuation;\n"
        "\n"
        "    vec3 magicalReflectDir = reflect(-magicalLightDir, norm);\n"
        "    float magicalSpec = pow(max(dot(viewDir, magicalReflectDir), 0.0), max(shininess * 0.5, 1.0));\n"
        "    vec3 magicalSpecular = magicalLightColor * magicalSpec * magicalAttenuation * 0.3;\n"
        "\n"
        "    vec3 magicalLighting = magicalDiffuse + magicalSpecular;\n"
        "\n"
        "    // COMBINE ALL LIGHTING WITH PROPER BASE\n"
        "    vec3 litColor = baseColor + flashlightLighting + magicalLighting;\n"
        "    litColor = clamp(litColor, vec3(0.0), vec3(1.0));\n"
        "\n"
        "    // Apply fog with safeguards\n"
        "    float distance = length(cameraPos - FragPos);\n"
        "    float fogRange = max(fogEnd - fogStart, 0.1);\n" // Prevent division by zero
        "    float fogFactor = clamp((fogEnd - distance) / fogRange, 0.0, 1.0);\n"
        "    vec3 finalColor = mix(fogColor, litColor, fogFactor);\n"
        "    finalColor = clamp(finalColor, vec3(0.0), vec3(1.0));\n"
        "    FragColor = vec4(finalColor, clamp(alpha, 0.0, 1.0));\n"
        "}\n";
}

glm::mat4 getLightSpaceMatrix(glm::vec3 lightPos, glm::vec3 lightDir) 
{
    float near_plane = 1.0f, far_plane = 25.0f;
    
    // Create light's view matrix
    glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0, 1.0, 0.0));
    
    return lightProjection * lightView;
}

glm::vec3 floorVertices[] = {
    // positions           // colors            // normals
    glm::vec3(-5.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3( 5.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3( 5.0f, 0.0f,  5.0f), glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f),

    glm::vec3(-5.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3( 5.0f, 0.0f,  5.0f), glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(-5.0f, 0.0f,  5.0f), glm::vec3(0.0f, 0.1f, 0.3f), glm::vec3(0.0f, 1.0f, 0.0f),
};

glm::vec3 skyboxCube[] = {  
    // LEFT FACE (X = -1) - Normal points right (+X)
    glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),   // bottom-left
    glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),  // top-left
    glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(0.25f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),  // bottom-right
    
    glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(0.0f, 0.25f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),  // top-left
    glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(0.25f, 0.25f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), // top-right
    glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.5f, 0.7f, 1.0f), glm::vec3(0.25f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), // bottom-right

    // RIGHT FACE (X = +1) - Normal points left (-X)
    glm::vec3(1.0f, -1.0f,  1.0f), glm::vec3(0.3f, 0.5f, 0.8f), glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),    // bottom-left
    glm::vec3(1.0f,  1.0f,  1.0f), glm::vec3(0.3f, 0.5f, 0.8f), glm::vec3(0.5f, 0.25f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),   // top-left  
    glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.5f, 0.8f), glm::vec3(0.75f, 0.5f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),   // bottom-right
    
    glm::vec3(1.0f,  1.0f,  1.0f), glm::vec3(0.3f, 0.5f, 0.8f), glm::vec3(0.5f, 0.25f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),   // top-left
    glm::vec3(1.0f,  1.0f, -1.0f), glm::vec3(0.3f, 0.5f, 0.8f), glm::vec3(0.75f, 0.25f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),  // top-right
    glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.5f, 0.8f), glm::vec3(0.75f, 0.5f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),   // bottom-right

    // BOTTOM FACE (Y = -1) - Normal points up (+Y)
    glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.2f, 0.3f, 0.4f), glm::vec3(0.25f, 0.75f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), // bottom-left
    glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.2f, 0.3f, 0.4f), glm::vec3(0.5f, 0.75f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),   // bottom-right
    glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.2f, 0.3f, 0.4f), glm::vec3(0.25f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),  // top-left
    
    glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.2f, 0.3f, 0.4f), glm::vec3(0.5f, 0.75f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),   // bottom-right
    glm::vec3(1.0f, -1.0f,  1.0f), glm::vec3(0.2f, 0.3f, 0.4f), glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),    // top-right
    glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.2f, 0.3f, 0.4f), glm::vec3(0.25f, 0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),  // top-left

    // TOP FACE (Y = +1) - Normal points down (-Y)
    glm::vec3(-1.0f, 1.0f,  1.0f), glm::vec3(0.6f, 0.8f, 1.0f), glm::vec3(0.25f, 0.25f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),  // bottom-left
    glm::vec3(1.0f, 1.0f,  1.0f), glm::vec3(0.6f, 0.8f, 1.0f), glm::vec3(0.5f, 0.25f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),    // bottom-right
    glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.6f, 0.8f, 1.0f), glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),   // top-left
    
    glm::vec3(1.0f, 1.0f,  1.0f), glm::vec3(0.6f, 0.8f, 1.0f), glm::vec3(0.5f, 0.25f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),    // bottom-right
    glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.6f, 0.8f, 1.0f), glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),     // top-right
    glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.6f, 0.8f, 1.0f), glm::vec3(0.25f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),   // top-left

    // FRONT FACE (Z = +1) - Normal points back (-Z)
    glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.4f, 0.6f, 0.9f), glm::vec3(0.25f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),   // bottom-left
    glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.4f, 0.6f, 0.9f), glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),     // bottom-right
    glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.4f, 0.6f, 0.9f), glm::vec3(0.25f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),   // top-left
    
    glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.4f, 0.6f, 0.9f), glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),     // bottom-right
    glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.4f, 0.6f, 0.9f), glm::vec3(0.5f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),     // top-right
    glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.4f, 0.6f, 0.9f), glm::vec3(0.25f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),   // top-left

    // BACK FACE (Z = -1) - Normal points forward (+Z)
    glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.4f, 0.7f), glm::vec3(0.75f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),   // bottom-left
    glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.4f, 0.7f), glm::vec3(1.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),   // bottom-right
    glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.3f, 0.4f, 0.7f), glm::vec3(0.75f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),   // top-left
    
    glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.3f, 0.4f, 0.7f), glm::vec3(1.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),   // bottom-right
    glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.3f, 0.4f, 0.7f), glm::vec3(1.0f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),   // top-right
    glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.3f, 0.4f, 0.7f), glm::vec3(0.75f, 0.25f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)    // top-left
};

// Magical light orb geometry
glm::vec3 lightOrbVertices[] = {
    // Simple sphere approximation using triangles (8 triangles for simplicity)
    // Top pyramid
    glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // top
    glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // front-right
    glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // front-left
    
    glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // top
    glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // front-left
    glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),   // back-left
    
    glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // top
    glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),   // back-left
    glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),   // back-right
    
    glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // top
    glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),   // back-right
    glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, 1.0f, 0.0f),    // front-right
    
    // Bottom pyramid
    glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),   // front-right
    glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // bottom
    glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),   // front-left
    
    glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),   // front-left
    glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // bottom
    glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // back-left
    
    glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // back-left
    glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // bottom
    glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // back-right
    
    glm::vec3(0.0f, 0.0f, -0.5f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // back-right
    glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),  // bottom
    glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(0.0f, -1.0f, 0.0f),   // front-right
};

glm::vec3 mushroomPlane[] = {
    // Triangle 1 - Normal points toward camera (0, 0, 1)
    glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-left
    glm::vec3( 1.0f, 2.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-right
    glm::vec3(-1.0f, 2.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-left

    // Triangle 2 - Normal points toward camera (0, 0, 1)
    glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-left
    glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-right
    glm::vec3( 1.0f, 2.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-right
};

glm::vec3 flowerPlane[] = {
    // Triangle 1 - Normal points toward camera (0, 0, 1)
    glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-left
    glm::vec3( 0.5f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-right
    glm::vec3(-0.5f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-left

    // Triangle 2 - Normal points toward camera (0, 0, 1)
    glm::vec3(-0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-left
    glm::vec3( 0.5f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-right
    glm::vec3( 0.5f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-right
};

glm::vec3 dragonflyPlane[] = {
    // Triangle 1 - Normal points toward camera (0, 0, 1)
    glm::vec3(-0.2f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-left
    glm::vec3( 0.2f, 0.4f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-right
    glm::vec3(-0.2f, 0.4f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-left

    // Triangle 2 - Normal points toward camera (0, 0, 1)
    glm::vec3(-0.2f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-left
    glm::vec3( 0.2f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // bottom-right
    glm::vec3( 0.2f, 0.4f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), // top-right
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
                          3*sizeof(glm::vec3), // stride - each vertex contain 2 vec3 (position, color)
                          (void*)0             // array buffer offset
                          );
    glEnableVertexAttribArray(0);


    glVertexAttribPointer(1,                            // attribute 1 matches aColor in Vertex Shader
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          3*sizeof(glm::vec3),
                          (void*)sizeof(glm::vec3)      // color is offseted a vec3 (comes after position)
                          );
    glEnableVertexAttribArray(1);

        // Normals (attribute 2)
    glVertexAttribPointer(
        2,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(glm::vec3),       // same stride
        (void*)(2 * sizeof(glm::vec3)) // offset: after position and color
    );
    glEnableVertexAttribArray(2);

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

   float sensitivity = 0.002f; // can be adjusted
   xoffset *= sensitivity;
   yoffset *= sensitivity;

   yaw += xoffset;
   pitch += yoffset;

   // Clamp pitch so camera doesn't flip vertically
   if (pitch > 89.0f)
       pitch = 89.0f;
   if (pitch < -89.0f)
       pitch = -89.0f;

   vec3 front;
   front.x = cos(radians(yaw)) * cos(radians(pitch));
   front.y = sin(radians(pitch));
   front.z = sin(radians(yaw)) * cos(radians(pitch));
   cameraFront = normalize(front);
}

//Sets up a model using an Element Buffer Object to refer to vertex data
GLuint setupModelEBO(string path, int& vertexCount)
{
    vector<int> vertexIndices;
    vector<glm::vec3> vertices;
    vector<glm::vec3> normals;
    vector<glm::vec2> UVs;

    loadOBJ2(path.c_str(), vertexIndices, vertices, normals, UVs);

    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertex positions (location 0)
    GLuint vertices_VBO;
    glGenBuffers(1, &vertices_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Create default white colors for all vertices (location 1)
    vector<glm::vec3> colors(vertices.size(), glm::vec3(1.0f, 1.0f, 1.0f));
    GLuint colors_VBO;
    glGenBuffers(1, &colors_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, colors_VBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), &colors.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);

    // UVs (location 2)
    GLuint uvs_VBO;
    glGenBuffers(1, &uvs_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, uvs_VBO);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * sizeof(glm::vec2), &UVs.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(2);

    // Normals (location 3) - MOVED TO CORRECT LOCATION
    GLuint normals_VBO;
    glGenBuffers(1, &normals_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, normals_VBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals.front(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(3);

    // EBO setup
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(int), &vertexIndices.front(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    vertexCount = vertexIndices.size();
    return VAO;
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "Enchanted Mangrove", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // tell glfw to retrieve the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
   //glEnable(GL_CULL_FACE);

   glfwWindowHint(GL_DEPTH_BITS, 24);
    
    // Enable Depth Test
    glEnable(GL_DEPTH_TEST); 

    // Load Textures
    GLuint mushroom1TextureID = loadTexture("Textures/mushroom.png");
    GLuint mushroom2TextureID = loadTexture("Textures/mushroom2.png");
    GLuint mushroom3TextureID = loadTexture("Textures/mushroom3.png");
    GLuint mushroom4TextureID = loadTexture("Textures/mushroom4.png");
    GLuint mushroom5TextureID = loadTexture("Textures/mushroom5.png");
    GLuint mushroom6TextureID = loadTexture("Textures/mushroom6.png");
    GLuint mushroomTextureIDs[] = {mushroom1TextureID, mushroom2TextureID, mushroom3TextureID, mushroom4TextureID, mushroom5TextureID, mushroom6TextureID};
    GLuint mushroomTextureID = loadTexture("Textures/mushroom_texture.png");
    GLuint flowerTextureID = loadTexture("Textures/flower.png");
    GLuint waterTextureID = loadTexture("Textures/water.png");
    GLuint dragonflyBodyTextureID = loadTexture("Textures/dragonfly.png");
    GLuint wingTextureID = loadTexture("Textures/wings.png");
    GLuint skyboxTextureID = loadTexture("Textures/skybox.png");
    GLuint plantTextureID = loadTexture("Textures/plant_texture.png");
    GLuint specialPlantTextureID = loadTexture("Textures/special_plant_texture.png");
    glClearColor(0.03f, 0.03f, 0.11f, 1.0f); // night sky

    // mushroom 3d model
    string mushroomPath = "Models/mushroom.obj";
    int mushroomVertices;

    GLuint mushroomVAO = setupModelEBO(mushroomPath, mushroomVertices);

    //plant 3d model
    string plantPath = "Models/plant.obj";
    int plantVertices;
    
    GLuint plantVAO = setupModelEBO(plantPath, plantVertices);

    // special plant model
    string specialPlantPath = "Models/special_plant.obj";
    int specialPlantVertices;
    GLuint specialPlantVAO = setupModelEBO(specialPlantPath, specialPlantVertices);

    
    // Compile and link shaders here ...
    int colorShaderProgram = compileAndLinkShaders(getVertexShaderSource(), getFragmentShaderSource());
    int texturedShaderProgram = compileAndLinkShaders(getTexturedVertexShaderSource(), getTexturedFragmentShaderSource());

    setupShadowMapping();

    // alpha location
    GLuint alphaLocation = glGetUniformLocation(colorShaderProgram, "alpha");
    
    // Define and upload geometry to the GPU here ...
    int floorVAO = createVertexArrayObject(floorVertices, sizeof(floorVertices));
    int mushroomPlaneVAO = createTexturedVertexArrayObject(mushroomPlane, sizeof(mushroomPlane));
    int flowerPlaneVAO = createTexturedVertexArrayObject(flowerPlane, sizeof(flowerPlane));
    int dragonflyPlaneVAO = createTexturedVertexArrayObject(dragonflyPlane, sizeof(dragonflyPlane));
    int skyboxVAO = createTexturedVertexArrayObject(skyboxCube, sizeof(skyboxCube));
    int lightOrbVAO = createVertexArrayObject(lightOrbVertices, sizeof(lightOrbVertices)); // magical light orb

    // mushroom scale values

    glm::vec3 mushroomScales[] = {
        glm::vec3(2,2,2),  // left front
        glm::vec3(1,1,1),  // left middle
        glm::vec3(3,3,3),  // left back
        glm::vec3(1,1,1), // right front
        glm::vec3(3,3,3), // right middle
        glm::vec3(2,2,2), // right back
    };

    
    glm::vec3 plantScales[] = {
        glm::vec3(4,4,4),  // left front
        glm::vec3(3,3,3),  // left middle
    };


    // Mushroom positions
    glm::vec3 mushroomPositions[] = {
        glm::vec3(-8.0f, 0.0f, 1.0f),  // left front
        glm::vec3( -12.0f, 0.0f, 4.0f),  // left middle
        glm::vec3( -10.0f, 0.0f, 6.0f),  // left back
        glm::vec3(-3.0f, 0.0f, 1.3f), // right front
        glm::vec3(2.0f, 0.0f, 4.3f), // right middle
        glm::vec3(-1.0f, 0.0f, 6.3f), // right back
    };

    // Plant positions
    int nbPlants = 2;
    vec3 plantPositions[] = {
        vec3(-7.0f, 0.0f, 2.5f),
        vec3(-1.0f, 0.0f, 3.3f)
    };

    int nbSpecialPlants = 3;
    vec3 specialPlantPosition1 = vec3(-3.0f, 0.0f, 9.5f);
    vec3 specialPlantPosition2 = vec3(4.0f, 0.0f, 3.0f);
    vec3 specialPlantPosition3 = vec3(-11.0f, 0.0f, 8.5f);

    vec3 flowerPosition = vec3(-2.0f, 0.0f, 8.0f);
    vec3 dragonflyPosition = vec3(-2.25f, 0.35f, 8.01f);
    vec3 wingPositionFront = vec3(-2.15f,0.43f, 8.02f);
    vec3 wingPositionBack = vec3(-2.15f,0.43f, 7.98f);

    bool specialPlant1Collected = false, specialPlant2Collected = false, specialPlant3Collected = false;

    // Variables to be used later in tutorial
    float angle = 0;
    float rotationSpeed = 180.0f;  // 180 degrees per second

    glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    // Entering Main Loop
while(!glfwWindowShouldClose(window))
{
    glm::vec3 fogColor = glm::vec3(0.15f, 0.25f, 0.4f);
    float fogStart = 2.5f;
    float fogEnd = 15.0f;

    // Timing
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    angle = 2.0f * sin(glfwGetTime());
    
    // Calculate magical light position
    float lightTime = glfwGetTime() * 0.8f;
    float lightX = -5.5f + 4.0f * sin(lightTime);
    float lightY = 8.0f + 1.5f * sin(lightTime * 2.0f);
    float lightZ = 4.0f + 2.0f * cos(lightTime * 0.6f);
    vec3 magicalLightPos = vec3(lightX, lightY, lightZ);
    
    // Calculate flashlight position
    vec3 lightOffset = cameraFront * 1.0f + cameraRight * 0.8f + -cameraUp * 0.3f;
    vec3 flashlightPos = cameraPos + lightOffset;
    
    // Calculate view matrix
    glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    
    // =========================
    // 1. SHADOW MAP GENERATION
    // =========================
    if (flashlightOn) {
        glm::mat4 lightSpaceMatrix = getLightSpaceMatrix(flashlightPos, cameraFront);
        
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Prepare ground matrix for shadow rendering
        glm::mat4 groundWorldMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.01f, 0.0f)) *
                                      glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f, 0.02f, 1000.0f));
        
        // Render scene from light's perspective
        renderSceneForShadows(lightSpaceMatrix, floorVAO, groundWorldMatrix,
                              mushroomVAO, mushroomPositions, mushroomScales, 6, mushroomVertices,
                              plantVAO, plantPositions, plantScales, nbPlants, plantVertices,
                              specialPlantVAO, specialPlantPosition1, specialPlantPosition2, specialPlantPosition3, specialPlantVertices,
                              specialPlant1Collected, specialPlant2Collected, specialPlant3Collected);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    // =========================
    // 2. REGULAR SCENE RENDERING
    // =========================
    glViewport(0, 0, 800, 600);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // ========== RENDER SKYBOX FIRST ==========
    glUseProgram(texturedShaderProgram);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    
    glm::mat4 skyboxViewMatrix = glm::mat4(glm::mat3(viewMatrix));
    GLuint texturedProjectionMatrixLocation = glGetUniformLocation(texturedShaderProgram, "projectionMatrix");
    glUniformMatrix4fv(texturedProjectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    GLuint texturedViewMatrixLocation = glGetUniformLocation(texturedShaderProgram, "viewMatrix");
    glUniformMatrix4fv(texturedViewMatrixLocation, 1, GL_FALSE, &skyboxViewMatrix[0][0]);
    
    GLuint worldMatrixLocation = glGetUniformLocation(texturedShaderProgram, "worldMatrix");
    glm::vec3 skyboxSize = glm::vec3(5.0f, 5.0f, 5.0f);
    glm::mat4 skyboxWorldMatrix = glm::scale(glm::mat4(1.0f), skyboxSize);
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &skyboxWorldMatrix[0][0]);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, skyboxTextureID);
    GLuint textureLocation = glGetUniformLocation(texturedShaderProgram, "textureSampler");
    glUniform1i(textureLocation, 0);
    
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    
    // ========== RENDER COLOR GEOMETRY WITH SHADOWS ==========
    glUseProgram(colorShaderProgramWithShadows);
    
    // Set up shadow uniforms
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(glGetUniformLocation(colorShaderProgramWithShadows, "shadowMap"), 1);
    glUniform1i(glGetUniformLocation(colorShaderProgramWithShadows, "enableShadows"), flashlightOn ? 1 : 0);
    
    if (flashlightOn) {
        glm::mat4 lightSpaceMatrix = getLightSpaceMatrix(flashlightPos, cameraFront);
        glUniformMatrix4fv(glGetUniformLocation(colorShaderProgramWithShadows, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    }
    
    // Set flashlight uniforms
    glUniform1f(glGetUniformLocation(colorShaderProgramWithShadows, "shininess"), 1.0f);
    glUniform3fv(glGetUniformLocation(colorShaderProgramWithShadows, "lightPos"), 1, &flashlightPos[0]);
    glUniform3fv(glGetUniformLocation(colorShaderProgramWithShadows, "lightDir"), 1, &cameraFront[0]);
    glUniform1f(glGetUniformLocation(colorShaderProgramWithShadows, "cutOff"), glm::cos(glm::radians(12.5f)));
    glUniform1f(glGetUniformLocation(colorShaderProgramWithShadows, "outerCutOff"), glm::cos(glm::radians(17.5f)));
    glUniform3f(glGetUniformLocation(colorShaderProgramWithShadows, "lightAmbient"), 0.1f, 0.1f, 0.1f);
    
    if (flashlightOn) {
        glUniform3f(glGetUniformLocation(colorShaderProgramWithShadows, "lightDiffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(colorShaderProgramWithShadows, "lightSpecular"), 1.0f, 1.0f, 1.0f);
    } else {
        glUniform3f(glGetUniformLocation(colorShaderProgramWithShadows, "lightDiffuse"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(colorShaderProgramWithShadows, "lightSpecular"), 0.0f, 0.0f, 0.0f);
    }
    
    // Set magical light uniforms
    if (magicalLightOn) {
        glUniform3fv(glGetUniformLocation(colorShaderProgramWithShadows, "magicalLightPos"), 1, &magicalLightPos[0]);
        glUniform3f(glGetUniformLocation(colorShaderProgramWithShadows, "magicalLightColor"), 1.0f, 0.4f, 0.8f);
        glUniform1f(glGetUniformLocation(colorShaderProgramWithShadows, "magicalLightIntensity"), 20.0f);
        glUniform1f(glGetUniformLocation(colorShaderProgramWithShadows, "magicalLightRadius"), 30.0f);
    } else {
        // Turn the light off by setting its intensity and color to 0
        glUniform3f(glGetUniformLocation(colorShaderProgramWithShadows, "magicalLightColor"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(colorShaderProgramWithShadows, "magicalLightIntensity"), 0.0f);
    }
    
    // Set fog uniforms
    GLuint fogColorLoc = glGetUniformLocation(colorShaderProgramWithShadows, "fogColor");
    GLuint fogStartLoc = glGetUniformLocation(colorShaderProgramWithShadows, "fogStart");
    GLuint fogEndLoc = glGetUniformLocation(colorShaderProgramWithShadows, "fogEnd");
    GLuint camPosLoc = glGetUniformLocation(colorShaderProgramWithShadows, "cameraPos");
    
    glUniform3fv(fogColorLoc, 1, &fogColor[0]);
    glUniform1f(fogStartLoc, fogStart);
    glUniform1f(fogEndLoc, fogEnd);
    glUniform3fv(camPosLoc, 1, &cameraPos[0]);
    
    // Set projection and view matrices
    GLuint projectionMatrixLocation = glGetUniformLocation(colorShaderProgramWithShadows, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    GLuint viewMatrixLocation = glGetUniformLocation(colorShaderProgramWithShadows, "viewMatrix");
    glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    
    worldMatrixLocation = glGetUniformLocation(colorShaderProgramWithShadows, "worldMatrix");
    GLuint alphaLocation = glGetUniformLocation(colorShaderProgramWithShadows, "alpha");
    
    // Draw ground
    glUniform1f(alphaLocation, 1.0f);
    glm::mat4 groundWorldMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.01f, 0.0f)) *
                                  glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f, 0.02f, 1000.0f));
    
    glBindVertexArray(floorVAO);
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &groundWorldMatrix[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // ========== RENDER TEXTURED GEOMETRY ==========
    glUseProgram(texturedShaderProgram);
    
    // Set all the uniforms for textured shader (same as before)
    glUniform3f(glGetUniformLocation(texturedShaderProgram, "lightAmbient"), 0.25f, 0.25f, 0.3f);
    glUniform3fv(glGetUniformLocation(texturedShaderProgram, "lightPos"), 1, &flashlightPos[0]);
    glUniform3fv(glGetUniformLocation(texturedShaderProgram, "lightDir"), 1, &cameraFront[0]);
    glUniform1f(glGetUniformLocation(texturedShaderProgram, "cutOff"), glm::cos(glm::radians(12.5f)));
    glUniform1f(glGetUniformLocation(texturedShaderProgram, "outerCutOff"), glm::cos(glm::radians(17.5f)));
    glUniform3f(glGetUniformLocation(texturedShaderProgram, "lightAmbient"), 0.1f, 0.1f, 0.1f);
    
    if (flashlightOn) {
        glUniform3f(glGetUniformLocation(texturedShaderProgram, "lightDiffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(texturedShaderProgram, "lightSpecular"), 1.0f, 1.0f, 1.0f);
        glUniform1f(glGetUniformLocation(texturedShaderProgram, "lightOpacity"), 0.5f);
    } else {
        glUniform3f(glGetUniformLocation(texturedShaderProgram, "lightDiffuse"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(texturedShaderProgram, "lightSpecular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(texturedShaderProgram, "lightOpacity"), 0.0f);
    }
    glUniform1f(glGetUniformLocation(texturedShaderProgram, "shininess"), 80.0f);
    
    // Set magical light uniforms
    if (magicalLightOn) {
        glUniform3fv(glGetUniformLocation(texturedShaderProgram, "magicalLightPos"), 1, &magicalLightPos[0]);
        glUniform3f(glGetUniformLocation(texturedShaderProgram, "magicalLightColor"), 1.0f, 0.4f, 0.8f);
        glUniform1f(glGetUniformLocation(texturedShaderProgram, "magicalLightIntensity"), 25.0f);
        glUniform1f(glGetUniformLocation(texturedShaderProgram, "magicalLightRadius"), 35.0f);
    } else {
        // Turn the light off by setting its intensity and color to 0
        glUniform3f(glGetUniformLocation(texturedShaderProgram, "magicalLightColor"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(texturedShaderProgram, "magicalLightIntensity"), 0.0f);
    }
    
    glUniform3f(glGetUniformLocation(texturedShaderProgram, "flashlightColor"), 1.0f, 1.0f, 0.6f);
    glUniform1f(glGetUniformLocation(texturedShaderProgram, "falloffSmoothness"), 30.0f);
    
    // Set fog uniforms
    fogColorLoc = glGetUniformLocation(texturedShaderProgram, "fogColor");
    fogStartLoc = glGetUniformLocation(texturedShaderProgram, "fogStart");
    fogEndLoc = glGetUniformLocation(texturedShaderProgram, "fogEnd");
    camPosLoc = glGetUniformLocation(texturedShaderProgram, "cameraPos");
    
    glUniform3fv(fogColorLoc, 1, &fogColor[0]);
    glUniform1f(fogStartLoc, fogStart);
    glUniform1f(fogEndLoc, fogEnd);
    glUniform3fv(camPosLoc, 1, &cameraPos[0]);
    
    // Set projection and view matrices
    texturedProjectionMatrixLocation = glGetUniformLocation(texturedShaderProgram, "projectionMatrix");
    glUniformMatrix4fv(texturedProjectionMatrixLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    texturedViewMatrixLocation = glGetUniformLocation(texturedShaderProgram, "viewMatrix");
    glUniformMatrix4fv(texturedViewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
    
    worldMatrixLocation = glGetUniformLocation(texturedShaderProgram, "worldMatrix");
    
    glActiveTexture(GL_TEXTURE0);
    textureLocation = glGetUniformLocation(texturedShaderProgram, "textureSampler");
    
    // Draw mushrooms
    glBindVertexArray(mushroomVAO);
    for (int i = 0; i < 6; ++i) {
        glBindTexture(GL_TEXTURE_2D, mushroomTextureID);
        glm::mat4 mushroomMatrix = glm::translate(glm::mat4(1.0f), mushroomPositions[i]) * 
                                   glm::scale(glm::mat4(1.0f), mushroomScales[i]);
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &mushroomMatrix[0][0]);
        glDrawElements(GL_TRIANGLES, mushroomVertices, GL_UNSIGNED_INT, 0);
    }
    
    // Draw 3D plants
    glBindVertexArray(plantVAO);
    glBindTexture(GL_TEXTURE_2D, plantTextureID);
    for(int i = 0; i < nbPlants; i++){
        mat4 plantMatrix = translate(mat4(1.0f), plantPositions[i]) * scale(mat4(1.0f), plantScales[i]);
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &plantMatrix[0][0]);
        glDrawElements(GL_TRIANGLES, plantVertices, GL_UNSIGNED_INT, 0);
    }
    
    // Draw special plants
    glBindVertexArray(specialPlantVAO);
    glBindTexture(GL_TEXTURE_2D, specialPlantTextureID);
    
    if(!specialPlant1Collected){
        mat4 specialPlant1Matrix = translate(mat4(1.0f), specialPlantPosition1) * scale(mat4(1.0f), vec3(2.0f, 4.0f, 2.0f));
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &specialPlant1Matrix[0][0]);
        glDrawElements(GL_TRIANGLES, specialPlantVertices, GL_UNSIGNED_INT, 0);
    }
    if(!specialPlant2Collected){
        mat4 specialPlant2Matrix = translate(mat4(1.0f), specialPlantPosition2) * scale(mat4(1.0f), vec3(2.0f, 4.0f, 2.0f));
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &specialPlant2Matrix[0][0]);
        glDrawElements(GL_TRIANGLES, specialPlantVertices, GL_UNSIGNED_INT, 0);
    }
    if(!specialPlant3Collected){
        mat4 specialPlant3Matrix = translate(mat4(1.0f), specialPlantPosition3) * scale(mat4(1.0f), vec3(2.0f, 4.0f, 2.0f));
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &specialPlant3Matrix[0][0]);
        glDrawElements(GL_TRIANGLES, specialPlantVertices, GL_UNSIGNED_INT, 0);
    }
    
    // Collection logic
    if(distance(cameraPos, specialPlantPosition1) <= 2){
        specialPlant1Collected = true;
    }
    if(distance(cameraPos, specialPlantPosition2) <= 2){
        specialPlant2Collected = true;
    }
    if(distance(cameraPos, specialPlantPosition3) <= 2){
        specialPlant3Collected = true;
    }
    
    // Draw flower and dragonfly (same as before)
    glm::vec3 stemOffset = glm::vec3(0.5f, -0.5f, 0.0f);
    float angleRadians = glm::radians(angle);
    
    glm::mat4 plantMatrix =
        glm::translate(glm::mat4(1.0f), flowerPosition) *
        glm::translate(glm::mat4(1.0f), stemOffset) *
        glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3(0, 0, 1)) *
        glm::translate(glm::mat4(1.0f), -stemOffset) * 
        glm::scale(mat4(1.0f), vec3(2,2,2));
    
    // Draw flower
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, flowerTextureID);
    glUniform1i(textureLocation, 0);
    
    glBindVertexArray(flowerPlaneVAO);
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &plantMatrix[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Draw dragonfly body
    glm::mat4 dragonflyLocalMatrix = glm::translate(glm::mat4(1.0f), dragonflyPosition - flowerPosition);
    glm::mat4 dragonflyWorldMatrix = plantMatrix * dragonflyLocalMatrix;
    
    glBindTexture(GL_TEXTURE_2D, dragonflyBodyTextureID);
    glBindVertexArray(dragonflyPlaneVAO);
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &dragonflyWorldMatrix[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Draw dragonfly wings with animation
    float flapAngle = 25.0f * glm::abs(sin(glfwGetTime() * 2.0f));
    glm::vec3 wingOffsetFront = wingPositionFront - flowerPosition;
    
    glm::mat4 wingFrontLocalMatrix =
        glm::translate(glm::mat4(1.0f), wingOffsetFront) *
        glm::rotate(glm::mat4(1.0f), glm::radians(flapAngle), glm::vec3(1, 0, 0));
    
    glm::mat4 wingFrontWorldMatrix = plantMatrix * wingFrontLocalMatrix;
    
    glBindTexture(GL_TEXTURE_2D, wingTextureID);
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &wingFrontWorldMatrix[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Back wing
    float flapAngleBack = -12.0f * glm::abs(sin(glfwGetTime() * 2.0f));
    glm::vec3 wingOffsetBack = wingPositionBack - flowerPosition;
    
    glm::mat4 wingBackLocalMatrix =
        glm::translate(glm::mat4(1.0f), wingOffsetBack) *
        glm::rotate(glm::mat4(1.0f), glm::radians(flapAngleBack), glm::vec3(1, 0, 0));
    
    glm::mat4 wingBackWorldMatrix = plantMatrix * wingBackLocalMatrix;
    
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &wingBackWorldMatrix[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
    
    // Handle inputs (same as before)
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fKeyPressed) {
            flashlightOn = !flashlightOn;
            fKeyPressed = true;
        }
    }
    else {
        fKeyPressed = false;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!mKeyPressed) {
            magicalLightOn = !magicalLightOn;
            mKeyPressed = true;
        }
    }
    else {
        mKeyPressed = false;
    }
    
    float cameraSpeed = 2.5 * deltaTime;
    
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
 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec3), (void*)0);
 glEnableVertexAttribArray(0);

 // Color attribute (location 1)
 glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec3), (void*)sizeof(glm::vec3));
 glEnableVertexAttribArray(1);

 // UV coordinate attribute (location 2) - using only X and Y components
 glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec3), (void*)(2*sizeof(glm::vec3)));
 glEnableVertexAttribArray(2);
 
 // Normal attribute (location 3)
 glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 4*sizeof(glm::vec3), (void*)(3*sizeof(glm::vec3)));
 glEnableVertexAttribArray(3);

 glBindBuffer(GL_ARRAY_BUFFER, 0);
 glBindVertexArray(0);

 return vertexArrayObject;
}

void setupShadowMapping() 
{
    // Generate depth map FBO
    glGenFramebuffers(1, &depthMapFBO);
    
    // Create depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Create shadow shader program
    shadowShaderProgram = compileAndLinkShaders(getShadowVertexShaderSource(), getShadowFragmentShaderSource());
    
    // Create new color shader with shadows
    colorShaderProgramWithShadows = compileAndLinkShaders(getVertexShaderSourceWithShadows(), getFragmentShaderSourceWithShadows());
}

void renderSceneForShadows(glm::mat4 lightSpaceMatrix, int floorVAO, glm::mat4 groundWorldMatrix,
                           GLuint mushroomVAO, glm::vec3* mushroomPositions, glm::vec3* mushroomScales,
                           int mushroomCount, int mushroomVertices,
                           GLuint plantVAO, glm::vec3* plantPositions, glm::vec3* plantScales,
                           int plantCount, int plantVertices,
                           GLuint specialPlantVAO, glm::vec3 specialPlantPosition1, glm::vec3 specialPlantPosition2,
                           glm::vec3 specialPlantPosition3, int specialPlantVertices,
                           bool specialPlant1Collected, bool specialPlant2Collected, bool specialPlant3Collected)
{
    glUseProgram(shadowShaderProgram);

    // Set light space matrix
    glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    GLuint worldMatrixLocation = glGetUniformLocation(shadowShaderProgram, "worldMatrix");

    // Render ground (unchanged)
    glBindVertexArray(floorVAO);
    glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &groundWorldMatrix[0][0]);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Render mushrooms (unchanged)
    glBindVertexArray(mushroomVAO);
    for (int i = 0; i < mushroomCount; ++i) {
        glm::mat4 mushroomMatrix = glm::translate(glm::mat4(1.0f), mushroomPositions[i]) *
                                   glm::scale(glm::mat4(1.0f), mushroomScales[i]);
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &mushroomMatrix[0][0]);
        glDrawElements(GL_TRIANGLES, mushroomVertices, GL_UNSIGNED_INT, 0);
    }

    // Render plants (unchanged)
    glBindVertexArray(plantVAO);
    for (int i = 0; i < plantCount; ++i) {
        glm::mat4 plantMatrix = glm::translate(glm::mat4(1.0f), plantPositions[i]) *
                                glm::scale(glm::mat4(1.0f), plantScales[i]);
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &plantMatrix[0][0]);
        glDrawElements(GL_TRIANGLES, plantVertices, GL_UNSIGNED_INT, 0);
    }

    // Render special plants ONLY if they are not collected
    glBindVertexArray(specialPlantVAO);
    if (!specialPlant1Collected) {
        glm::mat4 specialPlant1Matrix = glm::translate(glm::mat4(1.0f), specialPlantPosition1) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 4.0f, 2.0f));
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &specialPlant1Matrix[0][0]);
        glDrawElements(GL_TRIANGLES, specialPlantVertices, GL_UNSIGNED_INT, 0);
    }

    if (!specialPlant2Collected) {
        glm::mat4 specialPlant2Matrix = glm::translate(glm::mat4(1.0f), specialPlantPosition2) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 4.0f, 2.0f));
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &specialPlant2Matrix[0][0]);
        glDrawElements(GL_TRIANGLES, specialPlantVertices, GL_UNSIGNED_INT, 0);
    }

    if (!specialPlant3Collected) {
        glm::mat4 specialPlant3Matrix = glm::translate(glm::mat4(1.0f), specialPlantPosition3) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 4.0f, 2.0f));
        glUniformMatrix4fv(worldMatrixLocation, 1, GL_FALSE, &specialPlant3Matrix[0][0]);
        glDrawElements(GL_TRIANGLES, specialPlantVertices, GL_UNSIGNED_INT, 0);
    }
}