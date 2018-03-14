#define GLEW_STATIC
#include "glew\include\GL\glew.h"
#include "glfw\glfw3.h"

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

GLFWwindow* window;

// is 8 enough? on average 10 or what ever
const int num_probes_per_rec = 8;


struct ReceiverData {
	vec3 position;
	vec3 normal;

	struct Probe {
		float weight;
		vec3 position;
		float sh_coeffs[16];
	} visible_probes[num_probes_per_rec];
	int num_visible_probes;
};


typedef Eigen::Matrix4f mat4;
mat4 projection(float near, float far, float aspect) {
	mat4 ret;
	// obvious syntax is obvious
	ret <<
		aspect, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, -(far + near) / (far - near), -2 * near*far / (far - near),
		0, 0, -1, 0;

	float q = ret(3, 2);

	return ret;
}

mat4 translate(vec3 translation) {
	mat4 ret;
	ret <<
		1, 0, 0, translation.x(),
		0, 1, 0, translation.y(),
		0, 0, 1, translation.z(),
		0, 0, 0, 1;
	return ret;
}
mat4 look_at(vec3 eye, vec3 center, vec3 up) {

	vec3 f = (center - eye).normalized();
	vec3 u = up.normalized();
	vec3 s = f.cross(u).normalized();
	u = s.cross(f);

	mat4 res;
	res << s.x(), s.y(), s.z(), -s.dot(eye),
		u.x(), u.y(), u.z(), -u.dot(eye),
		-f.x(), -f.y(), -f.z(), f.dot(eye),
		0, 0, 0, 1;
	return res;
}
#include <fstream>;

GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	} else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}
#include <string>
#include <iostream>


void _check_gl_error(const char *file, int line) {
	GLenum err(glGetError());

	while (err != GL_NO_ERROR) {
		std::string error;

		switch (err) {
		case GL_INVALID_OPERATION:      error = "INVALID_OPERATION";      break;
		case GL_INVALID_ENUM:           error = "INVALID_ENUM";           break;
		case GL_INVALID_VALUE:          error = "INVALID_VALUE";          break;
		case GL_OUT_OF_MEMORY:          error = "OUT_OF_MEMORY";          break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:  error = "INVALID_FRAMEBUFFER_OPERATION";  break;
		}

		std::cerr << "GL_" << error.c_str() << " - " << file << ":" << line << std::endl;
		err = glGetError();
	}
}

#define check_gl_error() _check_gl_error(__FILE__,__LINE__)


struct 	DepthCubeMap {
	GLuint cube_map;
};


struct 	SHTextures {
	GLuint textures[4];
};


const int cube_map_size = 512;
DepthCubeMap gen_depth_cubemap() {

	GLuint cube_map;
	glGenTextures(1, &cube_map);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R16F,
			cube_map_size, cube_map_size, 0, GL_RED, GL_FLOAT, NULL);
	}
	DepthCubeMap ret;
	ret.cube_map = cube_map;
	return ret;
}

