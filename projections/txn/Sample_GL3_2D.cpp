#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include<map>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
	GLuint VertexArrayID;
	GLuint VertexBuffer;
	GLuint ColorBuffer;

	GLenum PrimitiveMode;
	GLenum FillMode;
	int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct Sprite {
    string name;
    float x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4, x5, y5, z5, x6, y6, z6, x7, y7, z7, x8, y8, z8;
		string orientation;
		glm::mat4 mvp,model,mvpf;
};
map <string, Sprite> cubeObjects;
int do_rot, floor_rel,turn_left=0,turn_right=0, turn_forward=0, turn_backward=0;
int key_press=0, rot_varLeft=0, rot_varRight=0, rot_varForward=0, rot_varBackward=0;
int XYplaneAngle=0, YZplaneAngle=0, reached_flag=0,main_flag=0;
float totalAngleRotatedX=0,totalAngleRotatedY=0, totalAngleRotatedZ=0;
GLuint programID;
double last_update_time, current_time;
glm::vec3 rect_pos, floor_pos;
float rectangle_rotation = 0;
VAO *fragileTile, *bridgeTilesLines,*tilesLines, *rectangleLines, *tiles, *waterHole,*bridgeSwitch, *bridgeTiles;
int targetReached=0, bridgeSwitchPressed=0;
int top_view=0, block_view=0, follower_view=0, default_view=1, helicopter_view=0;
int rowArrayCounterA=10, colArrayCounterA=11, rowArrayCounterB=11, colArrayCounterB=11;
float xa=0, ya=0, za=0, xb=0, yb=0, zb=0, xc=0, yc=0, zc=0;
int fallStatus=0,level=1;
float downfall=0;
/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	//    printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	//    fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	//    printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	//    fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	//    fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	//    fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void initGLEW(void){
	glewExperimental = GL_TRUE;
	if(glewInit()!=GLEW_OK){
		fprintf(stderr,"Glew failed to initialize : %s\n", glewGetErrorString(glewInit()));
	}
	if(!GLEW_VERSION_3_3)
		fprintf(stderr, "3.3 version not available\n");
}



/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW);
	 // Copy the vertices into VBO
	glVertexAttribPointer(
			0,                  // attribute 0. Vertices
			3,                  // size (x,y,z)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
			1,                  // attribute 1. Color
			3,                  // size (r,g,b)
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);

	return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
	GLfloat* color_buffer_data = new GLfloat [3*numVertices];
	for (int i=0; i<numVertices; i++) {
		color_buffer_data [3*i] = red;
		color_buffer_data [3*i + 1] = green;
		color_buffer_data [3*i + 2] = blue;
	}

	return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Enable Vertex Attribute 1 - Color
	glEnableVertexAttribArray(1);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float rectangle_rot_dir = 1;
bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Function is called first on GLFW_PRESS.

	if (action == GLFW_RELEASE) {
		switch (key) {
			case GLFW_KEY_C:
				rectangle_rot_status = !rectangle_rot_status;
				break;
			case GLFW_KEY_P:
				break;
			case GLFW_KEY_E:
				key_press=0;
			case GLFW_KEY_T:
				key_press=0;
			case GLFW_KEY_X:
				// do something ..
				break;
			default:
				break;
		}
	}
	else if (action == GLFW_PRESS) {
		switch (key) {
			case GLFW_KEY_ESCAPE:
				quit(window);
				break;
			case GLFW_KEY_E:
				key_press=key_press-1;
				break;
			case GLFW_KEY_T:
				key_press= key_press+1;
				break;
			default:
				break;
		}
	}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
			quit(window);
			break;
		case 'a':
			turn_left=1;
			break;
		case 'd':
			turn_right=1;
			break;
		case 'w':
			turn_forward=1;
			break;
		case 's':
			turn_backward=1;
			break;
		case 't':
			top_view=1;
			default_view=0;
			follower_view=0;
			block_view=0;
			helicopter_view=0;
			//xa=0;ya=0;za=0;xb=0;yb=0;zb=0;xc=0;yc=0;zc=0;
			break;
		case 'b':
			block_view=1;
			top_view=0;
			follower_view=0;
			default_view=0;
			helicopter_view=0;
			//xa=0;ya=0;za=0;xb=0;yb=0;zb=0;xc=0;yc=0;zc=0;
			break;
		case 'v':
			default_view=1;
			top_view=0;
			follower_view=0;
			helicopter_view=0;
			block_view=0;
		//	xa=0;ya=0;za=0;xb=0;yb=0;zb=0;xc=0;yc=0;zc=0;
			break;
		case 'f':
			follower_view=1;
			top_view=0;
			block_view=0;
			helicopter_view=0;
			default_view=0;
		//	xa=0;ya=0;za=0;xb=0;yb=0;zb=0;xc=0;yc=0;zc=0;
			break;
		case 'h':
			follower_view=0;
			top_view=0;
			block_view=0;
			default_view=0;
			helicopter_view=1;
		case ' ':
			do_rot^=1;
			break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			if (action == GLFW_RELEASE) {
				rectangle_rot_dir *= -1;
			}
			break;
		default:
			break;
	}
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
	int fbwidth=width, fbheight=height;
	glfwGetFramebufferSize(window, &fbwidth, &fbheight);
	//	glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));

	GLfloat fov = 110*(M_PI)/180.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// Store the projection matrix in a variable for future use
	// Perspective projection for 3D views
	Matrices.projection = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 1000.0f);

	// Ortho projection for 2D views
	//	Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

//1 is for the normal bricks
//2 is for the target water hole
//3 is for the bridgeSwitch
//4 is for the bridge bricks that will only be visible upon pressing the switch
//5 is for the fragile bricks on which you can't stand on one face but can stay with two faces.
//int arr[21][21];
//void arr_init()
//{

//if(level==1)'
int arr[21][21]={
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,1,1,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 1,1,1,1,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 1,1,2,1,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 1,1,1,1,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 1, 1,1,1,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 4, 4,4,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 4, 4,4,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,1, 5, 1,3,0,0,0,0,0,0,0,0},

{0,0,0,0,0,0,0,0,0,1, 5, 1,1,0,0,0,0,0,0,0,0},

