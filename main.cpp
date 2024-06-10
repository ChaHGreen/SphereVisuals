#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath> 
#include <glm/glm.hpp>
#define MAX_NUM_TRI     1024
#define NUM_PARTICLES   300
#define T_MAX           20
#include "Angel-yjc.h"
//bool lightingEnabled = false;
//typedef Angel::vec3  color3;
typedef Angel::vec3  point3;
typedef Angel::vec4  color4;
typedef Angel::vec4  point4;

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

// Flags
bool rolling_begin = false;
bool wireframe = false;
bool shadow = true;
int shading_type = 1;  //1: smooth, 0: flat
int lighting_on = 1;    //1: true, 0: false
int light_src = 1;    //1: point source, 0: spot light, 2: initial only directinal light
int fog = 0; // Fog Option   0: no fog, 1: Linear, 2: Exponential, 3: Exponential Square
int blending = 0; //1: blending, 0: no blending
int ground_tex = 1;
int sphereTex = 1; // 1: vertical 2: slanted 0: none
int frameMode = 1; //  0: none
int sphereCB = 1; // 1: Sphere is checkerboard 0: otherwise
int latticeMode = 0;
int firework = 1;


GLuint program;         /* shader program object id */
GLuint floor_buffer;    /* vertex buffer object id for floor */
GLuint axis_buffer;     /* vertex buffer object id for axis */
GLuint sphere_buffer;   /* vertex buffer object id for sphere */
GLuint color_buffer;     /* vertex buffer object id for color */
GLuint shadow_buffer;     /* vertex buffer object id for shadow */
GLuint firework_program;         /* shader program object id */
GLuint firework_buffer;


// Projection transformation parameters
GLfloat  fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.5, zFar = 30.0;
vec4 init_eye(7.0f, 3.0f, -10.0f, 1.0f); // initial viewer position
vec4 eye = init_eye;

// Floor variables
const int floor_NumVertices = 6; //(1 face)*(2 triangles/face)*(3 vertices/triangle)
point4 floor_points[floor_NumVertices]; // positions for all vertices
color4 floor_colors[floor_NumVertices]; // colors for all vertices
point4 floor_vertices[4] = {
    point4(5.0, 0.0,  8.0, 1.0),   // down R
    point4(5.0, 0.0, -4.0, 1.0),   // top R
    point4(-5.0, 0.0, -4.0, 1.0),   // top L
    point4(-5.0, 0.0,  8.0, 1.0)    // down L
};
std::vector<vec3> floor_normals;
// floor texture
//std::vector<vec2> quad_texCoord;
vec2 quad_texCoord[floor_NumVertices];


// Axis variables
const int axis_NumVertices = 6; // x, y, z axis, each having 2 vertices
point4 axis_points[axis_NumVertices];
color4 axis_colors[axis_NumVertices];

//Sphere variables
int animationFlag = 0; // 1: animation; 0: non-animation. Toggled by key 'b' or 'B'
float radius = 1.0f;
std::vector<point4> sphere_triangles;  // Sphere Vertex
std::vector<color4> sphere_color;      // Sphere Vertex Color
std::vector<vec3> sphere_normals;

vec4 sphereCenter = vec4(3.0, 1.0, 5.0, 1.0);   // Initial position of the center of the sphere
vec4 pathPoints[] = { vec4(3.0, 1.0, 5.0, 1.0), vec4(-1.0, 1.0, -4.0, 1.0), vec4(3.5, 1.0, -2.5, 1.0) };    // Rolling point list
int currentPoint = 0;    // Rolling position index
mat4 M = Angel::mat4(1.0);     // Acumulated rotation matrix
GLfloat angle = 0.0f;   // Acumulated rotation angle of one translation segment
GLfloat angleIncrement=0.02f;    // Angle increment during rolling --update when idle are called

// Shadow Variables
point4 light_source(-14.0f, 12.0f, -3.0f, 1.0f);
color4 shadow_color(0.25f, 0.25f, 0.25f, 0.65f);
vec4 plane(0.0, 1.0, 0.0, 0.0);
mat4 shadow_matrix;
std::vector<color4> shadow_colors;

void getShadowMatrix(point4 light_source, vec4 plane) {
    shadow_matrix = mat4(1.0f);
    shadow_matrix[3][1] = -1.0f / light_source[1];
    shadow_matrix[3][3] = 0;
    shadow_matrix = Translate(light_source.x, light_source.y, light_source.z)
        * shadow_matrix
        * Translate(-light_source.x, -light_source.y, -light_source.z);
}


// Model-view and projection matrices uniform location
GLuint  ModelView, Projection;

/*----- Shader Lighting Parameters -----*/

// directional light
color4 global_ambient(1.0f, 1.0f, 1.0f, 1.0f);   //global ambient light
color4 dist_light(0.0f, 0.0f, 0.0f, 1.0f);
color4 diffuse(0.8f, 0.8f, 0.8f, 1.0f);
color4 specular(0.2f, 0.2f, 0.2f, 1.0f);
vec4 direction(0.1f, 0.0f, -1.0f, 0.0f); // in eye coord

