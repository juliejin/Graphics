      /*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: lingjin
*/

#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"
#include<iostream>
#include <vector>
using namespace std;

#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;
OpenGLMatrix* matrix;
GLuint buffer;
BasicPipelineProgram* pipeLine;
void bindProgram();
void renderQuad();
GLuint program;
GLuint vaoPoint; //points
GLuint vaoSolid; //triangle
GLuint vaoWireFrame; //lines
GLuint BufferWireFrame;
GLuint BufferSolid;


//float vertices[9] = {0,0,-1,1,0,-1,0,1,-1};
vector<float> vertexPosition;
vector<float> vertexPositionSolid;
vector<float> vertexPositionWireFrame;
vector<float> colorArray;
vector<float> colorArraySolid;
vector<float> colorArrayWireFrame;
int mode = 0;
//const float rgbCoefficients[3] = {0.299f, 0.587f, 0.114f};
vector<float> getGrayValue(int i,int j);

bool beginAnimation = false;
bool beginScreenshot= false;
void makeAnimation();
void makeScreenshot();
int frame = 0;
const GLfloat fillColor[4] = {1.0, 1.0, 0.0, 0.0};
const GLfloat frameColor[4] = {1.0, 0.0, 0.0, 0.0};
bool scaleDown = true;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void displayFunc()
{
    
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    matrix->LoadIdentity();
    // matrix->LookAt(0, 0, 300.7817231611, 0, 0, -1, 0, 1, 0);
    matrix->LookAt(0, 250, 0, 128, 60, -128, 0, 1, 0);
    matrix->Rotate(landRotate[0], 1.0, 0.0, 0.0);
    matrix->Rotate(landRotate[1], 0.0, 1.0, 0.0);
    matrix->Rotate(landRotate[2], 0.0, 0.0, 1.0);
    matrix->Scale(landScale[0], landScale[1], landScale[2]);
    
    // get a handle to the program
    //pipeLine->Bind();
    program = pipeLine->GetProgramHandle();
    // get a handle to the modelViewMatrix shader variable
    GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
    // *** PPT3-Setting the Model-view Matrix: Core Profile ***//
    //matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    //matrix->LoadIdentity();
    float m[16];
    matrix->GetMatrix(m);
    glUniformMatrix4fv(h_modelViewMatrix, 1, GL_FALSE, m);
    
    
    // get a handle to the projectionMatrix shader variable
    GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    //matrix->LoadIdentity();
    float p[16];
    matrix->GetMatrix(p);
    glUniformMatrix4fv(h_projectionMatrix, 1, GL_FALSE, p);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    bindProgram();
    renderQuad();

    
  // render some stuff...
    glutSwapBuffers();
}


//make object to rotate, translate and scale up/down
void makeAnimation(){
    landRotate[0] +=2.0f;
    landRotate[1] +=2.0f;
    landRotate[2] += 2.0f;
    
    if(landTranslate[2]>0&&landTranslate[2]<0.6f)
    landTranslate[2] +=0.5f;
    else if(landTranslate[2]<=2)
    landTranslate[2]-=0.5f;
    
    
    if(landScale[0]>=1.0f){
        scaleDown=true;
    }
    if(landScale[0]<=0.2f){
        scaleDown=false;
    }
    
    if(!scaleDown){
        landScale[0] *= 1.01f;
        landScale[1] *= 1.01f;
        landScale[2] *= 1.01f;
    }else{
        landScale[0] *= 0.99f;
        landScale[1] *= 0.99f;
        landScale[2] *= 0.99f;
    }
    
    
    if (landRotate[2] > 360.0){
        landRotate[2] -= 360.0;
    }
    if (landRotate[1] > 360.0){
        landRotate[1] -= 360.0;
    }
    if (landRotate[0] > 360.0){
        landRotate[0] -= 360.0;
    }
    
}