{0,0,0,0,0,0,0,0,0,1, 5, 1,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,1, 1, 1,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0}
};
void arr_init()
{
//	if (level==2)
	int arrLevel[21][21]={
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 1, 1,1,1,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 1, 2,1,1,1,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 1, 1,1,1,1,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 1,1,0,1,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 1, 1,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,1, 0, 0,4,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,4,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,3,0, 1, 1,1,0,0,0,0,0,0,0,0},

	{0,0,0,0,0,0,0,0,1,0, 5, 1,1,0,0,0,0,0,0,0,0},

	{0,0,0,0,0,0,0,0,1,0, 5, 1,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,1,1, 1, 1,1,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0, 0, 0,0,0,0,0,0,0,0,0,0}
	};
	int i=0, j=0;
	for(i=0;i<21;i++)
	for(j=0;j<21;j++)
	arr[i][j]=arrLevel[i][j];

}
//else

VAO *rectangle, *cam, *floor_vao;

// Creates the rectangle object used in this sample code
void createRectangle ()
{
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.5, 1.0f,
		-0.5, -0.5,1.0f,
		0.5, -0.5, 1.0f,
		-0.5, 0.5, 1.0f,
		0.5, -0.5, 1.0f,
		0.5, 0.5, 1.0f,
		0.5, 0.5, 1.0f,
		0.5, -0.5, 1.0f,
		0.5, -0.5, -1.0f,
		0.5, 0.5, 1.0f,
		0.5, -0.5, -1.0f,
		0.5, 0.5, -1.0f,
		0.5, 0.5, -1.0f,
		0.5, -0.5, -1.0f,
		-0.5, -0.5, -1.0f,
		0.5, 0.5, -1.0f,
		-0.5, -0.5, -1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, -0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		-0.5, 0.5, 1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, 0.5, 1.0f,
		0.5, 0.5, 1.0f,
		-0.5, 0.5, -1.0f,
		0.5, 0.5, 1.0f,
		0.5, 0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		-0.5, -0.5, -1.0f,
		0.5, -0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		0.5, -0.5, -1.0f,
		0.5, -0.5, 1.0f,

	};

	GLfloat color_buffer_data [] = {
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
		228.0/255.0,  	55/255.0, 24/255.0,
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	rectangle = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
	//rectangleLines=create3DObject(GL_TRIANGLES,36,vertex_buffer_data,color_buffer_data,GL_LINE);
}
void createRectangleLines ()
{
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.5, 1.0f,
		-0.5, -0.5,1.0f,
		0.5, -0.5, 1.0f,
		-0.5, 0.5, 1.0f,
		0.5, -0.5, 1.0f,
		0.5, 0.5, 1.0f,
		0.5, 0.5, 1.0f,
		0.5, -0.5, 1.0f,
		0.5, -0.5, -1.0f,
		0.5, 0.5, 1.0f,
		0.5, -0.5, -1.0f,
		0.5, 0.5, -1.0f,
		0.5, 0.5, -1.0f,
		0.5, -0.5, -1.0f,
		-0.5, -0.5, -1.0f,
		0.5, 0.5, -1.0f,
		-0.5, -0.5, -1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, -0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		-0.5, 0.5, 1.0f,
		-0.5, 0.5, -1.0f,
		-0.5, 0.5, 1.0f,
		0.5, 0.5, 1.0f,
		-0.5, 0.5, -1.0f,
		0.5, 0.5, 1.0f,
		0.5, 0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		-0.5, -0.5, -1.0f,
		0.5, -0.5, -1.0f,
		-0.5, -0.5, 1.0f,
		0.5, -0.5, -1.0f,
		0.5, -0.5, 1.0f,

	};

	GLfloat color_buffer_data [] = {
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	rectangleLines=create3DObject(GL_TRIANGLES,36,vertex_buffer_data,color_buffer_data,GL_LINE);
}
void createTile()
{
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.25, 0.5,
		-0.5, -0.25,0.5,
		0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, -0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, -0.25, 0.5,

		-0.5,0.25,-0.5,
		0.5,0.5,0.5,
		0.0,0.0,0.0,

		-0.5,0.25,0.5,
		0.5,-0.25,0.5,
		0.0,0.0,0.5,

		0.5,0.25,0.5,
		0.5,-0.25,0.5,
		0.5,0.0,0.5,


	};

	GLfloat color_buffer_data [] = {
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
		147/255.0,245/255.0,95/255.0,
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	tiles=create3DObject(GL_TRIANGLES, 45, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createTilesLines()
{
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.25, 0.5,
		-0.5, -0.25,0.5,
		0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, -0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, -0.25, 0.5,
	};

	GLfloat color_buffer_data [] = {
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
	};

	tilesLines=create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createHoles() // create the hole in which the block will fall finally.
{
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.25, 0.5,
		-0.5, -0.25,0.5,
		0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, -0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, -0.25, 0.5,

	};

	GLfloat color_buffer_data [] = {
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	waterHole=create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createBridgeSwitch()
{
	GLfloat color_buffer_data[1000];
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.25, 0.5,
		-0.5, -0.25,0.5,
		0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, -0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, -0.25, 0.5,
	};

	if(bridgeSwitchPressed==0)
		GLfloat color_buffer_data [] = {
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
		47/255.0,87/255.0,158/255.0,
	};

	else
		GLfloat color_buffer_data []={
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
			66/255.0, 178/255.0, 200/255.0,
		};
	// create3DObject creates and returns a handle to a VAO that can be used later
	bridgeSwitch=create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createBridgeTiles()
{
	// GL3 accepts only Triangles. Quads are not supported
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.25, 0.5,
		-0.5, -0.25,0.5,
		0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, -0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, -0.25, 0.5,
	};

	GLfloat color_buffer_data [] = {
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
		133/255.0,191/255.0,32/255.0,
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	bridgeTiles=create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createBridgeTilesLines()
{
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.25, 0.5,
		-0.5, -0.25,0.5,
		0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, -0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, -0.25, 0.5,
	};

	GLfloat color_buffer_data [] = {
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
		0.0,0.0,0.0,
	};
	bridgeTilesLines=create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_LINE);
}
void createFragileTile()
{
	GLfloat vertex_buffer_data [] = {
		-0.5, 0.25, 0.5,
		-0.5, -0.25,0.5,
		0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, 0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, -0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		-0.5, 0.25, 0.5,
		0.5, 0.25, 0.5,
		-0.5, 0.25, -0.5,
		0.5, 0.25, 0.5,
		0.5, 0.25, -0.5,
		-0.5, -0.25, 0.5,
		-0.5, -0.25, -0.5,
		0.5, -0.25, -0.5,
		-0.5, -0.25, 0.5,
		0.5, -0.25, -0.5,
		0.5, -0.25, 0.5,
	};

	GLfloat color_buffer_data [] = {
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
		241/255.0,245/255.0,11/255.0,
	};

	fragileTile=create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void checkSpecialConditions()
{
	if(level==1)
	{
	if(cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].x1== 2.5 && cubeObjects["cube"].x4==2.5 && cubeObjects["cube"].x5==2.5
	 && cubeObjects["cube"].x8==2.5 && cubeObjects["cube"].x2==3.5 && cubeObjects["cube"].x3==3.5 && cubeObjects["cube"].x6==3.5 && cubeObjects["cube"].x7==3.5
  && cubeObjects["cube"].z1==-6)
	 	{targetReached=1; return;} // when you achieve the goal, you return 1;
	if(cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].z1==-1 && cubeObjects["cube"].z2==-1 && cubeObjects["cube"].z3==-1
	   && cubeObjects["cube"].z4==-1 && cubeObjects["cube"].z5==-2 && cubeObjects["cube"].z6==-2 && cubeObjects["cube"].z7==-2
		 && cubeObjects["cube"].z8==-2 && cubeObjects["cube"].x1==1.5 && cubeObjects["cube"].x4==1.5 && cubeObjects["cube"].x5==1.5 && cubeObjects["cube"].x8==1.5
		 && cubeObjects["cube"].x2==2.5 && cubeObjects["cube"].x3==2.5 && cubeObjects["cube"].x6==2.5 && cubeObjects["cube"].x7==2.5)
		{
			//cout << "switch detected" << endl;
			 bridgeSwitchPressed=1;
			 createBridgeSwitch(); // when the bridgeSwitch gets pressed, so switch changes colour from dark blue to light blue.
		 }
		 if((cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].z1==1 && cubeObjects["cube"].z2==1 && cubeObjects["cube"].x1==-0.5 &&
	 	cubeObjects["cube"].x2==0.5) || (cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].z1==0 && cubeObjects["cube"].z2==0 && cubeObjects["cube"].x1==-0.5 &&
	 	cubeObjects["cube"].x2==0.5) || (cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].z1==-1 && cubeObjects["cube"].z2==-1 && cubeObjects["cube"].x1==-0.5 &&
	 	cubeObjects["cube"].x2==0.5))
	 	{
	 	//	cout << "helloaw" << endl;
	 		arr[10][10]=0;arr[9][10]=0;arr[8][10]=0;
	 		fallStatus=1;
	 	}
	}
	 else if(level==2)
	 {
		 if(cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].x1== 0.5 && cubeObjects["cube"].x4==0.5 && cubeObjects["cube"].x5==0.5
	 	 && cubeObjects["cube"].x8==0.5 && cubeObjects["cube"].x2==1.5 && cubeObjects["cube"].x3==1.5 && cubeObjects["cube"].x6==1.5 && cubeObjects["cube"].x7==1.5
	   && cubeObjects["cube"].z1==-7)
	 	 	{
				targetReached=1;
			//	cout << "hellowe" << endl;
				 return;
			}

			if(cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].z1==-1 && cubeObjects["cube"].z2==-1 && cubeObjects["cube"].z3==-1
			   && cubeObjects["cube"].z4==-1 && cubeObjects["cube"].z5==-2 && cubeObjects["cube"].z6==-2 && cubeObjects["cube"].z7==-2
				 && cubeObjects["cube"].z8==-2 && cubeObjects["cube"].x1==-2.5 && cubeObjects["cube"].x4==-2.5 && cubeObjects["cube"].x5==-2.5 && cubeObjects["cube"].x8==-2.5
				 && cubeObjects["cube"].x2==-1.5 && cubeObjects["cube"].x3==-1.5 && cubeObjects["cube"].x6==-1.5 && cubeObjects["cube"].x7==-1.5)
				 {
		 	//		cout << "switch detected" << endl;
		 			 bridgeSwitchPressed=1;
		 			 createBridgeSwitch(); // when the bridgeSwitch gets pressed, so switch changes colour from dark blue to light blue.
		 		 }
			if((cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].z1==1 && cubeObjects["cube"].z2==1 && cubeObjects["cube"].x1==-0.5 &&
			cubeObjects["cube"].x2==0.5) || (cubeObjects["cube"].orientation=="alongY" && cubeObjects["cube"].z1==0 && cubeObjects["cube"].z2==0 && cubeObjects["cube"].x1==-0.5 &&
			cubeObjects["cube"].x2==0.5))
			{
			//	cout << "helloaw" << endl;
				arr[10][10]=0;arr[9][10]=0;
				fallStatus=1;
			}
	 }
	if(cubeObjects["cube"].orientation=="alongZ")
	{
		rowArrayCounterA=cubeObjects["cube"].z1+10-1;
		rowArrayCounterB=cubeObjects["cube"].z5+11-1;
		colArrayCounterB=cubeObjects["cube"].x2+11-1;
		colArrayCounterA=colArrayCounterB;
		//cout << rowArrayCounterA <<" " << colArrayCounterA << " " << rowArrayCounterB << " " << colArrayCounterB << endl;
		//cout <<arr[rowArrayCounterA][colArrayCounterA] << " " << arr[rowArrayCounterB][colArrayCounterB] << endl;
		if(arr[rowArrayCounterA][colArrayCounterA]*arr[rowArrayCounterB][colArrayCounterB]==0 || ((arr[rowArrayCounterA][colArrayCounterA]*arr[rowArrayCounterB][colArrayCounterB]==4
		|| arr[rowArrayCounterA][colArrayCounterA]*arr[rowArrayCounterB][colArrayCounterB]==16) && bridgeSwitchPressed==0) )
			{
				fallStatus=1;
			//	glfwTerminate();
			//	exit(EXIT_SUCCESS);
			}
		}
		else if(cubeObjects["cube"].orientation=="alongY")
		{
			rowArrayCounterA=cubeObjects["cube"].z1+10-1;
			colArrayCounterA=cubeObjects["cube"].x2+11-1;
			if(arr[rowArrayCounterA][colArrayCounterA]==0 || (arr[rowArrayCounterA][colArrayCounterA]==4 && bridgeSwitchPressed==0))
			{
				fallStatus=1;
				//glfwTerminate();
				//exit(EXIT_SUCCESS);
			}
		}
		else if(cubeObjects["cube"].orientation=="alongX")
		{
			rowArrayCounterA=cubeObjects["cube"].z1+10-1;
			rowArrayCounterB=rowArrayCounterA;
			colArrayCounterA=cubeObjects["cube"].x1+12-1;
			colArrayCounterB=cubeObjects["cube"].x2+11-1;
			if(arr[rowArrayCounterA][colArrayCounterA]*arr[rowArrayCounterB][colArrayCounterB]==0 || ((arr[rowArrayCounterA][colArrayCounterA]*arr[rowArrayCounterB][colArrayCounterB]==4
			|| arr[rowArrayCounterA][colArrayCounterA]*arr[rowArrayCounterB][colArrayCounterB]==16) && bridgeSwitchPressed==0))
			{
				fallStatus=1;
//				glfwTerminate();
	//			exit(EXIT_SUCCESS);
			}
		}

}
float camera_rotation_angle = 90;
int status=1;
float cubeHoleSliderVar=0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window)
{


	int i=0, j=0;
	glm::vec3 up, eye, target;
//	int fbwidth, fbheight;
//	glfwGetFramebufferSize(window, &fbwidth, &fbheight);
	//glViewport((int)(x*fbwidth), (int)(y*fbheight), (int)(w*fbwidth), (int)(h*fbheight));
	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram(programID);

	// Eye - Location of camera. Don't change unless you are sure!!

//	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	if(top_view==1)
  {
		//glm::vec3 eye(xa,ya,za);
		//glm::vec3 target(xb,yb,zb);
		//glm::vec3 up(xc,yc,zc);

		glm::vec3 eye(0,8,0);
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0,0,-1);
	Matrices.view=glm::lookAt(eye,target,up);
	}
	else if(block_view==1)
	{
		glm::vec3 eye(cubeObjects["cube"].x5,cubeObjects["cube"].y5+1, cubeObjects["cube"].z5-0.3);
		glm::vec3 target(3.5,0,-7);
		glm::vec3 up(0,1,0);
		Matrices.view=glm::lookAt(eye,target,up);
	}
	else if(default_view==1)
	{
		//glm::vec3 eye(xa,ya,za);
		//glm::vec3 target(xb,yb,zb);
		//glm::vec3 up(xc,yc,zc);
		glm::vec3 eye(3,5,3);
		glm::vec3 target(0,0,0);
		glm::vec3 up(0,1,0);
		Matrices.view=glm::lookAt(eye,target,up);
	}
	else if(follower_view==1)
	{
		glm::vec3 eye((cubeObjects["cube"].x1+cubeObjects["cube"].x2)/2.0,cubeObjects["cube"].y1+3, cubeObjects["cube"].z1);
		glm::vec3 target(cubeObjects["cube"].x2,0,cubeObjects["cube"].z5-5);
		glm::vec3 up(0,1,0);
		Matrices.view=glm::lookAt(eye,target,up);
	}
	else if(helicopter_view==1)
	{
		glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 4, 5*sin(camera_rotation_angle*M_PI/180.0f) );
		glm::vec3 target (0, 0, 0);
		glm::vec3 up (0, 1, 0);
		Matrices.view=glm::lookAt(eye,target, up);
	}