// positional light
color4 pos_ambient(0.0f, 0.0f, 0.0f, 1.0f);
color4 pos_diffuse(1.0f, 1.0f, 1.0f, 1.0f);
color4 pos_specular(1.0f, 1.0f, 1.0f, 1.0f);
vec4 pos_light(-14.0f, 12.0f, -3.0f, 1.0f); // in world coord
float const_att = 2.0f;
float linear_att = 0.01f;
float quad_att = 0.001f;

// spotlight
vec4 spot_light_pos(-6.0, 0.0, -4.5, 1.0); // in world coord
vec4 spot_light_dir = normalize(pos_light - spot_light_pos);
float spot_light_exp = 15.0f;
float spot_light_ang = 20.0f / 180.0f * M_PI; // angle in radian

// Floor lighting
// directional
color4 floor_diffuse(0.0f, 1.0f, 0.0f, 1.0f);
color4 floor_ambient(0.2f, 0.2f, 0.2f, 1.0f);
color4 floor_specular(0.0f, 0.0f, 0.0f, 1.0f);
color4 floor_ambient_product = (global_ambient + dist_light) * floor_ambient;
color4 floor_diffuse_product = diffuse * floor_diffuse;
color4 floor_specular_product = specular * floor_specular;
// positional
color4 floor_ambient_product_pos = pos_ambient * floor_ambient;
color4 floor_diffuse_product_pos = pos_diffuse * floor_diffuse;
color4 floor_specular_product_pos = pos_specular * floor_specular;



// Sphere lighting 
// directional
color4 sphere_diffuse(1.0f, 0.84f, 0.0f, 1.0f);
color4 sphere_ambient(0.2f, 0.2f, 0.2f, 1.0f);
color4 sphere_specular(1.0f, 0.84f, 0.0f, 1.0f);
float sphere_shininess = 125.0f;

color4 sphere_ambient_product = (global_ambient + dist_light) * sphere_ambient;
color4 sphere_diffuse_product = diffuse * sphere_diffuse;
color4 sphere_specular_product = specular * sphere_specular;
// positional
color4 sphere_ambient_product_pos = pos_ambient * sphere_ambient;
color4 sphere_diffuse_product_pos = pos_diffuse * sphere_diffuse;
color4 sphere_specular_product_pos = pos_specular * sphere_specular;


// Fog Variables
float linear_start = 0.0;
float linear_end = 18.0;
float exp_density = 0.09;
color4 fog_color(0.7, 0.7, 0.7, 0.5);

// Fireworks Variables
//int firework = 0; // 0: no firework; 1: enable firework
const int N = 300; // Number of particles
const float Tmax = 10000.0f;
point4 firework_point[N];
vec3 firework_velocity[N];
color4 firework_color[N];
float t = 0;
float t_sub;
float modified_t;

void updateTime() {
    t = fmod(glutGet(GLUT_ELAPSED_TIME) - t_sub, Tmax);
}

void initializeParticles() {
    for (int i = 0; i < N; i++) {
        // Each particle has the same initial position
        firework_point[i] = vec4(0.0f, 0.1f, 0.0f, 1.0f);

        // Assign random velocity
        firework_velocity[i].x = 2.0 * ((rand() % 256) / 256.0 - 0.5);
        firework_velocity[i].y = 1.2 * 2.0 * ((rand() % 256) / 256.0);
        firework_velocity[i].z = 2.0 * ((rand() % 256) / 256.0 - 0.5);
        //std::cout<<firework_velocity[i].x<<" "<<firework_velocity[i].y<<" "<<firework_velocity[i].z<<"\n";
        // Assign random color
        firework_color[i].x = (rand() % 256) / 256.0;
        firework_color[i].y = (rand() % 256) / 256.0;
        firework_color[i].z = (rand() % 256) / 256.0;
        firework_color[i].w = 1.0f;
    }
}


// ------------------------ setup fog variables in f-shader ------------------------------------
void SetUp_fog_Uniform_Vars() {
    glUniform1f(glGetUniformLocation(program, "linear_start"), linear_start);
    glUniform1f(glGetUniformLocation(program, "linear_end"), linear_end);
    glUniform1f(glGetUniformLocation(program, "fog_density"), exp_density);
    glUniform4fv(glGetUniformLocation(program, "fog_color"), 1, fog_color);
}

int Index = 0;
#define ImageWidth  64
#define ImageHeight 64
#define	stripeImageWidth 32
GLubyte Image[ImageHeight][ImageWidth][4];
GLubyte stripeImage[4 * stripeImageWidth];
// static GLuint textureName[2];
GLuint textureName1, textureName2;

