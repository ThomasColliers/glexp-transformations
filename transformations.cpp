#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <GL/glew.h>
#include <GL/glfw.h>
#include "berkelium/Berkelium.hpp"
#include <jsoncpp/json/json.h>

#include "ShaderManager.h"
#include "TriangleBatch.h"
#include "Batch.h"
#include "GeometryFactory.h"
#include "MatrixStack.h"
#include "Frustum.h"
#include "TransformPipeline.h"
#include "Math3D.h"
#include "TextureWindow.h"
#include "UIElement.h"
#include "Grid.h"

using namespace gliby;
using namespace Math3D;

// TODO: add a grid and axes to get a better idea of the world
// TODO: mouse support
// TODO: add arbitrary models

int mouse_x, mouse_y;
int window_w, window_h;
// shader stuff
ShaderManager* shaderManager;
GLuint perspectiveShader;
GLuint overlayShader;
// transformation stuff
Frame cameraFrame;
Frustum viewFrustum;
TransformPipeline transformPipeline;
MatrixStack modelViewMatrix;
MatrixStack projectionMatrix;
Matrix44f screenSpace;
Matrix44f objectTransform;
// objects
Geometry* geometry[2];
Grid* grid;
// texture
GLuint object_texture;
// ui windows
UIElement* uiElement;
// state
unsigned int current_geometry = 0;

void matrixUpdate(Json::Value* root){
    Json::Value& ref = *root;
    const Json::Value matrix = ref["matrix"];
    for(uint i = 0; i < matrix.size(); i++){
        if(matrix[i].isNumeric()) objectTransform[i] = matrix[i].asFloat();
        else objectTransform[i] = 0.0f;
    }
    std::cout << "handler reached" << std::endl;
}

void setupContext(void){
    // general state
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // setup transform pipeline
    transformPipeline.setMatrixStacks(modelViewMatrix,projectionMatrix);
    viewFrustum.setPerspective(35.0f, float(window_w)/float(window_h),1.0f,500.0f);
    projectionMatrix.loadMatrix(viewFrustum.getProjectionMatrix());
    modelViewMatrix.loadIdentity();
    loadIdentity44(objectTransform);
    cameraFrame.moveForward(-3.0f);
    makeOrthographicMatrix(screenSpace, 0.0f, float(window_w), 0.0f, float(window_h), -1.0f, 1.0f);

    // setup shaders
    std::vector<const char*>* searchPath = new std::vector<const char*>();
    searchPath->push_back("./shaders/");
    searchPath->push_back("/home/ego/projects/personal/gliby/shaders/");
    shaderManager = new ShaderManager(searchPath);
    ShaderAttribute attrs[] = {{0,"vVertex"},{3,"vTexCoord"}};
    perspectiveShader = shaderManager->buildShaderPair("simple_perspective.vp","simple_perspective.fp",sizeof(attrs)/sizeof(ShaderAttribute),attrs);
    overlayShader = shaderManager->buildShaderPair("overlay.vp","overlay.fp",sizeof(attrs)/sizeof(ShaderAttribute),attrs);

    // setup geometry
    TriangleBatch& sphereBatch = GeometryFactory::sphere(0.4f, 40, 40); 
    geometry[0] = &sphereBatch;
    Batch& cubeBatch = GeometryFactory::cube(0.4f);
    geometry[1] = &cubeBatch;
    grid = new Grid(shaderManager,1.0f,10);

    // setup object texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &object_texture);
    glBindTexture(GL_TEXTURE_2D, object_texture);
    glfwLoadTexture2D("icemoon.tga",0);
    GLfloat largest_anisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_anisotropy);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_anisotropy);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    // initialize berkelium
    if(!Berkelium::init(Berkelium::FileString::empty())){
        std::cerr << "Failed to initialize Berkelium!" << std::endl;
    }

    uiElement = new UIElement(300,200,10,10,screenSpace,window_w,window_h,overlayShader,true,true);
    uiElement->getWindow().registerCallback(L"matrixUpdate",&matrixUpdate);
    uiElement->load("page.html");
}

void receiveInput(){
    glfwGetMousePos(&mouse_x, &mouse_y);
    uiElement->mouseUpdate(mouse_x,mouse_y);
}

