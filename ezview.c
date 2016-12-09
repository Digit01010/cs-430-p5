#define GLFW_DLL 1

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

GLFWwindow* window;

#include "linmath.h"

#include <assert.h>

#define PI 3.1415926535

typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

// Holds an rgb triple of a pixel
typedef struct {
  unsigned char red, green, blue;
} Pixel;

// Holds information about the header of a ppm file
typedef struct Header {
   unsigned char magicNumber;
   unsigned int width, height, maxColor;
} Header;

// Function declarations
Header parseHeader(FILE *);
void readP3(Pixel *, Header, FILE *);
void readP6(Pixel *, Header, FILE *);
void skipComments(FILE *);

// (-1, 1)  (1, 1)
// (-1, -1) (1, -1)

/*Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0}},
  {{1, 1},  {0.99999, 0.99999}},
  {{-1, 1}, {0, 0.99999}},
  {{-1, -1},  {0, 0}},
};*/

/*Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0}},
  {{1, 1},  {0.99999, 0.99999}},
  {{-1, 1}, {0, 0.99999}},
  {{-1, 1}, {0, 0.99999}},
  {{-1, -1}, {0, 0}},
  {{1, -1}, {0.99999, 0}}
};*/

/*Vertex vertexes[] = {
  {{1, -1}, {0, 0.99999}},
  {{1, 1},  {0, 0}},
  {{-1, 1}, {0.99999, 0}},
  {{-1, 1}, {0.99999, 0}},
  {{-1, -1},{0.99999, 0.99999}},
  {{1, -1}, {0, 0.99999}}
};*/

Vertex vertexes[] = {
  {{-1, -1}, {0, 1}},
  {{-1, 1},  {0, 0}},
  {{1, 1}, {1, 0}},
  {{1, 1}, {1, 0}},
  {{1, -1},{1, 1}},
  {{-1, -1}, {0, 1}}
};

/*Vertex vertexes[] = {
  {{1, -1}, {1, 0}},
  {{1, 1},  {1, 1}},
  {{-1, 1}, {0, 1}},
  {{-1, 1}, {0, 1}},
  {{-1, -1},{0, 0}},
  {{1, -1}, {1, 0}}
};*/


/*Vertex vertexes[] = {
  {{1, -1}, {1, 0}},
  {{-1, 1}, {0, 1}},
  {{1, 1},  {1, 1}},

  {{-1, 1}, {0, 1}},
  {{1, -1}, {1, 0}},
  {{-1, -1},{0, 0}}
};*/


const GLubyte indexes[] = {
  0, 1, 2,
  2, 3, 0
};

mat4x4 current_transform;

//GLint mvp_location;

static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec2 TexCoordIn;\n"
"attribute vec2 vPos;\n"
"varying vec2 TexCoordOut;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    TexCoordOut = TexCoordIn;\n"
"}\n";

static const char* fragment_shader_text =
"varying highp vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
"}\n";

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_0 && action == GLFW_PRESS)
        mat4x4_identity(current_transform);
    if (key == GLFW_KEY_E && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        mat4x4 m;
        mat4x4_identity(m);
        mat4x4_rotate_Z(m, m, (float) -2*PI/80.0);
        mat4x4_mul(current_transform, m, current_transform);
    }
    if (key == GLFW_KEY_Q && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        mat4x4 m;
        mat4x4_identity(m);
        mat4x4_rotate_Z(m, m, (float) 2*PI/80.0);
        mat4x4_mul(current_transform, m, current_transform);
    }
    if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        mat4x4 m;
        mat4x4_translate(m, 0.0, 0.1, 0.0);
        mat4x4_mul(current_transform, m, current_transform);
    }
    if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        mat4x4 m;
        mat4x4_translate(m, 0.0, -0.1, 0.0);
        mat4x4_mul(current_transform, m, current_transform);
    }
    if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        mat4x4 m;
        mat4x4_translate(m, -0.1, 0.0, 0.0);
        mat4x4_mul(current_transform, m, current_transform);
    }
    if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        mat4x4 m;
        mat4x4_translate(m, 0.1, 0.0, 0.0);
        mat4x4_mul(current_transform, m, current_transform);
    }
}

void glCompileShaderOrDie(GLuint shader) {
  GLint compiled;
  glCompileShader(shader);
  glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader,
		  GL_INFO_LOG_LENGTH,
		  &infoLen);
    char* info = malloc(infoLen+1);
    GLint done;
    glGetShaderInfoLog(shader, infoLen, &done, info);
    printf("Unable to compile shader: %s\n", info);
    exit(1);
  }
}

// 4 x 4 image..
/*unsigned char image[] = {
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,

  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,

  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,

  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255
};*/

/*unsigned char image[] = {
  255, 0, 0,
  255, 0, 0,
  255, 0, 0,
  255, 255, 0,

  0, 255, 0,
  0, 255, 0,
  255, 255, 0,
  0, 255, 0,

  0, 0, 255,
  255, 255, 0,
  0, 0, 255,
  0, 0, 255,

  255, 255, 0,
  255, 0, 255,
  255, 0, 255,
  255, 0, 255
};*/

unsigned char image[] = {
255, 0, 0,
255, 0, 0,
255, 0, 0,
255, 0, 0,
255, 0, 0,

255, 255, 255,
255, 255, 255,
255, 0, 0,
255, 255, 255,
255, 255, 255,

255, 255, 255,
255, 255, 255,
255, 0, 0,
255, 255, 255,
255, 255, 255,

255, 0, 0,
255, 255, 255,
255, 0, 0,
255, 255, 255,
255, 255, 255,

255, 0, 0,
255, 0, 0,
255, 0, 0,
255, 255, 255,
255, 255, 255,
0, 255, 0
};