//	Matrices.view=glm::mat4(1.0f);
	glm::mat4 VP=Matrices.projection*Matrices.view;
	glm::mat4 scaleCube=glm::scale(glm::vec3(1,1,1));
	glm::mat4 trans_mat,trans_tile,rotateCube,prerotation;
/*############################################################################################################*/
//XYplaneAngle for turn_right and turn_left, YZplaneAngle for turn_forward and turn_backward.
	if(turn_right==1 && rot_varRight>=-90) //do this for the case when player presses the key 'd'
	{

		//rot_varRight=1;
		if(cubeObjects["cube"].orientation=="alongZ")
			{
				prerotation=glm::rotate((float)(0),glm::vec3(0,0,1));
				rotateCube=glm::rotate((float)(rot_varRight*M_PI/180),glm::vec3(0,0,1));
			}
		else if(cubeObjects["cube"].orientation=="alongY")
		{
			prerotation=glm::rotate((float)(M_PI/2.0f),glm::vec3(1,0,0));
			glm::mat4 aBitTrans=glm::translate(glm::vec3(0,0,-1));
			prerotation=aBitTrans*prerotation;
			rotateCube=glm::rotate((float)((rot_varRight*M_PI/180)),glm::vec3(0,0,1));
		}
		else if(cubeObjects["cube"].orientation=="alongX")
		{
				prerotation=glm::rotate((float)(-M_PI/2.0), glm::vec3(0,1,0));
				glm::mat4 aBitTrans=glm::translate(glm::vec3(-2,0,0));
				prerotation=aBitTrans*prerotation;
				rotateCube=glm::rotate((float)(rot_varRight*M_PI/180),glm::vec3(0,0,1));
		}

	cubeObjects["cube"].model=glm::mat4(1.0f);
//	glm::mat4 rotateCube=glm::rotate((float)(XYplaneAngle*M_PI/180.0f),glm::vec3(0,0,1));
	trans_mat=glm::translate(glm::vec3(-0.5,0.5,-1.0));

	glm::mat4 inverse_trans_mat=glm::translate(glm::vec3(cubeObjects["cube"].x2,cubeObjects["cube"].y2,cubeObjects["cube"].z2));
	cubeObjects["cube"].model*=inverse_trans_mat*rotateCube*prerotation*trans_mat;
	cubeObjects["cube"].mvp=VP*cubeObjects["cube"].model;
	rot_varRight=rot_varRight-2;
	}
	if(rot_varRight<-90)
			{
				rot_varRight=0;
				turn_right=0;

				if(cubeObjects["cube"].orientation=="alongZ")
				{
				cubeObjects["cube"].orientation="alongZ";
				cubeObjects["cube"].x1+=1; //update the new coordinates. Rotated along z, so only need to update the x-coordinates.
				cubeObjects["cube"].x2+=1;
				cubeObjects["cube"].x3+=1;
				cubeObjects["cube"].x4+=1;
				cubeObjects["cube"].x5+=1; //update the new coordinates. Rotated along z, so only need to update the x-coordinates.
				cubeObjects["cube"].x6+=1;
				cubeObjects["cube"].x7+=1;
				cubeObjects["cube"].x8+=1;
			  }

				else if(cubeObjects["cube"].orientation=="alongY")
				{
					cubeObjects["cube"].orientation="alongX";
					cubeObjects["cube"].y3+=-1.0;
					cubeObjects["cube"].y4+=-1.0;
					cubeObjects["cube"].y7+=-1.0;
					cubeObjects["cube"].y8+=-1.0;
					cubeObjects["cube"].x1+=1.0;
					cubeObjects["cube"].x2+=2.0;
					cubeObjects["cube"].x3+=2.0;
					cubeObjects["cube"].x4+=1.0;
					cubeObjects["cube"].x5+=1.0;
					cubeObjects["cube"].x6+=2.0;
					cubeObjects["cube"].x7+=2.0;
					cubeObjects["cube"].x8+=1.0;
				}

				else if(cubeObjects["cube"].orientation=="alongX")
				{
					cubeObjects["cube"].orientation="alongY";
					cubeObjects["cube"].x1+=2.0;
					cubeObjects["cube"].x2+=1.0;
					cubeObjects["cube"].x3+=1.0;
					cubeObjects["cube"].x4+=2.0;
					cubeObjects["cube"].x5+=2.0;
					cubeObjects["cube"].x6+=1.0;
					cubeObjects["cube"].x7+=1.0;
					cubeObjects["cube"].x8+=2.0;
					cubeObjects["cube"].y3+=1;
					cubeObjects["cube"].y4+=1;
					cubeObjects["cube"].y7+=1;
					cubeObjects["cube"].y8+=1;
				}
			}
