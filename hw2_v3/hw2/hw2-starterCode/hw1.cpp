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
//#include "SplinePipelineProgram.h"
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

struct Point
{
    double x;
    double y;
    double z;
};

struct FPoint {
    float x;
    float y;
    float z;
};

struct Spline
{
    int numControlPoints;
    Point * points;
};

ImageIO * heightmapImage;
OpenGLMatrix* matrix;
GLuint buffer;
BasicPipelineProgram* pipeLine;
//SplinePipelineProgram* splinePipeline;
void bindProgram();
void renderQuad();
GLuint program;

vector<float> groundVertices;
vector<float> groundUVs;
GLuint vaoGround; //points
GLuint groundBuffer;
GLuint groundTexture;
GLuint groundUVBuffer;

vector<float> skyVertices;
vector<float> skyUVs;
GLuint vaoSky;
GLuint skyTexture;
GLuint skyBuffer;
GLuint skyUVBuffer;

//vector<float> rollerCoasterVertices;
//vector<float> rollerCoasterColor;
GLuint vaoPoint;
GLuint splineBuffer;
GLuint splineTexture;

vector<float> getGrayValue(int i,int j);

bool beginAnimation = false;
bool beginScreenshot= false;
void makeAnimation();
void makeScreenshot();
int frame = 0;
const float fillColor[4] = {1.0, 1.0, 0.0, 0.0};
const float frameColor[4] = {1.0, 0.0, 0.0, 0.0};
bool scaleDown = true;

Spline * splines;
vector<FPoint> splinePoints;
// total number of splines
int numSplines;
int loadSplines(char * argv);
int initTexture(const char * imageFilename, GLuint textureHandle);
vector<float> splineColor;
vector<float> splinePoint;
Point calculatePoints(Point* point, float n, int j);
// write a screenshot to the specified filename
int cameraStep =0;

void setTextureUnit(GLint unit)
{
    glActiveTexture(unit); // select the active texture unit
    // get a handle to the “textureImage” shader variable
    GLint h_textureImage = glGetUniformLocation(program, "textureImage");
    // deem the shader variable “textureImage” to read from texture unit “unit”
    glUniform1i(h_textureImage, unit - GL_TEXTURE0);
}

FPoint computeNormal (const FPoint& unitTangent) {
    
    FPoint normal;
    normal.x = (unitTangent.y * 0 - unitTangent.z * 1);
    normal.y = (unitTangent.x * 0 - unitTangent.z * 0);
    normal.z = (unitTangent.x * 1 - unitTangent.y * 0);
    
    FPoint binormal;
    binormal.x = (normal.y * unitTangent.z - normal.z * unitTangent.y);
    binormal.y = -(normal.x * unitTangent.z - normal.z * unitTangent.x);
    binormal.z = (normal.x * unitTangent.y - normal.y * unitTangent.z);
    
    return binormal;
}

void computeCameraPosition (const FPoint& point0, const FPoint& point1, const FPoint& point2, const FPoint& point3) {
    
    const float u = 0.001f;
    FPoint tangent;
    tangent.x = 0.5f * (
                        (-point0.x + point2.x) +
                        (2 * point0.x - 5 * point1.x + 4 * point2.x - point3.x) * (2 * u) +
                        (-point0.x + 3 * point1.x - 3 * point2.x + point3.x) * (3 * u * u)
                        );
    
    tangent.y = 0.5f * (
                        (-point0.y + point2.y) +
                        (2 * point0.y - 5 * point1.y + 4 * point2.y - point3.y) * (2 * u) +
                        (-point0.y + 3 * point1.y - 3 * point2.y + point3.y) * (3 * u * u)
                        );
    
    tangent.z = 0.5f * (
                        (-point0.z + point2.z) +
                        (2 * point0.z - 5 * point1.z + 4 * point2.z - point3.z) * (2 * u) +
                        (-point0.z + 3 * point1.z - 3 * point2.z + point3.z) * (3 * u * u)
                        );
    
    // Compute the unit tangent
    float magnitude = std::sqrt (std::pow (tangent.x, 2) + std::pow (tangent.y, 2) + std::pow (tangent.z, 2));
    FPoint unitTangent;
    unitTangent.x = tangent.x / magnitude;
    unitTangent.y = tangent.y / magnitude;
    unitTangent.z = tangent.z / magnitude;
    
    // Compute the normal
    FPoint normal = computeNormal (unitTangent);
    
    // Set up our matrices -- start with the ModelView matrix default value
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    matrix->LoadIdentity();									// Set up the model matrix
    matrix->LookAt(
                   point0.x, point0.y + 0.5f, point0.z,
                   point0.x + unitTangent.x, point0.y + unitTangent.y, point0.z + unitTangent.z,
                   normal.x, normal.y, normal.z
                   );
}