void image_set_up(void)
{
    int i, j, c;

    /* --- Generate checkerboard image to the image array ---*/
    for (i = 0; i < ImageHeight; i++)
        for (j = 0; j < ImageWidth; j++)
        {
            c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0));

            if (c == 1) /* white */
            {
                c = 255;
                Image[i][j][0] = (GLubyte)c;
                Image[i][j][1] = (GLubyte)c;
                Image[i][j][2] = (GLubyte)c;
            }
            else  /* green */
            {
                Image[i][j][0] = (GLubyte)0;
                Image[i][j][1] = (GLubyte)150;
                Image[i][j][2] = (GLubyte)0;
            }

            Image[i][j][3] = (GLubyte)255;
        }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*--- Generate 1D stripe image to array stripeImage[] ---*/
    for (j = 0; j < stripeImageWidth; j++) {
        /* When j <= 4, the color is (255, 0, 0),   i.e., red stripe/line.
           When j > 4,  the color is (255, 255, 0), i.e., yellow remaining texture
         */
        stripeImage[4 * j] = (GLubyte)255;
        stripeImage[4 * j + 1] = (GLubyte)((j > 4) ? 255 : 0);
        stripeImage[4 * j + 2] = (GLubyte)0;
        stripeImage[4 * j + 3] = (GLubyte)255;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

//----------------------------------------------------------------------------
// Read input file and store sphere vertices
void readFile() {
    // Get user input file name
    std::string fileName;
    std::cout << "Enter your file name:  ";
    std::cin >> fileName;
    // Open file
    std::ifstream file;
    file.open(fileName);
    // Get correct file name when cannot find the file
    while (!file.is_open()) {
        std::cout << "File does not exist. Re-enter your file name:  ";
        std::cin >> fileName;
        file.open(fileName);
    }
    // Read first value for number of triangles
    int numOfTriangles;
    file >> numOfTriangles;
    int v;
    float x, y, z;
    // Read x,y,r of each row and store them in vectors
    for (int i = 0; i < numOfTriangles; i++) {
        file >> v;
        std::vector<point3> polygon;
        for (int j = 0; j < v; j++) {
            file >> x >> y >> z;
            sphere_triangles.emplace_back(x, y, z,1.0f);
        }
    }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Update the center of sphere adn the accumulate rotaion matrix during rolling
void updateSpherePosition() {
    vec4 direction = normalize(pathPoints[(currentPoint + 1) % 3] - pathPoints[currentPoint]);
    vec3 axis = cross(vec3(0.0, 1.0, 0.0), vec3(direction.x, direction.y, direction.z));

    float distanceMoved = angle * (2.0f * M_PI * radius / 360.0f);
    float totalPathLength = length(pathPoints[(currentPoint + 1) % 3] - pathPoints[currentPoint]);
    
    M = Rotate(angleIncrement, axis.x, axis.y, axis.z)*M;    // update at every rotation -- more natural

    if (distanceMoved >= totalPathLength) {
        currentPoint = (currentPoint + 1) % 3;
        angle = 0.0f;
    }
    else {
        sphereCenter = pathPoints[currentPoint] + direction * distanceMoved;

    }
}

//----------------------------------------------------------------------------
// Set the color of sphere&shadow color
void setColors() {
    for (size_t i = 0; i < sphere_triangles.size(); ++i) {
        sphere_color.emplace_back(1.0f, 0.84f, 0.0f,1.0f); // golden yellow
        shadow_colors.emplace_back(shadow_color);
    }
}

// Generate 2 triangles: 6 vertices and 6 colors
void floor() {
    floor_colors[0] = color4(0, 1, 0, 1); floor_points[0] = floor_vertices[1];
    floor_colors[1] = color4(0, 1, 0, 1); floor_points[1] = floor_vertices[2];
    floor_colors[2] = color4(0, 1, 0, 1); floor_points[2] = floor_vertices[3];
    floor_colors[3] = color4(0, 1, 0, 1); floor_points[3] = floor_vertices[3];
    floor_colors[4] = color4(0, 1, 0, 1); floor_points[4] = floor_vertices[0];
    floor_colors[5] = color4(0, 1, 0, 1); floor_points[5] = floor_vertices[1];
    floor_normals.resize(floor_NumVertices);
    vec4 u = floor_vertices[2] - floor_vertices[1];
    vec4 v = floor_vertices[0] - floor_vertices[1];
    vec3 normal = normalize(cross(u, v));
    for (int i = 0; i < floor_NumVertices; i++) {
        floor_normals[i] = normal;
    }
    /**
    quad_texCoord[0] = vec2(1.0, 1.0);
    quad_texCoord[1] = vec2(1.0, 0.0);
    quad_texCoord[2] = vec2(0.0, 0.0);

    quad_texCoord[3] = vec2(0.0, 0.0);
    quad_texCoord[4] = vec2(0.0, 1.0);
    quad_texCoord[5] = vec2(1.0, 1.0);
} **/
    quad_texCoord[0] = vec2(1.25f, 0.00f);
    quad_texCoord[1] = vec2(0.00f, 0.0f);
    quad_texCoord[2] = vec2(0.0f, 1.50f);

    quad_texCoord[3] = vec2(0.0f, 1.50f);
    quad_texCoord[4] = vec2(1.25f, 1.50f);
    quad_texCoord[5] = vec2(1.25f, 0.00f);
}
//----------------------------------------------------------------------------
// Generate x,y,z axis
void axes() {
    axis_colors[0] = color4(1, 0, 0, 1); axis_points[0] = point4(0.0f, 0.02f, 0.0f, 1.0f);	//x-axis
    axis_colors[1] = color4(1, 0, 0, 1); axis_points[1] = point4(10.0f, 0.02f, 0.0f, 1.0f);	//x-axis
    axis_colors[2] = color4(1, 0, 1, 1); axis_points[2] = point4(0.0f, 0.0f, 0.0f, 1.0f);	    //y-axis
    axis_colors[3] = color4(1, 0, 1, 1); axis_points[3] = point4(0.0f, 10.0f, 0.0f, 1.0f);	//y-axis
    axis_colors[4] = color4(0, 0, 1, 1); axis_points[4] = point4(0.0f, 0.02f, 0.0f, 1.0f);	//z-axis
    axis_colors[5] = color4(0, 0, 1, 1); axis_points[5] = point4(0.0f, 0.02f, 10.0f, 1.0f);	//z-axis
}

void init() {
    // Texture Mapping
    readFile();
    image_set_up();
    /*--- Create and Initialize a texture object ---*/
    // Generate and bind texture obj name(s)
    glGenTextures(1, &textureName1);
    glActiveTexture(GL_TEXTURE0);  // Set the active texture unit to be 0 
    glBindTexture(GL_TEXTURE_2D, textureName1);// Bind the texture to this texture unit
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImageWidth, ImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Image);
    /*--- Create and Initialize a texture for strip ---*/
    glGenTextures(1, &textureName2);      // Generate texture obj name(s)
    glActiveTexture(GL_TEXTURE1);  // Set the active texture unit to be 1 
    glBindTexture(GL_TEXTURE_1D, textureName2); // Bind the texture to this texture unit
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, stripeImageWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, stripeImage);

    /*--- shadow---*/
    // shadow  matrix
    getShadowMatrix(light_source, plane);

    // sphere
    setColors();  //for sphere&shadow color
    
    sphere_normals.resize(sphere_triangles.size());

    for (size_t n = 0; n < sphere_triangles.size() / 3; n++) {
        point4& p0 = sphere_triangles[n * 3 + 0];
        point4& p1 = sphere_triangles[n * 3 + 1];
        point4& p2 = sphere_triangles[n * 3 + 2];

        point4 u = p1 - p0;
        point4 v = p2 - p0;

        vec3 normal = cross(u, v);
        normal = normalize(normal); 

        sphere_normals[n * 3 + 0] = normal;
        sphere_normals[n * 3 + 1] = normal;
        sphere_normals[n * 3 + 2] = normal;
    }

    //  Set sphere normals 
    glGenBuffers(1, &sphere_buffer);  //generate VBO
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);   //bind VBO to color buffer
    // upload to gpu
    glBufferData(GL_ARRAY_BUFFER,
        sphere_triangles.size() * sizeof(point4) +
        sphere_normals.size()* sizeof(vec3) +
        sphere_color.size() * sizeof(color4),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        sphere_triangles.size() * sizeof(point4),
        sphere_triangles.data());
    glBufferSubData(GL_ARRAY_BUFFER,
        sphere_triangles.size() * sizeof(point4),
        sphere_normals.size() * sizeof(vec3),
        sphere_normals.data());
    glBufferSubData(GL_ARRAY_BUFFER,
        sphere_triangles.size() * sizeof(point4) + sphere_normals.size() * sizeof(vec3),
        sphere_color.size() * sizeof(color4),
        sphere_color.data());

    //shadow
    glGenBuffers(1, &shadow_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, shadow_buffer);
    glBufferData(GL_ARRAY_BUFFER, sphere_triangles.size() * sizeof(point4) + shadow_colors.size() * sizeof(color4), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_triangles.size() * sizeof(point4), sphere_triangles.data());
    glBufferSubData(GL_ARRAY_BUFFER, sphere_triangles.size() * sizeof(point4), shadow_colors.size() * sizeof(color4), shadow_colors.data());
    
    //floor
    floor();
    glGenBuffers(1, &floor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
    glBufferData(GL_ARRAY_BUFFER,
        floor_NumVertices * sizeof(point4) +
        floor_normals.size() * sizeof(vec3) +
        floor_NumVertices * sizeof(color4)+sizeof(quad_texCoord),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        floor_NumVertices * sizeof(point4),
        floor_points);
    glBufferSubData(GL_ARRAY_BUFFER,
        floor_NumVertices * sizeof(point4),
        floor_normals.size() * sizeof(vec3),
        floor_normals.data());
    glBufferSubData(GL_ARRAY_BUFFER,
        floor_NumVertices * sizeof(point4) + floor_normals.size() * sizeof(vec3),
        floor_NumVertices * sizeof(color4),
        floor_colors);
    glBufferSubData(GL_ARRAY_BUFFER,
        floor_NumVertices * sizeof(point4) + floor_normals.size() * sizeof(vec3)+ floor_NumVertices * sizeof(color4),
        sizeof(quad_texCoord),
        quad_texCoord);
   
    //axes
    axes();
    glGenBuffers(1, &axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axis_points) + sizeof(axis_colors),NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(axis_points), axis_points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(axis_points), sizeof(axis_colors),axis_colors);

    // Create and initialize a vertex buffer object for fireworks, to be used in display()
    initializeParticles();
    glGenBuffers(1, &firework_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, firework_buffer);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(firework_point) + sizeof(firework_color) + sizeof(firework_velocity),
        NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(firework_point), firework_point);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(firework_point), sizeof(firework_color),
        firework_color);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(firework_point) + sizeof(firework_color),
        sizeof(firework_velocity), firework_velocity);

    // Load shaders and create a shader program (to be used in display())
    program = InitShader("vshader53.glsl", "fshader53.glsl");
    firework_program = InitShader("vFireworks.glsl", "fFireworks.glsl");
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.529f, 0.807f, 0.92f, 0.0f);
    glLineWidth(2.0);
}