/*###############################################################################################*/
if(turn_left==1 && rot_varLeft<=90) // when player presses 'a'
{
	//rot_varLeft=1;
	if(cubeObjects["cube"].orientation=="alongZ")
	{
		prerotation=glm::rotate((float)(0), glm::vec3(0,0,1));
		rotateCube=glm::rotate((float)(rot_varLeft*M_PI/180),glm::vec3(0,0,1));
	}
	else if(cubeObjects["cube"].orientation=="alongY")
	{

			prerotation=glm::rotate((float)(M_PI/2.0),glm::vec3(1,0,0));
			glm::mat4 aBitTrans=glm::translate(glm::vec3(0,0,-1));
			prerotation=aBitTrans*prerotation;
			rotateCube=glm::rotate((float)(rot_varLeft*M_PI/180),glm::vec3(0,0,1));
	}
	else if(cubeObjects["cube"].orientation=="alongX")
	{

		prerotation=glm::rotate((float)(-M_PI/2.0),glm::vec3(0,1,0));
		glm::mat4 aBitTrans=glm::translate(glm::vec3(0,0,-1));
		prerotation=aBitTrans*prerotation;
		rotateCube=glm::rotate((float)(rot_varLeft*M_PI/180),glm::vec3(0,0,1));
	}
	cubeObjects["cube"].model=glm::mat4(1.0f);
	trans_mat=glm::translate(glm::vec3(0.5,0.5,-1.0));

	glm::mat4 inverse_trans_mat=glm::translate(glm::vec3(cubeObjects["cube"].x1,cubeObjects["cube"].y1,cubeObjects["cube"].z1));
	cubeObjects["cube"].model*=inverse_trans_mat*rotateCube*prerotation*trans_mat;
	cubeObjects["cube"].mvp=VP*cubeObjects["cube"].model;
	rot_varLeft=rot_varLeft+2;
}
if(rot_varLeft>90)
		{
			rot_varLeft=0;
			turn_left=0;

			if(cubeObjects["cube"].orientation=="alongZ")
			{
			cubeObjects["cube"].orientation="alongZ";
			cubeObjects["cube"].x1-=1; //update the new coordinates. Rotated along z, so only need to update the x-coordinates.
			cubeObjects["cube"].x2-=1;
			cubeObjects["cube"].x3-=1;
			cubeObjects["cube"].x4-=1;
			cubeObjects["cube"].x5-=1; //update the new coordinates. Rotated along z, so only need to update the x-coordinates.
			cubeObjects["cube"].x6-=1;
			cubeObjects["cube"].x7-=1;
			cubeObjects["cube"].x8-=1;
		 }
		 else if(cubeObjects["cube"].orientation=="alongX")
		 {
			 cubeObjects["cube"].orientation="alongY";
			 cubeObjects["cube"].x1+=-1;
			 cubeObjects["cube"].x2+=-2;
			 cubeObjects["cube"].x3+=-2;
			 cubeObjects["cube"].x4+=-1;
			 cubeObjects["cube"].x5+=-1;
			 cubeObjects["cube"].x6+=-2;
			 cubeObjects["cube"].x7+=-2;
			 cubeObjects["cube"].x8+=-1;
			 cubeObjects["cube"].y3+=1;
			 cubeObjects["cube"].y4+=1;
			 cubeObjects["cube"].y7+=1;
			 cubeObjects["cube"].y8+=1;
		 }
		 else if(cubeObjects["cube"].orientation=="alongY")
		 {
			 cubeObjects["cube"].orientation="alongX";
			 cubeObjects["cube"].x1+=-2;
			 cubeObjects["cube"].x2+=-1;
			 cubeObjects["cube"].x3+=-1;
			 cubeObjects["cube"].x4+=-2;
			 cubeObjects["cube"].x5+=-2;
			 cubeObjects["cube"].x6+=-1;
			 cubeObjects["cube"].x7+=-1;
			 cubeObjects["cube"].x8+=-2;
			 cubeObjects["cube"].y3+=-1;
			 cubeObjects["cube"].y4+=-1;
			 cubeObjects["cube"].y7+=-1;
			 cubeObjects["cube"].y8+=-1;

		 }
		}

