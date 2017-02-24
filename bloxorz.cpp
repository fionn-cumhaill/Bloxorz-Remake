#include <bits/stdc++.h>

#include <mpg123.h>
#include <ao/ao.h>
#include <assert.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

#define BITS 8

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
    glm::mat4 projectionO, projectionP, projectionMoveCount;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID;
} Matrices;

typedef struct Block {
  int x;
  int y;
  string state;
} Block;

typedef struct goal {
  int x;
  int y;
} Goal;

typedef struct Tile {
  bool exists;
  float z;
  int id;
  string state;
  string type;
} Tile;

int proj_type;

int score, moveCount, level = 1, rotateDirection[4];
float angle, helicopterDepth, updateTime = 3;
double mouseX, mouseY, last_update_time, current_time;
string baseLoc;
VAO *textTile[7], *textBackground, *tile[10][20], *tileBorder[10][20], *blockStanding, *blockSleepingX, *blockSleepingY;
bool selected, gameOver, levelUp, takingInput, * keyStates = new bool[500];
static const float screenLeftX = -11.0;
static const float screenRightX = 11.0;
static const float screenTopY = 11.0;
static const float screenBottomY = -11.0;
float displayLeft = -11.0, displayRight = 5.0, displayTop = 11.0, displayBottom = -11.0, horizontalZoom = 0, verticalZoom = 0;
float camera_rotation_angle = 90;

Block blockInfo;
Goal goalInfo;
Tile tileInfo[10][20];

GLFWwindow* windowCopy;
GLuint programID;

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
	// printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	// fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	// printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	// fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	// fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	// fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    // fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
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
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
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
 * Game specific code *
 **************************/
// Convention followed everywhere is up, down, left, right (correspond to 0, 1, 2 and 3 respectively)

void checkAndSelect();
void resetMouseCoordinates();
void resetSelectState();
void resetTileState();
void toggleView();
void rotateBlock(string direction);

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if(!takingInput)
    return;
  // Function is called first on GLFW_PRESS.
  if (action == GLFW_RELEASE) {
    keyStates[key] = false;
  }
  else if (action == GLFW_PRESS) {
    keyStates[key] = true;
    switch (key) {
        case GLFW_KEY_Q:
            quit(window);
            break;
        case GLFW_KEY_SPACE:
            toggleView();
            break;
        case GLFW_KEY_UP:
            rotateBlock("up");
            break;
        case GLFW_KEY_DOWN:
            rotateBlock("down");
            break;
        case GLFW_KEY_LEFT:
            rotateBlock("left");
            break;
        case GLFW_KEY_RIGHT:
            rotateBlock("right");
            break;
        case GLFW_KEY_O:
            proj_type = 0;
            break;
        case GLFW_KEY_P:
            proj_type = 1;
            break;
        case GLFW_KEY_W:
            proj_type = 2;
            break;
        case GLFW_KEY_S:
            proj_type = 3;
            break;
        case GLFW_KEY_A:
            proj_type = 4;
            break;
        case GLFW_KEY_D:
            proj_type = 5;
            break;
        case GLFW_KEY_U:
            proj_type = 6;
            break;
        case GLFW_KEY_J:
            proj_type = 7;
            break;
        case GLFW_KEY_H:
            proj_type = 8;
            break;
        case GLFW_KEY_K:
            proj_type = 9;
            break;
        case GLFW_KEY_V:
            proj_type = 10;
            break;
        default:
            break;
    }
  }
}

void keyStateCheck()
{
  // Mouse Controls
  if(keyStates[GLFW_MOUSE_BUTTON_LEFT])
  {
    glfwGetCursorPos(windowCopy, &mouseX, &mouseY);
    mouseX = (mouseX*2*screenRightX/550) - screenRightX;
    mouseY = screenTopY - (mouseY*2*screenTopY/550);
    // Mouse selection
    checkAndSelect();
  }

  else if(keyStates[GLFW_KEY_T])
  {
    angle -= 1;
  }

  else if(keyStates[GLFW_KEY_R])
  {
    angle += 1;
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
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
  switch (button) {
      case GLFW_MOUSE_BUTTON_LEFT:
          if (action == GLFW_RELEASE) {
              keyStates[button] = false;
              resetMouseCoordinates();
              resetSelectState();
          }
          else if (action == GLFW_PRESS) {
              keyStates[button] = true;
          }
          break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                keyStates[button] = false;
                resetMouseCoordinates();
                resetSelectState();
              }
            else if (action == GLFW_PRESS) {
                keyStates[button] = true;
            }
            break;
      default:
          break;
  }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  // Only in case of Helicopter View
  if(proj_type == 10)
  {
    if(yoffset == 1)
    {
      helicopterDepth += 0.5f;
    }
    else if(yoffset == -1)
    {
      helicopterDepth -= 0.5f;
    }
  }
}

void getMoveCount();
void draw();

void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    GLfloat fov = M_PI/2;
    
    glViewport(0, 0, 550, 550);
    // Perspective projection for 3D views
    Matrices.projectionP = glm::perspective(fov, (GLfloat) (fbwidth - 550) / (GLfloat) fbheight, 0.1f, 500.0f);
    // Ortho projection for 2D views
    Matrices.projectionO = glm::ortho(screenLeftX, screenRightX, screenBottomY, screenTopY, 0.1f, 500.0f);
    
    draw();
    
    glViewport(550, 0, 1100, 550);
    // Ortho projection for displaying MoveCount
    Matrices.projectionO = glm::ortho(screenLeftX, screenRightX*3, screenBottomY, screenTopY, 0.1f, 500.0f);
    
    getMoveCount();
}