const int sh_samples = 64;
SHTextures gen_sh_textures() {
	SHTextures ret;
	glGenTextures(4, ret.textures);
	for (int i = 0; i < 4; i++) {
		glBindTexture(GL_TEXTURE_2D, ret.textures[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
			sh_samples, sh_samples, 0, GL_RGBA, GL_FLOAT, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	return ret;
}




vec3 cube_map_dir[6] = {
	vec3(1.0f, 0.0f, 0.0f),
	vec3(-1.0f, 0.0f, 0.0f),
	vec3(0.0f, 1.0f, 0.0f),
	vec3(0.0f, -1.0f, 0.0f),
	vec3(0.0f, 0.0f, 1.0f),
	vec3(0.0f, 0.0f, -1.0f)
};

vec3 cube_map_up[6] = {
	vec3(0.0f, -1.0f, 0.0f),
	vec3(0.0f, -1.0f, 0.0f),
	vec3(0.0f, 0.0f, 1.0f),
	vec3(0.0f, 0.0f, -1.0f),
	vec3(0.0f, -1.0f, 0.0f),
	vec3(0.0f, -1.0f, 0.0f)
};

// assumes that the currently bound vao is the entire scene to be rendered.
std::vector<DepthCubeMap> render_probe_depth(int num_indices, std::vector<vec3>probes) {
	GLuint shadow_map_program = LoadShaders("shadow_map.vert", "shadow_map.frag");
	glUseProgram(shadow_map_program);

	//hmm we might want to calculate this more accuratly
	//it would improve our accuracy.
	// oh we have the max radius though. that should be our far plane.
	// near should be voxel_size * 0.5, right? Interesting, being to close to a surface would reduce accuracy (for points further away)
	mat4 proj = projection(0.1, 100.0, 1);

	GLuint matrix_uniform_location = glGetUniformLocation(shadow_map_program, "matrix");
	GLuint light_uniform_location = glGetUniformLocation(shadow_map_program, "light_pos");

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create the depth buffer
	GLuint depth_buffer;
	glGenRenderbuffers(1, &depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 1024);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	glViewport(0, 0, cube_map_size, cube_map_size);

	std::vector<DepthCubeMap> depth_maps;
	for (vec3 probe_pos : probes) {
		DepthCubeMap depth = gen_depth_cubemap();


		for (int i = 0; i < 6; i++) {
			mat4 mat = proj * look_at(probe_pos, probe_pos + cube_map_dir[i], cube_map_up[i]);
			glUniformMatrix4fv(matrix_uniform_location, 1, false, mat.data());
			glUniform3fv(light_uniform_location, 1, probe_pos.data());
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, depth.cube_map, 0);

			{// check for errors
				check_gl_error();
				auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE) {
					auto a = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
					auto b = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
					auto c = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER;
					auto d = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER;
					printf("framebuffer incomplete..");
					__debugbreak();
				}
			}

			glClearColor(0, 0.5, 0.5, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void *)0);
		}
		check_gl_error();
		depth_maps.push_back(depth);
	}

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &depth_buffer);
	return depth_maps;
}
#include <chrono>
// assumes that the currently bound vao is the entire scene to be rendered.
void render_receivers(int num_indices, std::vector<ReceiverData*> receivers, std::vector<DepthCubeMap> depth_maps) {

	GLuint shadow_map_program = LoadShaders("shader.vert", "local_transport.frag");
	glUseProgram(shadow_map_program);

	//hmm we might want to calculate this more accuratly
	//it would improve our accuracy.
	// oh we have the max radius though. that should be our far plane.
	// near should be voxel_size * 0.5, right? Interesting, being to close to a surface would reduce accuracy (for points further away)
	mat4 proj = projection(0.1, 100.0, 1);

	GLuint matrix_uniform_location = glGetUniformLocation(shadow_map_program, "matrix");
	GLuint probe_depth_uniform_location = glGetUniformLocation(shadow_map_program, "probe_depth");
	GLuint probe_pos_uniform_location = glGetUniformLocation(shadow_map_program, "probe_pos");

	GLuint receiver_normal_uniform_location = glGetUniformLocation(shadow_map_program, "receiver_normal");
	GLuint receiver_pos_uniform_location = glGetUniformLocation(shadow_map_program, "receiver_pos");



	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create the depth buffer
	GLuint depth_buffer;
	glGenRenderbuffers(1, &depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1024, 1024);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	glViewport(0, 0, sh_samples, sh_samples);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE); // just add them togehter, we need to do this when we reduce anyway.

	auto start_time = std::chrono::high_resolution_clock::now();
	SHTextures shs = gen_sh_textures();
	for (ReceiverData *receiver : receivers) {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (int probe_index = 0; probe_index < receiver->num_visible_probes; probe_index++) {
			vec3 probe_pos = receiver->visible_probes[probe_index].position;
			for (int cube_map_index = 0; cube_map_index < 6; cube_map_index++) {

				mat4 mat = proj * look_at(receiver->position, receiver->position + cube_map_dir[cube_map_index], cube_map_up[cube_map_index]);
				glUniformMatrix4fv(matrix_uniform_location, 1, false, mat.data());
				glUniform3fv(receiver_pos_uniform_location, 1, receiver->position.data());
				glUniform3fv(receiver_normal_uniform_location, 1, receiver->normal.data());
				glUniform3fv(probe_pos_uniform_location, 1, probe_pos.data());



				// PERF: cross iteration dep, should possibly unroll
				// PERF: uses only half of the color attatchments
				//       either use twice as many sh-coeffs (maps badly, 36 > 32 -> only sh5)
				//       or render two probes at the same time should be fine.
				

				// PERF: we can totally render all probes in the same pass
				// 
				// increase viewport size
				// render interlaced, ie probe_index = ((x&3)|((y&3)<<2)) gives 16 probes
				// where we samlpe is arbitrary, we're not even uniform at this point
				//
				// Extract in custom ~mipmapping stage
				for (int i = 0; i < 4; i++){
					glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, shs.textures[i], 0);
				}

				GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3 };
				glDrawBuffers(4, DrawBuffers); // is this neccessary??
				#if 0
				{// check for errors
					check_gl_error();
					auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
					if (status != GL_FRAMEBUFFER_COMPLETE) {
						auto a = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
						auto b = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
						auto c = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER;
						auto d = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER;
						printf("framebuffer incomplete..");
						__debugbreak();
					}
				}
				#endif			
				glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void *)0);
			}

			// Reduction:
			// @Robustness
			// the filtering for mipmapping is not specified... so this really is not right.
			// but in practice I *guess* that it works... heh 
			// we should possibly either do this with a few passes with a pixel shader or compute shader 
			// but for now meh.
			// also hint which filtering... however is box fast or good? idk, probably fast? or is it precision?
			// glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST/GL_FASTEST), 

			// PERF: glgetteximage slow as fuck?
			// PBO
			// compute shader?

			for (int i = 0; i < 4; i++) {
				glBindTexture(GL_TEXTURE_2D, shs.textures[i]);
				glGenerateMipmap(GL_TEXTURE_2D);
			}

			for (int i = 0; i < 4; i++) {
				glBindTexture(GL_TEXTURE_2D, shs.textures[i]);
				glGetTexImage(GL_TEXTURE_2D, 6, GL_RGBA, GL_FLOAT, &receiver->visible_probes[probe_index].sh_coeffs[i * 4]);
			}
		}

		static int num_done = 0;

		auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
		auto approx = elapsed / (float)(++num_done) * receivers.size();
		std::cout << "elapsed "<<std::chrono::duration_cast<std::chrono::minutes>(elapsed).count() << " minutes out of ~"<<std::chrono::duration_cast<std::chrono::minutes>(approx).count() <<" minutes..." << std::endl;

		printf("recs done: %d\n", num_done);
	}

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &depth_buffer);
}

