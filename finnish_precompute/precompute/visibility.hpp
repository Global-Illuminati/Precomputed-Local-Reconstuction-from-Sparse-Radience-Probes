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
const int num_sh_coeffs = 16;
const int sh_samples = 64;

struct 	DepthCubeMap {
	GLuint cube_map;
};

struct 	SHTextures {
	GLuint textures[8];
};

struct ReceiverData {
	vec3 position;
	vec3 normal;
	ivec2 px;

	struct Probe {
		int index;
		float weight;
		vec3 position;
		DepthCubeMap depth_cube_map;
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
#include <fstream>

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
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F,
			cube_map_size, cube_map_size, 0, GL_RGBA, GL_BYTE, NULL);
	}
	DepthCubeMap ret;
	ret.cube_map = cube_map;
	return ret;
}

// needs to be a power of two squared for mipmaping to work nicely
SHTextures gen_sh_textures() {
	SHTextures ret;
	glGenTextures(8, ret.textures);
	for (int i = 0; i < 8; i++) {
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

void check_fbo() {
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

#undef PI
#include "redsvd-h.h"


#pragma optmize("",on)
std::vector<Receiver> compute_receivers_gpu(int num_indices, GLuint *normals, GLuint *positions) {
	GLuint shader = LoadShaders("comp_recs.vert", "comp_recs.frag");
	glUseProgram(shader);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// horrible approximate conservative rasterization
	// msaa should be better,

	int samples_per_side = 4;
	int size = 1024;
	int ssize = samples_per_side * size;
	int num_pbo_bytes = ssize * ssize*(sizeof(vec3) * 2);
	check_gl_error();

	GLuint pbo;
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, num_pbo_bytes, NULL, GL_STREAM_READ);

	GLuint textures[2];
	glGenTextures(2, textures);
	for (int i = 0; i < 2; i++) {
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, ssize, ssize, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i], 0);
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	GLuint DrawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, DrawBuffers);

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	glViewport(0, 0, ssize, ssize);

	check_fbo();

	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void *)0);

	std::vector<Receiver> ret;

	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, 0);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, (void*)(ssize*ssize * sizeof(vec3)));

	vec3* rec_verts = (vec3 *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	vec3* rec_norms = &rec_verts[ssize*ssize];

	check_gl_error();
	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			vec3 pos_ack = vec3(0, 0, 0);
			vec3 norm_ack = vec3(0, 0, 0);
			float num_hit = 0.0;
			for (int dx = 0; dx < samples_per_side; dx++) {
				for (int dy = 0; dy < samples_per_side; dy++) {
					int xx = x * samples_per_side + dx;
					int yy = y * samples_per_side + dy;

					vec3 p = rec_verts[yy*ssize + xx];
					vec3 n = rec_norms[yy*ssize + xx];
					if (n != p || p != vec3(0, 0, 0)) {
						norm_ack += n;
						pos_ack += p;
						num_hit += 1.0;
					}
				}
			}
			if (num_hit != 0.0) {
				Receiver rec;
				rec.pos = pos_ack / num_hit;
				rec.norm = norm_ack.normalized();
				rec.px = ivec2(x, y);
				ret.push_back(rec);
			}
		}
	}
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	//glDeleteTextures(2, textures);
	*positions = textures[0];
	*normals = textures[1];

	glDeleteFramebuffers(1, &fbo);
	glDeleteBuffers(1, &pbo);
	glDeleteProgram(shader);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	check_gl_error();
	return ret;
}
#pragma optmize("",on)


void store_matrix(Eigen::MatrixXf mat, FILE *file) {
	int w = mat.cols();
	int h = mat.rows();

	fwrite(&w, sizeof(w), 1, file);
	fwrite(&h, sizeof(h), 1, file);

	fwrite(mat.data(), sizeof(float), mat.size(), file);
}

void store_matrixi(MatrixXi16 mat, FILE *file) {
	int w = mat.cols();
	int h = mat.rows();

	fwrite(&w, sizeof(w), 1, file);
	fwrite(&h, sizeof(h), 1, file);

	fwrite(mat.data(), sizeof(short), mat.size(), file);
}