if(turn_forward==1 && rot_varForward>-90)
{
	if(cubeObjects["cube"].orientation=="alongX")
	{

		prerotation=glm::rotate((float)(M_PI/2.0),glm::vec3(0,1,0));
		glm::mat4 aBitTrans=glm::translate(glm::vec3(-2,0,0));
		prerotation=aBitTrans*prerotation;
		rotateCube=glm::rotate((float)(rot_varForward*M_PI/180),glm::vec3(1,0,0));
	}
	else if(cubeObjects["cube"].orientation=="alongY")
	{

		prerotation=glm::rotate((float)(-M_PI/2.0),glm::vec3(1,0,0));
		glm::mat4 aBitTrans=glm::translate(glm::vec3(0,0,1));
		prerotation=aBitTrans*prerotation;
		rotateCube=glm::rotate((float)(rot_varForward*M_PI/180),glm::vec3(1,0,0));
	}
	else if(cubeObjects["cube"].orientation=="alongZ")
	{

		prerotation=glm::rotate((float)(0),glm::vec3(1,0,0));
//		glm::mat4 aBitTrans=glm::translate(glm::vec3(0,0,1));
	//	prerotation=aBitTrans*prerotation;
		rotateCube=glm::rotate((float)(rot_varForward*M_PI/180),glm::vec3(1,0,0));
	}
	cubeObjects["cube"].model=glm::mat4(1.0f);
	trans_mat=glm::translate(glm::vec3(-0.5,0.5,1.0));

	glm::mat4 inverse_trans_mat=glm::translate(glm::vec3(cubeObjects["cube"].x6,cubeObjects["cube"].y6,cubeObjects["cube"].z6));
	cubeObjects["cube"].model*=inverse_trans_mat*rotateCube*prerotation*trans_mat;
	cubeObjects["cube"].mvp=VP*cubeObjects["cube"].model;
	rot_varForward=rot_varForward-2;
}
//cout << YZplaneAngle << endl;

