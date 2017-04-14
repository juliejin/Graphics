/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Ling Jin
 * *************************
*/

#ifdef WIN32
#include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
#include <GL/gl.h>
#include <GL/glut.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <cmath>
#include <iostream>
#include <string>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];


const double PI = 3.141592f;
const double MAX_DIST = -1e8;
const double BIAS = 1e-16;


bool gUseAA = false;
const unsigned int SSAA_SAMPLES = 4;

using namespace std;

struct Vertex
{
    double position[3];
    double color_diffuse[3];
    double color_specular[3];
    double normal[3];
    double shininess;
};

struct Triangle
{
    Vertex v[3];
};

struct Sphere
{
    double position[3];
    double color_diffuse[3];
    double color_specular[3];
    double shininess;
    double radius;
};

struct Light
{
    double position[3];
    double color[3];
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);


// Color definition
struct Color {
    double mR;
    double mG;
    double mB;
    
    Color () : mR(0), mG(0), mB(0) {
    }
    Color (double r, double g, double b) : mR(r), mG(g), mB(b) {
    }
    Color& operator += (const Color& other) {
        mR += other.mR;
        if (mR > 1.0f) {
            mR = 1.0f;
        }
        
        else if (mR < 0) {
            mR = 0;
        }
        
        mG += other.mG;
        if (mG > 1.0f) {
            mG = 1.0f;
        }
        
        else if (mG < 0) {
            mG = 0;
        }
        
        mB += other.mB;
        if (mB > 1.0f) {
            mB = 1.0f;
        }
        
        else if (mB < 0) {
            mB = 0;
        }
        
        return *this;
    }
};

struct Vector3 {
    double mX;
    double mY;
    double mZ;
    
    Vector3 () : mX(0), mY(0), mZ(0) {}
    Vector3 (double x, double y, double z) : mX(x), mY(y), mZ(z) {}
  
    static Vector3 cross (const Vector3& a, const Vector3& b){
        double x = a.mY * b.mZ - a.mZ * b.mY;
        double y = a.mZ * b.mX - a.mX * b.mZ;
        double z = a.mX * b.mY - a.mY * b.mX;
        return Vector3 (x, y, z);
    };
 
    inline double dot (const Vector3& other) const { return mX * other.mX + mY * other.mY + mZ * other.mZ; }
    inline double distance () const { return std::pow(mX, 2) + std::pow(mY, 2) + std::pow(mZ, 2); }
    inline double magnitude () const { return std::sqrt(distance()); }
    
    Vector3 operator+ (const Vector3& other) const { return Vector3 (mX + other.mX, mY + other.mY, mZ + other.mZ); }
    Vector3 operator- (const Vector3& other) const { return Vector3 (mX - other.mX, mY - other.mY, mZ - other.mZ); }
    Vector3 operator- () const { return Vector3 (-mX, -mY, -mZ); }
    Vector3 operator* (double scalar) const { return Vector3 (mX * scalar, mY * scalar, mZ * scalar); }
    
    Vector3& operator += (const Vector3& other) {
        mX += other.mX;
        mY += other.mY;
        mZ += other.mZ;
        return *this;
    }
    Vector3& normalize(){
        double normal = distance();
        if (normal > 0) {
            double inverse = 1.0f / std::sqrt(normal);
            mX *= inverse;
            mY *= inverse;
            mZ *= inverse;
        }
        return *this;
    };
};

class Ray {
private:
    Vector3 mOrigin;
    Vector3 mDirection;
    
public:
    Ray () {}
    Ray (const Vector3& origin, const Vector3& direction) : mOrigin (origin), mDirection (direction) {}
 
