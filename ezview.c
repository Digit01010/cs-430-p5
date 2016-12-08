#define GLFW_DLL 1

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>

#include <stdlib.h>
#include <stdio.h>

GLFWwindow* window;

#include "linmath.h"

#include <assert.h>

typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

// Holds an rgb triple of a pixel
typedef struct Pixel {
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
void writeP3(Pixel *, Header, FILE *);
void readP6(Pixel *, Header, FILE *);
void writeP6(Pixel *, Header, FILE *);
void skipComments(FILE *);

// (-1, 1)  (1, 1)
// (-1, -1) (1, -1)

/*Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0}},
  {{1, 1},  {0.99999, 0.99999}},
  {{-1, 1}, {0, 0.99999}},
  {{-1, -1},  {0, 0}},
};*/

Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0}},
  {{1, 1},  {0.99999, 0.99999}},
  {{-1, 1}, {0, 0.99999}},
  {{-1, 1}, {0, 0.99999}},
  {{-1, -1},  {0, 0}},
  {{1, -1}, {0.99999, 0}}
};


const GLubyte indexes[] = {
  0, 1, 2,
  2, 3, 0
};

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
"varying lowp vec2 TexCoordOut;\n"
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

unsigned char image[] = {
  255, 0, 0,
  255, 0, 0,
  255, 0, 0,
  255, 0, 0,

  0, 255, 0,
  0, 255, 0,
  0, 255, 0,
  0, 255, 0,

  0, 0, 255,
  0, 0, 255,
  0, 0, 255,
  0, 0, 255,

  255, 0, 255,
  255, 0, 255,
  255, 0, 255,
  255, 0, 255
};

int main(int argc, char *argv[])
{
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
    
    int image_width = 4;
    int image_height = 4;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, 
		 GL_UNSIGNED_BYTE, image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);

    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        mat4x4 m, p, mvp;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4x4_identity(m);
        //mat4x4_rotate_Z(m, m, (float) glfwGetTime());
        mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, p, m);

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

// Writes P3 data
void writeP3(Pixel *buffer, Header h, FILE *fh) {
  // Write the header
  fprintf(fh, "P%d\n%d %d\n%d\n", h.magicNumber, h.width, h.height, h.maxColor);
  // Write the ascii data
  for (int i = 0; i < h.width * h.height; i++) {
     fprintf(fh, "%d\n%d\n%d\n", buffer[i].red, buffer[i].green, buffer[i].blue);
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

// Writes P6 data
void writeP6(Pixel *buffer, Header h, FILE *fh) {
  // Write header
  fprintf(fh, "P%d\n%d %d\n%d\n", h.magicNumber, h.width, h.height, h.maxColor);
  // Write binary data
  for (int i = 0; i < h.width * h.height; i++) {
     fputc(buffer[i].red, fh);
     fputc(buffer[i].green, fh);
     fputc(buffer[i].blue, fh);
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