void initialize()
{
  int i, j;
  
  // glEnable(GL_SCISSOR_TEST);
  score = 0;
  level = 1;
  angle = 0.0f;
  helicopterDepth = 7.0f;
  moveCount = 0;
  gameOver = false;
  levelUp = true;
  takingInput = true;
  srand((unsigned)time(0));

  baseLoc = "back";
  // Initializing the pressed state of all keys to false
  for(i = 0; i < 500; i++)
  {
    keyStates[i] = false;
  }

  blockInfo.state = "standing";

  resetMouseCoordinates();
  resetSelectState();
  resetTileState();
}

void checkAndSelect()
{
  int i, flag = 0;

  if(selected)
    return;
}

void resetTileState ()
{
  int i, j;
  for(i = 0; i < 10; i++)
  {
    for(j = 0; j < 20; j++)
    {
      tileInfo[i][j].exists = false;
    }
  }
}

void resetMouseCoordinates()
{
  mouseX = 100.0;
  mouseY = 100.0;
}

void resetSelectState()
{
  int i;
  selected = false;
}

void resetRotateDirection()
{
  int i;
  for(i = 0; i < 4; i++)
  {
    rotateDirection[i] = 0;
  }
}

void toggleView ()
{
  helicopterDepth = 7.0f;
  proj_type = (proj_type + 1)%11;
}