void drawFirework()
{
    glUseProgram(firework_program);
    glPointSize(3.0);
    //glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINTS);

    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, firework_buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(firework_program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(firework_program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point4) * N));

    GLuint vVelocity = glGetAttribLocation(firework_program, "vVelocity");
    glEnableVertexAttribArray(vVelocity);
    glVertexAttribPointer(vVelocity, 3, GL_FLOAT, GL_FALSE, 0,
        BUFFER_OFFSET(sizeof(point4) * N + sizeof(color4) * N));
    // the offset is the (total) size of the previous vertex attribute array(s)

  /* Draw a sequence of geometric objs (triangles) from the vertex buffer
     (using the attributes specified in each enabled vertex attribute array) */
    glDrawArrays(GL_POINTS, 0, N);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vColor);
    glDisableVertexAttribArray(vVelocity);
}

//----------------------------------------------------------------------
// SetUp_Lighting_Uniform_Vars(mat4 mv):
// Set up lighting parameters that are uniform variables in shader.
//
// Note: "LightPosition" in shader must be in the Eye Frame.
//       So we use parameter "mv", the model-view matrix, to transform
//       light_position to the Eye Frame.
//----------------------------------------------------------------------

// Function to setup lighting uniform variables for directional lighting (sphere and floor (part c))
void SetUp_Lighting_Uniform_Vars(bool isSphere, color4 ambient_product, color4 diffuse_product, color4 specular_product) {
    glUniform1i(glGetUniformLocation(program, "SphereFlag"), isSphere);
    glUniform4fv(glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product);
    glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product);
    glUniform4fv(glGetUniformLocation(program, "SpecularProduct"), 1, specular_product);
    glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, direction); // Already in Eye Frame
    glUniform1i(glGetUniformLocation(program, "ShadingType"), shading_type);
    if (isSphere) glUniform1f(glGetUniformLocation(program, "Shininess"), sphere_shininess);
    else glUniform1f(glGetUniformLocation(program, "Shininess"), 1.0);
}