void computeCameraForRide (int index) {
    FPoint point1 = splinePoints[index];
    FPoint point2 = splinePoints[index + 1];
    FPoint point3 = splinePoints[index + 2];
    FPoint point4 = splinePoints[index + 3];
    
    computeCameraPosition (point1, point2, point3, point4);
}

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
   // matrix->LookAt(0, 0, 40, 0, 0, -1, 0, 1, 0);
    if (cameraStep < splinePoints.size () - 3) {
        computeCameraForRide (cameraStep);
        cameraStep ++;
    }
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
    
   // bindProgram();
    
    renderQuad();
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
    if(beginScreenshot==true){
        makeScreenshot();
    }
    
    /*if (cameraStep < splinePoints.size () - 3) {
        computeCameraForRide (cameraStep);
        cameraStep ++;
    }*/
  // make the screen update 
  glutPostRedisplay();
}

void makeScreenshot(){
    if (beginScreenshot && frame < 1000) {
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
    /*case 'a':
          mode = 0;
          break;
    case 'b':
          mode = 1;
          break;
    case 'c':
          mode = 2;
          break;*/
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

Point calculatePoints(Point* point, float n,int j){
    Point returnPoint;
    
    float tempA = n*n*n*(-0.5)+n*n+ (-0.5)*n+0.0;
    float tempB = n*n*n*(1.5)+n*n*(-2.5)+1.0;
    float tempC = n*n*n*(-1.5)+n*n*2.0+ 0.5*n;
    float tempD = n*n*n*0.5+n*n*(-0.5);
    
    returnPoint.x = tempA*point[j].x+tempB*point[j+1].x+tempC*point[j+2].x+tempD*point[j+3].x;
    returnPoint.y = tempA*point[j].y+tempB*point[j+1].y+tempC*point[j+2].y+tempD*point[j+3].y;
    returnPoint.z = tempA*point[j].z+tempB*point[j+1].z+tempC*point[j+2].z+tempD*point[j+3].z;
    return returnPoint;
}

void initScene(int argc, char *argv[])
{

//glClearColor(120.0 / 255.0, 180 / 255.0, 95.0 / 255.0, 0.0);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    
    glEnable(GL_DEPTH_TEST);
    
    matrix = new OpenGLMatrix();
  
    
  /* for (int i=0; i<numSplines; i++) {
        for (int j=0; j<splines[i].numControlPoints-3; j++){
            for(float k=0.0;k<1.0;k+=0.01){
                Point point = calculatePoints(splines[i].points,k,j);
                
                splinePoint.push_back(point.x);
                splinePoint.push_back(point.y);
                splinePoint.push_back(point.z);
               // cout<<point.x<<" "<<point.y<<" "<<point.z<<endl;
                splineColor.push_back(1.0);
                splineColor.push_back(0.0);
                splineColor.push_back(1.0);
                splineColor.push_back(0.0);
            }
        }
    }*/
    
    
    //glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB| GLUT_DEPTH);
    pipeLine = new BasicPipelineProgram();
    pipeLine->Init("../openGLHelper-starterCode");
    pipeLine->Bind();
    program = pipeLine->GetProgramHandle();
    bindProgram();
    computeCameraForRide(0);
    
    /*glGenBuffers(1, &rollerCoasterBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, rollerCoasterBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(splinePoint.size()+splineColor.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    splinePoint.size()*sizeof(float), splinePoint.data());//sizeof(float)*vector.size();
    glBufferSubData(GL_ARRAY_BUFFER, splinePoint.size()*sizeof(float),
                    splineColor.size()*sizeof(float), splineColor.data());
    
    glGenVertexArrays(1, &vaoPoint);
    glBindVertexArray(vaoPoint);
    program = pipeLine->GetProgramHandle();
    glBindBuffer(GL_ARRAY_BUFFER, rollerCoasterBuffer);
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    const void * offset = (const void*) 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
    GLuint loc2 = glGetAttribLocation(program, "color");
    glEnableVertexAttribArray(loc2);
    const void * offset1 = (const void*)(splinePoint.size()*sizeof(float));
    glVertexAttribPointer(loc2, 4, GL_FLOAT, GL_FALSE, 0, offset1);*/
    
  // do additional initialization here...
}
void renderQuad()
{
    /*draw ground*/
    setTextureUnit(GL_TEXTURE0);
    glBindVertexArray(vaoGround);
    glBindTexture(GL_TEXTURE_2D, groundTexture);
    glDrawArrays(GL_TRIANGLES, 0, groundVertices.size()/3);
    glBindVertexArray(0);
    
    /***Draw Sky***/
    setTextureUnit(GL_TEXTURE0);
    glBindVertexArray(vaoSky);
    glBindTexture(GL_TEXTURE_2D, skyTexture);
    glDrawArrays(GL_TRIANGLES, 0, skyVertices.size()/3);
    
    glBindVertexArray(vaoPoint);
    GLint first = 0;
    GLsizei numberOfVertices = splinePoint.size();
    glBindTexture(GL_TEXTURE_2D, splineTexture);
    glDrawArrays(GL_TRIANGLES, first, numberOfVertices/3);
    
   glBindVertexArray(0);
    
    
}

int main (int argc, char ** argv)
{
    if (argc<2)
    {
        printf ("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }
    
    // load the splines from the provided filename
    loadSplines(argv[1]);
    
    printf("Loaded %d spline(s).\n", numSplines);
    for(int i=0; i<numSplines; i++)
        printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);
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

    
    return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
    // read the texture image
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);
    
    if (err != ImageIO::OK)
    {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }
    
    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }
    
    // allocate space for an array of pixels
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA
    
    // fill the pixelsRGBA array with the image pixels
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque
            
            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }
    
    // bind the texture
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    
    // initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);
    
    // generate the mipmaps for this texture
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // query support for anisotropic texture filtering
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // set anisotropic texture filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);
    
    // query for any errors
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }
    
    // de-allocate the pixel array -- it is no longer needed
    delete [] pixelsRGBA;
    
    return 0;
}