void store_matrix(Eigen::MatrixXf mat, char *path) {
	FILE *file = fopen(path, "wb");
	store_matrix(mat, file);
	fclose(file);
}

void store_matrixi(MatrixXi16 mat, char *path) {
	FILE *file = fopen(path, "wb");
	store_matrixi(mat, file);
	fclose(file);
}

#pragma optimize("", on);

// assumes that the currently bound vao is the entire scene to be rendered.
std::vector<DepthCubeMap> render_probe_depth(int num_indices, std::vector<vec3>probes) {
	static GLuint shadow_map_program = LoadShaders("shadow_map.vert", "shadow_map.frag");
	glUseProgram(shadow_map_program);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
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
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, cube_map_size, cube_map_size);
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

			check_fbo();

			GLuint DrawBuffer = GL_COLOR_ATTACHMENT0;

			glDrawBuffers(1, &DrawBuffer);

			glClearColor(0, 0.0, 0.0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void *)0);
		}
		check_gl_error();
		depth_maps.push_back(depth);
	}

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &depth_buffer);
	check_gl_error();
	return depth_maps;
}
#include <chrono>
// assumes that the currently bound vao is the entire scene to be rendered.
void render_receivers(int num_indices, std::vector<ReceiverData> receivers, std::vector<DepthCubeMap> depth_maps, int num_probes) {

	GLuint visibility_shader = LoadShaders("shader.vert", "visibility.frag");
	GLuint spherical_harmonics_shader = LoadShaders("full_screen.vert", "spherical_harmonics.frag");


	//hmm we might want to calculate this more accuratly
	//it would improve our accuracy.
	// oh we have the max radius though. that should be our far plane.
	// near should be voxel_size * 0.5, right? Interesting, being to close to a surface would reduce accuracy (for points further away)

	float near_plane = 0.01;
	mat4 proj = projection(near_plane, 100.0, 1);

	GLuint matrix_uniform_location = glGetUniformLocation(visibility_shader, "matrix");
	GLuint probe_pos_uniform_location = glGetUniformLocation(visibility_shader, "probe_pos");
	GLuint probe_weight_uniform_location = glGetUniformLocation(visibility_shader, "probe_weight");

	GLuint receiver_normal_uniform_location = glGetUniformLocation(visibility_shader, "receiver_normal");
	GLuint receiver_pos_uniform_location = glGetUniformLocation(visibility_shader, "receiver_pos");
	GLuint sh_samples_uniform_location = glGetUniformLocation(visibility_shader, "num_sh_samples");

	int num_pbo_bytes = num_probes_per_rec * receivers.size() * sizeof(float) * num_sh_coeffs;
	GLuint pbo;
	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, num_pbo_bytes, NULL, GL_STREAM_READ);

	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create the depth buffer
	GLuint depth_buffer;
	glGenRenderbuffers(1, &depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, sh_samples, sh_samples);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
	glViewport(0, 0, sh_samples, sh_samples);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	check_gl_error();


	SHTextures visibility[6];
	for (int i = 0; i < 6; i++) visibility[i] = gen_sh_textures();


	SHTextures shs = gen_sh_textures();

	auto start_time = std::chrono::high_resolution_clock::now();
	check_gl_error();
	glDisable(GL_CULL_FACE);

	int num_written_shs = 0;
	for (int receiver_index = 0; receiver_index < receivers.size(); receiver_index++) {
		ReceiverData *receiver = &receivers[receiver_index];
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		vec3 probe_positions[num_probes_per_rec];
		for (int i = 0; i < num_probes_per_rec; i++)
			probe_positions[i] = receiver->visible_probes[i].position;
		float probe_weights[num_probes_per_rec];

		for (int i = 0; i < num_probes_per_rec; i++)
			probe_weights[i] = receiver->visible_probes[i].weight;

		GLenum DrawBuffers[num_probes_per_rec];
		for (int i = 0; i < num_probes_per_rec; i++) {
			DrawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}

		glUseProgram(visibility_shader);

		// @robustness assumes vec3 has no padding.
		glUniform3fv(probe_pos_uniform_location, num_probes_per_rec, probe_positions[0].data());
		glUniform1fv(probe_weight_uniform_location, num_probes_per_rec, probe_weights);
		glUniform3fv(receiver_pos_uniform_location, 1, receiver->position.data());
		glUniform3fv(receiver_normal_uniform_location, 1, receiver->normal.data());
		glUniform1i(sh_samples_uniform_location, sh_samples);



		// render the visibility 
		for (int cube_map_index = 0; cube_map_index < 6; cube_map_index++) {
			for (int i = 0; i < num_probes_per_rec; i++) {
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_CUBE_MAP, receiver->visible_probes[i].depth_cube_map.cube_map);
			}

			// offset the recevier position by the location to avoid culling neccesary geometry with the near plane
			// still might be problematic though for positons parallel to the plane 
			// doesn't work nicely
			vec3 rec_pos = receiver->position;// -cube_map_dir[cube_map_index] * near_plane*0.9;

			mat4 mat = proj * look_at(rec_pos, rec_pos + cube_map_dir[cube_map_index], cube_map_up[cube_map_index]);
			glUniformMatrix4fv(matrix_uniform_location, 1, false, mat.data());

			for (int i = 0; i < num_probes_per_rec; i++) {
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, visibility[cube_map_index].textures[i], 0);
			}

			glDrawBuffers(receiver->num_visible_probes, DrawBuffers);

			check_fbo();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (void *)0);
		}

		glDisable(GL_DEPTH_TEST);
		glUseProgram(spherical_harmonics_shader);
		for (int i = 0; i < 8; i++)
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, shs.textures[i], 0);
		// Output spherical harmonics, two probes per iteration.
		for (int i = 0; i < num_probes_per_rec; i += 2) {
			if (receiver->num_visible_probes <= i) break;
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			for (int cube_map_index = 0; cube_map_index < 6; cube_map_index++) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, visibility[cube_map_index].textures[i]);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, visibility[cube_map_index].textures[i + 1]);

				glDrawBuffers(8, DrawBuffers);

				check_fbo();
				// draws a full screen triangle with vertices overridden by vertex shader
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}
			// @Robustness
			// the filtering for mipmapping is not specified... so this really is not right.
			// but in practice I *guess* that it works... heh 
			// we should possibly either do this with a few passes with a pixel shader or compute shader 
			// but for now meh.
			// also hint which filtering... however is box fast or good? idk, probably fast? or is it precision?
			// glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST/GL_FASTEST), 

			// might possibly be some room for optimization as well
			// could do the mipmapping for all 8 textures in one go.


			for (int j = 0; j < 4; j++) {
				// average the shs samples 
				glBindTexture(GL_TEXTURE_2D, shs.textures[j]);
				glGenerateMipmap(GL_TEXTURE_2D);

				// output to pbo
				glGetTexImage(GL_TEXTURE_2D, std::log2(sh_samples), GL_RGBA, GL_FLOAT, (void *)(sizeof(float)*(num_written_shs)));
				num_written_shs += 4;
			}

			// only output this if inside of radius 
			if (receiver->num_visible_probes > i + 1) {
				for (int j = 0; j < 4; j++) {
					// average the shs samples 
					glBindTexture(GL_TEXTURE_2D, shs.textures[j + 4]);
					glGenerateMipmap(GL_TEXTURE_2D);

					// output to pbo
					glGetTexImage(GL_TEXTURE_2D, std::log2(sh_samples), GL_RGBA, GL_FLOAT, (void *)(sizeof(float)*(num_written_shs)));
					num_written_shs += 4;
				}
			}
		}

		static int num_done = 0;

		auto elapsed = std::chrono::high_resolution_clock::now() - start_time;
		auto approx = elapsed / (float)(++num_done) * receivers.size();
		std::cout << "elapsed " << std::chrono::duration_cast<std::chrono::minutes>(elapsed).count() << " minutes out of ~" << std::chrono::duration_cast<std::chrono::minutes>(approx).count() << " minutes..." << std::endl;

		printf("recs done: %d\n", num_done);
	}
	float* sh_coeffs = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	int num_coeffs_per_probe = num_sh_coeffs;

	Eigen::SparseMatrix<float> coeff_matrix(receivers.size(), num_probes*num_coeffs_per_probe);
	std::vector<Eigen::Triplet<float>> coefficients;

	Eigen::MatrixXf full_mat_nz(num_probes_per_rec*num_sh_coeffs, receivers.size());
	Eigen::Matrix<int16_t, Eigen::Dynamic, Eigen::Dynamic> probe_indices(num_probes_per_rec, receivers.size());
	probe_indices.fill(-1);

	int num_handled_coefficients = 0;
	for (int receiver_index = 0; receiver_index < receivers.size(); receiver_index++) {
		for (int probe_index = 0; probe_index < receivers[receiver_index].num_visible_probes; probe_index++) {
			for (int i = 0; i < num_coeffs_per_probe; i++) {
				float coeff = sh_coeffs[num_handled_coefficients++];
				if (std::isnan(coeff)) {
					__debugbreak();
					coeff = 0.0f;
				}
				full_mat_nz(probe_index*num_sh_coeffs + i, receiver_index) = coeff;
				coefficients.push_back({
					receiver_index,
					receivers[receiver_index].visible_probes[probe_index].index * num_coeffs_per_probe + i,
					coeff
					});
			}
			probe_indices(probe_index, receiver_index) = receivers[receiver_index].visible_probes[probe_index].index;
		}
	}

	if (num_handled_coefficients != num_written_shs)
		__debugbreak(); // we fucked up the transfer to/out of the pbo 



	store_matrix(full_mat_nz, PRECOMP_ASSET_FOLDER "full_nz.matrix");
	store_matrixi(probe_indices, PRECOMP_ASSET_FOLDER "probe_indices.imatrix");

	//coeff_matrix.setFromTriplets(coefficients.begin(), coefficients.end());

	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &depth_buffer);
}
#pragma optimize("", on);