    bool intersects (const Sphere& sphere, Vector3& intersection){
       
        Vector3 position = Vector3(sphere.position[0], sphere.position[1], sphere.position[2]);
        Vector3 dist = mOrigin - position;
   
        double t0 = MAX_DIST;
        double t1 = MAX_DIST;
      
        double a = mDirection.dot(mDirection);
        double b = 2 * mDirection.dot (dist);
        double c = dist.dot (dist) - std::pow (sphere.radius, 2);
        double quad = std::pow(b, 2) - (4 * a * c);
        if (quad < 0) {
            return false;
        }
        
        else if (std::abs(quad) < BIAS) {
            double q = -0.5f * b / a;
            t0 = q;
            t1 = q;
        }
        
        else {
            double q = (b > 0) ? -0.5f * (b + std::sqrt(quad)) : -0.5f * (b - std::sqrt(quad));
            t0 = q / a;
            t1 = c / q;
        }
        
        if (t0 < 0 && t1 < 0) {
            return false;
        }

        if (t0 > t1 && t1 > 0) {
            t0 = t1;
        }
        
        intersection = mOrigin + (mDirection * t0);
        return true;
    };
    
    bool intersects (const Triangle& triangle, Vector3& intersection){

        Vector3 vertexA = Vector3(triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
        Vector3 vertexB = Vector3(triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
        Vector3 vertexC = Vector3(triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);
 
        Vector3 normal = Vector3::cross(vertexB - vertexA, vertexC - vertexA);
        normal.normalize ();
        
        double dir = normal.dot(mDirection);
        if (std::abs(dir) < BIAS) {
            return false;
        }
   
        Vector3 dist = vertexA - mOrigin;
        double d = dist.dot(normal);
        
        double t = d / dir;
        if (t < 0) {
            return false;
        }
        
        intersection = mOrigin + (mDirection * t);
        
        if (normal.dot(Vector3::cross(vertexB - vertexA, intersection - vertexA)) < 0
            || normal.dot(Vector3::cross(vertexC - vertexB, intersection - vertexB)) < 0
            || normal.dot(Vector3::cross(vertexA - vertexC, intersection - vertexC)) < 0) {
            return false;
        }
        
        return true;
    };
};

double computeLightMagnitude (const Vector3& lightDirection, const Vector3& normal) {
    
    double retVal = lightDirection.dot (normal);
    if (retVal > 1.0f) {
        retVal = 1.0f;
    }
    
    else if (retVal < 0.0f) {
        retVal = 0.0f;
    }
    
    return retVal;
}

double computeReflectionMagnitude (double lightMagnitude, const Vector3& lightDirection, const Vector3& intersection, const Vector3& normal) {
 
    Vector3 direction = -intersection;
    direction.normalize ();
    
    // Find the reflection vector, then compute the dot product to get the reflection magnitude
    Vector3 reflection (2 * lightMagnitude * normal.mX - lightDirection.mX, 2 * lightMagnitude * normal.mY - lightDirection.mY, 2 * lightMagnitude * normal.mZ - lightDirection.mZ);
    reflection.normalize ();
    double retVal = reflection.dot (direction);
    if (retVal > 1.0f) {
        retVal = 1.0f;
    }
    
    else if (retVal < 0.0f) {
        retVal = 0.0f;
    }
    
    return retVal;
}

Color calculateSphereLighting (const Sphere& sphere, const Light& light, const Vector3& intersection) {
    
    Vector3 normal = intersection - Vector3 (sphere.position[0], sphere.position[1], sphere.position[2]);
    normal.normalize ();

    Vector3 lightPosition (light.position[0], light.position[1], light.position[2]);
    Vector3 lightDirection = lightPosition - intersection;
    lightDirection.normalize ();
    
    double lightMagnitude = computeLightMagnitude (lightDirection, normal);
    double reflectionMagnitude = computeReflectionMagnitude (lightMagnitude, lightDirection, intersection, normal);
    
    Color diffuse (sphere.color_diffuse[0], sphere.color_diffuse[1], sphere.color_diffuse[2]);
    Color specular (sphere.color_specular[0], sphere.color_specular[1], sphere.color_specular[2]);
    double shininess = sphere.shininess;

    double r = light.color[0] * (diffuse.mR * lightMagnitude + (specular.mR * std::pow (reflectionMagnitude, shininess)));
    double g = light.color[1] * (diffuse.mG * lightMagnitude + (specular.mG * std::pow (reflectionMagnitude, shininess)));
    double b = light.color[2] * (diffuse.mB * lightMagnitude + (specular.mB * std::pow (reflectionMagnitude, shininess)));
    return Color (r, g, b);
}

Color calculateTriangleLighting (const Triangle& triangle, const Light& light, const Vector3& intersection) {
    
    Vector3 vertexA (triangle.v[0].position[0], triangle.v[0].position[1], triangle.v[0].position[2]);
    Vector3 vertexB (triangle.v[1].position[0], triangle.v[1].position[1], triangle.v[1].position[2]);
    Vector3 vertexC (triangle.v[2].position[0], triangle.v[2].position[1], triangle.v[2].position[2]);
    
    Vector3 vAvB = vertexB - vertexA;
    Vector3 vAvC = vertexC - vertexA;
    
    Vector3 planar = Vector3::cross (vAvB, vAvC);
    float denominator = planar.dot(planar);
    
    // Compute values u and v
    Vector3 edgeCB = vertexC - vertexB;
    Vector3 distB = intersection - vertexB;
    Vector3 cpCB = Vector3::cross(edgeCB, distB);
    
    Vector3 edgeAC = vertexA - vertexC;
    Vector3 distC = intersection - vertexC;
    Vector3 cpAC = Vector3::cross(edgeAC, distC);
    
    double u = planar.dot(cpCB) / denominator;
    double v = planar.dot(cpAC) / denominator;
    double w = 1.0f - u - v;
    
    // Get triangle normals
    Vector3 normal (
                    u * triangle.v[0].normal[0] + v * triangle.v[1].normal[0] + w * triangle.v[2].normal[0],
                    u * triangle.v[0].normal[1] + v * triangle.v[1].normal[1] + w * triangle.v[2].normal[1],
                    u * triangle.v[0].normal[2] + v * triangle.v[1].normal[2] + w * triangle.v[2].normal[2]
                    );
    normal.normalize ();
    
    Vector3 lightPosition (light.position[0], light.position[1], light.position[2]);
    Vector3 lightDirection = lightPosition - intersection;
    lightDirection.normalize ();
    

    double lightMagnitude = computeLightMagnitude (lightDirection, normal);
    double reflectionMagnitude = computeReflectionMagnitude (lightMagnitude, lightDirection, intersection, normal);
    
    Color diffuse (
                   u * triangle.v[0].color_diffuse[0] + v * triangle.v[1].color_diffuse[0] + w * triangle.v[2].color_diffuse[0],
                   u * triangle.v[0].color_diffuse[1] + v * triangle.v[1].color_diffuse[1] + w * triangle.v[2].color_diffuse[1],
                   u * triangle.v[0].color_diffuse[2] + v * triangle.v[1].color_diffuse[2] + w * triangle.v[2].color_diffuse[2]
                   );
    
    Color specular (
                    u * triangle.v[0].color_specular[0] + v * triangle.v[1].color_specular[0] + w * triangle.v[2].color_specular[0],
                    u * triangle.v[0].color_specular[1] + v * triangle.v[1].color_specular[1] + w * triangle.v[2].color_specular[1],
                    u * triangle.v[0].color_specular[2] + v * triangle.v[1].color_specular[2] + w * triangle.v[2].color_specular[2]
                    );
    
    double shininess = u * triangle.v[0].shininess + v * triangle.v[1].shininess + w * triangle.v[2].shininess;

    double r = light.color[0] * (diffuse.mR * lightMagnitude + (specular.mR * std::pow (reflectionMagnitude, shininess)));
    double g = light.color[1] * (diffuse.mG * lightMagnitude + (specular.mG * std::pow (reflectionMagnitude, shininess)));
    double b = light.color[2] * (diffuse.mB * lightMagnitude + (specular.mB * std::pow (reflectionMagnitude, shininess)));
    return Color (r, g, b);
}

Color performSphereCollisionTest (Ray& ray, const Color& color, double& closest) {
    
    Color retVal = color;
    for (int i = 0; i < num_spheres; i++) {
        
 
        Vector3 intersection (0, 0, MAX_DIST);
        if (ray.intersects (spheres[i], intersection) && intersection.mZ > closest) {
            
            retVal = Color (0, 0, 0);
            
            for (int j = 0; j < num_lights; j++) {
              
                Vector3 lightPosition (lights[j].position[0], lights[j].position[1], lights[j].position[2]);
                
                Vector3 origin = intersection;
                Vector3 direction = lightPosition - origin;
                Ray shadow (origin, direction.normalize ());
                
                bool isLit = true;
               
                for (int k = 0; k < num_spheres; k++) {
                    Vector3 intersect;
                    if (shadow.intersects (spheres[k], intersect) && k != i) {
                        
                        Vector3 x = intersect - intersection;
                        Vector3 y = lightPosition - intersection;
                        if (x.magnitude () < y.magnitude ()) {
                            isLit = false;
                            break;
                        }
                    }
                }
                
                for (int k = 0; k < num_triangles; k++) {
                    Vector3 intersect;
                    if (shadow.intersects (triangles[k], intersect)) {
                       
                        Vector3 x = intersect - intersection;
                        Vector3 y = lightPosition - intersection;
                        if (x.magnitude () < y.magnitude ()) {
                            isLit = false;
                            break;
                        }
                    }
                }
                
                if (isLit) {
                    retVal += calculateSphereLighting (spheres[i], lights[j], intersection);
                }
            }
            
            closest = intersection.mZ;
        }
    }
    
    return retVal;
}


Color performTriangleCollisionTest (Ray& ray, const Color& color, double& closest) {
    
    Color retVal = color;
    for (int i = 0; i < num_triangles; i++) {
        
       
        Vector3 intersection (0, 0, MAX_DIST);
        if (ray.intersects (triangles[i], intersection) && intersection.mZ > closest) {
   
            retVal = Color (0, 0, 0);
            
            for (int j = 0; j < num_lights; j++) {
                Vector3 lightPosition (lights[j].position[0], lights[j].position[1], lights[j].position[2]);
                
           
                Vector3 origin = intersection;
                Vector3 direction = lightPosition - origin;
                Ray shadow (origin, direction.normalize ());
                
                // If the object is lit, add color to it
                bool isLit = true;
                
                for (int k = 0; k < num_spheres; k++) {
                    Vector3 intersect;
                    if (shadow.intersects (spheres[k], intersect)) {
 
                        Vector3 x = intersect - intersection;
                        Vector3 y = lightPosition - intersection;
                        if (x.magnitude () < y.magnitude ()) {
                            isLit = false;
                            break;
                        }
                    }
                }
                
                for (int k = 0; k < num_triangles; k++) {
                    Vector3 intersect;
                    if (shadow.intersects (triangles[k], intersect) && k != i) {
                        
                        // Make sure that the intersection point is not past the light
                        Vector3 x = intersect - intersection;
                        Vector3 y = lightPosition - intersection;
                        if (x.magnitude () < y.magnitude ()) {
                            isLit = false;
                            break;
                        }
                    }
                }
                
                if (isLit) {
                    retVal += calculateTriangleLighting (triangles[i], lights[j], intersection);
                }
            }
            
            // Update the closest intersection point
            closest = intersection.mZ;
        }
    }
    
    return retVal;
}

Color trace (Ray& ray) {

    Color retVal (1, 1, 1);
    double closest = MAX_DIST;
    
    retVal = performSphereCollisionTest (ray, retVal, closest);
    
    retVal = performTriangleCollisionTest (ray, retVal, closest);

    retVal += Color (ambient_light[0], ambient_light[1], ambient_light[2]);

    return retVal;
}

Ray* calculateRaysFromCamera (double x, double y) {
    
    Ray* rays = new Ray[SSAA_SAMPLES];
    
    {
        double ratio = (double)WIDTH / (double)HEIGHT;
        double angle = std::tan ((fov / 2.0) * (PI / 180.0));

        double xNDC = (x + 0.25) / (double)WIDTH;
        double yNDC = (y + 0.25) / (double)HEIGHT;
        
        double xScreen = (2 * xNDC) - 1;
        double yScreen = (2 * yNDC) - 1;

        double xActual = xScreen * angle * ratio;
        double yActual = yScreen * angle;
        Vector3 origin;
        Vector3 dir (xActual, yActual, -1);
        rays[0] = Ray (origin, dir.normalize ());
    }
    
    {
        double ratio = (double)WIDTH / (double)HEIGHT;
        double angle = std::tan ((fov / 2.0) * (PI / 180.0));
        
        double xNDC = (x + 0.75) / (double)WIDTH;
        double yNDC = (y + 0.25) / (double)HEIGHT;
        
        double xScreen = (2 * xNDC) - 1;
        double yScreen = (2 * yNDC) - 1;
 
        double xActual = xScreen * angle * ratio;
        double yActual = yScreen * angle;
 
        Vector3 origin;
        Vector3 dir (xActual, yActual, -1);
        rays[1] = Ray (origin, dir.normalize ());
    }
    
    {
        double ratio = (double)WIDTH / (double)HEIGHT;
        double angle = std::tan ((fov / 2.0) * (PI / 180.0));
        
        double xNDC = (x + 0.25) / (double)WIDTH;
        double yNDC = (y + 0.75) / (double)HEIGHT;
     
        double xScreen = (2 * xNDC) - 1;
        double yScreen = (2 * yNDC) - 1;

        double xActual = xScreen * angle * ratio;
        double yActual = yScreen * angle;
        
        Vector3 origin;
        Vector3 dir (xActual, yActual, -1);
        rays[2] = Ray (origin, dir.normalize ());
    }
    
    {
        double ratio = (double)WIDTH / (double)HEIGHT;
        double angle = std::tan ((fov / 2.0) * (PI / 180.0));
        
        double xNDC = (x + 0.75) / (double)WIDTH;
        double yNDC = (y + 0.75) / (double)HEIGHT;
        
        double xScreen = (2 * xNDC) - 1;
        double yScreen = (2 * yNDC) - 1;
        
        double xActual = xScreen * angle * ratio;
        double yActual = yScreen * angle;
        
        Vector3 origin;
        Vector3 dir (xActual, yActual, -1);
        rays[3] = Ray (origin, dir.normalize ());
    }
    return rays;
}

Ray calculateRayFromCamera (double x, double y) {
    
    double ratio = (double)WIDTH / (double)HEIGHT;
    double angle = std::tan ((fov / 2.0) * (PI / 180.0));
    
    double xNDC = (x + 0.5) / (double)WIDTH;
    double yNDC = (y + 0.5) / (double)HEIGHT;
    
    double xScreen = (2 * xNDC) - 1;
    double yScreen = (2 * yNDC) - 1;
    
    // From the screen coordinates, get the actual coordinates
    double xActual = xScreen * angle * ratio;
    double yActual = yScreen * angle;
 
    Vector3 origin;
    Vector3 dir (xActual, yActual, -1);
    return Ray (origin, dir.normalize ());
}

void draw_scene() {
    
    for(unsigned int x = 0; x < WIDTH; x++) {
        
        // Begin drawing
        glPointSize(2.0);
        glBegin(GL_POINTS);
        
        for(unsigned int y = 0; y < HEIGHT; y++) {
            
            // Compute the average of the four rays
            double r = 0;
            double g = 0;
            double b = 0;
            
            // If SSAA is enabled, average the values
            if (gUseAA) {
                Ray* rays = calculateRaysFromCamera (x, y);
                for (int i = 0; i < SSAA_SAMPLES; i++) {
                    Color color = trace (rays[i]);
                    r += color.mR;
                    g += color.mG;
                    b += color.mB;
                }
                r /= SSAA_SAMPLES;
                g /= SSAA_SAMPLES;
                b /= SSAA_SAMPLES;
                delete [] rays;
            }
            
            else {
                Ray ray = calculateRayFromCamera (x, y);
                Color color = trace (ray);
                r = color.mR;
                g = color.mG;
                b = color.mB;
            }
            
            plot_pixel(x, y, r * 255, g * 255, b * 255);
        }
        
        glEnd();
        glFlush();
    }
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
    glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    buffer[y][x][0] = r;
    buffer[y][x][1] = g;
    buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    plot_pixel_display(x,y,r,g,b);
    if(mode == MODE_JPEG)
        plot_pixel_jpeg(x,y,r,g,b);
}


void save_jpg()
{
    printf("Saving JPEG file: %s\n", filename);
    
    ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
    if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
        printf("Error in Saving\n");
    else 
        printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
    if(strcasecmp(expected,found))
    {
        printf("Expected '%s ' found '%s '\n", expected, found);
        printf("Parse error, abnormal abortion\n");
        exit(0);
    }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
    char str[100];
    fscanf(file,"%s",str);
    parse_check(check,str);
    fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
    printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
    char str[100];
    fscanf(file,"%s",str);
    parse_check("rad:",str);
    fscanf(file,"%lf",r);
    printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
    char s[100];
    fscanf(file,"%s",s);
    parse_check("shi:",s);
    fscanf(file,"%lf",shi);
    printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
    FILE * file = fopen(argv,"r");
    int number_of_objects;
    char type[50];
    Triangle t;
    Sphere s;
    Light l;
    fscanf(file,"%i", &number_of_objects);
    
    printf("number of objects: %i\n",number_of_objects);
    
    parse_doubles(file,"amb:",ambient_light);
    
    for(int i=0; i<number_of_objects; i++)
    {
        fscanf(file,"%s\n",type);
        printf("%s\n",type);
        if(strcasecmp(type,"triangle")==0)
        {
            printf("found triangle\n");
            for(int j=0;j < 3;j++)
            {
                parse_doubles(file,"pos:",t.v[j].position);
                parse_doubles(file,"nor:",t.v[j].normal);
                parse_doubles(file,"dif:",t.v[j].color_diffuse);
                parse_doubles(file,"spe:",t.v[j].color_specular);
                parse_shi(file,&t.v[j].shininess);
            }
            
            if(num_triangles == MAX_TRIANGLES)
            {
                printf("too many triangles, you should increase MAX_TRIANGLES!\n");
                exit(0);
            }
            triangles[num_triangles++] = t;
        }
        else if(strcasecmp(type,"sphere")==0)
        {
            printf("found sphere\n");
            
            parse_doubles(file,"pos:",s.position);
            parse_rad(file,&s.radius);
            parse_doubles(file,"dif:",s.color_diffuse);
            parse_doubles(file,"spe:",s.color_specular);
            parse_shi(file,&s.shininess);
            
            if(num_spheres == MAX_SPHERES)
            {
                printf("too many spheres, you should increase MAX_SPHERES!\n");
                exit(0);
            }
            spheres[num_spheres++] = s;
        }
        else if(strcasecmp(type,"light")==0)
        {
            printf("found light\n");
            parse_doubles(file,"pos:",l.position);
            parse_doubles(file,"col:",l.color);
            
            if(num_lights == MAX_LIGHTS)
            {
                printf("too many lights, you should increase MAX_LIGHTS!\n");
                exit(0);
            }
            lights[num_lights++] = l;
        }
        else
        {
            printf("unknown type in scene description:\n%s\n",type);
            exit(0);
        }
    }
    return 0;
}

void display()
{
}

void init()
{
    glMatrixMode(GL_PROJECTION);
    glOrtho(0,WIDTH,0,HEIGHT,1,-1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Clear to white
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
   
    static int once=0;
    if(!once)
    {
        draw_scene();
        if(mode == MODE_JPEG)
            save_jpg();
    }
    once=1;
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
    }
}

int main(int argc, char ** argv)
{
    if ((argc < 2) || (argc > 4))
    {	
        printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
        exit(0);
    }
    // If there are four arguments, check to see if we should use SSAA
    if(argc == 4) {
        std::string aa (argv[3]); 
        if (aa != "ssaa") {
            exit (0);
        }
        else {
            gUseAA = true;
        }
        mode = MODE_JPEG;
        filename = argv[2];
    }
    
    //normal
    if(argc == 3)
    {
        mode = MODE_JPEG;
        filename = argv[2];
    }
    else if(argc == 2)
        mode = MODE_DISPLAY;
    
    glutInit(&argc,argv);
    loadScene(argv[1]);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(WIDTH,HEIGHT);
    int window = glutCreateWindow("Ray Tracer");
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboardFunc);
    init();
    glutMainLoop();
}

