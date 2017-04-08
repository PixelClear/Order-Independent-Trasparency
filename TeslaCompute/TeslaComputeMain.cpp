#include<iostream>
#include<string>
#include<vector>
#include<fstream>
#include <GL/glew.h>
#include<GL/freeglut.h>
#include<glm/glm.hpp>
#include<glm/ext.hpp>
#include "TeslaCompute.h"

#define MAX_FRAMEBUFFER_WIDTH 2048
#define MAX_FRAMEBUFFER_HEIGHT 2048

using std::cout;
using std::string;
using std::ifstream;
using std::ofstream;

/*
Data to render quad
*/

GLuint IBO;
GLuint quad_render_vbo;
GLuint quad_render_vao;
GLuint resolve_render_obj_vs;
GLuint resolve_render_obj_fs;
GLuint resolve_render_prog;

string resolve_vertex_shader_source;
string resolve_fragment_shader_source;

GLfloat quad_vertexData[] = {
	1.0f,  -1.0f,  0.0f,
	1.0f, 1.0f, 0.0f ,
	-1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f
}; // 4 ve

GLfloat quad_texData[] = {
	1.0f,  0.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	0.0f, 1.0f
}; // 

static const GLushort quad_vertex_indices[] =
{
	0,1,2,2,3,1
};


/*General application related stuff*/
glm::mat4 projection;
glm::mat4 view;
glm::mat4 model;
float cameraZoom = 5.0f;
float cameraX = 0.0f;


GLfloat eye_world_pos [] = {0.0,0.0,0.0};
GLfloat light_dir [] = {125.0,125.0,0.0};
GLfloat light_color [] = {0.5,0.5,1.0};


/*OIT related stuff*/
/*Texture to contain pointers of head node of linked list*/
GLuint head_pointer_texture;
/*PBO to clear the texture of head pointers*/
GLuint head_pointer_clear_buffer;

/*Atomic counter buffer this works as allocator for linked list*/
GLuint atomic_counter_buffer;

/*This programm actually contains nodes of linked list node of linked list is just vec4*/
GLuint linked_list_buffer;
GLuint linked_list_texture;

string vertex_source;
string fragment_source;

GLuint vs_shader_object;
GLuint fs_shader_object;
GLuint render_prog;

GLuint Model_matrix_location;
GLuint View_matrix_location;
GLuint Projection_matrix_location;
GLuint alpha_factor_location;

GLuint VAO;
GLuint VBO[1];

int window_height;
int window_width;

unsigned int* data;

/*Instance data*/
GLuint MVP_texture;
GLuint MVP_texture_buffer;

#define INSTANCE_COUNT 5

bool ReadFile(const char* pFileName, string& outFile)
{
    ifstream f(pFileName);
    
    bool ret = false;
    
    if (f.is_open()) {
        string line;
        while (getline(f, line)) {
            outFile.append(line);
            outFile.append("\n");
        }
        
        f.close();
        
        ret = true;
    }
    else {
       cout << "Error Loading file";
    }
    
    return ret;
}