if(rot_varForward<=-90)
{
	//cout << "Inside rot_varForward < -90 condition " << endl;

	turn_forward=0;
	rot_varForward=0;

	if(cubeObjects["cube"].orientation=="alongX")
		{
		cubeObjects["cube"].orientation="alongX";//will remain the same. Think.....
		cubeObjects["cube"].z1+=-1;
		cubeObjects["cube"].z2+=-1;
		cubeObjects["cube"].z3+=-1;
		cubeObjects["cube"].z4+=-1;
		cubeObjects["cube"].z5+=-1;
		cubeObjects["cube"].z6+=-1;
		cubeObjects["cube"].z7+=-1;
		cubeObjects["cube"].z8+=-1;
	}
	else if(cubeObjects["cube"].orientation=="alongY")
	{
		cubeObjects["cube"].orientation="alongZ";//orientation will change accordingly depending upon the previous iteration
	//	cout << "Current cube orientation " << cubeObjects["cube"].orientation << endl;
		cubeObjects["cube"].y3+=-1;
		cubeObjects["cube"].y4+=-1;
		cubeObjects["cube"].y7+=-1;
		cubeObjects["cube"].y8+=-1;
		cubeObjects["cube"].z1-=1;
		cubeObjects["cube"].z2-=1;
		cubeObjects["cube"].z3-=1;
		cubeObjects["cube"].z4-=1;
		cubeObjects["cube"].z5-=2;
		cubeObjects["cube"].z6-=2;
		cubeObjects["cube"].z7-=2;
		cubeObjects["cube"].z8-=2;
	}
	else if(cubeObjects["cube"].orientation=="alongZ")
	{
		cubeObjects["cube"].orientation="alongY";
		cubeObjects["cube"].z1+=-2.0;
		cubeObjects["cube"].z2+=-2.0;
		cubeObjects["cube"].z3+=-2.0;
		cubeObjects["cube"].z4+=-2.0;
		cubeObjects["cube"].z5+=-1.0;
		cubeObjects["cube"].z6+=-1.0;
		cubeObjects["cube"].z7+=-1.0;
		cubeObjects["cube"].z8+=-1.0;
		cubeObjects["cube"].y3+=1.0;
		cubeObjects["cube"].y4+=1.0;
		cubeObjects["cube"].y7+=1.0;
		cubeObjects["cube"].y8+=1.0;
	}

}
//cout << cubeObjects["cube"].z6 << endl;
/*#################################################################################################*/
if(turn_backward==1 && rot_varBackward<90)
{

	if(cubeObjects["cube"].orientation=="alongX")
	{

		prerotation=glm::rotate((float)(-M_PI/2.0),glm::vec3(0,1,0));
		glm::mat4 aBitTrans=glm::translate(glm::vec3(-2,0,0));
		prerotation=aBitTrans*prerotation;
		rotateCube=glm::rotate((float)(rot_varBackward*M_PI/180),glm::vec3(1,0,0));
	}
	else if(cubeObjects["cube"].orientation=="alongY")
	{

		prerotation=glm::rotate((float)(M_PI/2.0),glm::vec3(1,0,0));
		glm::mat4 aBitTrans=glm::translate(glm::vec3(0,0,-1));
		prerotation=aBitTrans*prerotation;
		rotateCube=glm::rotate((float)(rot_varBackward*M_PI/180),glm::vec3(1,0,0));
	}
	else if(cubeObjects["cube"].orientation=="alongZ")
	{

		prerotation=glm::rotate((float)(0),glm::vec3(1,0,0));
//		glm::mat4 aBitTrans=glm::translate(glm::vec3(0,0,1));
	//	prerotation=aBitTrans*prerotation;
		rotateCube=glm::rotate((float)(rot_varBackward*M_PI/180),glm::vec3(1,0,0));
	}

	cubeObjects["cube"].model=glm::mat4(1.0f);
  trans_mat=glm::translate(glm::vec3(-0.5,0.5,-1.0));
	glm::mat4 inverse_trans_mat=glm::translate(glm::vec3(cubeObjects["cube"].x2,cubeObjects["cube"].y2,cubeObjects["cube"].z2));
	cubeObjects["cube"].model*=inverse_trans_mat*rotateCube*prerotation*trans_mat;
	cubeObjects["cube"].mvp=VP*cubeObjects["cube"].model;
	rot_varBackward=rot_varBackward+2;
}
if(rot_varBackward>=90)
{
		rot_varBackward=0;
		turn_backward=0;
		if(cubeObjects["cube"].orientation=="alongX")
		{
			cubeObjects["cube"].orientation="alongX";
			cubeObjects["cube"].z1+=1;
			cubeObjects["cube"].z2+=1;
			cubeObjects["cube"].z3+=1;
			cubeObjects["cube"].z4+=1;
			cubeObjects["cube"].z5+=1;
			cubeObjects["cube"].z6+=1;
			cubeObjects["cube"].z7+=1;
			cubeObjects["cube"].z8+=1;
		}
		else if(cubeObjects["cube"].orientation=="alongY")
		{
			cubeObjects["cube"].orientation="alongZ";
			cubeObjects["cube"].z1+=2;
			cubeObjects["cube"].z2+=2;
			cubeObjects["cube"].z3+=2;
			cubeObjects["cube"].z4+=2;
			cubeObjects["cube"].z5+=1;
			cubeObjects["cube"].z6+=1;
		  cubeObjects["cube"].z7+=1;
			cubeObjects["cube"].z8+=1;
			cubeObjects["cube"].y3+=-1;
			cubeObjects["cube"].y4+=-1;
			cubeObjects["cube"].y7+=-1;
			cubeObjects["cube"].y8+=-1;
		}
		else if(cubeObjects["cube"].orientation=="alongZ")
		{
			cubeObjects["cube"].orientation="alongY";
			cubeObjects["cube"].z1+=1;
			cubeObjects["cube"].z2+=1;
			cubeObjects["cube"].z3+=1;
			cubeObjects["cube"].z4+=1;
			cubeObjects["cube"].z5+=2;
			cubeObjects["cube"].z6+=2;
			cubeObjects["cube"].z7+=2;
			cubeObjects["cube"].z8+=2;
			cubeObjects["cube"].y3+=1;
			cubeObjects["cube"].y4+=1;
			cubeObjects["cube"].y7+=1;
			cubeObjects["cube"].y8+=1;
		}
}