void keyCallback(int id, int state){
    // close window on esc
    if(id == GLFW_KEY_ESC && state == GLFW_RELEASE){
        glfwCloseWindow();
    }
    // switch geometry on space
    if(id == GLFW_KEY_SPACE && state == GLFW_RELEASE){
        ++current_geometry;
        if(current_geometry > (sizeof(geometry)/sizeof(Geometry*))-1) current_geometry = 0;
    }
    // pass to ui elements
    bool shift_down = glfwGetKey(287) == GLFW_PRESS;
    bool ctrl_down = glfwGetKey(289) == GLFW_PRESS;
    bool alt_down = glfwGetKey(291) == GLFW_PRESS;
    uiElement->keyEvent(id,state,shift_down,ctrl_down,alt_down);
}

void charCallback(int character, int action){
    uiElement->charEvent(character,action);
}

void mouseCallback(int id, int state){
    uiElement->mouseEvent(id,state);
}

void render(void){
    // update berkelium
    Berkelium::update();

    // drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // setup up camera
    cameraFrame.moveForward(3.0f);
    cameraFrame.rotateWorld(0.01f, 0.0f, 1.0f, 0.0f);
    cameraFrame.moveForward(-3.0f);
    Matrix44f mCamera;
    cameraFrame.getCameraMatrix(mCamera);

    // set up camera transformation
    modelViewMatrix.pushMatrix();
    modelViewMatrix.multMatrix(mCamera);

    // model
    modelViewMatrix.pushMatrix();
    modelViewMatrix.multMatrix(objectTransform);
    glUseProgram(perspectiveShader);
    glBindTexture(GL_TEXTURE_2D, object_texture);
    glUniformMatrix4fv(glGetUniformLocation(perspectiveShader,"mvpMatrix"), 1, GL_FALSE, transformPipeline.getModelViewProjectionMatrix()); // TODO: should cache the uniform location
    glUniform1i(glGetUniformLocation(perspectiveShader,"textureUnit"), 0);
    geometry[current_geometry]->draw();
    modelViewMatrix.popMatrix();

    // grid
    GLfloat color[] = {1.0f,1.0f,1.0f,0.8f};
    grid->draw(transformPipeline.getModelViewProjectionMatrix(),color);

    // pop off transformations
    modelViewMatrix.popMatrix();

    // overlay
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(overlayShader);
    uiElement->draw();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void resizeCallback(int width, int height){
    window_w = width;
    window_h = height;
    glViewport(0,0,window_w,window_h);
    // update projection matrix
    viewFrustum.setPerspective(35.0f, float(window_w)/float(window_h),1.0f,500.0f);
    projectionMatrix.loadMatrix(viewFrustum.getProjectionMatrix());
    // update orthographic matrix
    makeOrthographicMatrix(screenSpace, 0.0f, float(window_w), 0.0f, float(window_h), -1.0f, 1.0f);
    // update UI elements
    if(uiElement) uiElement->resize(width,height);
}

int main(int argc, char **argv){
    // even with glfwSwapInterval set I get a fixed tear line so this env var will force the driver to vsync properly
    putenv((char*) "__GL_SYNC_TO_VBLANK=1");

    // init glfw and window
    if(!glfwInit()){
        std::cerr << "GLFW init failed" << std::endl;
        return -1;
    }
    glfwSwapInterval(1);
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 8);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 4);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    if(!glfwOpenWindow(800,600,8,8,8,0,24,0,GLFW_WINDOW)){
        std::cerr << "GLFW window opening failed" << std::endl;
        return -1;
    }
    window_w = 800; window_h = 600;
    glfwSetKeyCallback(keyCallback);
    glfwSetCharCallback(charCallback);
    glfwSetMouseButtonCallback(mouseCallback);
    glfwSetWindowSizeCallback(resizeCallback);
    glfwSetWindowTitle("gltest");

    // init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(err != GLEW_OK){
        std::cerr << "Glew error: " << glewGetErrorString(err) << std::endl;
    }

    // setup context
    setupContext();

    // main loop
    while(glfwGetWindowParam(GLFW_OPENED)){
        receiveInput();
        render();
        glfwSwapBuffers();
    }

    Berkelium::destroy();
    glfwTerminate();
}