void load_mesh(Mesh *mesh) {
	GLuint vao, vertex_buffer, index_buffer, normal_buffer;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vertex_buffer);
	glGenBuffers(1, &normal_buffer);
	glGenBuffers(1, &index_buffer);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, mesh->num_verts * sizeof(vec3), mesh->verts, GL_STATIC_DRAW);


	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, mesh->num_verts * sizeof(vec3), mesh->normals, GL_STATIC_DRAW);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_indices * sizeof(unsigned int), mesh->indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
}

bool init_gl() {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return false;
	}

	glfwWindowHint(GLFW_SAMPLES, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(1024, 768, "pre-compute!", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window.");
		getchar();
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return false;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	return true;
}

#include <queue>

int visibility(std::vector<Receiver> recs, std::vector<vec3> probe_locations, Mesh *mesh, float radius) {

	if (!init_gl()) {
		printf("error while initializing opengl");
		return -1;
	}

	load_mesh(mesh);

#if 0 // to visualize shadow map at probe_loc
	vec3 probe_loc = vec3(0, 0.5, 0);
	probe_locations.clear();
	probe_locations.push_back(probe_loc);
#endif

	printf("generating depth information for probes...");

	auto depth_maps = render_probe_depth(mesh->num_indices, probe_locations);
	DepthCubeMap point_light_shadows = depth_maps[0];

	float inv_radius = 1.0 / radius;
	std::vector<ReceiverData *> receivers;
	receivers.reserve(recs.size());

	printf("finding closest probes for each receivers...");

	for (Receiver receiver : recs) {
		ReceiverData *rec_data = (ReceiverData *)calloc(1, sizeof(ReceiverData));
		rec_data->normal = receiver.norm;
		rec_data->position = receiver.pos;
		std::priority_queue <vec3, std::vector<vec3>, std::greater<int>> prio_queue;

		// jesus christ do I love c++ templates 
		std::priority_queue<vec3, std::vector<vec3>,
			std::function<bool(vec3, vec3)> >
			closest_probes([receiver](vec3 a, vec3 b) -> bool {
			vec3 da = a - receiver.pos;
			vec3 db = b - receiver.pos;
			return (da.dot(da) < db.dot(db));
		});

		for (int i = 0; i < min(num_probes_per_rec, probe_locations.size()); i++) {
			closest_probes.push(probe_locations[i]);
		}

		for (int i = num_probes_per_rec; i < probe_locations.size(); i++) {
			closest_probes.push(probe_locations[i]);
			closest_probes.pop();
		}

		while (!closest_probes.empty()) {
			vec3 probe = closest_probes.top();
			closest_probes.pop();
			float dist = (probe - rec_data->position).norm();
			rec_data->visible_probes[rec_data->num_visible_probes].position = probe;
			rec_data->visible_probes[rec_data->num_visible_probes].weight = w(inv_radius*dist);
			++rec_data->num_visible_probes;
		}
		receivers.push_back(rec_data);
	}

	GLint maxAttach = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);

	printf("calculating spherical harmonics etc...");

	render_receivers(mesh->num_indices, receivers, depth_maps);





#if 0
	// to visualize shadow map at probe_loc
	GLuint program = LoadShaders("shader.vert", "shader.frag");
	check_gl_error();
	glUseProgram(program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	vec3 camera_pos = vec3(0, 6.0, 0.01);
	glViewport(0, 0, 1024, 768);
	do {
		float camera_speed = 0.05;
		// trying to break tree.js's record in worst camera 
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			camera_pos.x() -= camera_speed;
		}if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			camera_pos.x() += camera_speed;
		}if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			camera_pos.z() -= camera_speed;
		}if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			camera_pos.z() += camera_speed;
		}if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
			camera_pos.y() -= camera_speed;
		}if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			camera_pos.y() += camera_speed;
		}


		mat4 view = look_at(camera_pos, vec3(0, 0, 0), vec3(0, 1, 0));


		mat4 proj = projection(0.1, 100.0, 768 / 1024.0);
		mat4 mat = proj * view;


		static GLuint matrix_location = glGetUniformLocation(program, "matrix");
		static GLuint light_location = glGetUniformLocation(program, "light_pos");

		check_gl_error();
		glUniformMatrix4fv(matrix_location, 1, false, mat.data());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, point_light_shadows.cube_map);
		check_gl_error();

		glUniform3fv(light_location, 1, probe_loc.data());

		glClearColor(0, 0.5, 0.5, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, mesh->num_indices, GL_UNSIGNED_INT, (void *)0);

		check_gl_error();
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);
#endif

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}