//cout << cubeObjects["cube"].orientation << endl;

glm::mat4 finalRoatationMat;
//Finally draw whaterver object you got
if(turn_right!=1 && turn_left!=1 && turn_forward!=1 && turn_backward!=1 && status==1)
{
			reached_flag=0;
			cubeObjects["cube"].model=glm::mat4(1.0f);
			trans_mat=glm::translate(glm::vec3(   (cubeObjects["cube"].x1+cubeObjects["cube"].x7)/2.0f,
			            (cubeObjects["cube"].y1+cubeObjects["cube"].y7)/2.0f, (cubeObjects["cube"].z1+cubeObjects["cube"].z7)/2.0f  ));
									//translate to the center of the cube given from the origin and then draw there.


			if(cubeObjects["cube"].orientation=="alongZ")
				finalRoatationMat=glm::rotate(float(0), glm::vec3(0,0,1));
			else if(cubeObjects["cube"].orientation=="alongY")
				finalRoatationMat=glm::rotate(float(-M_PI/2.0), glm::vec3(1,0,0));
			else if(cubeObjects["cube"].orientation=="alongX")
				finalRoatationMat=glm::rotate(float(-M_PI/2.0),glm::vec3(0,1,0));



				if(fallStatus==1)
				{
					glm::mat4 oneMoremat=glm::translate(glm::vec3(0,-downfall,0));
					cubeObjects["cube"].model*=oneMoremat*trans_mat*finalRoatationMat;
					downfall+=0.2;
				}
				if(fallStatus==1 && downfall>=4)
				{glfwTerminate();exit(EXIT_SUCCESS);}
			if(fallStatus!=1)
				cubeObjects["cube"].model*=trans_mat*finalRoatationMat*scaleCube;
			cubeObjects["cube"].mvp=VP*cubeObjects["cube"].model;
}
glUniformMatrix4fv(Matrices.MatrixID,1,GL_FALSE,&cubeObjects["cube"].mvp[0][0]);
draw3DObject(rectangle);
draw3DObject(rectangleLines);

checkSpecialConditions();
if(targetReached==1)
{
//		cout << "Hello" << endl;
		if(cubeHoleSliderVar<4)
		{
			glm::mat4 down_trans=glm::translate(glm::vec3(0,-cubeHoleSliderVar,0));
			cubeObjects["cube"].mvp=VP*down_trans*cubeObjects["cube"].model;
			glUniformMatrix4fv(Matrices.MatrixID,1,GL_FALSE,&cubeObjects["cube"].mvp[0][0]);
			draw3DObject(rectangle);
	//		draw3DObject(rectangleLines);
			cubeHoleSliderVar=cubeHoleSliderVar+0.1;
		}
		status=0;
		if(cubeHoleSliderVar>=4)
		{
			fallStatus=0;
			targetReached=0; //reinitialize the goal achieve state
			cubeHoleSliderVar=0; // reinitialize the sliding transition
			level=2; //increase the level
			bridgeSwitchPressed=0; // reset the bridgeSwitchPressed;
			status=1;
			createBridgeSwitch();//has to again reset the colour of the switch from light blue to dark blue.
			cubeObjects["cube"].x1=-0.5; // initalizing each and every corner of the cube(8 vertices);
			cubeObjects["cube"].x2=0.5;
			cubeObjects["cube"].x3=0.5;
			cubeObjects["cube"].x4=-0.5;
			cubeObjects["cube"].x5=-0.5;
			cubeObjects["cube"].x6=0.5;
			cubeObjects["cube"].x7=0.5;
			cubeObjects["cube"].x8=-0.5;
			cubeObjects["cube"].y1=-0.5;
			cubeObjects["cube"].y2=-0.5;
			cubeObjects["cube"].y3=0.5;
			cubeObjects["cube"].y4=0.5;
			cubeObjects["cube"].y5=-0.5;
			cubeObjects["cube"].y6=-0.5;
			cubeObjects["cube"].y7=0.5;
			cubeObjects["cube"].y8=0.5;
			cubeObjects["cube"].z1=1.0;
			cubeObjects["cube"].z2=1.0;
			cubeObjects["cube"].z3=1.0;
			cubeObjects["cube"].z4=1.0;
			cubeObjects["cube"].z5=-1.0;
			cubeObjects["cube"].z6=-1.0;
			cubeObjects["cube"].z7=-1.0;
			cubeObjects["cube"].z8=-1.0;
			cubeObjects["cube"].orientation="alongZ";
			//glfwTerminate();
			//exit(EXIT_SUCCESS);
		}
}