// Function to setup lighting uniform variables for positianl light (part d)
void SetUp_Pos_Lighting_Uniform_Vars(mat4 mv, color4 pos_ambient_product, color4 pos_diffuse_product, color4 pos_specular_product) {
    glUniform4fv(glGetUniformLocation(program, "PosAmbientProduct"), 1, pos_ambient_product);
    glUniform4fv(glGetUniformLocation(program, "PosDiffuseProduct"), 1, pos_diffuse_product);
    glUniform4fv(glGetUniformLocation(program, "PosSpecularProduct"), 1, pos_specular_product);
    glUniform1f(glGetUniformLocation(program, "SpotLightExponent"), spot_light_exp);
    glUniform1f(glGetUniformLocation(program, "SpotLightAngle"), spot_light_ang);
    glUniform1f(glGetUniformLocation(program, "ConstAtt"), const_att);
    glUniform1f(glGetUniformLocation(program, "LinearAtt"), linear_att);
    glUniform1f(glGetUniformLocation(program, "QuadAtt"), quad_att);
}

void drawaxes(GLuint buffer, int num_vertices, GLenum mode)
{
    // disable lighting for axes and shadow
    glUniform1i(glGetUniformLocation(program, "uUseLighting"), 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    GLuint vColor = glGetAttribLocation(program, "vColor");

    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4) * num_vertices));

    glDrawArrays(mode, 0, num_vertices);

    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vColor);

}