void InitInstanceData()
{
	glGenBuffers(1,&MVP_texture_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, MVP_texture_buffer);
	glBufferData(GL_TEXTURE_BUFFER, INSTANCE_COUNT * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	
	glGenTextures(1, &MVP_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, MVP_texture);
	glTexBuffer(GL_TEXTURE_BUFFER,GL_RGBA32F, MVP_texture_buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void PrepareOITTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &head_pointer_texture);
	glBindTexture(GL_TEXTURE_2D, head_pointer_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, MAX_FRAMEBUFFER_WIDTH, MAX_FRAMEBUFFER_HEIGHT,0, GL_RED_INTEGER, GL_UNSIGNED_INT,NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Bind texture to image unit and not sampler
	glBindImageTexture(0, head_pointer_texture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

	//Create Buffer for clearing head pointer structure
	glGenBuffers(1, &head_pointer_clear_buffer);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, head_pointer_clear_buffer);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, MAX_FRAMEBUFFER_WIDTH*MAX_FRAMEBUFFER_HEIGHT*sizeof(GLuint),NULL,GL_STATIC_DRAW);
	data = (GLuint*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	memset(data,0x00, MAX_FRAMEBUFFER_WIDTH*MAX_FRAMEBUFFER_HEIGHT*sizeof(GLuint));
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	//Create Atomic Counter Buffer
	glGenBuffers(1,&atomic_counter_buffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomic_counter_buffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

	//Create linked list storage buffer
	glGenBuffers(1,&linked_list_buffer);
	glBindBuffer(GL_TEXTURE_BUFFER, linked_list_buffer);
	glBufferData(GL_TEXTURE_BUFFER,MAX_FRAMEBUFFER_WIDTH*MAX_FRAMEBUFFER_HEIGHT*3*sizeof(glm::vec4),NULL,GL_DYNAMIC_COPY);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	//Bind this buffer to texture 
	glGenTextures(1, &linked_list_texture);
	glBindTexture(GL_TEXTURE_BUFFER, linked_list_texture);
	glTexBuffer(GL_TEXTURE_BUFFER,GL_RGBA32UI,linked_list_buffer);
	glBindTexture(GL_TEXTURE_BUFFER, 0);

	glBindImageTexture(1, linked_list_texture , 0 , GL_FALSE, 0, GL_WRITE_ONLY,GL_RGBA32UI);
	
}

void LoadModel(void) {

    
    std::string inputfile = "model.obj";
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    printf("Loading model: %s", inputfile.c_str() );
    bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile.c_str());

    if (!err.empty()) {
	printf("%s\n", err.c_str() );
    }
    if (!ret) {
	exit(1);
    }

    mesh.vertices = shapes[0].mesh.positions;
    mesh.faces = shapes[0].mesh.indices;
    mesh.normals = shapes[0].mesh.normals;
	    
	glGenVertexArrays(1,&VAO);
	glBindVertexArray(VAO);

    (glGenBuffers(1, &mesh.indexVbo));
    (glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexVbo));
    (glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* mesh.faces.size(), mesh.faces.data(), GL_STATIC_DRAW));

    (glGenBuffers(1, &mesh.vertexVbo));
    (glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexVbo));
    (glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*mesh.vertices.size(), mesh.vertices.data() , GL_STATIC_DRAW));


    (glGenBuffers(1, &mesh.normalVbo));
    (glBindBuffer(GL_ARRAY_BUFFER, mesh.normalVbo));
    (glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*mesh.normals.size(), mesh.normals.data() , GL_STATIC_DRAW));

    (glEnableVertexAttribArray(0));
    (glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexVbo));
    (glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));

    (glEnableVertexAttribArray(1));
    (glBindBuffer(GL_ARRAY_BUFFER, mesh.normalVbo));
    (glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0));
	
}