// Create the background that the level number will be displayed on
void createBackground ()
{
  const GLfloat vertex_buffer_data [] = {
    -100, 100, 4,   // vertex 1
    -100, -100, 4,  // vertex 2
    100, 100, 4,

    -100, -100, 4,
    100, 100, 4,
    100, -100, 4
   };

   const GLfloat color_buffer_data [] = {
     0, 0, 0,
     0, 0, 0,
     0, 0, 0,

     0, 0, 0,
     0, 0, 0,
     0, 0, 0
   };

   // create3DObject creates and returns a handle to a VAO that can be used later
   textBackground = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// Create the tiles used for displaying text
void createTextTile ()
{
  int j;

  for(j = 0; j < 7; j++)
  {
    // GL3 accepts only Triangles. Quads are not supported
    const GLfloat vertex_buffer_data [] = {
      -0.15, 0, 5, // vertex 1
      +0.15, 0, 5, // vertex 2
      +0.15, 3, 5, // vertex 3

      +0.15, 3, 5, // vertex 3
      -0.15 , 3, 5, // vertex 4
      -0.15, 0, 5  // vertex 1
    };

    const GLfloat color_buffer_data [] = {
      0, 0.6f, 1, // color 1
      0, 0.6f, 1, // color 2
      0, 0.6f, 1, // color 3

      0, 0.6f, 1, // color 3
      0, 0.6f, 1, // color 4
      0, 0.6f, 1, // color 1
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    textTile[j] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  }
}

// Create the grid that the block will move on
void createTiles ()
{
  int i, j, c, initX = -7, initY = 5, id = 0;
  FILE * file;
  float r1, g1, b1, r2, g2, b2, z;

  switch (level) {
      case 1:
          file = fopen("level01.txt", "r");
          break;
      case 2:
          file = fopen("level02.txt", "r");
          break;
      case 3:
          file = fopen("level09.txt", "r");
          break;
      case 4:
          file = fopen("level04.txt", "r");
          break;
      case 5:
          file = fopen("level05.txt", "r");
          break;
      case 6:
          file = fopen("level06.txt", "r");
          break;
      case 7:
          file = fopen("level07.txt", "r");
          break;
      case 8:
          file = fopen("level08.txt", "r");
          break;
      case 9:
          file = fopen("level03.txt", "r");
          break;
      default:
          break;
  }

  for(i = 0; i < 10; i++)
  {
    for(j = 0; j < 20; j++)
    {
      z = 0.0f;
      c = getc(file);
      if(c == 'o' || c == 'S' || c == '.' || c == 'B' || c == 'X' || c == 'b' || c == 'x')
      {

        tileInfo[i][j].exists = true;
        tileInfo[i][j].id = c;
        tileInfo[i][j].z = 0.0f;

        if(c == 'S')
        {
          blockInfo.x = (int)initX;
          blockInfo.y = (int)initY;
        }

        if(c == 'S' || c == 'o')
        {
          r1 = 0.6f;
          g1 = 1.0f;
          b1 = 0;

          r2 = 0;
          g2 = 0.3f;
          b2 = 0;

          tileInfo[i][j].type = "normal";
          tileInfo[i][j].state = "present";
        }

        else if(c == '.')
        {
          r1 = 1.0f;
          g1 = 0.6f;
          b1 = 0.3f;

          r2 = 1.0f;
          g2 = 0.2f;
          b2 = 0.2f;

          tileInfo[i][j].type = "fragile";
          tileInfo[i][j].state = "present";
        }

        else if(c == 'X' || c == 'B')
        {
          r1 = 0.76f;
          g1 = 0.76f;
          b1 = 0.76f;

          r2 = 0.42f;
          g2 = 0.42f;
          b2 = 0.42f;

          tileInfo[i][j].type = "bridgeLightSwitch";
          tileInfo[i][j].state = "present";

          if(c == 'X')
          {
            tileInfo[i][j].type = "bridgeHeavySwitch";
            
            r1 = 0.8f;
            g1 = 0.0f;
            b1 = 0.8f;
          }
        }

        else if(c == 'x' || c == 'b')
        {
          r1 = 0.76f;
          g1 = 0.76f;
          b1 = 0.76f;

          r2 = 0.42f;
          g2 = 0.42f;
          b2 = 0.42f;

          tileInfo[i][j].type = "bridgeLight";
          tileInfo[i][j].state = "absent";
          tileInfo[i][j].id -= 32;

          if(c == 'x')
          {
            tileInfo[i][j].type = "bridgeHeavy";
            
            r1 = 0.8f;
            g1 = 0.0f;
            b1 = 0.8f;
          }
        }

        GLfloat vertex_buffer_data [] = {
          initX, initY, z, // vertex 1
          initX, initY-1, z, // vertex 2
          initX+1, initY, z, // vertex 3

          initX+1, initY, z, // vertex 3
          initX, initY-1, z, // vertex 2
          initX+1, initY-1, z,  // vertex 4

          initX, initY, z, // vertex 1
          initX, initY, z-0.2, // vertex 5
          initX, initY-1, z, // vertex 2

          initX, initY, z-0.2, // vertex 5
          initX, initY-1, z-0.2, // vertex 6
          initX, initY-1, z, // vertex 2

          initX+1, initY, z, // vertex 3
          initX+1, initY, z-0.2, // vertex 7
          initX+1, initY-1, z, // vertex 4

          initX+1, initY, z-0.2, // vertex 7
          initX+1, initY-1, z-0.2, // vertex 8
          initX+1, initY-1, z, // vertex 4

          initX, initY, z, // vertex 1
          initX+1, initY, z, // vertex 3
          initX, initY, z-0.2, // vertex 5

          initX+1, initY, z-0.2, // vertex 7
          initX+1, initY, z, // vertex 3
          initX, initY, z-0.2, // vertex 5

          initX, initY-1, z, // vertex 2
          initX+1, initY-1, z, // vertex 4
          initX, initY-1, z-0.2, // vertex 6

          initX+1, initY-1, z-0.2, // vertex 8
          initX+1, initY-1, z, // vertex 4
          initX, initY-1, z-0.2, // vertex 6

          initX, initY, z-0.2, // vertex 5
          initX, initY-1, z-0.2, // vertex 6
          initX+1, initY, z-0.2, // vertex 7

          initX+1, initY, z-0.2, // vertex 7
          initX, initY-1, z-0.2, // vertex 6
          initX+1, initY-1, z-0.2  // vertex 8
        };

        GLfloat color_buffer_data [] = {
          r1, g1, b1, // color 1
          r1, g1, b1, // color 2
          r1, g1, b1, // color 3

          r1, g1, b1, // color 1
          r1, g1, b1, // color 3
          r1, g1, b1,  // color 4

          r2, g2, b2, // color 1
          r2, g2, b2, // color 5
          r2, g2, b2, // color 2

          r2, g2, b2, // color 5
          r2, g2, b2, // color 6
          r2, g2, b2, // color 2

          r2, g2, b2, // color 3
          r2, g2, b2, // color 7
          r2, g2, b2, // color 4

          r2, g2, b2, // color 7
          r2, g2, b2, // color 8
          r2, g2, b2, // color 4

          r2, g2, b2, // color 1
          r2, g2, b2, // color 3
          r2, g2, b2, // color 5

          r2, g2, b2, // color 7
          r2, g2, b2, // color 3
          r2, g2, b2, // color 5

          r2, g2, b2, // color 2
          r2, g2, b2, // color 4
          r2, g2, b2, // color 6

          r2, g2, b2, // color 8
          r2, g2, b2, // color 4
          r2, g2, b2, // color 6

          r1, g1, b1, // color 5
          r1, g1, b1, // color 6
          r1, g1, b1, // color 7

          r1, g1, b1, // color 7
          r1, g1, b1, // color 6
          r1, g1, b1, // color 8
        };

        // create3DObject creates and returns a handle to a VAO that can be used later
        tile[i][j] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);

        GLfloat vertex_buffer_data2 [] = {
          initX, initY, z+0.01f, // vertex 1
          initX, initY-1, z+0.01f, // vertex 2

          initX+1, initY, z+0.01f, // vertex 3
          initX, initY, z+0.01f, // vertex 1

          initX, initY-1, z+0.01f, // vertex 2
          initX+1, initY-1, z+0.01f,  // vertex 4

          initX+1, initY, z+0.01f, // vertex 3
          initX+1, initY-1, z+0.01f  // vertex 4
        };

        GLfloat color_buffer_data2 [] = {
          r2, g2, b2, // color 1
          r2, g2, b2, // color 2

          r2, g2, b2, // color 3
          r2, g2, b2, // color 1

          r2, g2, b2, // color 2
          r2, g2, b2, // color 4

          r2, g2, b2, // color 3
          r2, g2, b2, // color 4
        };

        tileBorder[i][j] = create3DObject(GL_LINES, 8, vertex_buffer_data2, color_buffer_data2, GL_FILL);
      }
      else if(c == 'T')
      {
        goalInfo.x = (int) initX;
        goalInfo.y = (int) initY;
      }
      else if(c == '\n' || c == EOF)
      {
        break;
      }
      initX++;
    }
    if(c == EOF)
    {
      fclose(file);
      break;
    }
    initX = -7;
    initY--;
  }
}

void createBlock ()
{
      // GL3 accepts only Triangles. Quads are not supported
    GLfloat vertex_buffer_data1 [] = {
      0, 0, 0, // vertex 1
      0, -1, 0, // vertex 2
      1, 0, 0, // vertex 3

      1, 0, 0, // vertex 3
      0, -1, 0, // vertex 2
      1, -1, 0,  // vertex 4

      0, 0, 0, // vertex 1
      0, 0, 2, // vertex 5
      0, -1, 0, // vertex 2

      0, 0, 2, // vertex 5
      0, -1, 2, // vertex 6
      0, -1, 0, // vertex 2

      1, 0, 0, // vertex 3
      1, 0, 2, // vertex 7
      1, -1, 0, // vertex 4

      1, 0, 2, // vertex 7
      1, -1, 2, // vertex 8
      1, -1, 0, // vertex 4

      0, 0, 0, // vertex 1
      1, 0, 0, // vertex 3
      0, 0, 2, // vertex 5

      1, 0, 2, // vertex 7
      1, 0, 0, // vertex 3
      0, 0, 2, // vertex 5

      0, -1, 0, // vertex 2
      1, -1, 0, // vertex 4
      0, -1, 2, // vertex 6

      1, -1, 2, // vertex 8
      1, -1, 0, // vertex 4
      0, -1, 2, // vertex 6

      0, 0, 2, // vertex 5
      0, -1, 2, // vertex 6
      1, 0, 2, // vertex 7

      1, 0, 2, // vertex 7
      0, -1, 2, // vertex 6
      1, -1, 2  // vertex 8
    };

    GLfloat color_buffer_data1 [] = {
      1.0f, 0.6f, 0.2f, // color 1
      1.0f, 0.6f, 0.2f, // color 2
      1.0f, 0.6f, 0.2f, // color 3

      1.0f, 0.6f, 0.2f, // color 3
      1.0f, 0.6f, 0.2f, // color 2
      1.0f, 0.6f, 0.2f, // color 4

      0.6f, 0.3f, 0, // color 1
      0.6f, 0.3f, 0, // color 5
      0.6f, 0.3f, 0, // color 2

      0.6f, 0.3f, 0, // color 5
      0.6f, 0.3f, 0, // color 6
      0.6f, 0.3f, 0, // color 2

      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 4

      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 8
      0.6f, 0.3f, 0, // color 4

      0.6f, 0.3f, 0, // color 1
      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 5

      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 5

      0.6f, 0.3f, 0, // color 2
      0.6f, 0.3f, 0, // color 4
      0.6f, 0.3f, 0, // color 6

      0.6f, 0.3f, 0, // color 8
      0.6f, 0.3f, 0, // color 4
      0.6f, 0.3f, 0, // color 6

      1.0f, 0.6f, 0.2f, // color 5
      1.0f, 0.6f, 0.2f, // color 6
      1.0f, 0.6f, 0.2f, // color 7

      1.0f, 0.6f, 0.2f, // color 7
      1.0f, 0.6f, 0.2f, // color 6
      1.0f, 0.6f, 0.2f, // color 8
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    blockStanding = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data1, color_buffer_data1, GL_FILL);

    GLfloat vertex_buffer_data2 [] = {
      0, 0, 0, // vertex 1
      0, -1, 0, // vertex 2
      2, 0, 0, // vertex 3

      2, 0, 0, // vertex 3
      0, -1, 0, // vertex 2
      2, -1, 0,  // vertex 4

      0, 0, 0, // vertex 1
      0, 0, 1, // vertex 5
      0, -1, 0, // vertex 2

      0, 0, 1, // vertex 5
      0, -1, 1, // vertex 6
      0, -1, 0, // vertex 2

      2, 0, 0, // vertex 3
      2, 0, 1, // vertex 7
      2, -1, 0, // vertex 4

      2, 0, 1, // vertex 7
      2, -1, 1, // vertex 8
      2, -1, 0, // vertex 4

      0, 0, 0, // vertex 1
      2, 0, 0, // vertex 3
      0, 0, 1, // vertex 5

      2, 0, 1, // vertex 7
      2, 0, 0, // vertex 3
      0, 0, 1, // vertex 5

      0, -1, 0, // vertex 2
      2, -1, 0, // vertex 4
      0, -1, 1, // vertex 6

      2, -1, 1, // vertex 8
      2, -1, 0, // vertex 4
      0, -1, 1, // vertex 6

      0, 0, 1, // vertex 5
      0, -1, 1, // vertex 6
      2, 0, 1, // vertex 7

      2, 0, 1, // vertex 7
      0, -1, 1, // vertex 6
      2, -1, 1  // vertex 8
    };

    GLfloat color_buffer_data2 [] = {
      0.6f, 0.3f, 0, // color 1
      0.6f, 0.3f, 0, // color 2
      0.6f, 0.3f, 0, // color 3

      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 2
      0.6f, 0.3f, 0, // color 4

      1.0f, 0.6f, 0.2f, // color 1
      1.0f, 0.6f, 0.2f, // color 5
      1.0f, 0.6f, 0.2f, // color 2

      1.0f, 0.6f, 0.2f, // color 5
      1.0f, 0.6f, 0.2f, // color 6
      1.0f, 0.6f, 0.2f, // color 2

      1.0f, 0.6f, 0.2f, // color 3
      1.0f, 0.6f, 0.2f, // color 7
      1.0f, 0.6f, 0.2f, // color 4

      1.0f, 0.6f, 0.2f, // color 7
      1.0f, 0.6f, 0.2f, // color 8
      1.0f, 0.6f, 0.2f, // color 4

      0.6f, 0.3f, 0, // color 1
      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 5

      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 5

      0.6f, 0.3f, 0, // color 2
      0.6f, 0.3f, 0, // color 4
      0.6f, 0.3f, 0, // color 6

      0.6f, 0.3f, 0, // color 8
      0.6f, 0.3f, 0, // color 4
      0.6f, 0.3f, 0, // color 6

      0.6f, 0.3f, 0, // color 5
      0.6f, 0.3f, 0, // color 6
      0.6f, 0.3f, 0, // color 7

      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 6
      0.6f, 0.3f, 0, // color 8
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    blockSleepingX = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data2, color_buffer_data2, GL_FILL);

    GLfloat vertex_buffer_data3 [] = {
      0, 0, 0, // vertex 1
      0, -2, 0, // vertex 2
      1, 0, 0, // vertex 3

      1, 0, 0, // vertex 3
      0, -2, 0, // vertex 2
      1, -2, 0,  // vertex 4

      0, 0, 0, // vertex 1
      0, 0, 1, // vertex 5
      0, -2, 0, // vertex 2

      0, 0, 1, // vertex 5
      0, -2, 1, // vertex 6
      0, -2, 0, // vertex 2

      1, 0, 0, // vertex 3
      1, 0, 1, // vertex 7
      1, -2, 0, // vertex 4

      1, 0, 1, // vertex 7
      1, -2, 1, // vertex 8
      1, -2, 0, // vertex 4

      0, 0, 0, // vertex 1
      1, 0, 0, // vertex 3
      0, 0, 1, // vertex 5

      1, 0, 1, // vertex 7
      1, 0, 0, // vertex 3
      0, 0, 1, // vertex 5

      0, -2, 0, // vertex 2
      1, -2, 0, // vertex 4
      0, -2, 1, // vertex 6

      1, -2, 1, // vertex 8
      1, -2, 0, // vertex 4
      0, -2, 1, // vertex 6

      0, 0, 1, // vertex 5
      0, -2, 1, // vertex 6
      1, 0, 1, // vertex 7

      1, 0, 1, // vertex 7
      0, -2, 1, // vertex 6
      1, -2, 1  // vertex 8
    };

    GLfloat color_buffer_data3 [] = {
      0.6f, 0.3f, 0, // color 1
      0.6f, 0.3f, 0, // color 2
      0.6f, 0.3f, 0, // color 3

      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 2
      0.6f, 0.3f, 0, // color 4

      0.6f, 0.3f, 0, // color 1
      0.6f, 0.3f, 0, // color 5
      0.6f, 0.3f, 0, // color 2

      0.6f, 0.3f, 0, // color 5
      0.6f, 0.3f, 0, // color 6
      0.6f, 0.3f, 0, // color 2

      0.6f, 0.3f, 0, // color 3
      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 4

      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 8
      0.6f, 0.3f, 0, // color 4

      1.0f, 0.6f, 0.2f, // color 1
      1.0f, 0.6f, 0.2f, // color 3
      1.0f, 0.6f, 0.2f, // color 5

      1.0f, 0.6f, 0.2f, // color 7
      1.0f, 0.6f, 0.2f, // color 3
      1.0f, 0.6f, 0.2f, // color 5

      1.0f, 0.6f, 0.2f, // color 2
      1.0f, 0.6f, 0.2f, // color 4
      1.0f, 0.6f, 0.2f, // color 6

      1.0f, 0.6f, 0.2f, // color 8
      1.0f, 0.6f, 0.2f, // color 4
      1.0f, 0.6f, 0.2f, // color 6

      0.6f, 0.3f, 0, // color 5
      0.6f, 0.3f, 0, // color 6
      0.6f, 0.3f, 0, // color 7

      0.6f, 0.3f, 0, // color 7
      0.6f, 0.3f, 0, // color 6
      0.6f, 0.3f, 0, // color 8
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    blockSleepingY = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data3, color_buffer_data3, GL_FILL);
}

void rotateBlock (string direction)
{
  if(direction == "up")
  {
    rotateDirection[0]++;
  }

  if(direction == "down")
  {
    rotateDirection[1]++;
  }

  if(direction == "left")
  {
    rotateDirection[2]++;
  }

  if(direction == "right")
  {
    rotateDirection[3]++;
  }
  moveCount++;
}

void drawBridge(int id, string type)
{
  int i, j;

  for(i = 0; i < 10; i++)
  {
    for(j = 0; j < 20; j++)
    {
      if(tileInfo[i][j].exists && tileInfo[i][j].id == id && tileInfo[i][j].type + "Switch" == type && tileInfo[i][j].state == "absent")
      {
        tileInfo[i][j].state = "present";
      }
    }
  }
}

void updateBlockCoordinates()
{
  int i, flag = 0;

  for(i = 0; i < 4; i++)
  {
    if(rotateDirection[i])
    {
      resetRotateDirection ();
      if(blockInfo.state == "standing")
      {
        flag++;
        if(i > 1)
        {
          blockInfo.state = "sleepingX";
          if(i == 3)
          {
            blockInfo.x += 1;
          }
          else
          {
            blockInfo.x -= 2;
          }
        }
        else
        {
          blockInfo.state = "sleepingY";
          {
            if(i == 0)
            {
              blockInfo.y += 2;
            }
            else
            {
              blockInfo.y -= 1;
            }
          }
        }
      }
      else if(blockInfo.state == "sleepingX" && flag == 0)
      {
        flag++;
        if(i > 1)
        {
          blockInfo.state = "standing";
          if(i == 3)
          {
            blockInfo.x += 2;
          }
          else
          {
            blockInfo.x -= 1;
          }
        }
        else
        {
          {
            if(i == 0)
            {
              blockInfo.y += 1;
            }
            else
            {
              blockInfo.y -= 1;
            }
          }
        }
      }
      else if(flag == 0)
      {
        if(i > 1)
        {
          if(i == 3)
          {
            blockInfo.x += 1;
          }
          else
          {
            blockInfo.x -= 1;
          }
        }
        else
        {
          blockInfo.state = "standing";
          {
            if(i == 0)
            {
              blockInfo.y += 1;
            }
            else
            {
              blockInfo.y -= 2;
            }
          }
        }
      }
    }
  }
}

// Draw the level Number
void drawNumber(int Number)
{
  float y = -1.5;
  glm::vec3 eye (0, 0, 7);
  glm::vec3 target (0, 0, 0);
  glm::vec3 up (0, 1, 0);
  Matrices.view = glm::lookAt(eye, target, up);
  glm::mat4 VP = Matrices.projectionO * Matrices.view;
  glm::mat4 MVP;	// MVP = Projection * View * Model
  glm::mat4 rotateTile = glm::rotate((float)(-90.0f*M_PI/180.0f), glm::vec3(0,0,1));
  glm::mat4 translateTile = glm::translate (glm::vec3(0.0f, 0.0f, 0.0f));

  if(Number != 1 && Number != 4 && Number != 7)
  {
    Matrices.model = glm::mat4(1.0f);
    translateTile = glm::translate (glm::vec3(-2.1f, y-6.5f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile*rotateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[0]);
  }

  if(Number == 0 || Number == 2 || Number == 6 || Number == 8)
  {
    Matrices.model = glm::mat4(1.0f);
    translateTile = glm::translate (glm::vec3(-2.3f, y-6.4f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[1]);
  }

  if(Number != 0 && Number != 1 && Number != 7)
  {
    Matrices.model = glm::mat4(1.0f);
    translateTile = glm::translate (glm::vec3(-2.1f, y-3.4f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile*rotateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[2]);
  }

  if(Number != 2)
  {
    Matrices.model = glm::mat4(1.0f);
    translateTile = glm::translate (glm::vec3(1.2f, y-6.4f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[3]);
  }

  if(Number != 1 && Number != 2 && Number != 3 && Number != 7)
  {
    Matrices.model = glm::mat4(1.0f);
    translateTile = glm::translate (glm::vec3(-2.3f, y-3.4f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[4]);
  }

  if(Number != 1 && Number != 4)
  {
    Matrices.model = glm::mat4(1.0f);
    translateTile = glm::translate (glm::vec3(-2.1f, y-0.2f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile*rotateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[5]);
  }

  if(Number != 5 && Number != 6)
  {
    Matrices.model = glm::mat4(1.0f);
    translateTile = glm::translate (glm::vec3(1.2f, y-3.2f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[6]);
  }
}

// Draw the word 'Level'
void drawWord()
{
  float y = 1.0f;
  glm::vec3 eye (0, 0, 7);
  glm::vec3 target (0, 0, 0);
  glm::vec3 up (0, 1, 0);
  Matrices.view = glm::lookAt(eye, target, up);
  glm::mat4 VP = Matrices.projectionO * Matrices.view;
  glm::mat4 MVP;	// MVP = Projection * View * Model
  glm::mat4 rotateTile = glm::rotate((float)(-90.0f*M_PI/180.0f), glm::vec3(0,0,1));
  glm::mat4 translateTile = glm::translate (glm::vec3(-10.1f, 0.0f, 0.0f)); // glTranslatef

  // Draw L
  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-10.1f, y+0.0f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[0]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(6.5f, y+0.0f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[0]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-10.5f, y+0.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[1]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(6.1f, y+0.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[1]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-10.5f, y+3.2f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[4]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(6.1f, y+3.2f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[4]);

  // Draw E
  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-6.1f, y+0.0f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[0]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(2.4f, y+0.0f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[0]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-6.5f, y+0.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[1]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(2.0f, y+0.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[1]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-6.5f, y+3.2f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[4]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(2.0f, y+3.2f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[4]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-6.1f, y+3.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[2]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(2.4f, y+3.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[2]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-6.1f, y+6.20f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[5]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(2.4f, y+6.20f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[5]);

  // Draw V
  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-2.1f, y+0.0f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile*rotateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[0]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-2.3f, y+0.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[1]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(-2.3f, y+3.2f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[4]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(1.2f, y+0.1f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[3]);

  Matrices.model = glm::mat4(1.0f);
  translateTile = glm::translate (glm::vec3(1.2f, y+3.2f, 0.0f)); // glTranslatef
  Matrices.model *= translateTile;
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(textTile[6]);
}

void drawMoveCount(int Number, int index)
{  
  glm::vec3 eye (0, 0, 7);
  glm::vec3 target (0, 0, 0);
  glm::vec3 up (0, 1, 0);
  Matrices.view = glm::lookAt(eye, target, up);
  glm::mat4 VP = Matrices.projectionO * Matrices.view;
  glm::mat4 MVP;	// MVP = Projection * View * Model
  glm::mat4 rotateTile = glm::rotate((float)(-90.0f*M_PI/180.0f), glm::vec3(0,0,1));

  if(Number != 1 && Number != 4 && Number != 7)
  {
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTile = glm::translate (glm::vec3(screenRightX - 10.0f - 3.75f*index, 0.0f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile*rotateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[0]);
  }

  if(Number == 0 || Number == 2 || Number == 6 || Number == 8)
  {
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTile = glm::translate (glm::vec3(screenRightX - 10.0f - 3.75f*index - 0.05f, 0.1f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[1]);
  }

  if(Number != 0 && Number != 1 && Number != 7)
  {
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTile = glm::translate (glm::vec3(screenRightX - 10.0f - 3.75f*index, 3.1f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile*rotateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[2]);
  }

  if(Number != 2)
  {
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTile = glm::translate (glm::vec3(screenRightX - 10.0f - 3.75f*index + 3.05f, 0.1f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[3]);
  }

  if(Number != 1 && Number != 2 && Number != 3 && Number != 7)
  {
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTile = glm::translate (glm::vec3(screenRightX - 10.0f - 3.75f*index - 0.05f, 3.2f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[4]);
  }

  if(Number != 1 && Number != 4)
  {
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTile = glm::translate (glm::vec3(screenRightX - 10.0f - 3.75f*index, 6.20f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile*rotateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[5]);
  }

  if(Number != 5 && Number != 6)
  {
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTile = glm::translate (glm::vec3(screenRightX - 10.0f - 3.75f*index + 3.05f, 3.2f, 0.0f)); // glTranslatef
    Matrices.model *= translateTile;
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(textTile[6]);
  }
}

void getMoveCount()
{
  int i, temp = moveCount;
  for(i = 0; i < 3; i++)
  {
    drawMoveCount(temp%10, i);
    temp /= 10;
  }
}

void setCamera(int view, glm::vec3 * eye, glm::vec3 * target, glm::vec3 * up)
{
  int i, j;
  j = blockInfo.x + 7;
  i = 5 - blockInfo.y;
  float x = 0.0f, y = 0.0f, z = 0.5f, targetX = 0.0f, targetY = 0.0f, targetZ = 0.0f, upAngle, temp1, temp2;
  
  * up = glm::vec3 (0, 1, 0);
  
  // In case Block is standing and view is NOT chase cam
  if(blockInfo.state == "standing")
  {
    z = 1.5f;
  }

  // Viewing/Chasing in +y direction (upwards)
  if(view == 2 || view == 6)
  {
    x = 0.5f;
    // Specific case for Chase Cam
    if(view == 6)
    {
      y = -3.0f;
      z = 4.0f;
    }

    targetX = ((float)blockInfo.x) + x;
    while(tileInfo[i--][j].exists)
    targetY = 5-i;
  }

  // Viewing/Chasing in -y direction (downwards)
  else if(view == 3 || view == 7)
  {
    x = 0.5f;
    y = -1.0f;
    * up = glm::vec3 (0, -1, 0);
    if(blockInfo.state == "sleepingY")
    {
      y = -2.0f;
      z = 4.0f;
    }

    // Specific case for Chase Cam
    if(view == 7)
    {
      y = 3.0f;
      z = 4.0f;
    }

    targetX = ((float)blockInfo.x) + x;
    while(tileInfo[i++][j].exists)
    targetY = 5-i;
  }

  // Viewing/Chasing in -x direction (leftwards)
  else if(view == 4 || view == 8)
  {
    y = -0.5f;
    * up = glm::vec3 (-1, 0, 0);
    // Specific case for Chase Cam
    if(view == 8)
    {
      x = 3.0f;
      z = 4.0f;
    }

    while(tileInfo[i][j--].exists)
    targetX = j-7;
    targetY = ((float)blockInfo.y) + y;
  }

  // Viewing/Chasing in +x direction (rightwards)
  else if(view == 5 || view == 9)
  {
    x = 1.0f;
    y = -0.5f;
    * up = glm::vec3 (1, 0, 0);
    if(blockInfo.state == "sleepingX")
    {
      x = 2.0f;
    }

    // Specific case for Chase Cam
    if(view == 9)
    {
      x = -3.0f;
      z = 4.0f;
    }

    while(tileInfo[i][j++].exists)
    targetX = j - 7;
    targetY = ((float)blockInfo.y) + y;
  }

  * eye = glm::vec3 (((float)blockInfo.x)+x, ((float)blockInfo.y)+y, z);
  * target = glm::vec3 (targetX, targetY, targetZ);

  // Helicopter View
  if(view == 10)
  {
    x = ((float)blockInfo.x) + cos(angle*M_PI/180.0f)*5.0f;
    y = ((float)blockInfo.y) + sin(angle*M_PI/180.0f)*5.0f;
    if(keyStates[GLFW_MOUSE_BUTTON_LEFT])
    {
      x = mouseX;
      y = mouseY;
      angle = atan2(y,x);
    }
    * eye = glm::vec3 (x, y, helicopterDepth);
    * target = glm::vec3 (((float)blockInfo.x) + 0.5f, ((float)blockInfo.y) - 0.5f, 2.0f);

    temp1 = sqrt(x*x + y*y);

    upAngle = 90.0f + ((float)atan2(helicopterDepth, temp1));

    * up = glm::vec3 (cos(upAngle*M_PI/180.0f)*cos((angle + 180.0f)*M_PI/180.0f),
                      cos(upAngle*M_PI/180.0f)*sin((angle + 180.0f)*M_PI/180.0f),
                      sin(upAngle*M_PI/180.0f));
  }
}

/* Render the scene with openGL */
void draw ()
{
    int i, j;
    float angle = 0;
    
    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);
    
    glm::vec3 eye (0, 0, 7);
    glm::vec3 target (0, 0, 0);
    glm::vec3 up(0, 1, 0);
    
    Matrices.view = glm::lookAt(eye, target, up);
    glm::mat4 VP = Matrices.projectionP * Matrices.view;
    glm::mat4 MVP;
    
    if(proj_type == 0)
    {
      VP = Matrices.projectionO * Matrices.view;
    }
    else if(proj_type == 1)
    {
      eye = glm::vec3 (0, -6, 7);
      target = glm::vec3 (0, 0, 0);
      Matrices.view = glm::lookAt(eye, target, up);
      VP = Matrices.projectionP * Matrices.view;
    }
    else if(proj_type > 1)
    {
      setCamera(proj_type, &eye, &target, &up);
      Matrices.view = glm::lookAt(eye, target, up);
      VP = Matrices.projectionP * Matrices.view;
    }
    Matrices.model = glm::mat4(1.0f);
    
    if(levelUp)
    {
      resetTileState();
      createTiles();
      createBlock();
    
      proj_type = 0;
      Matrices.model = glm::mat4(1.0f);
      VP = Matrices.projectionO * Matrices.view;
      MVP = VP * Matrices.model;
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
      draw3DObject(textBackground);
    
      drawWord();
      drawNumber(level);
    
      takingInput = false;
      return;
    }
    
    takingInput = true;
    
    // Draw Tiles
    for(i = 0; i < 10; i++)
    {
      for(j = 0; j < 20; j++)
      {
        if(tileInfo[i][j].exists && tileInfo[i][j].state == "present")
        {
          Matrices.model = glm::mat4(1.0f);
          glm::mat4 tileTranslation = glm::translate (glm::vec3((0.0f, 0.0f, tileInfo[i][j].z))); // glTranslatef
          Matrices.model *= (tileTranslation);
          MVP = VP * Matrices.model;
          glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
          draw3DObject(tile[i][j]);
          draw3DObject(tileBorder[i][j]);
        }
      }
    }
    
    int flag = 0;
    
    // Draw Block
    Matrices.model = glm::mat4(1.0f);
    
    updateBlockCoordinates();
    
    glm::mat4 blockTranslation = glm::translate (glm::vec3((float)blockInfo.x, (float)blockInfo.y, 0.0f)); // glTranslatef
    Matrices.model *= (blockTranslation);
    MVP = VP * Matrices.model; // MVP = p * V * M
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    
    if(blockInfo.state == "standing")
    {
      draw3DObject(blockStanding);
      if (blockInfo.x == goalInfo.x && blockInfo.y == goalInfo.y)
      {
        levelUp = true;
        last_update_time = glfwGetTime();
        level++;
      }
      else if(!tileInfo[5-blockInfo.y][blockInfo.x+7].exists ||
              tileInfo[5-blockInfo.y][blockInfo.x+7].type == "fragile" ||
              tileInfo[5-blockInfo.y][blockInfo.x+7].state == "absent")
      {
        gameOver = true;
      }
    
      else if(tileInfo[5-blockInfo.y][blockInfo.x+7].type == "bridgeLightSwitch" ||
              tileInfo[5-blockInfo.y][blockInfo.x+7].type == "bridgeHeavySwitch")
      {
        drawBridge(tileInfo[5-blockInfo.y][blockInfo.x+7].id, tileInfo[5-blockInfo.y][blockInfo.x+7].type);
      }
    }
    else if(blockInfo.state == "sleepingX")
    {
      draw3DObject(blockSleepingX);
      if(!(blockInfo.x == goalInfo.x && blockInfo.y == goalInfo.y) &&
        (!(blockInfo.x + 1 == goalInfo.x && blockInfo.y == goalInfo.y)) &&
        ((!tileInfo[5-blockInfo.y][blockInfo.x+7].exists || !tileInfo[5-blockInfo.y][blockInfo.x+8].exists) ||
        (tileInfo[5-blockInfo.y][blockInfo.x+7].state == "absent" || tileInfo[5-blockInfo.y][blockInfo.x+8].state == "absent")))
      {
        gameOver = true;
      }
      else if(tileInfo[5-blockInfo.y][blockInfo.x+7].type == "bridgeLightSwitch")
      {
        drawBridge(tileInfo[5-blockInfo.y][blockInfo.x+7].id, tileInfo[5-blockInfo.y][blockInfo.x+7].type);
      }
      else if(tileInfo[5-blockInfo.y][blockInfo.x+8].type == "bridgeLightSwitch")
      {
        drawBridge(tileInfo[5-blockInfo.y][blockInfo.x+8].id, tileInfo[5-blockInfo.y][blockInfo.x+8].type);
      }
    }
    else
    {
      draw3DObject(blockSleepingY);
      if(!(blockInfo.x == goalInfo.x && blockInfo.y == goalInfo.y) &&
        (!(blockInfo.x == goalInfo.x && blockInfo.y - 1 == goalInfo.y)) &&
        ((!tileInfo[5-blockInfo.y][blockInfo.x+7].exists || !tileInfo[6-blockInfo.y][blockInfo.x+7].exists) ||
        (tileInfo[5-blockInfo.y][blockInfo.x+7].state == "absent" || tileInfo[6-blockInfo.y][blockInfo.x+7].state == "absent")))
      {
        gameOver = true;
      }
      else if(tileInfo[5-blockInfo.y][blockInfo.x+7].type == "bridgeLightSwitch")
      {
        drawBridge(tileInfo[5-blockInfo.y][blockInfo.x+7].id, tileInfo[5-blockInfo.y][blockInfo.x+7].type);
      }
      else if(tileInfo[6-blockInfo.y][blockInfo.x+7].type == "bridgeLightSwitch")
      {
        drawBridge(tileInfo[6-blockInfo.y][blockInfo.x+7].id, tileInfo[6-blockInfo.y][blockInfo.x+7].type);
      }
    }
}
/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, scroll_callback);

    windowCopy = window;
    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models

  // Generate the VAO, VBOs, vertices data & copy into the array buffer
  createTiles ();
  createBlock ();
  createBackground();
  createTextTile ();
  
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

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
  int width = 1100;
  int height = 550;

  mpg123_handle *mh;
  unsigned char * buffer;
  size_t buffer_size;
  size_t done;
  int i, bufferCount, err;

  int defaultDriver;
  ao_device *dev;
  
  ao_sample_format format;
  int channels, encoding;
  long rate;

  proj_type = 0;

    GLFWwindow* window = initGLFW(width, height);

  initialize();

	initGL (window, width, height);

  last_update_time = glfwGetTime();

  ao_initialize();
  
  defaultDriver = ao_default_driver_id();
  mpg123_init();
  mh = mpg123_new(NULL, &err);
  buffer_size = 4096;
  
  buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));
  /* open the file and get the decoding format */
  mpg123_open(mh, "bomberman.mp3");
  mpg123_getformat(mh, &rate, &channels, &encoding);
  
  /* set the output format and open the output device */
  format.bits = mpg123_encsize(encoding) * BITS;
  format.rate = rate;
  format.channels = channels;
  format.byte_format = AO_FMT_NATIVE;
  format.matrix = 0;
  dev = ao_open_live(defaultDriver, &format, NULL);

    /* Draw in loop */
    while (!glfwWindowShouldClose(window) && !gameOver && level != 10) {

      	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // OpenGL Draw commands
        keyStateCheck();
        
        reshapeWindow(window, 1100, 550);
        
        /* Play sound */
        if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
        {
          ao_play(dev, (char *)buffer, done);
        }
        else
        {
          mpg123_seek(mh, 0, SEEK_SET);
        }

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= updateTime) { // atleast 1s elapsed since last frame
            // do something every 0.5 seconds ..
            if(levelUp)
            {
              levelUp = false;
            }
            last_update_time = current_time;
        }
    }
    
    if(level < 10)
      printf("\n\nGame Over!\n______________________\n\nYou Final Move Count is %d\n\n", moveCount);
    
    else
      printf("\n\nYou beat the game!\n______________________\n\nYou Final Move Count is %d\n\n", moveCount);
    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