void drawObj(GLuint buffer, int num_vertices, GLenum mode, int isfloor)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    GLuint vNormal = glGetAttribLocation(program, "vNormal");
    GLuint vColor = glGetAttribLocation(program, "vColor");
    GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");

    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4) * num_vertices));

    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4) * num_vertices + sizeof(vec3) * num_vertices));

    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4) * num_vertices + sizeof(vec3) * num_vertices + sizeof(point4) * num_vertices));
    glDrawArrays(mode, 0, num_vertices);

    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vNormal);
    glDisableVertexAttribArray(vColor);
    glDisableVertexAttribArray(vTexCoord);

    //glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawFloor(mat4 mv, mat4 p)
{
    glUniform1i(glGetUniformLocation(program, "shadow"), false);
    glUniform1i(glGetUniformLocation(program, "lattice_mode"), 0);
    mat3 normal_matrix;
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);
    glUniform1i(glGetUniformLocation(program, "tex_mode"), ground_tex);
    glUniform1i(glGetUniformLocation(program, "fog_mode"), fog);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);   // kill FILL even wireframe
    if (lighting_on) {
        glUniform4fv(glGetUniformLocation(program, "SpotLightPosition"), 1, mv * pos_light);
        normal_matrix = NormalMatrix(mv, 1);
        mat4 nm = mat4WithUpperLeftMat3(normal_matrix);
        glUniform4fv(glGetUniformLocation(program, "SpotLightDirection"), 1, nm * spot_light_dir);
        SetUp_Lighting_Uniform_Vars(false, floor_ambient_product, floor_diffuse_product, floor_specular_product);
        SetUp_Pos_Lighting_Uniform_Vars(mv, floor_ambient_product_pos, floor_diffuse_product_pos, floor_specular_product_pos);
        glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);
        glUniform1i(glGetUniformLocation(program, "uUseLighting"), 1);
    }
    else {
        glUniform1i(glGetUniformLocation(program, "uUseLighting"), 0);
    }
    //glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);
    //glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"),1, GL_TRUE, normal_matrix);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);   // kill FILL even wireframe
    drawObj(floor_buffer, floor_NumVertices, GL_TRIANGLES, 1);
    glUniform1i(glGetUniformLocation(program, "tex_mode"), 0);
  

}

void drawShadow(mat4 mv, mat4 p)
{
    glUniform1i(glGetUniformLocation(program, "shadow"), true);
    glUniform1i(glGetUniformLocation(program, "lattice_mode"), latticeMode);
    glUniform1i(glGetUniformLocation(program, "fog_mode"), fog);
    if (blending == 1)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);    // allows semi-transparent shadow
    }
    //mv = LookAt(eye, at, up) * shadow_matrix * Translate(sphereCenter.x, sphereCenter.y, sphereCenter.z) * M;
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    drawaxes(shadow_buffer, sphere_triangles.size(), GL_TRIANGLES);
    //glDepthMask(GL_TRUE);
    if (blending) {
        glDisable(GL_BLEND);
    }
    glUniform1i(glGetUniformLocation(program, "shadow"), false);
    glUniform1i(glGetUniformLocation(program, "lattice_mode"), 0);
}



