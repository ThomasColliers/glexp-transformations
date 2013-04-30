#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <GL/glew.h>
#include <GL/glfw.h>
#include "berkelium/Berkelium.hpp"

#include "ShaderManager.h"
#include "TriangleBatch.h"
#include "GeometryFactory.h"
#include "MatrixStack.h"
#include "Frustum.h"
#include "TransformPipeline.h"
#include "Math3D.h"

using namespace gliby;
using namespace Math3D;

// TODO: Spawn a sphere
// TODO: Spawn other objects (plane, cube...)
// TODO: Render UI in overlay, create class to do so
// TODO: Create a UI to switch models
// TODO: Create a UI to change matrices

int mouse_x, mouse_y;
int window_w, window_h;
std::string current_path;
// shader stuff
ShaderManager* shaderManager;
GLuint shader;
// transformation stuff
Frame cameraFrame;
Frustum viewFrustum;
TransformPipeline transformPipeline;
MatrixStack modelViewMatrix;
MatrixStack projectionMatrix;
Matrix44f screenSpace;
// ui windows
/*GLTextureWindow* objectPicker;
GLTextureWindow* matrixSettings;*/

void setupContext(void){
    // general state
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // setup transform pipeline
    transformPipeline.setMatrixStacks(modelViewMatrix,projectionMatrix);
    viewFrustum.setPerspective(35.0f, float(window_w)/float(window_h),1.0f,500.0f);
    projectionMatrix.loadMatrix(viewFrustum.getProjectionMatrix());
    modelViewMatrix.loadIdentity();
    cameraFrame.moveForward(-3.0f);
    makeOrthographicMatrix(screenSpace, 0.0f, float(window_w), 0.0f, float(window_h), -1.0f, 1.0f);

    // setup shaders
    std::vector<const char*>* searchPath = new std::vector<const char*>();
    searchPath->push_back("./shaders/");
    searchPath->push_back("/home/ego/projects/personal/gliby/shaders/");
    // TODO: setup shaders, shader attributes

    // setup geometry
    TriangleBatch& sphereBatch = GeometryFactory::sphere(0.2f, 20, 20); 
    // TODO: other objects, initialize texture

    // initialize berkelium
    if(!Berkelium::init(Berkelium::FileString::empty())){
        std::cerr << "Failed to initialize Berkelium!" << std::endl;
    }
    // create berkelium windows
    glActiveTexture(GL_TEXTURE0);
    // TODO: do first with existing class, then move to new namespaced one with more advanced callback support

}

void receiveInput(){
    glfwGetMousePos(&mouse_x, &mouse_y);
}

void keyCallback(int id, int state){
    if(id == GLFW_KEY_ESC && state == GLFW_RELEASE){
        glfwCloseWindow();
    }
}

void charCallback(int character, int action){

}

void mouseCallback(int id, int state){

}

void render(void){
    // update berkelium
    Berkelium::update();

    // setup up camera
    
    // drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // model

    // overlay
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    
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
}

int main(int argc, char **argv){
    // get current path using boost
    current_path = boost::filesystem::system_complete(argv[0]).parent_path().parent_path().string();

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