void idleFunc()
{
  // do some stuff... 
   
  // for example, here, you can save the screenshots to disk (to make the animation)
    if(beginAnimation==true){
        makeAnimation();
    }
    if(beginScreenshot==true){
        makeScreenshot();
    }
  // make the screen update 
  glutPostRedisplay();
}

void makeScreenshot(){
    if (beginScreenshot && frame < 300) {
        frame++;
        saveScreenshot((std::to_string(frame)+".jpg").c_str());
    } else {
       beginScreenshot = false;
    }	
}

void reshapeFunc(int w, int h)
{
 // glViewport(0, 0, w, h);
  //projection
    GLfloat aspect = (GLfloat) w / (GLfloat) h;
    glViewport(0, 0, w, h);
    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    matrix->LoadIdentity();
    matrix->Perspective(90.0, 1.0*aspect, 0.01, 1000.0);
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  // setup perspective matrix...
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
        //matrix->Rotate(theta[0], 1.0, 0.0, 0.0);
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
    case 'a':
          mode = 0;
          break;
    case 'b':
          mode = 1;
          break;
    case 'c':
          mode = 2;
          break;
    case 'd':
          beginAnimation=true;
          beginScreenshot = true;
          break;
    case 'e':
          beginAnimation=false;
          beginScreenshot=false;
    break;
  }
}

vector<float> getGrayValue(int i,int j){
    vector<float> colorValue;
    if (heightmapImage->getBytesPerPixel() == 3) {
        for (int channel = 0; channel < heightmapImage->getBytesPerPixel(); channel++) {
            colorValue.push_back( heightmapImage->getPixel(i, j, channel)/255.0);
        }
        
    } else {
        //grayValue = heightmapImage->getPixel(i, j, 0);
        colorValue.push_back(heightmapImage->getPixel(i, j, 0)/255.0);
        colorValue.push_back(heightmapImage->getPixel(i, j, 0)/255.0);
        colorValue.push_back(heightmapImage->getPixel(i, j, 0)/255.0);

    }
    //cout<<colorValue[0]<<" ";
    //cout<<colorValue[1]<<" ";
    //cout<<colorValue[2]<<endl;
    return colorValue;
}

void initScene(int argc, char *argv[])
{
  // load the image from a jpeg disk file to main memory
  heightmapImage = new ImageIO();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, heightmapImage->getWidth(), heightmapImage->getHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, heightmapImage->getPixels());