void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // clear buffer
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);

    glUseProgram(program); // Use the shader program --> initialized in init()

    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");
    GLint uLightingEnabledLoc = glGetUniformLocation(program, "uLightingEnabled");
    GLint SphereFlag = glGetUniformLocation(program, "SphereFlag");
    glUniform1i(glGetUniformLocation(program, "LightSource"), light_src);
    glUniform1i(glGetUniformLocation(program, "fog_mode"), fog);
    glUniform1i(glGetUniformLocation(program, "shadow"), false);
    glUniform1i(glGetUniformLocation(program, "texture_2D"), 0);
    glUniform1i(glGetUniformLocation(program, "texture_1D"), 1);
    glUniform1i(glGetUniformLocation(program, "lattice_mode"), latticeMode);
    glUniform1i(glGetUniformLocation(program, "frame_mode"), frameMode);
    glUniform1i(glGetUniformLocation(program, "sphereCB"), sphereCB);
    glUniform1i(glGetUniformLocation(program, "fog_mode"), fog);


    //Projection matrix to the shader
    mat4  p = Perspective(fovy, aspect, zNear, zFar);   // return 4*4 matrix 
    glUniformMatrix4fv(Projection, 1, GL_TRUE, p);
    //vec4 eye(7.0, 3.0, -10.0, 1.0); // VRP
    // set up  model view matrix
    vec4 at(0.0, 0.0, 0.0, 1.0);
    vec4 up(0.0, 1.0, 0.0, 0.0);
    mat3 normal_matrix;

    mat4 mv = LookAt(eye, at, up);
    
    // sphere  -- rotate
    glUniform1i(glGetUniformLocation(program, "sphereTex"), sphereTex);
    glUniform4fv(glGetUniformLocation(program, "SpotLightPosition"), 1, mv * pos_light);
    normal_matrix = NormalMatrix(mv, 1);
    mat4 nm = mat4WithUpperLeftMat3(normal_matrix);
    glUniform4fv(glGetUniformLocation(program, "SpotLightDirection"), 1, nm * spot_light_dir);
    mv = LookAt(eye, at, up) * Translate(sphereCenter.x, sphereCenter.y, sphereCenter.z) * M;//rotate around y- 
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);   // send to the shader program
    if (lighting_on and !wireframe){
        SetUp_Lighting_Uniform_Vars(true, sphere_ambient_product, sphere_diffuse_product, sphere_specular_product);
        SetUp_Pos_Lighting_Uniform_Vars(mv, sphere_ambient_product_pos, sphere_diffuse_product_pos, sphere_specular_product_pos);
        SetUp_fog_Uniform_Vars();
        glUniform1i(glGetUniformLocation(program, "uUseLighting"), 1);
        normal_matrix = NormalMatrix(mv, 1);
        glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"), 1, GL_TRUE, normal_matrix);
    }
    else{
        glUniform1i(glGetUniformLocation(program, "uUseLighting"), 0);
    }
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    drawObj(sphere_buffer, sphere_triangles.size(), GL_TRIANGLES,0);
    glUniform1i(glGetUniformLocation(program, "sphereTex"), 0);
    glUniform1i(glGetUniformLocation(program, "sphereCB"), 0);
    glUniform1i(glGetUniformLocation(program, "lattice_mode"), 0);
    
    // for the floor
 
    glDepthMask(GL_FALSE);
    mv = LookAt(eye, at, up);
    drawFloor(mv, p);
    
    if (shadow && eye[1] > 0) {
        mv = LookAt(eye, at, up) * shadow_matrix * Translate(sphereCenter.x, sphereCenter.y, sphereCenter.z) * M;
        drawShadow(mv, p);
    }

    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    mv = LookAt(eye, at, up);
    drawFloor(mv, p);
    if (shadow && eye[1] > 0) {
        mat4 shadow_mv = mv * shadow_matrix * Translate(sphereCenter.x, sphereCenter.y, sphereCenter.z) * M;
        drawShadow(shadow_mv, p);
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);


    glUniform1i(glGetUniformLocation(program, "shadow"), false);
        mv = LookAt(eye, at, up);
        glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawaxes(axis_buffer, axis_NumVertices, GL_LINES);
    
    /*----- Draw Fireworks -----*/
    if (firework == 1) {
        glUseProgram(firework_program); // Use the shader program
        ModelView = glGetUniformLocation(firework_program, "ModelView");
        Projection = glGetUniformLocation(firework_program, "Projection");
        glUniform1f(glGetUniformLocation(firework_program, "t"), t);

        p = Perspective(fovy, aspect, zNear, zFar);
        glUniformMatrix4fv(Projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major
        mv = LookAt(eye, at, up);
        glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
        updateTime();
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINTS);
        drawFirework();

    }
    glutSwapBuffers();
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void idle(void) {
    angle = angle + angleIncrement;
    updateSpherePosition();
    glutPostRedisplay();
}
void reshape(int width, int height)   // graphics window rehape
{
    glViewport(0, 0, width, height);
    aspect = (GLfloat)width / (GLfloat)height;   // projection aspect_ratio
    glutPostRedisplay();
}
void keyboard(unsigned char key, int x, int y)
{
    //using namespace std::chrono;
    switch (key) {
    case 033: // Escape Key
    case 'X': eye[0] += 1.0; break;
    case 'x': eye[0] -= 1.0; break;
    case 'Y': eye[1] += 1.0; break;
    case 'y': eye[1] -= 1.0; break;
    case 'Z': eye[2] += 1.0; break;
    case 'z': eye[2] -= 1.0; break;
    case 'b': case 'B':
        //lastUpdateTime = steady_clock::now();    // initialize lastUpdateTime
        glutIdleFunc(idle); // Begin rolling
        animationFlag = 1;
        rolling_begin = true;
        break;
    case 'V': case 'v':
        sphereTex = 1;
        break;
    case 's': case 'S':
        sphereTex = 2;
        break;
    case 'o': case 'O':
        frameMode = 1;
        break;
    case 'e': case 'E':
        frameMode = 2;
        break;
    case 'u': case 'U':
        latticeMode = 1; // Upright
        break;
    case 't': case 'T':
        latticeMode = 2; // Tilted
        break;
    case 'l': case 'L':
        latticeMode = 0; // No lattice
        break;
    }
    glutPostRedisplay();
}

void myMouse(int button, int state, int x, int y) {
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        if (rolling_begin && animationFlag == 1) {
            animationFlag = 0;
            glutIdleFunc(NULL);
        }
        else if (rolling_begin && animationFlag == 0) {
            animationFlag = 1;
            glutIdleFunc(idle);
        }
    }
}
void shadow_menu(int id) {
    switch (id) {
    case 1:
        shadow = false;
        break;
    case 2:
        shadow = true;
        break;
    }
    glutPostRedisplay();
}

void wireframe_menu(int id) {
    switch (id) {
    case 1:
        wireframe = false;
        break;
    case 2:
        wireframe = true;
        break;
    }
    glutPostRedisplay();
}

void shading_menu(int id) {
    switch (id) {
    case 1:
        shading_type = 0;  // Flat Shading
        break;
    case 2:
        shading_type = 1;   // Smooth Shading
        break;
    }
    wireframe = false;
    glutPostRedisplay();
}

void lighting_menu(int id) {
    switch (id) {
    case 1:
        lighting_on = 0;
        shading_type = 0;
        break;
    case 2:
        lighting_on = 1;
        break;
    }
    glutPostRedisplay();
}

void light_src_menu(int id) {
    switch (id) {
    case 1: light_src = 0; break;  // Spot Light 
    case 2: light_src = 1; break;  // Point Source
    }
    glutPostRedisplay();
}

void fog_menu(int id) {
    switch (id) {
    case 1: fog = 0; break; // No fog break;
    case 2: fog = 1; break;   // Linear fog
    case 3: fog = 2; break;   // Exp fog   
    case 4: fog = 3; break;  // Exp Sqr fog
    }
    glutPostRedisplay();
}

void blending_menu(int id) {
    switch (id) {
    case 1: blending = 1; break; // blending
    case 2: blending = 0; break; // No blending
    }
    glutPostRedisplay();
}