void load_mesh(Mesh *mesh) {
	GLuint vao, vertex_buffer, index_buffer, normal_buffer, lm_uv_buffer;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vertex_buffer);
	glGenBuffers(1, &normal_buffer);
	glGenBuffers(1, &index_buffer);
	glGenBuffers(1, &lm_uv_buffer);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, mesh->num_verts * sizeof(vec3), mesh->verts, GL_STATIC_DRAW);


	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, mesh->num_verts * sizeof(vec3), mesh->normals, GL_STATIC_DRAW);

	if (mesh->lightmap_uv) {
		glBindBuffer(GL_ARRAY_BUFFER, lm_uv_buffer);
		glBufferData(GL_ARRAY_BUFFER, mesh->num_verts * sizeof(vec2), mesh->lightmap_uv, GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->num_indices * sizeof(unsigned int), mesh->indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
	if (mesh->lightmap_uv) {
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, lm_uv_buffer);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
	}
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

	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	return true;
}

#include <queue>


Eigen::MatrixXf load_matrix(char *file) {
	FILE *f = fopen(file, "rb");
	int cols, rows;

	fread(&cols, sizeof(int), 1, f);
	fread(&rows, sizeof(int), 1, f);

	float *data = (float *)malloc(cols*rows * sizeof(float));
	fread(data, sizeof(float), cols*rows, f);
	fclose(f);

	return Eigen::Map<Eigen::MatrixXf>(data, rows, cols);
}