glClearColor(120.0 / 255.0, 180 / 255.0, 95.0 / 255.0, 0.0);
  //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    
    glEnable(GL_DEPTH_TEST);
    
    matrix = new OpenGLMatrix();
    //float colors[3][4] =
   // {{1.0, 0.0, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0}};
   // int bytesPerPixel = heightmapImage->getBytesPerPixel();
    
    
    for (int i = 0;i< heightmapImage->getWidth();i++){
        for(int j=0;j<heightmapImage->getHeight();j++){
            
           vector<float> colorValue=getGrayValue(i,j);
            vertexPosition.push_back((float)i);
            vertexPosition.push_back((float)heightmapImage->getPixel(i,j,0)*0.5f);//current system 100%
            vertexPosition.push_back((float)-j);
            /*colorArray.push_back((float)heightmapImage->getPixel(i,j,0));
            colorArray.push_back((float)heightmapImage->getPixel(i,j,1));
            colorArray.push_back((float)heightmapImage->getPixel(i,j,2));
             */
            /*colorArray.push_back(1-1.0f * grayValue / 255.0f);
            colorArray.push_back(1-1.0f * grayValue / 255.0f);
            colorArray.push_back(1-0.0f * grayValue / 255.0f);*/
            colorArray.push_back(colorValue[0]);
            colorArray.push_back(colorValue[1]);
            colorArray.push_back(colorValue[2]);
            colorArray.push_back(0.0f);
        }
    }
    
    
    
    //triangle vertices
    for (int i = 0;i<heightmapImage->getWidth()-1;i++){
        for(int j=0;j<heightmapImage->getHeight()-1;j++){
                vertexPositionSolid.push_back((float)i);
                vertexPositionSolid.push_back((float)heightmapImage->getPixel(i,j,0)*0.2f);//current system 100%
                vertexPositionSolid.push_back((float)-j);
                /*grayValue = getGrayValue(i,j);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-1.0f * grayValue / 255.0f);*/
                vector<float> colorValue=getGrayValue(i,j);
                colorArraySolid.push_back(colorValue[0]);
                colorArraySolid.push_back(colorValue[1]);
                colorArraySolid.push_back(colorValue[2]);
                colorArraySolid.push_back(0.0f);
            
                vertexPositionSolid.push_back((float)i+1);
                vertexPositionSolid.push_back((float)heightmapImage->getPixel(i+1,j,0)*0.2f);//current system 100%
                vertexPositionSolid.push_back((float)-j);
                //grayValue = getGrayValue(i+1,j);
                /*colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-1.0f * grayValue / 255.0f);*/
                colorValue=getGrayValue(i+1,j);
                colorArraySolid.push_back(colorValue[0]);
                colorArraySolid.push_back(colorValue[1]);
                colorArraySolid.push_back(colorValue[2]);
                colorArraySolid.push_back(0.0f);

                vertexPositionSolid.push_back((float)i+1);
                vertexPositionSolid.push_back((float)heightmapImage->getPixel(i+1,j+1,0)*0.2f);//current system 100%
                vertexPositionSolid.push_back((float)-(j+1));
                /*grayValue = getGrayValue(i+1,j+1);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-1.0f * grayValue / 255.0f);
                 */
               colorValue=getGrayValue(i+1,j+1);
                colorArraySolid.push_back(colorValue[0]);
                colorArraySolid.push_back(colorValue[1]);
                colorArraySolid.push_back(colorValue[2]);
                colorArraySolid.push_back(0.0f);
            
            
            
                vertexPositionSolid.push_back((float)i);
                vertexPositionSolid.push_back((float)heightmapImage->getPixel(i,j,0)*0.2f);//current system 100%
                vertexPositionSolid.push_back((float)-j);
               /* grayValue = getGrayValue(i,j);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-1.0f * grayValue / 255.0f);*/
            colorValue=getGrayValue(i,j);
            colorArraySolid.push_back(colorValue[0]);
            colorArraySolid.push_back(colorValue[1]);
            colorArraySolid.push_back(colorValue[2]);
                colorArraySolid.push_back(0.0f);
            
                vertexPositionSolid.push_back((float)i);
                vertexPositionSolid.push_back((float)heightmapImage->getPixel(i,j+1,0)*0.2f);//current system 100%
                vertexPositionSolid.push_back((float)-(j+1));
                /*grayValue = getGrayValue(i,j+1);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-1.0f * grayValue / 255.0f);
                 */
                colorValue=getGrayValue(i,j+1);
                colorArraySolid.push_back(colorValue[0]);
                colorArraySolid.push_back(colorValue[1]);
                colorArraySolid.push_back(colorValue[2]);
                colorArraySolid.push_back(0.0f);
                
                vertexPositionSolid.push_back((float)i+1);
                vertexPositionSolid.push_back((float)heightmapImage->getPixel(i+1,j+1,0)*0.2f);//current system 100%
                vertexPositionSolid.push_back((float)-(j+1));
                /*grayValue = getGrayValue(i+1,j+1);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-0.0f * grayValue / 255.0f);
                colorArraySolid.push_back(1-1.0f * grayValue / 255.0f);*/
            colorValue=getGrayValue(i+1,j+1);
            colorArraySolid.push_back(colorValue[0]);
            colorArraySolid.push_back(colorValue[1]);
            colorArraySolid.push_back(colorValue[2]);
                colorArraySolid.push_back(0.0f);
            
        }
    }
    
    
    //line vertices
    for (int i = 0;i< heightmapImage->getWidth();i++){
        for(int j=0;j<heightmapImage->getHeight();j++){
                if(i != heightmapImage->getWidth()-1){
                    vertexPositionWireFrame.push_back((float)i);
                    vertexPositionWireFrame.push_back((float)heightmapImage->getPixel(i,j,0)*0.5f);//current system 100%
                    vertexPositionWireFrame.push_back((float)-j);
                    /*grayValue=getGrayValue(i,j);
                    colorArrayWireFrame.push_back(1-1.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);*/
                    vector<float> colorValue=getGrayValue(i,j);
                    colorArrayWireFrame.push_back(colorValue[0]);
                    colorArrayWireFrame.push_back(colorValue[1]);
                    colorArrayWireFrame.push_back(colorValue[2]);
                    colorArrayWireFrame.push_back(0.0f);
                    
                    vertexPositionWireFrame.push_back((float)i+1);
                    vertexPositionWireFrame.push_back((float)heightmapImage->getPixel(i+1,j,0)*0.5f);//current system 100%
                    vertexPositionWireFrame.push_back((float)-j);
                   /* grayValue=getGrayValue(i+1,j);
                    colorArrayWireFrame.push_back(1-1.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);*/
                    colorValue=getGrayValue(i+1,j);
                    colorArrayWireFrame.push_back(colorValue[0]);
                    colorArrayWireFrame.push_back(colorValue[1]);
                    colorArrayWireFrame.push_back(colorValue[2]);
                    colorArrayWireFrame.push_back(0.0f);
                }
                if(j!= heightmapImage->getHeight()-1){
                    vertexPositionWireFrame.push_back((float)i);
                    vertexPositionWireFrame.push_back((float)heightmapImage->getPixel(i,j,0)*0.5f);//current system 100%
                    vertexPositionWireFrame.push_back((float)-j);
                    /*grayValue=getGrayValue(i,j);
                    colorArrayWireFrame.push_back(1-1.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);*/
                    vector<float> colorValue=getGrayValue(i,j);
                    colorArrayWireFrame.push_back(colorValue[0]);
                    colorArrayWireFrame.push_back(colorValue[1]);
                    colorArrayWireFrame.push_back(colorValue[2]);
                    colorArrayWireFrame.push_back(0.0f);
                    
                    vertexPositionWireFrame.push_back((float)i);
                    vertexPositionWireFrame.push_back((float)heightmapImage->getPixel(i,j+1,0)*0.5f);//current system 100%
                    vertexPositionWireFrame.push_back((float)-(j+1));
                    /*grayValue=getGrayValue(i,j+1);
                    colorArrayWireFrame.push_back(1-1.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);
                    colorArrayWireFrame.push_back(1-0.0f * grayValue / 255.0f);*/
                    colorValue=getGrayValue(i,j+1);
                    colorArrayWireFrame.push_back(colorValue[0]);
                    colorArrayWireFrame.push_back(colorValue[1]);
                    colorArrayWireFrame.push_back(colorValue[2]);
                    colorArrayWireFrame.push_back(0.0f);
                }
        }
    }
    
    
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(vertexPosition.size()+colorArray.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertexPosition.size()*sizeof(float), vertexPosition.data());//sizeof(float)*vector.size();
    glBufferSubData(GL_ARRAY_BUFFER, vertexPosition.size()*sizeof(float),
                        colorArray.size()*sizeof(float), colorArray.data());
    
   glGenBuffers(1, &BufferWireFrame);
    glBindBuffer(GL_ARRAY_BUFFER, BufferWireFrame);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(vertexPositionWireFrame.size()+colorArrayWireFrame.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertexPositionWireFrame.size()*sizeof(float), vertexPositionWireFrame.data());//sizeof(float)*vector.size();
    glBufferSubData(GL_ARRAY_BUFFER, vertexPositionWireFrame.size()*sizeof(float),
                    colorArrayWireFrame.size()*sizeof(float), colorArrayWireFrame.data());
    
    
    glGenBuffers(1, &BufferSolid);
    glBindBuffer(GL_ARRAY_BUFFER, BufferSolid);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(vertexPositionSolid.size()+colorArraySolid.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertexPositionSolid.size()*sizeof(float), vertexPositionSolid.data());//sizeof(float)*vector.size();
    glBufferSubData(GL_ARRAY_BUFFER, vertexPositionSolid.size()*sizeof(float),
                    colorArraySolid.size()*sizeof(float), colorArraySolid.data());
    
    //glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB| GLUT_DEPTH);
    pipeLine = new BasicPipelineProgram();
    pipeLine->Init("../openGLHelper-starterCode");
    pipeLine->Bind();
    
    
  // do additional initialization here...
}
void renderQuad()
{
    if(mode==0){
        glBindVertexArray(vaoPoint);
        GLint first = 0;
        //pipeLine->SetColor(frameColor);
        GLsizei numberOfVertices = heightmapImage->getHeight()*heightmapImage->getWidth();
        glDrawArrays( GL_POINTS, first, numberOfVertices);
        //glDrawElements(GL_POINTS, numberOfVertices,GL_FLOAT,&vertexPosition);
    }else if(mode ==1){
        glBindVertexArray(vaoSolid);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        GLint first = 0;
       // pipeLine->SetColor(fillColor);
        GLsizei numberOfVertices = (heightmapImage->getHeight()-1)*(heightmapImage->getWidth()-1)*6;
        glDrawArrays(GL_TRIANGLES, first, numberOfVertices);
        
         
    }else if (mode ==2){
        glBindVertexArray(vaoWireFrame);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        GLint first = 0;
       // pipeLine->SetColor(frameColor);
        GLsizei numberOfVertices = ((heightmapImage->getHeight()-1)*heightmapImage->getWidth()+
                                    heightmapImage->getHeight()*(heightmapImage->getWidth()-1))*2;
        glDrawArrays(GL_LINES, first, numberOfVertices);
    }
    glBindVertexArray(0);
    
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw 
  glutDisplayFunc(displayFunc);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}

