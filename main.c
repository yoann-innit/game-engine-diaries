#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h> 
#include <malloc.h>
    #define SCREEN_WIDTH 800
    #define SCREEN_HEIGHT 600
    #define MAX_VERTICES 512

typedef struct Vec3 {
    float x, y, z;
}Vec3;


typedef struct Vertex {
    Vec3 pos;
    Vec3 color;
}Vertex;

typedef struct Vertices {
    float vertices[MAX_VERTICES];
    int indices[MAX_VERTICES];
    unsigned int attributes_count;
    unsigned int vertices_count;
    unsigned int indices_count;
} Vertices;

typedef struct Object
{
    float* color_points[MAX_VERTICES];
    int color_points_count;
} Object;

typedef enum CALL_STATUS
{
    SUCCESS=0, ERROR=1
} CALL_STATUS;

void logger(CALL_STATUS status, const char * func, const char* msg)
{
    if (status == SUCCESS)
    {
        printf("[SUCCESS] %s -> %s\n", func, msg);
    }
    if (status == ERROR)
    {
        printf("[ERROR] %s -> %s\n", func, msg);
    }
}

bool comp_vec3(Vec3 a, Vec3 b)
{
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}

/*window management*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLFWwindow *init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Cracking Graphics", NULL, NULL);
    if (!window)
    {
        printf("%s", "Failed to create GLFW window\n");
        return NULL;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    return window;
}

void release_window()
{
    glfwTerminate();
}
/*end window management*/

/*utilities*/
const char* read_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    if (file_size < 0) {
        perror("Failed to determine file size");
        fclose(file);
        return NULL;
    }

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    if (read_size != file_size) {
        perror("Failed to read the file completely");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';

    fclose(file);

    return buffer;
}

/*end utilities*/

/*shaders managements*/

typedef struct Shader {
    unsigned int id;
} Shader;

void create_shader(const char *filename, GLenum type, Shader *shader, int *status)
{
    const char* code = read_file(filename);
    if (!code)
    {
        *status = ERROR;
        logger(ERROR, "read_file", "couldnt open shader file");
        return;
    }
    int ok;
    char log[512];

    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &code, NULL);
    glCompileShader(id);
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        *status = ERROR;
        glGetShaderInfoLog(id, 512, NULL, log);
        logger(ERROR, "glGetShaderInfoLog", log);
        return;
    }
    shader->id = id;
    *status = SUCCESS;
    logger(SUCCESS, "create_shader", "shader created succesfully");

}

void attach_shader_to_program(Shader shader, unsigned int program)
{
    glAttachShader(program, shader.id);
    logger(SUCCESS, "attach_shader_to_program", "shader attached to program");
}

void create_shader_program(unsigned int* id, Shader  vert, Shader frag, int *status)
{
    unsigned int program = glCreateProgram();
    attach_shader_to_program(vert, program);
    attach_shader_to_program(frag, program);

    int ok;
    char log[512];

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        *status = ERROR;
        glGetProgramInfoLog(program, 512, NULL, log);
        logger(ERROR, "glGetProgramInfoLog", log);
        return;
    }
    logger(SUCCESS, "create_program", "program created succesfully");
    *status = SUCCESS;
    *id = program;
}

void use_program(unsigned int program)
{
    glUseProgram(program);
}
/*end shaders management*/

/* start primitives */

void add_vertex(Vertices *vertices, Vec3 pos, Object *obj)
{
    /*Position*/
    vertices->vertices[vertices->vertices_count] = pos.x;
    vertices->vertices_count++;
    vertices->vertices[vertices->vertices_count++] = pos.y;
    vertices->vertices[vertices->vertices_count++] = pos.z;

    /*Color => default to white : set for global vertices and store pointers in object for later updates*/
    vertices->vertices[vertices->vertices_count++] = 1.0f;
    obj->color_points[obj->color_points_count] = &vertices->vertices[vertices->vertices_count-1];
    obj->color_points_count++;

    vertices->vertices[vertices->vertices_count++] = 1.0f;
    obj->color_points[obj->color_points_count++] = &vertices->vertices[vertices->vertices_count - 1];

    vertices->vertices[vertices->vertices_count++] = 1.0f;
    obj->color_points[obj->color_points_count++] = &vertices->vertices[vertices->vertices_count - 1];

}

void link_vertices(Vertices* vertices, Vec3 pos[], int count, int *status, Object *object)
{
    for (int i = 0; i < count; i++, vertices->indices_count++)
    {
        int indice = get_vertex_indice(vertices, pos[i]);
        vertices->indices[vertices->indices_count] = indice;
    }
    *status = SUCCESS;
}

int get_vertex_indice(Vertices *vertices, Vec3 pos)
{
    int count = vertices->vertices_count;
    int att_count = vertices->attributes_count;

    int vertex_idx = 0;

    for (int i = 0, vertex_idx = 0; i < count - (3 * (att_count - 1)); i += 3 * (att_count - 1), vertex_idx++)
    {
        Vec3 curr = (Vec3){ vertices->vertices[i], vertices->vertices[i + 1],vertices->vertices[i + 2] };
        if (comp_vec3(curr, pos))
        {
            logger(SUCCESS, "get_vertex_indice", "found vertex indice");
            return vertex_idx/2;
        }
    }
    logger(ERROR, "get_vertex_indice", "couldnt get vertex indice");
    return -1;
}