int loadSplines(char * argv)
{
    char * cName = (char *) malloc(128 * sizeof(char));
    FILE * fileList;
    FILE * fileSpline;
    int iType, i = 0, j, iLength;
    
    // load the track file
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf ("can't open file\n");
        exit(1);
    }
    
    // stores the number of splines in a global variable
    fscanf(fileList, "%d", &numSplines);
    
    splines = (Spline*) malloc(numSplines * sizeof(Spline));
    
    // reads through the spline files
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        
        if (fileSpline == NULL)
        {
            printf ("can't open file\n");
            exit(1);
        }
        
        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);
        
        // allocate memory for all the points
        splines[j].points = (Point *)malloc(iLength * sizeof(Point));
        splines[j].numControlPoints = iLength;
        
        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
                      &splines[j].points[i].x,
                      &splines[j].points[i].y,
                      &splines[j].points[i].z) != EOF)
        {
            i++;
        }
    }
    
    free(cName);
    
    return 0;
}

void drawGround() {
    
    // Generate corners -- Bottom left
  GLfloat bl[3] = { -400+400, -400, -400 };
    
    // Generate corners -- Top Left
    GLfloat tl[3] = { -400+400, 400, -400 };
    
    // Generate corners -- Top Right
    GLfloat tr[3] = { -400+400, 400, 400 };
    
    // Generate corners -- Bottom Right
    GLfloat br[3] = { -400+400, -400, 400 };
    
   /* GLfloat bl[3] = { -128+128, -128, -128 };
    
    // Generate corners -- Top Left
    GLfloat tl[3] = { -128+128, 128, -128};
    
    // Generate corners -- Top Right
    GLfloat tr[3] = { -128+128, 128, 128 };
    
    // Generate corners -- Bottom Right
    GLfloat br[3] = { -128+128, -128, 128 };*/
    // Push our coordinates into the vertex buffer as two triangles (clockwise)
    groundVertices.push_back(tl[0]);
    groundVertices.push_back(tl[1]);
    groundVertices.push_back(tl[2]);
    groundVertices.push_back(tr[0]);
    groundVertices.push_back(tr[1]);
    groundVertices.push_back(tr[2]);
    groundVertices.push_back(bl[0]);
    groundVertices.push_back(bl[1]);
    groundVertices.push_back(bl[2]);
    groundVertices.push_back(bl[0]);
    groundVertices.push_back(bl[1]);
    groundVertices.push_back(bl[2]);
    groundVertices.push_back(tr[0]);
    groundVertices.push_back(tr[1]);
    groundVertices.push_back(tr[2]);
    groundVertices.push_back(br[0]);
    groundVertices.push_back(br[1]);
    groundVertices.push_back(br[2]);
    
    // Generate corners -- Bottom left
    GLfloat bl_uv[3] = { 1, 0 };
    
    // Generate corners -- Top Left
    GLfloat tl_uv[3] = { 0, 0 };
    
    // Generate corners -- Top Right
    GLfloat tr_uv[3] = { 0, 1 };
    
    // Generate corners -- Bottom Right
    GLfloat br_uv[3] = { 1, 1};
    
    // Push our uv data to the buffer
    groundUVs.push_back(tl_uv[0]);
    groundUVs.push_back(tl_uv[1]);
    groundUVs.push_back(tr_uv[0]);
    groundUVs.push_back(tr_uv[1]);
    groundUVs.push_back(bl_uv[0]);
    groundUVs.push_back(bl_uv[1]);
    groundUVs.push_back(bl_uv[0]);
    groundUVs.push_back(bl_uv[1]);
    groundUVs.push_back(tr_uv[0]);
    groundUVs.push_back(tr_uv[1]);
    groundUVs.push_back(br_uv[0]);
    groundUVs.push_back(br_uv[1]);
    
    // Load our texture
    glGenTextures (1, &groundTexture);
    initTexture ("textures/ground.jpg", groundTexture);
    // Handle triangle VAO and VBOs
    
    glGenBuffers(1, &groundBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, groundBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(groundVertices.size()+groundUVs.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    groundVertices.size()*sizeof(float), groundVertices.data());//sizeof(float)*vector.size();
    glBufferSubData(GL_ARRAY_BUFFER, groundVertices.size()*sizeof(float),
                    groundUVs.size()*sizeof(float), groundUVs.data());
    
    glGenVertexArrays(1, &vaoGround);
    glBindVertexArray(vaoGround);
    program = pipeLine->GetProgramHandle();
    glBindBuffer(GL_ARRAY_BUFFER, groundBuffer);
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    const void * offset = (const void*) 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
    GLuint loc2 = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc2);
    //cout<<glGetError()<<endl;
    const void * offset1 = (const void*)(groundVertices.size()*sizeof(float));
    glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, offset1);
   

}