void ground_tex_menu(int id) {
    switch (id) {
    case 1: ground_tex = 1; break; // Texture On
    case 2: ground_tex = 0; break;  // Off
    }
    glutPostRedisplay();
}

void sphere_tex_menu(int id) {
    switch (id) {
    case 1:
        sphereTex = 0;
        sphereCB = 0;
        break; // No
    case 2:
        sphereTex = 1;
        sphereCB = 0;   // Yes - Contour Lines
        break;
    case 3: sphereCB = 1; break;  // Yes - Checkerboard 
    }
    glutPostRedisplay();
}
void firework_menu(int id) {
    switch (id) {
    case 1:
        firework = 0;
        break; // No
    case 2:
        firework = 1;
        break;
    }
    glutPostRedisplay();
}


void main_menu(int id) {
    switch (id) {
    case 1:
        eye = init_eye;
        break;
    case 2:
        exit(EXIT_SUCCESS);
        break;
    case 3:
        if (wireframe) {
            wireframe = false;
        }
        else {
            wireframe = true;
        }
        break;
    }
    glutPostRedisplay();
}

void menu() {
    GLuint shadow_sub = glutCreateMenu(shadow_menu);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);
    GLuint wireframe_sub = glutCreateMenu(wireframe_menu);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);
    GLuint shading_sub = glutCreateMenu(shading_menu);
    glutAddMenuEntry(" Flat Shading ", 1);
    glutAddMenuEntry(" Smooth Shading ", 2);
    GLuint lighting_sub = glutCreateMenu(lighting_menu);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);
    GLuint light_src_sub = glutCreateMenu(light_src_menu);
    glutAddMenuEntry(" Spot Light ", 1);
    glutAddMenuEntry(" Point Source ", 2);
    GLuint fog_sub = glutCreateMenu(fog_menu);
    glutAddMenuEntry(" No Fog ", 1);
    glutAddMenuEntry(" Linear  ", 2);
    glutAddMenuEntry(" Exponential  ", 3);
    glutAddMenuEntry(" Exponential Square ", 4);
    GLuint blending_sub = glutCreateMenu(blending_menu);
    glutAddMenuEntry(" Yes ", 1);
    glutAddMenuEntry(" No ", 2);
    GLuint ground_tex_sub = glutCreateMenu(ground_tex_menu);
    glutAddMenuEntry(" Yes ", 1);
    glutAddMenuEntry(" No ", 2);
    GLuint sphere_tex_sub = glutCreateMenu(sphere_tex_menu);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes - Contour Lines ", 2);
    glutAddMenuEntry(" Yes - Checkerboard ", 3);
    GLuint firework_sub = glutCreateMenu(firework_menu);
    glutAddMenuEntry(" No ", 1);
    glutAddMenuEntry(" Yes ", 2);

    int menu_ID = glutCreateMenu(main_menu);
    glutSetMenuFont(menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddSubMenu(" Shadow ", shadow_sub);
    glutAddSubMenu(" Enable Lighting ", lighting_sub);
    glutAddSubMenu(" Wire Frame Sphere ", wireframe_sub);
    glutAddSubMenu(" Shading ", shading_sub);
    glutAddSubMenu(" Light Source ", light_src_sub);
    glutAddSubMenu(" Fog Options ", fog_sub);
    glutAddSubMenu(" Blending Shadow ", blending_sub);
    glutAddSubMenu(" Texture Mapped Ground ", ground_tex_sub);
    glutAddSubMenu(" Texture Mapped Sphere ", sphere_tex_sub);
    glutAddSubMenu(" Fireworks ", firework_sub);
    glutAddMenuEntry(" Default View Point ", 1);
    glutAddMenuEntry(" Quit ", 2);
    glutAttachMenu(GLUT_LEFT_BUTTON);
}
int main(int argc, char** argv) {
    glutInit(&argc, argv);
#ifdef __APPLE__ // Enable core profile of OpenGL 3.2 on macOS.
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowSize(512, 512);
    glutCreateWindow("Assignment4");

#ifdef __APPLE__ // on macOS
    // Core profile requires to create a Vertex Array Object (VAO).
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#else           // on Linux or Windows, we still need glew
    /* Call glewInit() and error checking */
    int err = glewInit();
    if (GLEW_OK != err)
    {
        printf("Error: glewInit failed: %s\n", (char*)glewGetErrorString(err));
        exit(1);
    }
#endif
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("OpenGL version supported %s\n", glGetString(GL_VERSION));
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    //glutIdleFunc(idle);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(NULL); // Initially, show the sphere standing still
    glutKeyboardFunc(keyboard);
    glutMouseFunc(myMouse);
    menu();
    /*
    int menu_ID;
    menu_ID = glutCreateMenu(myMenu);
    glutSetMenuFont(menu_ID, GLUT_BITMAP_HELVETICA_18);
    glutAddMenuEntry(" Default View Point ", 1);
    glutAddMenuEntry(" Quit ", 2);
    glutAttachMenu(GLUT_LEFT_BUTTON);
    */

    init();
    glutMainLoop();
    return 0;
}