for(i=0;i<21;i++)
{
	for(j=0;j<21;j++)
	{
			glm::mat4 mvpsmall=glm::mat4(1.0f);
			trans_tile=glm::translate(glm::vec3(j-10,-0.75,i-10+0.5));

			mvpsmall=VP*trans_tile;
			glUniformMatrix4fv(Matrices.MatrixID,1,GL_FALSE,&mvpsmall[0][0]);
			if(arr[i][j]==1)
			{
				draw3DObject(tiles);
				draw3DObject(tilesLines);
			}
			if(arr[i][j]==2)
				draw3DObject(waterHole);
			if(arr[i][j]==3)
				draw3DObject(bridgeSwitch);
			if(arr[i][j]==4 && bridgeSwitchPressed==1)
				{
					draw3DObject(bridgeTiles);
					draw3DObject(bridgeTilesLines);
				}
			if(arr[i][j]==5)
			{
				draw3DObject(fragileTile);
			}
	}
}
//int fbwidth=600, fbheight=600;

//cout <<"rowArrayCounterB " << rowArrayCounterB << " colArrayCounterB " << colArrayCounterB << endl;


 //draw the final cube. Camera matrix (view)
	/*  if(doV)
	    Matrices.view = glm::lookAt(eye, target, up); // Fixed camera for 2D (ortho) in XY plane
	    else
	    Matrices.view = glm::mat4(1.0f);

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	glm::mat4 VP;
	if (doP)
	VP = Matrices.projection * Matrices.view;
	else
	VP = Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	glm::mat4 MVP;	// MVP = Projection * View * Model

	// Load identity to model matrix
	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateRectangle = glm::translate (rect_pos);        // glTranslatef
	glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));
	Matrices.model *= (translateRectangle * rotateRectangle);
	if(floor_rel)
	Matrices.model = Matrices.model * glm::translate(floor_pos);
	if(doM)
	MVP = VP * Matrices.model;
	else
	MVP = VP;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(rectangle);

	// Load identity to model matrix
	Matrices.model = glm::mat4(1.0f);
	 */

	//	glm::mat4 translateCam = glm::translate(eye);
	//	glm::mat4 rotateCam = glm::rotate((float)((90 - camera_rotation_angle)*M_PI/180.0f), glm::vec3(0,1,0));
	//	Matrices.model *= (translateCam * rotateCam);
	//	MVP = VP * Matrices.model;
	//	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	//	draw3DObject(cam);
	//
	//	Matrices.model = glm::translate(floor_pos);
	//	MVP = VP * Matrices.model;
	//	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	//
	//	// draw3DObject draws the VAO given to it using current MVP matrix
	//	draw3DObject(floor_vao);

}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height){
	GLFWwindow* window; // window desciptor/handle

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

	if (!window) {
		exit(EXIT_FAILURE);
		glfwTerminate();
	}

	glfwMakeContextCurrent(window);
	//    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
	glfwSwapInterval( 1 );
	glfwSetFramebufferSizeCallback(window, reshapeWindow);
	glfwSetWindowSizeCallback(window, reshapeWindow);
	glfwSetWindowCloseCallback(window, quit);
	glfwSetKeyCallback(window, keyboard);      // general keyboard input
	glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling
	glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

	return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
	/* Objects should be created before any other gl function and shaders */
	// Create the models
	createRectangleLines();
	createRectangle();
	createTilesLines();
	createTile();
	createHoles();
	createBridgeSwitch();
	createBridgeTiles();
	createBridgeTilesLines();
	createFragileTile();
	cubeObjects["cube"].x1=-0.5; // initalizing each and every corner of the cube(8 vertices);
	cubeObjects["cube"].x2=0.5;
	cubeObjects["cube"].x3=0.5;
	cubeObjects["cube"].x4=-0.5;
	cubeObjects["cube"].x5=-0.5;
	cubeObjects["cube"].x6=0.5;
	cubeObjects["cube"].x7=0.5;
	cubeObjects["cube"].x8=-0.5;
	cubeObjects["cube"].y1=-0.5;
	cubeObjects["cube"].y2=-0.5;
	cubeObjects["cube"].y3=0.5;
	cubeObjects["cube"].y4=0.5;
	cubeObjects["cube"].y5=-0.5;
	cubeObjects["cube"].y6=-0.5;
	cubeObjects["cube"].y7=0.5;
	cubeObjects["cube"].y8=0.5;
	cubeObjects["cube"].z1=1.0;
	cubeObjects["cube"].z2=1.0;
	cubeObjects["cube"].z3=1.0;
	cubeObjects["cube"].z4=1.0;
	cubeObjects["cube"].z5=-1.0;
	cubeObjects["cube"].z6=-1.0;
	cubeObjects["cube"].z7=-1.0;
	cubeObjects["cube"].z8=-1.0;
	cubeObjects["cube"].orientation="alongZ";
	//initial orientation of the cube. It lies along the z-axis.
	//	createCam();
	//	createFloor();

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

	// Background color of the scene
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	// cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	// cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	// cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	// cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;
	rect_pos = glm::vec3(0, 0, 0);
	floor_pos = glm::vec3(0, 0, 0);
	do_rot = 0;
	floor_rel = 1;

	GLFWwindow* window = initGLFW(width, height);
	initGLEW();
	initGL (window, width, height);


	last_update_time = glfwGetTime();
	/* Draw in loop */
	while (!glfwWindowShouldClose(window)) {
		// clear the color and depth in the frame buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(level==2)
			arr_init();
		draw(window);
		// OpenGL Draw commands
			current_time = glfwGetTime();
			if(do_rot)
				camera_rotation_angle += 90*(current_time - last_update_time); // Simulating camera rotation
			if(camera_rotation_angle > 720)
				camera_rotation_angle -= 720;
			last_update_time = current_time;
/*			draw(window, 0, 0, 0.5, 0.5, 1, 1, 1);
			draw(window, 0.5, 0, 0.5, 0.5, 0, 1, 1);
			draw(window, 0, 0.5, 0.5, 0.5, 1, 0, 1);
			draw(window, 0.5, 0.5, 0.5, 0.5, 0, 0, 1);
		 */

		// Swap Frame Buffer in double buffering
		glfwSwapBuffers(window);

		// Poll for Keyboard and mouse events
		glfwPollEvents();
	}

	glfwTerminate();
	//    exit(EXIT_SUCCESS);

}