void drawSky() {
    
   /* {
        // Generate corners -- Bottom left
       GLfloat bl[3] = { -400-10+400, -400, -400 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { -400-10+400, 400, -400 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { -400-10+400, 400, 400 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { -400-10+400, -400, 400 };*/
       
        
       /* GLfloat bl[3] = { -128+127, -128*2, -128*2 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { -128+127, 128*2, -128*2};
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { -128+127, 128*2, 128*2 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { -128+127, -128*2, 128*2 };*/
        
        // Push our coordinates into the vertex buffer as two triangles (clockwise)
      /*  skyVertices.push_back(tl[0]);
        skyVertices.push_back(tl[1]);
        skyVertices.push_back(tl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(br[0]);
        skyVertices.push_back(br[1]);
        skyVertices.push_back(br[2]);
    }*/
    
    {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -400-10+400, -400, 400 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { -400-10+400, 400, 400 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { 400-10+400, 400, 400 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { 400-10+400, -400, 400 };
        /*GLfloat bl[3] = { (-128+128)*2-1, -128*2, 128*2 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { (-128+128)*2-1, 128*2, 128*2 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { (128+128)*2-1, 128*2, 128*2 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { (128+128)*2-1, -128*2, 128*2 };*/
      
        skyVertices.push_back(tl[0]);
        skyVertices.push_back(tl[1]);
        skyVertices.push_back(tl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(br[0]);
        skyVertices.push_back(br[1]);
        skyVertices.push_back(br[2]);
    }
    
    {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { 400-10+400, -400, 400 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { 400-10+400, 400, 400 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { 400-10+400, 400, -400 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { 400-10+400, -400, -400 };
       /* GLfloat bl[3] = { (128+128)*2-1, -128*2, 128*2 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { (128+128)*2-1, 128*2, 128*2 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { (128+128)*2-1, 128*2, -128*2 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { (128+128)*2-1, -128*2, -128*2 };*/
        
        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.push_back(tl[0]);
        skyVertices.push_back(tl[1]);
        skyVertices.push_back(tl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(br[0]);
        skyVertices.push_back(br[1]);
        skyVertices.push_back(br[2]);
    }
    
    {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -400-10+400, -400, -400 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { -400-10+400, 400, -400 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { 400-10+400, 400, -400 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { 400-10+400, -400, -400 };
       /* GLfloat bl[3] = { (-128+128)*2-1, -128*2, -128*2 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { (-128+128)*2-1, 128*2, -128*2 };
        
        // Generate corners - Top Right
        GLfloat tr[3] = { (128+128)*2-1, 128*2, -128*2 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { (128+128)*2-1, -128*2, -128*2 };*/
        
        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.push_back(tl[0]);
        skyVertices.push_back(tl[1]);
        skyVertices.push_back(tl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(br[0]);
        skyVertices.push_back(br[1]);
        skyVertices.push_back(br[2]);
    }
    
    {
        // Generate corners -- Bottom left
       GLfloat bl[3] = { -400-10+400, 400, -400 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { 400-10+400, 400, -400 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { 400-10+400, 400, 400 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { -400-10+400, 400, 400 };
       /* GLfloat bl[3] = { (-128+128)*2-1, 128*2, -128*2 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { (128+128)*2-1, 128*2, -128*2 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { (128+128)*2-1, 128*2, 128*2 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { (-128+128)*2-1, 128*2, 128*2 };*/
        
        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.push_back(tl[0]);
        skyVertices.push_back(tl[1]);
        skyVertices.push_back(tl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(br[0]);
        skyVertices.push_back(br[1]);
        skyVertices.push_back(br[2]);
    }
    
    
    {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -400-10+400, -400, -400 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { 400-10+400, -400, -400 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { 400-10+400, -400, 400 };
        
        // Generate corners -- Bottom Right
        GLfloat br[3] = { -400-10+400, -400, 400 };
        
        // Generate corners -- Bottom left
        /*GLfloat bl[3] = { (-128+128)*2-1, -128*2, -128*2 };
        
        // Generate corners -- Top Left
        GLfloat tl[3] = { (128+128)*2-1, -128*2, -128*2 };
        
        // Generate corners -- Top Right
        GLfloat tr[3] = { (128+128)*2-1, -128*2, 128*2 };
        
        // Generate corners -- Bottom Righ
        GLfloat br[3] = { (-128+128)*2-1, -128*2, 128*2 };*/
        
        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.push_back(tl[0]);
        skyVertices.push_back(tl[1]);
        skyVertices.push_back(tl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(bl[0]);
        skyVertices.push_back(bl[1]);
        skyVertices.push_back(bl[2]);
        skyVertices.push_back(tr[0]);
        skyVertices.push_back(tr[1]);
        skyVertices.push_back(tr[2]);
        skyVertices.push_back(br[0]);
        skyVertices.push_back(br[1]);
        skyVertices.push_back(br[2]);
    }
    
    for (int i = 0; i <6; i++) {
        // Calculate colors
       // GLfloat color = 1.0f;
        GLfloat colorBL[3] = { 1, 0 };
        GLfloat colorTL[3] = { 0, 0 };
        GLfloat colorTR[3] = { 0, 1 };
        GLfloat colorBR[3] = { 1, 1 };
        
        // Push our color data to the buffer
        skyUVs.push_back(colorTL[0]);
        skyUVs.push_back(colorTL[1]);
        skyUVs.push_back(colorTR[0]);
        skyUVs.push_back(colorTR[1]);
        skyUVs.push_back(colorBL[0]);
        skyUVs.push_back(colorBL[1]);
        skyUVs.push_back(colorBL[0]);
        skyUVs.push_back(colorBL[1]);
        skyUVs.push_back(colorTR[0]);
        skyUVs.push_back(colorTR[1]);
        skyUVs.push_back(colorBR[0]);
        skyUVs.push_back(colorBR[1]);

   }
    
    // Load our texture
    glGenTextures (1, &skyTexture);
    initTexture ("textures/sky.jpg", skyTexture);
    
    glGenBuffers(1, &skyBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, skyBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(skyVertices.size()+skyUVs.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    skyVertices.size()*sizeof(float), skyVertices.data());//sizeof(float)*vector.size();
    glBufferSubData(GL_ARRAY_BUFFER, skyVertices.size()*sizeof(float),
                    skyUVs.size()*sizeof(float), skyUVs.data());
    
    glGenVertexArrays(1, &vaoSky);
    glBindVertexArray(vaoSky);
    program = pipeLine->GetProgramHandle();
    glBindBuffer(GL_ARRAY_BUFFER, skyBuffer);
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    const void * offset = (const void*) 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
    GLuint loc2 = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc2);
    //cout<<glGetError()<<endl;
    const void * offset1 = (const void*)(skyVertices.size()*sizeof(float));
    glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, offset1);
}


void drawSplines () {
    // Now, generate the full list of spline points
    float color = 1.0f;
    for (int i = 0; i < numSplines; i++) {
        for (int j = 0; j < splines[i].numControlPoints - 3; j++) {
            Point point0 = splines[i].points[j];
            Point point1 = splines[i].points[j + 1];
            Point point2 = splines[i].points[j + 2];
            Point point3 = splines[i].points[j + 3];
            
            // Brute force our spline points
            for (float u = 0; u < 1.0f; u += 0.004f) {
                FPoint fpoint;
                fpoint.x = 0.5f * (
                                   (2 * point1.x) +
                                   (-point0.x + point2.x) * u +
                                   (2 * point0.x - 5 * point1.x + 4 * point2.x - point3.x) * (u * u) +
                                   (-point0.x + 3 * point1.x - 3 * point2.x + point3.x) * (u * u * u)
                                   );
                
                fpoint.y = 0.5f * (
                                   (2 * point1.y) +
                                   (-point0.y + point2.y) * u +
                                   (2 * point0.y - 5 * point1.y + 4 * point2.y - point3.y) * (u * u) +
                                   (-point0.y + 3 * point1.y - 3 * point2.y + point3.y) * (u * u * u)
                                   );
                
                fpoint.z = 0.5f * (
                                   (2 * point1.z) +
                                   (-point0.z + point2.z) * u +
                                   (2 * point0.z - 5 * point1.z + 4 * point2.z - point3.z) * (u * u) +
                                   (-point0.z + 3 * point1.z - 3 * point2.z + point3.z) * (u * u * u)
                                   );
                
                splinePoints.push_back(fpoint);
            }
        }
    }
    
    // For every four spline points, compute the triangles for the track
    for (int i = 0; i < splinePoints.size() - 3; i++) {
        // Get four points from the list
        FPoint point0 = splinePoints[i];
        FPoint point1 = splinePoints[i + 1];
        FPoint point2 = splinePoints[i + 2];
        FPoint point3 = splinePoints[i + 3];
        
        // Compute the tangent -- solved with C++ calculus!
        const float u = 0.004f;
        FPoint tangent;
        tangent.x = 0.5f * (
                            (-point0.x + point2.x) +
                            (2 * point0.x - 5 * point1.x + 4 * point2.x - point3.x) * (2 * u) +
                            (-point0.x + 3 * point1.x - 3 * point2.x + point3.x) * (3 * u * u)
                            );
        
        tangent.y = 0.5f * (
                            (-point0.y + point2.y) +
                            (2 * point0.y - 5 * point1.y + 4 * point2.y - point3.y) * (2 * u) +
                            (-point0.y + 3 * point1.y - 3 * point2.y + point3.y) * (3 * u * u)
                            );
        
        tangent.z = 0.5f * (
                            (-point0.z + point2.z) +
                            (2 * point0.z - 5 * point1.z + 4 * point2.z - point3.z) * (2 * u) +
                            (-point0.z + 3 * point1.z - 3 * point2.z + point3.z) * (3 * u * u)
                            );
        
        // Compute the unit tangent!
        float magnitude = std::sqrt (std::pow (tangent.x, 2) + std::pow (tangent.y, 2) + std::pow (tangent.z, 2));
        FPoint unitTangent;
        unitTangent.x = tangent.x / magnitude;
        unitTangent.y = tangent.y / magnitude;
        unitTangent.z = tangent.z / magnitude;
        
        // Compute the normal between two vectors -- used the world y axis to get one axis
        //(a2b3−a3b2)i−(a1b3−a3b1)j+(a1b2−a2b1)k
        FPoint normal;
        normal.x = (unitTangent.y * 0 - unitTangent.z * 1);
        normal.y = (unitTangent.x * 0 - unitTangent.z * 0);
        normal.z = (unitTangent.x * 1 - unitTangent.y * 0);
        
        // Compute the binormal by crossing the normal with the tangent
        FPoint binormal;
        binormal.x = (normal.y * unitTangent.z - normal.z * unitTangent.y);
        binormal.y = (normal.x * unitTangent.z - normal.z * unitTangent.x);
        binormal.z = (normal.x * unitTangent.y - normal.y * unitTangent.z);
        
        // Get the array of points for the track rail mesh
        const float scalar = 0.07f;
        GLfloat p[] = {
            // Front face
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            
            // Left face
            // V6
            point3.x + scalar * (normal.x - binormal.x), point3.y + scalar * (normal.y - binormal.y), point3.z + scalar * (normal.z - binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            
            // Right face
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
            
            // Top face
            // V6
            point3.x + scalar * (normal.x - binormal.x), point3.y + scalar * (normal.y - binormal.y), point3.z + scalar * (normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            
            // Bottom face
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            
            // Back face
            // V6
            point3.x + scalar * (normal.x - binormal.x), point3.y + scalar * (normal.y - binormal.y), point3.z + scalar * (normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
        };
        
        //splinePoint.insert (splinePoint.end(), p, p + 108);
        for(int i=0;i<108;i++){
            //p[i]*=20;
           // p[i]-=100;
            splinePoint.push_back(p[i]);
        }
        
        for (int j = 0; j < 36; j++) {
            //float color = 0.5f;
            //GLfloat colors[] = { color, color, color, color };
            //splineColor.push_back (colors[0]);
            //splineColor.push_back (colors[1]);
            //splineColor.push_back (colors[2]);
            //splineColor.push_back (colors[3]);
            GLfloat colorBL[3] = { 1, 0 };
            GLfloat colorTL[3] = { 0, 0 };
            GLfloat colorTR[3] = { 0, 1 };
            GLfloat colorBR[3] = { 1, 1 };
            splineColor.push_back(colorTL[0]);
            splineColor.push_back(colorTL[1]);
            splineColor.push_back(colorTR[0]);
            splineColor.push_back(colorTR[1]);
            splineColor.push_back(colorBL[0]);
            splineColor.push_back(colorBL[1]);
            splineColor.push_back(colorBL[0]);
            splineColor.push_back(colorBL[1]);
            splineColor.push_back(colorTR[0]);
            splineColor.push_back(colorTR[1]);
            splineColor.push_back(colorBR[0]);
            splineColor.push_back(colorBR[1]);
        }
       
    }
    
    glGenTextures (1, &splineTexture);
    initTexture ("textures/track.jpg", splineTexture);
    
    glGenBuffers(1, &splineBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, splineBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*(splinePoint.size()+splineColor.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    splinePoint.size()*sizeof(float), splinePoint.data());//sizeof(float)*vector.size();
    glBufferSubData(GL_ARRAY_BUFFER, splinePoint.size()*sizeof(float),
                    splineColor.size()*sizeof(float), splineColor.data());
    
    glGenVertexArrays(1, &vaoPoint);
    glBindVertexArray(vaoPoint);
    program = pipeLine->GetProgramHandle();
    glBindBuffer(GL_ARRAY_BUFFER, splineBuffer);
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    const void * offset = (const void*) 0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);
    GLuint loc2 = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc2);
    //cout<<glGetError()<<endl;
    const void * offset1 = (const void*)(splinePoint.size()*sizeof(float));
    glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, offset1);
}


void bindProgram() {
    
    // Bind the shaders
    pipeLine->Bind();
    
    // First, generate our terrain mesh.
    drawGround();
    
    drawSky();
    
    // Next, generate the track
    // splinePipeline->Bind();
     drawSplines();
}