bool InitResolvePassShaders()
{
	int success;

	ReadFile("resolve_linkedlist_shader.vs",resolve_vertex_shader_source);
	ReadFile("resolve_linkedlist_shader.fs",resolve_fragment_shader_source);

	const char* p[1];
	p[0] = resolve_vertex_shader_source.c_str();
	
	resolve_render_obj_vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(resolve_render_obj_vs,1,p,NULL);
	glCompileShader(resolve_render_obj_vs);
	glGetShaderiv(resolve_render_obj_vs, GL_COMPILE_STATUS, &success);
	
    if (!success) {
        char InfoLog[1024];
        glGetShaderInfoLog(resolve_render_obj_vs, 1024, NULL, InfoLog);
        std::cout <<"Error compiling : '%s'\n" <<InfoLog;
        return false;
    }

	const char* p1[1];
	p1[0] = resolve_fragment_shader_source.c_str();
	
	resolve_render_obj_fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(resolve_render_obj_fs,1,p1,NULL);
	glCompileShader(resolve_render_obj_fs);
	glGetShaderiv(resolve_render_obj_fs, GL_COMPILE_STATUS, &success);
	
    if (!success) {
        char InfoLog[1024];
        glGetShaderInfoLog(resolve_render_obj_fs, 1024, NULL, InfoLog);
        std::cout <<"Error compiling : '%s'\n" <<InfoLog;
        return false;
    }


	resolve_render_prog = glCreateProgram();
	glAttachShader(resolve_render_prog,resolve_render_obj_vs);
	glAttachShader(resolve_render_prog,resolve_render_obj_fs);

	glLinkProgram(resolve_render_prog);
	glGetProgramiv(resolve_render_prog, GL_LINK_STATUS, &success);
	
    if (!success) {
        char InfoLog[1024];
        glGetProgramInfoLog(resolve_render_prog, 1024, NULL, InfoLog);
        std::cout <<"Error Linking: '%s'\n" <<InfoLog;
        return false;
    }

	glValidateProgram(resolve_render_prog);
	glGetProgramiv(resolve_render_prog, GL_VALIDATE_STATUS, &success);
    if (!success) {
		char InfoLog[1024];
		glGetProgramInfoLog(resolve_render_prog, 1024, NULL, InfoLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", InfoLog);
     return false;
    }
	
	glDetachShader(resolve_render_prog, resolve_render_obj_vs);
	glDetachShader(resolve_render_prog, resolve_render_obj_fs);
	return true;
}

void PrepareQuadVBO()
{
	glGenVertexArrays(1, &quad_render_vao);
	glBindVertexArray(quad_render_vao);
	
	glGenBuffers(1,&IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_vertex_indices), quad_vertex_indices,GL_STATIC_DRAW);

	glGenBuffers(1, &quad_render_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_render_vbo);
	glBufferData(GL_ARRAY_BUFFER,sizeof(quad_vertexData), quad_vertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}

/*We are going to build and resolve the */
void BuildLinkedList()
{
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	/*As we are building linkedlist per frame we need to reset counter to 0*/
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomic_counter_buffer);
	data = (GLuint *) glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	data[0] = 0;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

	/*Same we need to clear head pointers texture*/
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER,head_pointer_clear_buffer);
	glBindTexture(GL_TEXTURE_2D, head_pointer_texture);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,window_width,window_height, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glBindTexture(GL_TEXTURE_2D,0);
	
	/*Bind image for head pointer read and write*/
	glBindImageTexture(0, head_pointer_texture,0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	/*Bind linked list buffer for read- write*/
	glBindImageTexture(1,linked_list_texture, 0 , GL_FALSE, 0 , GL_WRITE_ONLY, GL_RGBA32UI);

	glUseProgram(render_prog);

	glBindVertexArray(VAO);
	
	glm::mat4 model[10];
	view = glm::lookAt(glm::vec3(0.0,0.0,0.0),glm::vec3(0.0,0.0,-5.0),glm::vec3(0.0,1.0,0.0));
	int j = 0;
	for(int i = 0 ; i <INSTANCE_COUNT; i++)
	{
		
		model[i] = glm::translate(glm::mat4(1.0),glm::vec3(j-1.0,-1.5,-cameraZoom-j));
		model[i] = glm::rotate(model[i],cameraX,glm::vec3(0.0,1.0,0.0));
		j++;
	}

	j=0;
	for(int i = 5 ; i <INSTANCE_COUNT + 5; i++)
	{
		model[i] = glm::translate(glm::mat4(1.0),glm::vec3(j-1.0,1.5,-cameraZoom-j));
		model[i] = glm::rotate(model[i],cameraX,glm::vec3(0.0,1.0,0.0));
		j++;
	}
		
	glActiveTexture(GL_TEXTURE0);
	glBindBuffer(GL_TEXTURE_BUFFER,MVP_texture_buffer);
	glBufferData(GL_TEXTURE_BUFFER,sizeof(model),model,GL_DYNAMIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	
	glUniformMatrix4fv(View_matrix_location, 1, GL_FALSE , &view[0][0]);
	glUniformMatrix4fv(Projection_matrix_location, 1, GL_FALSE, &projection[0][0]);
	glUniform1f(alpha_factor_location, 0.1);
    
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDrawElementsInstanced(GL_TRIANGLES,mesh.faces.size(),GL_UNSIGNED_INT,0,INSTANCE_COUNT*2);
	glDisable(GL_BLEND);

	glBindVertexArray(0);
	glUseProgram(0);
}

void ResolveLinkedList()
{
	/*Bind image for head pointer read and write*/
	glBindImageTexture(0, head_pointer_texture,0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

	/*Bind linked list buffer for read- write*/
	glBindImageTexture(1,linked_list_texture, 0 , GL_FALSE, 0 , GL_WRITE_ONLY, GL_RGBA32UI);

	glUseProgram(resolve_render_prog);
	glBindVertexArray(quad_render_vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}




bool InitLinkedListShaders()
{
	int success;

	ReadFile("Build_list.vs",vertex_source);
	ReadFile("Build_list.fs",fragment_source);

	const char* p[1];
	p[0] = vertex_source.c_str();

	vs_shader_object = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs_shader_object,1,p,NULL);
	glCompileShader(vs_shader_object);
	glGetShaderiv(vs_shader_object, GL_COMPILE_STATUS, &success);
	
    if (!success) {
        char InfoLog[1024];
        glGetShaderInfoLog(vs_shader_object, 1024, NULL, InfoLog);
        std::cout <<"Error compiling : '%s'\n" <<InfoLog;
        return false;
    }

	const char* p1[1];
	p1[0] = fragment_source.c_str();

	fs_shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs_shader_object,1,p1,NULL);
	glCompileShader(fs_shader_object);
	glGetShaderiv(fs_shader_object, GL_COMPILE_STATUS, &success);
	
    if (!success) {
        char InfoLog[1024];
        glGetShaderInfoLog(fs_shader_object, 1024, NULL, InfoLog);
        std::cout <<"Error compiling : '%s'\n" <<InfoLog;
        return false;
    }


	render_prog = glCreateProgram();
	glAttachShader(render_prog,vs_shader_object);
	glAttachShader(render_prog,fs_shader_object);

	glLinkProgram(render_prog);
	glGetProgramiv(render_prog, GL_LINK_STATUS, &success);
	
    if (!success) {
        char InfoLog[1024];
        glGetProgramInfoLog(render_prog, 1024, NULL, InfoLog);
        std::cout <<"Error Linking: '%s'\n" <<InfoLog;
        return false;
    }

	glValidateProgram(render_prog);
	glGetProgramiv(render_prog, GL_VALIDATE_STATUS, &success);
    if (!success) {
		char InfoLog[1024];
		glGetProgramInfoLog(render_prog, 1024, NULL, InfoLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", InfoLog);
     return false;
    }

	Model_matrix_location = glGetUniformLocation(render_prog, "Model_matrix");
	View_matrix_location = glGetUniformLocation(render_prog, "View_matrix");
	Projection_matrix_location = glGetUniformLocation(render_prog, "Projection_matrix");
	alpha_factor_location = glGetUniformLocation(render_prog, "alpha_factor");
	glUniform1i(Model_matrix_location , 0);

	return true;
}

void Key(unsigned char key, int x, int y)
{
	switch(key)
	{
	case 'z' :
		cameraZoom -= 0.1;
		break;
	case 'a':
		cameraZoom +=0.1;
		break;
	case 'r':
		cameraX +=0.1;
		break;

	case 'w':
		cameraX -= 0.1;
	}

}

void Display()
{
	BuildLinkedList();
	ResolveLinkedList();
	glutSwapBuffers();
	glutPostRedisplay();
}

void ChangeSize(int w, int h)
{
	glClearColor(0.0,0.0,0.0,1.0);
	glViewport(0,0,w,h);
	projection = glm::perspective(45.0f,w/(float)h,0.1f, 100.0f);
	window_width = w;
	window_height = h;
}

void Clear()
{
	glDeleteVertexArrays(1,&VAO);
	glDeleteBuffers(1,VBO);
	glDeleteBuffers(1,&IBO);
	glDeleteShader(vs_shader_object);
	glDeleteShader(fs_shader_object);
	glDeleteProgram(render_prog);
	glDeleteBuffers(1,&linked_list_buffer);
	glDeleteTextures(1, &linked_list_texture);
	glDeleteBuffers(1,&atomic_counter_buffer);
	glDeleteTextures(1,&head_pointer_texture);
	glDeleteBuffers(1, &head_pointer_clear_buffer);
	glDeleteBuffers(1,&quad_render_vbo);
	glDeleteVertexArrays(1,&quad_render_vao);
	glDeleteShader(resolve_render_obj_vs);
	glDeleteShader(resolve_render_obj_fs);
	glDeleteProgram(resolve_render_prog);
	glDeleteTextures(1,&MVP_texture);
	glDeleteBuffers(1,&MVP_texture_buffer);
}

int main(int argc, char* argv[])
{
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH|GLUT_DOUBLE);
	window_width = 1350;
	window_height = 700;
	glutInitWindowPosition(0,0);
	glutInitWindowSize(window_width,window_height);
	glutCreateWindow("TeslaTessellate");

	GLenum res = glewInit();
    if (res != GLEW_OK) {
       std::cout<<"Error: '%s'\n"<<glewGetErrorString(res);
        return false;
    }

	
	glutDisplayFunc(Display);
	glutKeyboardFunc(Key);
	glutReshapeFunc(ChangeSize);
	
	if(!InitLinkedListShaders())
	{
		std::cout<<"Error in build list shaders"<<std::endl;
		return -1;
	}
#ifdef _DEBUG
	std::cout<<"\nError: '%s'\n"<<glewGetErrorString(glGetError());
#endif
    if(!InitResolvePassShaders())
	{
		std::cout<<"Error in build list shaders"<<std::endl;
		return -1;
	}
#ifdef _DEBUG
	std::cout<<"\nError: '%s'\n"<<glewGetErrorString(glGetError());
#endif
	InitInstanceData();
#ifdef _DEBUG	
	std::cout<<"\nError: '%s'\n"<<glewGetErrorString(glGetError());
#endif
	PrepareOITTextures();
#ifdef _DEBUG	
	std::cout<<"\nError: '%s'\n"<<glewGetErrorString(glGetError());
#endif
	LoadModel();
#ifdef _DEBUG
	std::cout<<"\nError: '%s'\n"<<glewGetErrorString(glGetError());
#endif
	PrepareQuadVBO();
#ifdef _DEBUG
	std::cout<<"\nError: '%s'\n"<<glewGetErrorString(glGetError());
#endif		
	glutMainLoop();

	Clear();
	return 0;
}