MatrixXi16 load_imatrix(char *file) {
	FILE *f = fopen(file, "rb");
	int cols, rows;

	fread(&cols, sizeof(int), 1, f);
	fread(&rows, sizeof(int), 1, f);

	int16_t *data = (int16_t *)malloc(cols*rows * sizeof(int));
	fread(data, sizeof(int16_t), cols*rows, f);
	fclose(f);

	return Eigen::Map<MatrixXi16>(data, rows, cols);
}
#pragma optimize ("", on)
void recompute_matrix_factorization(std::vector<Receiver> *recs) {
	auto full_mat_nz = load_matrix(PRECOMP_ASSET_FOLDER "full_nz.matrix");
	auto probe_indices = load_imatrix(PRECOMP_ASSET_FOLDER "probe_indices.imatrix");

	int num_probes = probe_indices.maxCoeff() + 1;
	int num_receivers = probe_indices.cols();
	int col_length = probe_indices.rows();
	Eigen::MatrixXf full(num_probes * 16, num_receivers);
	full.setZero();

	int new_num_receivers = 0;
	for (int i = 0; i < num_receivers; i++) {
		bool non_zero = false;
		for (int c = 0; c < col_length; c++) {
			int16_t probe = probe_indices(c, i);
			if (probe == -1)continue;

			auto src = &full_mat_nz(c * 16, i);
			for (int i = 0; i < 16; i++) {
				if (src[i] != 0) {
					non_zero = true;
					break;
				}
			}
			auto dst = &full(probe * 16, new_num_receivers);
			memmove(dst, src, sizeof(float) * 16);
		}
		if (non_zero)++new_num_receivers;
		(*recs)[new_num_receivers] = (*recs)[i]; // removes all nonzero receivers.
	}

	recs->resize(new_num_receivers);
	full.conservativeResize(Eigen::NoChange_t(), new_num_receivers);
	Eigen::SparseMatrix<float> f = full.sparseView();
	f.makeCompressed();
	auto sd = smooth_dictionary_learning(&f, recs, 0.01f, 2048);

#if 1
	{
		// add padding
		static int px_map[1024][1024];
		memset(px_map, 0xff, sizeof(px_map));
		// set all neighbours
		for (int i = 0; i < new_num_receivers; i++) {
			Receiver rec = (*recs)[i];
			px_map[rec.px.x()][rec.px.y()] = i;
			for (int dx = -1; dx <= 1; dx++) {
				for (int dy = -1; dy <= 1; dy++) {
					px_map[rec.px.x() + dx][rec.px.y() + dy] = i;
				}
			}
		}
		// clear those already in recs
		for (int i = 0; i < new_num_receivers; i++) {
			Receiver rec = (*recs)[i];
			px_map[rec.px.x()][rec.px.y()] = -1;
		}

		struct PaddingInfo {
			int receiver_index;
			ivec2 px;
		};

		std::vector<PaddingInfo> padding;
		// add the padding receivers
		for (int x = 0; x < 1024; x++) {
			for (int y = 0; y < 1024; y++) {
				if (px_map[x][y] != -1) {
					PaddingInfo pad = {};
					pad.px = ivec2(x, y);
					pad.receiver_index = px_map[x][y];
					padding.push_back(pad);
				}
			}
		}
		sd.code.conservativeResize(sd.code.rows(), sd.code.cols() + padding.size());
		for (int q = 0; q < padding.size(); q++) {
			PaddingInfo pd = padding[q];
			SparseVector<float> x = sd.code.col(new_num_receivers + q);
			SparseVector<float> y = sd.code.col(new_num_receivers);

			sd.code.col(new_num_receivers+q) = y;
			Receiver rec;
			rec.px.x() = pd.px.x();
			rec.px.y() = pd.px.y();
			recs->push_back(rec);
		}
		new_num_receivers += padding.size();
	}
#endif

	{ // output new matrices
		store_matrix(sd.dictionary, PRECOMP_ASSET_FOLDER "dictionary.matrix");
		FILE *coeffs = fopen(PRECOMP_ASSET_FOLDER "coeffs.matrix", "wb");
		FILE *coeff_indices = fopen(PRECOMP_ASSET_FOLDER "coeff_idx.imatrix", "wb");
		fwrite(&new_num_receivers, sizeof(int), 1, coeffs);
		fwrite(&new_num_receivers, sizeof(int), 1, coeff_indices);
		int num_nz_atoms = 8;
		fwrite(&num_nz_atoms, sizeof(int), 1, coeffs);
		fwrite(&num_nz_atoms, sizeof(int), 1, coeff_indices);


		for (int i = 0; i < sd.code.cols(); i++) {
			SparseVector<float> col = sd.code.col(i);
			int num_nz = col.nonZeros();
			int16_t nz_indices[8];
			for (int i = 0; i < num_nz; i++) nz_indices[i] = col.innerIndexPtr()[i];

			float *nz_coeffs = col.valuePtr();

			fwrite(nz_indices, sizeof(int16_t), num_nz, coeff_indices);
			fwrite(nz_coeffs, sizeof(float), num_nz, coeffs);

			//always write coeffs even if they're not used
			static int16_t invalid_index[8] = { -1,-1,-1,-1,-1,-1,-1,-1 };
			static float invalid_data[8] = { 0,0,0,0,0,0,0,0 };
			fwrite(invalid_index, sizeof(int16_t), 8 - num_nz, coeff_indices);
			fwrite(invalid_data, sizeof(float), 8 - num_nz, coeffs);
		}
		fclose(coeffs);
		fclose(coeff_indices);
	}
}