int main(int argc, char *argv[])
{

   if (argc != 2) {
     fprintf(stderr, "Error: Incorrect number of arguments.\n");
     printf("Usage: ezview inputFile\n");
     return(1);
   }
   
  FILE* input = fopen(argv[1], "r");
  if (input == NULL) {
    fprintf(stderr, "Error: Unable to open input file.");
    return 1;
  }
    
  // Get header information from input file
  Header inHeader = parseHeader(input);
  
  if (inHeader.maxColor > 255) {
    fprintf(stderr, "Error: Maximum color greater than 255 not supported.\n");
    return 1;
  }
  
  // Create buffer and read data from input using appropriate function.
  Pixel *buffer = malloc(sizeof(Pixel) * inHeader.width * inHeader.height);
  if (inHeader.magicNumber == 3) {
    readP3(buffer, inHeader, input);
  }
  else if (inHeader.magicNumber == 6) {
    readP6(buffer, inHeader, input);
  }
  else {
    fprintf(stderr, "Error: Input magic number not supported.\n");
  }
    fclose(input);

    //for (int i=0; i<inHeader.width * inHeader.height;i++) {
	//  printf("%d, %d, %d \n", buffer[i].red, buffer[i].green, buffer[i].blue);
	//}

    unsigned char *raw_data = malloc(sizeof(unsigned char)*4*inHeader.width * inHeader.height);
    for (int i=0; i<inHeader.width * inHeader.height;i++) {
	  raw_data[i*4] = buffer[i].red;
	  raw_data[i*4+1] = buffer[i].green;
	  raw_data[i*4+2] = buffer[i].blue;
	  raw_data[i*4+3] = 255;
	  //printf("%d, %d, %d \n", buffer[i].red, buffer[i].green, buffer[i].blue);
	}


    GLFWwindow* window;
    GLuint vertex_buffer, index_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    // gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    //glGenBuffers(1, &vertex_buffer);
    //glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);
     
    // Create Buffer
    glGenBuffers(1, &vertex_buffer);

    // Map GL_ARRAY_BUFFER to this buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

    // Send the data
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes), indexes, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    // more error checking! glLinkProgramOrDie!

    mvp_location = glGetUniformLocation(program, "MVP");
    printf("%d\n", mvp_location);
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLint tex_location = glGetUniformLocation(program, "Texture");
    assert(tex_location != -1);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) 0);

    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) (sizeof(float) * 2));
    
    int image_width = 5;
    int image_height = 5;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, 
	//	 GL_UNSIGNED_BYTE, image);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inHeader.width, inHeader.height, 0, GL_RGBA, 
		 GL_UNSIGNED_BYTE, raw_data);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);
    
    mat4x4_identity(current_transform);
    //mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    //mat4x4_mul(current_transform, p, current_transform);

    while (!glfwWindowShouldClose(window))
    {
        float ratio, imgratio;
        int width, height;
        mat4x4 m, p, mvp;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;
        
		imgratio = inHeader.width / (float) inHeader.height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4x4_identity(m);
        //mat4x4_rotate_Z(m, m, (float) glfwGetTime());
        //mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        //mat4x4_mul(mvp, p, m);
        mat4x4_ortho(p, -imgratio, imgratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, p, m);
        mat4x4_mul(mvp, current_transform, mvp);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}

// Parses the data in the header and moves the position to the
// beginning of the data.
Header parseHeader(FILE *fh) {
  Header h;
  
  // Check if there is a magic number
  if (fgetc(fh) != 'P') {
    fprintf(stderr, "Error: Malformed input magic number. \n");
    exit(1);
  }
  
  // Parse magic number
  fscanf(fh, "%d ", &h.magicNumber);
  
  skipComments(fh);
  
  // Parse width
  fscanf(fh, "%d ", &h.width);
  
  skipComments(fh);
  
  // Parse height
  fscanf(fh, "%d ", &h.height);
  
  skipComments(fh);
  
  // Parse maximum color value
  fscanf(fh, "%d", &h.maxColor);
  
  // Skip single whitespace character before data
  fgetc(fh);
  
  // Check if any parsing encountered an error.
  if (ferror(fh) != 0) {
     fprintf(stderr, "Error: Unable to read header.");
     exit(1);
  }
   
  return h;
}

// Reads P3 data
void readP3(Pixel *buffer, Header h, FILE *fh) {
  // Read RGB triples
  for (int i = 0; i < h.width * h.height; i++) {
     fscanf(fh, "%d %d %d", &buffer[i].red, &buffer[i].green, &buffer[i].blue);
  }
  if (ferror(fh) != 0) {
     fprintf(stderr, "Error: Unable to read data.");
     exit(1);
  }
}


// Reads P6 data
void readP6(Pixel *buffer, Header h, FILE *fh) {
  // Read RGB triples
  for (int i = 0; i < h.width * h.height; i++) {
     buffer[i].red = fgetc(fh);
     buffer[i].green = fgetc(fh);
     buffer[i].blue = fgetc(fh);
  }
  if (ferror(fh) != 0) {
     fprintf(stderr, "Error: Unable to read data.");
     exit(1);
  }
}


// Skips lines that begin with '#'
void skipComments(FILE *fh) {
  char c = fgetc(fh);
  // Skip all comment lines
  while (c == '#') {
    // Go to the end of the line
    do {
      c = fgetc(fh);
      // Comments are potentially not closed
      if (c == EOF) {
        fprintf(stderr, "Error: Reached EOF when parsing comment.");
        exit(1);
      }
    } while (c != '\n');
    
    // Get the next character to test for a comment line
    c = fgetc(fh);
  }
  
  // The standard library does not have a peek so we get and unget instead.
  ungetc(c, fh);
}

//! [code]