// NOTES: Shadowmapping
// We would like the shadow map to be conservative
// ie we would prefere errors where actualy lit points are evaluated as in shadow 
// than the other way around.
// ie we'd rather exclude a probe from contributing than add wrong samples.
// I've not yet investigted if there's a good way to do this.
// pcf + ceil maybe? or maybe even variance shadow maps and ceil.

// NOTE: Cube Shadow Maps
// Obviously we need 360 shadows from the probes
// this is usually done with cube maps.
// which seem to work well. But if we get perf problems
// we could also consider doing paraboiloid shadow maps
// with tesselation during shadow creation
// (not neccessary during evaluation which will be our hot spot by a lot)

// Cube map output
//
// We also need hemisphere output of this whole thing 
// currently the plan is to do cube maps here as well 
// I don't think that using a paraboloid mapping would be beneficial 
// since we'd need to tesselate during receiver coeffs generation, and we're probably vertex bound anyway

// However the sampling on the cube map is not optimal: 
// 
// Optimally we'd like to sample approximatly accordning to cos(phi) 
// where phi is the angle to the surface normal
// ie sample more close to the normal and more sparsely further away
// since we know that we have a cos(phi) in the lightning equation anyway. (imporance sampling)
// What we actually do is sample uniformly on the projection plane (near plane, far plane, whatever plane)
// so to get d/dx a (where x is normalized so that bottom of triangle is of length 1) 

// 
//      /     |  
//     /      | 
//    / a     |  
//   ---------| x
//        1

// which is equal to d/dx arctan(x) = 1/(x^2+1)

// ie:
// let u = gl_FragCoord.xy*2-1;
// sphere sample area is: 1/(1+x^2) * 1/(1+y^2) where x = u.x/sqrt((u.x^2+1^2)) and y is similar
// x^2 = u.x^2/(u.x^2+1) 
// sample area = (u.x^2+1)/(2*u.x^2+1) * (u.y^2+1)/(2*u.y^2+1)
// and to divide by ~2.807 to normalize (assuming continous, 2.808 is just the integral) 
// this all assumes 90 degrees of fov
// max differnece is actually only 4/9, which isn't that bad (not counting the cos)
// though it could be way better
// so if we sampled optimally we might get away with using maybe a factor of three or four less samples
// While this is quite a lot 
// We will probably be limited by the vertex shader anyway.
// So reducing the number of texels is probably not that important
// But we could get more early out then right?
// Where we'd now probably have to tesselate or some other wierd thing.