#pragma optmize("", on)
#define RECOMP_FACTORIZATION
int visibility(std::vector<vec3> probe_locations, Mesh *mesh) {


	if (!init_gl()) {
		printf("error while initializing opengl");
		return -1;
	}

	load_mesh(mesh);
	GLuint positions, normals;
	std::vector<Receiver> recs = compute_receivers_gpu(mesh->num_indices, &normals, &positions);

#ifdef RECOMP_FACTORIZATION
	// recomputes the matrix factorization based on the previous computation of the matrices
	recompute_matrix_factorization(&recs);
	char *px_map_path = PRECOMP_ASSET_FOLDER "receiver_px_map_comp.imatrix";
#else
	char *px_map_path = PRECOMP_ASSET_FOLDER "receiver_px_map.imatrix";
#endif

	{
		FILE *f = fopen(px_map_path, "wb");
		int num_recs = recs.size();
		int num_comps = 2;

		fwrite(&num_comps, sizeof(num_comps), 1, f);
		fwrite(&num_recs, sizeof(num_recs), 1, f);
		for (Receiver rec : recs) {
			fwrite(rec.px.data(), sizeof(int), 2, f);
		}
		fclose(f);
	}

#ifdef RECOMP_FACTORIZATION
	// removes the receivers that was zero in the previous iteration. 
	// so let's not continue past here!
	return 0;
#endif

#if 0 // to visualize shadow map at probe_loc
	vec3 probe_loc = vec3(0, 0.5, 0);
	probe_locations.clear();
	probe_locations.push_back(probe_loc);
#endif

	printf("generating depth information for probes...\n");

	auto depth_maps = render_probe_depth(mesh->num_indices, probe_locations);

	std::vector<ReceiverData> receivers;
	receivers.reserve(recs.size());

	printf("finding closest probes for each receivers...\n");

#define REC_RADIUS

	float radius = FLT_MAX;
	for (Receiver receiver : recs) {
		ReceiverData rec_data = {};
		rec_data.normal = receiver.norm;
		rec_data.position = receiver.pos;
		rec_data.px = receiver.px;

		// jesus christ do I love c++ templates 
		std::priority_queue<int, std::vector<int>,
			std::function<bool(int, int)>>
			closest_probes([receiver, probe_locations](int ia, int ib) -> bool {
			vec3 da = probe_locations[ia] - receiver.pos;
			vec3 db = probe_locations[ib] - receiver.pos;
			return (da.dot(da) < db.dot(db));
		});

		for (int i = 0; i < min(num_probes_per_rec + 1, probe_locations.size()); i++) {
			closest_probes.push(i);
		}

		for (int i = num_probes_per_rec + 1; i < probe_locations.size(); i++) {
			closest_probes.push(i);
			closest_probes.pop();
		}

		closest_probes.pop();
		closest_probes.pop();





		float dist = (probe_locations[closest_probes.top()] - rec_data.position).norm();

		//@fix when num probes is less than num_probes_per_rec
		closest_probes.pop();
#ifdef REC_RADIUS
		radius = dist;
#else 
		radius = min(radius, dist);
#endif
		int i = closest_probes.size() - 1;
		while (!closest_probes.empty()) {
			int probe_index = closest_probes.top();
			vec3 probe = probe_locations[probe_index];
			closest_probes.pop();
			float dist = (probe - rec_data.position).norm();
			rec_data.visible_probes[i].position = probe;
			rec_data.visible_probes[i].index = probe_index;
			rec_data.visible_probes[i].depth_cube_map = depth_maps[probe_index];
#ifdef REC_RADIUS
			rec_data.visible_probes[i].weight = w(dist / radius);
#endif
			i--;
		}
		receivers.push_back(rec_data);
	}

#if 1
	float avg_num_visble_probes = 0.0;
	int min_num_visible_probes = num_probes_per_rec;
	int max_num_visible_probes = 0;
	int num_with_zero_visible = 0;
	for (ReceiverData &receiver : receivers) {
		receiver.num_visible_probes = 0.0;
		for (int i = 0; i < num_probes_per_rec; i++) {
			float dist = (receiver.visible_probes[i].position - receiver.position).norm();
#ifndef REC_RADIUS
			receiver->visible_probes[i].weight = w(dist / radius);
#endif
			if (receiver.visible_probes[i].weight > 0.0) ++receiver.num_visible_probes;
			//receiver->num_visible_probes = 8;
	}
		avg_num_visble_probes += receiver.num_visible_probes / (float)receivers.size();
		max_num_visible_probes = max(max_num_visible_probes, receiver.num_visible_probes);
		min_num_visible_probes = min(min_num_visible_probes, receiver.num_visible_probes);

		if (receiver.num_visible_probes == 0)++num_with_zero_visible;
}
	printf("avg_num_visble probes: %f\n", avg_num_visble_probes);
	printf("min_num_visble probes: %d\n", min_num_visible_probes);
	printf("max_num_visble probes: %d\n", max_num_visible_probes);
	printf("perc none visible : %f\n", num_with_zero_visible / (float)receivers.size());
#endif





	GLint maxAttach = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);

	printf("calculating spherical harmonics etc...");