void bindProgram()
{
    // bind our buffer, so that glVertexAttribPointer refers
    if(mode==0){
        glGenVertexArrays(1, &vaoPoint);
        glBindVertexArray(vaoPoint);
        program = pipeLine->GetProgramHandle();
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        GLuint loc = glGetAttribLocation(program, "position");
        glEnableVertexAttribArray(loc);
        const void * offset = (const void*) 0;
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
        GLuint loc2 = glGetAttribLocation(program, "color");
        glEnableVertexAttribArray(loc2);
        const void * offset1 = (const void*)(vertexPosition.size()*sizeof(float));
        glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset1);
    }else if(mode ==1){
        glGenVertexArrays(1, &vaoSolid);
        glBindVertexArray(vaoSolid);
        program = pipeLine->GetProgramHandle();
        glBindBuffer(GL_ARRAY_BUFFER, BufferSolid);
        GLuint loc = glGetAttribLocation(program, "position");
        glEnableVertexAttribArray(loc);
        const void * offset = (const void*) 0;
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
        GLuint loc2 = glGetAttribLocation(program, "color");
        glEnableVertexAttribArray(loc2);
        const void * offset1 = (const void*)(vertexPositionSolid.size()*sizeof(float));
        glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset1);
    }else if (mode ==2){
        glGenVertexArrays(1, &vaoWireFrame);
        glBindVertexArray(vaoWireFrame);
        program = pipeLine->GetProgramHandle();
        glBindBuffer(GL_ARRAY_BUFFER, BufferWireFrame);
        GLuint loc = glGetAttribLocation(program, "position");
        glEnableVertexAttribArray(loc);
        const void * offset = (const void*) 0;
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
        GLuint loc2 = glGetAttribLocation(program, "color");
        glEnableVertexAttribArray(loc2);
        const void * offset1 = (const void*)(vertexPositionWireFrame.size()*sizeof(float));
        glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset1);
    }
    
   
    glBindVertexArray(0);
}