Object *create_triangle(Vertices *vertices, Vec3 top, Vec3 left, Vec3 right, int *status)
{
    Object* triangle = malloc(sizeof(Object));
    triangle->color_points_count = 0;
    add_vertex(vertices, top, triangle);
    add_vertex(vertices, left, triangle);
    add_vertex(vertices, right, triangle);

    CALL_STATUS status_link;

    link_vertices(vertices, (Vec3[]) { top, left, right }, 3, &status_link, triangle);
    if (status_link != SUCCESS)
    {
        logger(ERROR, "create_triangle", "couldn't create triangle");
        *status = ERROR;
        return;
    }
    *status = SUCCESS;
    logger(SUCCESS, "create triangle", "triangle created");
    return triangle;
}

//void create_rectangle(Vertices* vertices, Vec3 topleft, Vec3 topright, Vec3 bottomright, Vec3 bottomleft, int* status)
//{
//    add_vertex(vertices, topleft);
//    add_vertex(vertices, topright);
//    add_vertex(vertices, bottomleft);
//    add_vertex(vertices, bottomright);
//
//    CALL_STATUS status_link;
//
//    link_vertices(vertices, (Vec3[]) { topleft, topright, bottomleft }, 3, & status_link);
//    link_vertices(vertices, (Vec3[]) { bottomleft, topright, bottomright }, 3, & status_link);
//    if (status_link != SUCCESS)
//    {
//        logger(ERROR, "create_rectangle", "couldn't create rectangle");
//        *status = ERROR;
//        return;
//    }
//    *status = SUCCESS;
//    logger(SUCCESS, "create rectangle", "rectangle created");
//}

void set_object_color(Object *obj, Vec3 color)
{
    int count = obj->color_points_count;

    for (int i = 0; i < count - 2; i += 3)
    {
        *obj->color_points[i] = color.x;
        *obj->color_points[i+1] = color.y;
        *obj->color_points[i+2] = color.z;
    }
}

void load_primitives_in_gpu(Vertices *vertices)
{
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);


    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices->vertices_count, vertices->vertices, GL_STATIC_DRAW);

    /*set and enable vertex attributes*/
    for (int i = 0, offset = 0; i < vertices->attributes_count; i++, offset+=3)
    {
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(sizeof(float) * offset));
        glEnableVertexAttribArray(i);
    }

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * vertices->indices_count, vertices->indices, GL_STATIC_DRAW);
} 

/*end primitives*/
void handle_inputs(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

void draw(Vertices *vertices)
{
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, vertices->indices_count , GL_UNSIGNED_INT, 0);
}

void run(GLFWwindow* window, Vertices *vertices, int shaderprogram)
{
      while (!glfwWindowShouldClose(window))
      {
            handle_inputs(window);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            use_program(shaderprogram);
            draw(vertices);

            glfwSwapBuffers(window);
            glfwPollEvents();
      }
}

int main()
{
    GLFWwindow* window = init();
    if (!window)
    {
        logger(ERROR, "init", "couldnt initialize opengl");
        return -1;
    }
    /*set and load shaders in memory*/
    Shader vert_shader, frag_shader; 

    unsigned int basic_shader_program;

    CALL_STATUS vert_status, frag_status, prog_status;

    create_shader("basic.vert", GL_VERTEX_SHADER, &vert_shader, &vert_status);
    if (vert_status != SUCCESS)
    {
        logger(ERROR, "create_shader", "couldnt create vert shader");
        return -1;
    }
    create_shader("basic.frag", GL_FRAGMENT_SHADER, &frag_shader, &frag_status);
    if (frag_status != SUCCESS)
    {
        logger(ERROR, "create_shader", "couldnt create frag shader");
        return -1;
    }

    create_shader_program(&basic_shader_program, vert_shader, frag_shader, &prog_status);

    if (prog_status != SUCCESS)
    {
        logger(ERROR, "create_program", "couldnt create shader program");
        return -1;
    }

    /*set and laod objects in memory*/
    Vertices vertices;
    vertices.vertices_count = 0;
    vertices.indices_count = 0;
    vertices.attributes_count = 2;
    CALL_STATUS status_triangle;
    Object* triangle = create_triangle(&vertices, (Vec3) { 0.0f, 0.5f, 0.0f }, (Vec3) { -0.5f, 0.0f, 0.0f }, (Vec3) { 0.5f, 0.0f, 0.0f }, & status_triangle);
    Object* triangle2 = create_triangle(&vertices, (Vec3) { 0.6f, 0.5f, 0.0f }, (Vec3) { 0.6f, 0.0f, 0.0f }, (Vec3) { 0.9f, 0.0f, 0.0f }, & status_triangle);
    //create_rectangle(&vertices, (Vec3) { -0.5f, 0.5f, 0.0f }, (Vec3) { 0.5f, 0.5f, 0.0f }, (Vec3) { 0.5f, -0.5f, 0.0f }, (Vec3) { -0.5f, -0.5f, 0.0f }, & status_triangle);
    //create_rectangle(&vertices, (Vec3) { 0.8f, 0.5f, 0.0f }, (Vec3) { 0.9f, 0.5f, 0.0f }, (Vec3) { 0.9f, -0.5f, 0.0f }, (Vec3) { 0.8f, -0.5f, 0.0f }, & status_triangle);
    if (status_triangle != SUCCESS)
    {
        return -1;
    }
    set_object_color(triangle, (Vec3) { 1.0f, 0.0f, 0.0f });
    set_object_color(triangle2, (Vec3) { 0.0f, 1.0f, 0.0f });

    load_primitives_in_gpu(&vertices);

    run(window, &vertices, basic_shader_program);

    release_window();
    glDeleteShader(vert_shader.id);
    glDeleteShader(frag_shader.id);
    glDeleteProgram(basic_shader_program);
    free(triangle);
    return 0;
}