#if 1
	render_receivers(mesh->num_indices, receivers, depth_maps, probe_locations.size());
#else
	// to visualize shadow map at probe_loc
	GLuint program = LoadShaders("shader.vert", "shader.frag");
	//GLuint program = LoadShaders("full_screen.vert", "shader.frag");

	check_gl_error();
	glUseProgram(program);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	vec3 camera_pos = vec3(0, 6.0, 0.01);
	glViewport(0, 0, 1024, 768);
	static int active_probe = 0;
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

		static bool down_r = false;
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			if (!down_r) ++active_probe;
			if (active_probe == probe_locations.size()) active_probe = 0;
			down_r = true;
		} else {
			down_r = false;
		}

		static bool down_f = false;
		if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
			if (!down_f) --active_probe;
			if (active_probe < 0) active_probe = probe_locations.size() - 1;
			down_f = true;
		} else {
			down_f = false;
		}
		static int active_view = 0;
		static bool down_t = false;
		if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
			if (!down_t) ++active_view;
			if (active_view == 6) active_view = 0;
			down_t = true;
		} else {
			down_t = false;
		}

		static bool down_g = false;
		if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
			if (!down_g) --active_view;
			if (active_view < 0) active_view = 5;
			down_g = true;
		} else {
			down_g = false;
		}





		mat4 view = look_at(camera_pos, vec3(0, 0, 0), vec3(0, 1, 0));


		mat4 proj = projection(0.1, 100.0, 768 / 1024.0);
		mat4 mat = proj * view;
#if 0
		mat4 mat = proj * look_at(probe_locations[active_probe],
			probe_locations[active_probe] + cube_map_dir[active_view], cube_map_up[active_view]);
#endif


		static GLuint matrix_location = glGetUniformLocation(program, "matrix");

		static GLuint light_location = glGetUniformLocation(program, "light_pos");

		check_gl_error();
		glUniformMatrix4fv(matrix_location, 1, false, mat.data());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, depth_maps[active_probe].cube_map);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, positions);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, normals);



		check_gl_error();

		glUniform3fv(light_location, 1, probe_locations[active_probe].data());

		glClearColor(1.0, 0.5, 0.5, 1);
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






#pragma optmize("", on)
