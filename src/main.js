'using strict';

////////////////////////////////////////////////////////////////////////////////




var compressed = false;
t_scene = {
	support_radius: 6.0,
	name: "t_scene",
	cameraPos: vec3.fromValues(0.0, 17.6906, 0.0),
	cameraRot: quat.fromEuler(quat.create(), -90, 0,0),
}
sponza = {
	name: "sponza",
	support_radius:10.0,
	cameraPos: vec3.fromValues(-15, 3, 0),
	cameraRot: quat.fromEuler(quat.create(), 15, -90, 0),
}

sponza_tp = {
	name: "sponza_with_teapot",
	support_radius:10.0,
	cameraPos: vec3.fromValues(-15, 3, 0),
	cameraRot: quat.fromEuler(quat.create(), 15, -90, 0),
}


living_room = {
	name: "living_room",
	support_radius: 10.0,
	cameraPos: vec3.fromValues(2.62158, 1.68613, 3.62357),
	cameraRot: quat.fromEuler(quat.create(), 90-101, 180-70.2, 180+180),
}

scene = living_room;
use_baked_light = false;
var srgb = false;


var stats;	
var gui;

var settings = {
	target_fps: 60,
	environment_brightness: 0.0,
    num_sh_coeffs_to_render: 16,
	rotate_light: false,
	view_gi_lightmap:false,
	view_lightmap:false,
	redraw_global_illumination:false,
	lightmap_only: false,
	view_padded: false,
	view_trans_pc: false,
	view_shadow_map: false,
};

var sceneSettings = {
	ambientColor: new Float32Array([0.15, 0.15, 0.15, 1.0]),
};

////////////////////////////////////////////////////////////////////////////////

var app;

var gpuTimePanel;
var picoTimer;

var defaultShader;
var shadowMapShader;
var lightMapShader;
var texturedByLightmapShader;
var dynamicShader;
var shadowMapDynamicShader;

var blitTextureDrawCall;
var environmentDrawCall;

var sceneUniforms;

var shadowMapSize = 4096;
var shadowMapFramebuffer;

var lightMapSize = 4096;
var lightMapFramebuffer;

var giLightMapSize = 1024;

var probeRadianceFramebuffer;
var probeRadianceDrawCall;

var camera;
var directionalLight;
var meshes = [];
var staticMeshes = [];
var dynamicMeshes = [];

var probeDrawCall;
var hideProbes = false;
var probeVisualizeSHDrawCall;
var probeVisualizeRawDrawCall;
var probeLocations;
var probeVisualizeMode = 'raw';
var probeVisualizeUnlit = true;

var bakedDirect;

var num_probes;
var num_relight_rays;
var num_sh_coefficients;
var relight_uvs;
var relight_uvs_texture;
var relight_shs;
var relight_shs_texture;
var relight_dirs;
var relight_dirs_texture;
var probe_pos_texture;
var dict;

var u_texture;
var full_texture;

var px_map;
var probe_indices;
var dictionary_coeffs;

var calcGIShader;
var applyDictDrawCall;
var GIDrawCall;




window.addEventListener('DOMContentLoaded', function () {

	init();
	resize();

	window.addEventListener('resize', resize, false);
	requestAnimationFrame(render);

}, false);

////////////////////////////////////////////////////////////////////////////////
// Utility

function checkWebGL2Compability() {

	var c = document.createElement('canvas');
	var webgl2 = c.getContext('webgl2');
	if (!webgl2) {
		var message = document.createElement('p');
		message.id = 'no-webgl2-error';
		message.innerHTML = 'WebGL 2.0 doesn\'t seem to be supported in this browser and is required for this demo! ' +
			'It should work on most modern desktop browsers though.';
		canvas.parentNode.replaceChild(message, document.getElementById('canvas'));
		return false;
	}
	return true;

}
function isDataTexture(imageName) {
	return imageName.indexOf('_ddn') != -1
		  || imageName.indexOf('_spec') != -1
		  || imageName.indexOf('_normal') != -1;
}

function loadTexture(imageName, options) {

	if (!options) {

		var options = {};
		options['minFilter'] = PicoGL.LINEAR_MIPMAP_NEAREST;
		options['magFilter'] = PicoGL.LINEAR;
		options['mipmaps'] = true;
		if(srgb)
		{
			if (isDataTexture(imageName)) {
				options['internalFormat'] = PicoGL.RGB8;
				options['format'] = PicoGL.RGB;
			} else {
				options['internalFormat'] = PicoGL.SRGB8_ALPHA8;
				options['format'] = PicoGL.RGBA;
			}
		}
	}

	var texture = app.createTexture2D(1, 1, options);
	texture.data(new Uint8Array([200, 200, 200, 256]));

	var image = document.createElement('img');
	image.onload = function() {

		texture.resize(image.width, image.height);
		texture.data(image);

			// HACK: set anisotropy
		var ext = app.gl.getExtension('EXT_texture_filter_anisotropic');
		app.gl.bindTexture(PicoGL.TEXTURE_2D, texture.texture);
		var maxAniso = app.gl.getParameter(ext.MAX_TEXTURE_MAX_ANISOTROPY_EXT);
		app.gl.texParameterf(PicoGL.TEXTURE_2D, ext.TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
	};
	image.src = 'assets/' + imageName;
	return texture;

}


function makeSingleColorTexture(color) {
	if(srgb)
	{
		var options = {};
		options['minFilter'] = PicoGL.NEAREST;
		options['magFilter'] = PicoGL.NEAREST;
		options['mipmaps'] = false;
		options['format'] = PicoGL.RGBA;
		options['internalFormat'] = PicoGL.SRGB8_ALPHA8;
		options['type'] = PicoGL.UNSIGNED_BYTE;
		var side = 32;
		var arr =  [];
		for (var i = 0; i < side * side; i++) {
			var colorByte = [color[0] * 255.99, color[1] * 255.99, color[2] * 255.99, 255];
			arr = arr.concat(colorByte);
		}
		var image_data = new Uint8Array(arr);
		return app.createTexture2D(image_data, side, side, options);
	}
	else{
		var options = {};
		options['minFilter'] = PicoGL.NEAREST;
		options['magFilter'] = PicoGL.NEAREST;
		options['mipmaps'] = false;
		options['format'] = PicoGL.RGBA;
		options['internalFormat'] = PicoGL.RGBA8;
		options['type'] = PicoGL.UNSIGNED_BYTE;
		var side = 32;
		var arr =  [];
		for (var i = 0; i < side * side; i++) {
			var colorByte = [color[0] * 255.99, color[1] * 255.99, color[2] * 255.99, 255];
			arr = arr.concat(colorByte);
		}
		var image_data = new Uint8Array(arr);
		return app.createTexture2D(image_data, side, side, options);
	}
}


// num_relight_rays_per_probe * num_probes (RG32F where R,G = u,v coordinates)
// ex. 100 * 72
function makeTextureFromRelightUVs(relight_uvs) {
    var options = {};
    options['minFilter'] = PicoGL.NEAREST;
    options['magFilter'] = PicoGL.NEAREST;
    options['mipmaps'] = false;
    options['format'] = PicoGL.RG;
    options['internalFormat'] = PicoGL.RG32F;
    options['type'] = PicoGL.FLOAT;
    image_data = new Float32Array(relight_uvs.reduce( (a,b) => a.concat(b)).map( x => x==-1?-1:x/giLightMapSize));
    return app.createTexture2D(image_data, num_relight_rays, num_probes, options);
}

// num_sh_coefficients * num_relight_rays
// ex. 16 * 100
function makeTextureFromRelightSHs(relight_shs) {
    var options = {};
    options['minFilter'] = PicoGL.NEAREST;
    options['magFilter'] = PicoGL.NEAREST;
    options['mipmaps'] = false;
    options['format'] = PicoGL.RED;
    options['internalFormat'] = PicoGL.R32F;
    options['type'] = PicoGL.FLOAT;
    var image_data = new Float32Array(relight_shs.reduce( (a,b) => a.concat(b)));
    return app.createTexture2D(image_data, num_sh_coefficients, num_relight_rays, options);
}

// For debugging purposes
// num_relight_rays_per_probe * 1 (RGB32F where R,G,B = x,y,z coordinates of unit length direction vector)
// ex. 100 * 1
function makeTextureFromRelightDirs(relight_dirs) {
    var options = {};
    options['minFilter'] = PicoGL.NEAREST;
    options['magFilter'] = PicoGL.NEAREST;
    options['mipmaps'] = false;
    options['format'] = PicoGL.RGB;
    options['internalFormat'] = PicoGL.RGB32F;
    options['type'] = PicoGL.FLOAT;
    image_data = new Float32Array(relight_dirs.reduce( (a,b) => a.concat(b)));
    return app.createTexture2D(image_data, num_relight_rays, 1, options);
}

// num_probes * 1 (RGB32F where R,G,B = x,y,z coordinates of probe position)
// ex. 72 * 1
function makeTextureFromProbePositions(probe_positions) {
    var options = {};
    options['minFilter'] = PicoGL.NEAREST;
    options['magFilter'] = PicoGL.NEAREST;
    options['mipmaps'] = false;
    options['format'] = PicoGL.RGB;
    options['internalFormat'] = PicoGL.RGB32F;
    options['type'] = PicoGL.FLOAT;
    image_data = new Float32Array(probe_positions);
    console.log("num_probes: " + num_probes)
    return app.createTexture2D(image_data, num_probes, 1, options);
}

function makeTextureFromMatrix1(matrix) {
	var options = {};
	options['minFilter'] = PicoGL.NEAREST;
	options['magFilter'] = PicoGL.NEAREST;
	options['mipmaps'] = false;
	options['format'] = PicoGL.RED;
	options['internalFormat'] = PicoGL.R32F;
	options['type'] = PicoGL.FLOAT;
	return app.createTexture2D(matrix.col_major_data, matrix.rows, matrix.cols, options);
}

function makeTexturefromFloatArr(data) {
	var options = {};
	options['minFilter'] = PicoGL.NEAREST;
	options['magFilter'] = PicoGL.NEAREST;
	options['mipmaps'] = false;
	options['format'] = PicoGL.RGBA;
	options['internalFormat'] = PicoGL.RGBA16F;
	options['type'] = PicoGL.FLOAT;

	// @ROBUSTNESS, spec requires support of only 1024x1024 but if the exist on my laptop maybe it's fine?
	var max_size = 1<<14;

	var aligned_length = (data.length/4 + max_size-1) & ~(max_size-1);
	image_data = new Float32Array(aligned_length*4);
	image_data.set(data);
	console.log("sizes:",max_size, aligned_length>>14);
	return app.createTexture2D(image_data, max_size, aligned_length >> 14,  options);
}

function makeShader(name, shaderLoaderData) {

	var programData = shaderLoaderData[name];
	var program = app.createProgram(programData.vertexSource, programData.fragmentSource);
	return program;

}

function debugMoveDynamicObject() {
	var keys = camera.keys;
    var translation = vec3.fromValues(
        Math.sign(keys['right']  - keys['left']),
        0.0,
        Math.sign(keys['down'] - keys['up'])
    );
    translation = vec3.fromValues(translation[0] - translation[2], 0.0, translation[0] + translation[2]);
    if (dynamicMeshes.length > 0 && dynamicMeshes[0] !== undefined) {
    	mat4.translate(dynamicMeshes[0].modelMatrix, dynamicMeshes[0].modelMatrix, translation);
    }
}


function loadDynamicObject(directory, objFilename, mtlFilename, modelMatrix) {

    var objLoader = new OBJLoader();
    var mtlLoader = new MTLLoader();

    var path = 'assets/' + directory;

    objLoader.load(path + objFilename, function(objects) {
        mtlLoader.load(path + mtlFilename, function(materials) {
            objects.forEach(function(object) {

                var material = materials[object.material];
                // var diffuseMap  = (material.properties.map_Kd)   ? directory + material.properties.map_Kd   : 'default_diffuse.png';
                var diffuseTexture;
				if (material.properties.map_Kd) {
                    diffuseTexture = loadTexture(directory + material.properties.map_Kd);
                } else {
					diffuseTexture = makeSingleColorTexture(material.properties.Kd);
				}
                var specularMap = (material.properties.map_Ks)   ? directory + material.properties.map_Ks   : 'default_specular.jpg';
                var normalMap   = (material.properties.map_norm) ? directory + material.properties.map_norm : 'default_normal.jpg';

                var vertexArray = createVertexArrayFromMeshInfo(object);


                var drawCall = app.createDrawCall(dynamicShader, vertexArray)
                    .uniformBlock('SceneUniforms', sceneUniforms)
                    .texture('u_diffuse_map', diffuseTexture)
                    .texture('u_specular_map', loadTexture(specularMap))
                    .texture('u_normal_map', loadTexture(normalMap))
                    .texture('u_probe_pos_texture', probe_pos_texture)
					.uniform('u_num_probes', num_probes)
					.uniform('u_probe_support_radius_squared', scene.support_radius * scene.support_radius );


                var shadowMappingDrawCall = app.createDrawCall(shadowMapDynamicShader, vertexArray);

                meshes.push({
                    modelMatrix: modelMatrix || mat4.create(),
                    drawCall: drawCall,
                    shadowMapDrawCall: shadowMappingDrawCall,
					isDynamic: true
                });

                dynamicMeshes.push(meshes[meshes.length-1]);

            });
        });
    });

}

function loadObjectUV2(directory, objFilename, mtlFilename, modelMatrix) {

	var objLoader = new OBJLoader();
	var mtlLoader = new MTLLoader();

	var path = 'assets/' + directory;

	objLoader.load(path + objFilename, function(objects) {
		mtlLoader.load(path + mtlFilename, function(materials) {
			objects.forEach(function(object) {

				var material = materials[object.material];
				// var diffuseMap  = (material.properties.map_Kd)   ? directory + material.properties.map_Kd   : 'default_diffuse.png';
                var diffuseTexture;
                if (material.properties.map_Kd) {
                    diffuseTexture = loadTexture(directory + material.properties.map_Kd);
                } else {
                    diffuseTexture = makeSingleColorTexture(material.properties.Kd);
                }
                var specularMap = (material.properties.map_Ks)   ? directory + material.properties.map_Ks   : 'default_specular.jpg';
				var normalMap   = (material.properties.map_norm) ? directory + material.properties.map_norm : 'default_normal.jpg';

				var vertexArray = createVertexArrayFromMeshInfoUV2(object);

				var drawCall, lightMappingDrawCall;
                if (bakedDirect) {
                    drawCall = app.createDrawCall(defaultShader, vertexArray)
                        .uniformBlock('SceneUniforms', sceneUniforms)
                        .texture('u_diffuse_map', diffuseTexture);

                    lightMappingDrawCall = app.createDrawCall(lightMapShader, vertexArray)
                        .uniformBlock('SceneUniforms', sceneUniforms)
                        .texture('u_diffuse_map', diffuseTexture);
                } else {
                    drawCall = app.createDrawCall(defaultShader, vertexArray)
                        .uniformBlock('SceneUniforms', sceneUniforms)
                        .texture('u_diffuse_map', diffuseTexture)
                        .texture('u_specular_map', loadTexture(specularMap))
                        .texture('u_normal_map', loadTexture(normalMap));

                    lightMappingDrawCall = app.createDrawCall(lightMapShader, vertexArray)
                        .uniformBlock('SceneUniforms', sceneUniforms)
                        .texture('u_diffuse_map', diffuseTexture)
                        .texture('u_specular_map', loadTexture(specularMap))
                        .texture('u_normal_map', loadTexture(normalMap));
                }

				
				var shadowMappingDrawCall = app.createDrawCall(shadowMapShader, vertexArray);

				var texturedByLightmapDrawCall = app.createDrawCall(texturedByLightmapShader, vertexArray)
                    .uniformBlock('SceneUniforms', sceneUniforms)
                    .texture('u_diffuse_map', diffuseTexture)
                    .texture('u_specular_map', loadTexture(specularMap))
                    .texture('u_normal_map', loadTexture(normalMap));
				
				meshes.push({
					modelMatrix: modelMatrix || mat4.create(),
					drawCall: drawCall,
					shadowMapDrawCall: shadowMappingDrawCall,
					lightmapDrawCall: lightMappingDrawCall,
                    texturedByLightmapDrawCall: texturedByLightmapDrawCall,
                    isDynamic: false
				});

                staticMeshes.push(meshes[meshes.length-1]);

			});
		});
	});

}

////////////////////////////////////////////////////////////////////////////////
// Initialization etc.

function init() {

	if (!checkWebGL2Compability()) {
		return;
	}

	var canvas = document.getElementById('canvas');
	app = PicoGL.createApp(canvas, { antialias: true });
	app.floatRenderTargets();

	stats = new Stats();
	stats.showPanel(1); // (frame time)
	document.body.appendChild(stats.dom);

	gpuTimePanel = stats.addPanel(new Stats.Panel('MS (GPU)', '#ff8', '#221'));
	picoTimer = app.createTimer();

	gui = new dat.GUI();
	gui.add(settings, 'target_fps', 0, 120);
	gui.add(settings, 'environment_brightness', 0.0, 2.0);
    gui.add(settings, 'num_sh_coeffs_to_render', 0, 16);
    gui.add(settings, 'rotate_light');
    gui.add(settings, 'view_gi_lightmap');
    gui.add(settings, 'view_lightmap');
    gui.add(settings, 'redraw_global_illumination');
    gui.add(settings, 'lightmap_only');
    gui.add(settings, 'view_padded');
	gui.add(settings, 'view_trans_pc');
	gui.add(settings, 'view_shadow_map');
	
	//////////////////////////////////////
	// Basic GL state

	app.clearColor(0, 0, 0, 1);
	app.cullBackfaces();
	app.noBlend();

	//////////////////////////////////////
	// Camera stuff

	var cameraPos, cameraRot;

	camera = new Camera(scene.cameraPos, scene.cameraRot);

	//////////////////////////////////////
	// Scene setup

	if (scene == t_scene && use_baked_light) {
        bakedDirect = loadTexture('t_scene/baked_direct.png', {'minFilter': PicoGL.LINEAR_MIPMAP_NEAREST,
            'magFilter': PicoGL.LINEAR,
            'mipmaps': true,
            'flipY': true});
    }

	directionalLight = new DirectionalLight();
	setupDirectionalLightShadowMapFramebuffer(shadowMapSize);
	setupLightmapFramebuffer(lightMapSize);
	setupGILightmapFramebuffer(giLightMapSize);
	if(!compressed) setupPaddingFbo(giLightMapSize);

	setupSceneUniforms();

	var shaderLoader = new ShaderLoader('src/shaders/');
	shaderLoader.addShaderFile('common.glsl');
	shaderLoader.addShaderFile('scene_uniforms.glsl');
	shaderLoader.addShaderFile('mesh_attributes.glsl');
	shaderLoader.addShaderFile('mesh_attributes_dynamic.glsl');
	shaderLoader.addShaderProgram('unlit', 'unlit.vert.glsl', 'unlit.frag.glsl');
	shaderLoader.addShaderProgram('default', 'default.vert.glsl', 'default.frag.glsl');
    shaderLoader.addShaderProgram('default_lm', 'default_lm.vert.glsl', 'default_lm.frag.glsl');
    shaderLoader.addShaderProgram('default_lm_baked_direct', 'default_lm.vert.glsl', 'default_lm_baked_direct.frag.glsl');
    shaderLoader.addShaderProgram('dynamic_lm', 'dynamic_lm.vert.glsl', 'dynamic_lm.frag.glsl');
	shaderLoader.addShaderProgram('environment', 'environment.vert.glsl', 'environment.frag.glsl');
	shaderLoader.addShaderProgram('textureBlit', 'screen_space.vert.glsl', 'texture_blit.frag.glsl');
    shaderLoader.addShaderProgram('shadowMapping', 'shadow_mapping.vert.glsl', 'shadow_mapping.frag.glsl');
    shaderLoader.addShaderProgram('shadowMappingDynamic', 'shadow_mapping_dynamic.vert.glsl', 'shadow_mapping.frag.glsl');
    shaderLoader.addShaderProgram('lightMapping', 'direct_lightmap.vert.glsl', 'direct_lightmap.frag.glsl');
    shaderLoader.addShaderProgram('lightMapping_baked_direct', 'direct_lightmap.vert.glsl', 'direct_lightmap_baked_direct.frag.glsl');
	shaderLoader.addShaderProgram('calc_gi', 'calc_gi_dict.vert.glsl', 'calc_gi_dict.frag.glsl');
	shaderLoader.addShaderProgram('calc_gi_uncompressed', 'calc_gi_uncompressed.vert.glsl', 'calc_gi_uncompressed.frag.glsl');
	shaderLoader.addShaderProgram('apply_dictionary', 'screen_space.vert.glsl', 'apply_dictionary.frag.glsl');

    shaderLoader.addShaderProgram('probeRadiance', 'probe_radiance.vert.glsl', 'probe_radiance.frag.glsl');
    shaderLoader.addShaderProgram('probeVisualizeSH', 'probe_visualize_sh.vert.glsl', 'probe_visualize_sh.frag.glsl');
    shaderLoader.addShaderProgram('probeVisualizeRaw', 'probe_visualize_raw.vert.glsl', 'probe_visualize_raw.frag.glsl');
    shaderLoader.addShaderProgram('texturedByLightmap', 'textured_by_lightmap.vert.glsl', 'textured_by_lightmap.frag.glsl');
    shaderLoader.addShaderProgram('padShader', 'screen_space.vert.glsl', 'padd.frag.glsl');

	var create_gi_draw_call = function(compressed)
	{
		if(px_map && calcGIShader && probe_indices && (!compressed || dictionary_coeffs))
		{
			var buf_px_map = app.createVertexBuffer(PicoGL.INT, 2, px_map.col_major_data);
			var buf_probe_indices = app.createVertexBuffer(PicoGL.INT, 4, probe_indices.col_major_data)
			
			var vertexArray = app.createVertexArray()
			.vertexIntegerAttributeBuffer(0, buf_px_map)
			.vertexIntegerAttributeBuffer(1, buf_probe_indices);

			if(compressed)
			{
				var buf_dict_coeffs = app.createVertexBuffer(PicoGL.FLOAT_MAT2x4, 4, dictionary_coeffs.col_major_data);
				vertexArray.vertexAttributeBuffer(2, buf_dict_coeffs);
			}

			GIDrawCall = app.createDrawCall(calcGIShader, vertexArray, PicoGL.POINTS);
			console.log('gi_draw_call_creation');
		}
	}


	shaderLoader.load(function(data) {


		var fullscreenVertexArray = createFullscreenVertexArray();

		var textureBlitShader = makeShader('textureBlit', data);
		blitTextureDrawCall = app.createDrawCall(textureBlitShader, fullscreenVertexArray);

		var environmentShader = makeShader('environment', data);
		environmentDrawCall = app.createDrawCall(environmentShader, fullscreenVertexArray)
		.texture('u_environment_map', loadTexture('environments/ocean.jpg', {}));

		var probeVertexArray = createSphereVertexArray(0.05, 32, 32);
        var unlitShader = makeShader('unlit', data);
        var probeVisualizeSHShader = makeShader('probeVisualizeSH', data);
        var probeVisualizeRawShader = makeShader('probeVisualizeRaw', data);
        setupProbeDrawCalls(probeVertexArray, unlitShader, probeVisualizeSHShader, probeVisualizeRawShader);

        if (bakedDirect) {
            defaultShader = makeShader('default_lm_baked_direct', data);
            lightMapShader = makeShader('lightMapping_baked_direct', data);
		} else {
            defaultShader = makeShader('default_lm', data);
            lightMapShader = makeShader('lightMapping', data);
        }

        dynamicShader = makeShader('dynamic_lm', data);

		shadowMapShader = makeShader('shadowMapping', data);

        shadowMapDynamicShader = makeShader('shadowMappingDynamic', data);


		var applyDictShader = makeShader('apply_dictionary', data);
		if(compressed)
		{
			calcGIShader = makeShader('calc_gi', data);
		}
		else{
			calcGIShader = makeShader('calc_gi_uncompressed', data);
		}
		create_gi_draw_call(compressed);
		

		applyDictDrawCall = app.createDrawCall(applyDictShader,fullscreenVertexArray);

        var probeRadianceShader = makeShader('probeRadiance', data);
        probeRadianceDrawCall = app.createDrawCall(probeRadianceShader, fullscreenVertexArray);

		loadObjectUV2(scene.name + '/', scene.name + '.obj_2xuv', scene.name + '.mtl');


        if (false) {
            let m = mat4.create();
            let r = quat.fromEuler(quat.create(), 0, 45, 0);
            let t = vec3.fromValues(0, 1, 0);
            let s = vec3.fromValues(0.06, 0.06, 0.06);
            mat4.fromRotationTranslationScale(m, r, t, s);
            //loadDynamicObject('teapot/', 'teapot.obj', 'default.mtl', m);
        }

        if (false) {
            let m = mat4.create();
            let r = quat.fromEuler(quat.create(), 0, 0, 0);
            let t = vec3.fromValues(-0.4, 2.8, -7.5);
            let s = vec3.fromValues(1.5, 1.5, 1.5);
            mat4.fromRotationTranslationScale(m, r, t, s);
            loadDynamicObject('living_room/', 'living_room.obj', 'living_room.mtl', m);
        }



		padDrawCall = app.createDrawCall(makeShader('padShader', data), fullscreenVertexArray);

		texturedByLightmapShader = makeShader('texturedByLightmap', data);


	});

	var dat_loader = new DatLoader();
	var num_loads = 3;
	
	var loading_done = function(){
		--num_loads;
		if(num_loads == 0){
			// this must be done after both relight uvs and relight shs has loaded
			setupProbeRadianceFramebuffer();
 		}
	}

	var precompute_folder = 'assets/' + scene.name + '/precompute/';


    var suffix = ""; //"";
    var relight_uvs_dir = precompute_folder + "relight_uvs" + suffix + ".dat";
    var relight_shs_dir = precompute_folder + "relight_shs" + suffix + ".dat";
    var relight_directions_dir = precompute_folder+"relight_directions" + suffix + ".dat";


    dat_loader.load(relight_uvs_dir,
	function(value) {
		relight_uvs = value;
		num_probes = relight_uvs.length;
        num_relight_rays = relight_uvs[0].length / 2;
        relight_uvs_texture = makeTextureFromRelightUVs(relight_uvs);
		loading_done();
	});

	dat_loader.load(relight_shs_dir,
	function(value) {
		relight_shs = value;
        num_sh_coefficients = relight_shs[0].length;
		console.log(num_sh_coefficients);
        num_relight_rays = relight_shs.length;

        relight_shs_texture = makeTextureFromRelightSHs(relight_shs);

		loading_done();
	});

    dat_loader.load(relight_directions_dir,
        function(value) {
            relight_dirs = value;
            num_relight_rays = relight_dirs.length;
            relight_dirs_texture = makeTextureFromRelightDirs(relight_dirs);
            loading_done();
        });

	dat_loader.load(precompute_folder + "probes.dat", function(value) {
		probeLocations = value.reduce( (a,b) => a.concat(b) );
		num_probes = probeLocations.length / 3;
        probe_pos_texture = makeTextureFromProbePositions(probeLocations);
	});

	var matrix_loader = new MatrixLoader();
	if (compressed)
	{
		matrix_loader.load(precompute_folder + "dictionary.matrix", function(dict_matrix){
			dict = makeTextureFromMatrix1(dict_matrix);
			setupApplyDictFramebuffer(dict_matrix.cols,1); 
		}, Float32Array);
	}
	var px_map_path = compressed ? "receiver_px_map_comp.imatrix":"receiver_px_map.imatrix" ;

	matrix_loader.load( precompute_folder +  px_map_path, function(px_map_mat){
		px_map = px_map_mat;
		create_gi_draw_call();
	}, Int32Array);

	if (compressed)
	{
		matrix_loader.load(precompute_folder +  "coeff_idx.imatrix", function(probe_indices_mat){
			probe_indices = probe_indices_mat;
			create_gi_draw_call();
		}, Int16Array);

		matrix_loader.load(precompute_folder +  "coeffs.matrix", function(dictionary_coeff_mat){
			dictionary_coeffs = dictionary_coeff_mat;
			create_gi_draw_call();
		}, Float32Array);
	}
	else
	{
		matrix_loader.load(precompute_folder +  "probe_indices.imatrix", function(probe_indices_mat){
			probe_indices=probe_indices_mat;
			create_gi_draw_call();
		}, Int16Array);
	}



	
	matrix_loader.load(precompute_folder +  "full_nz.matrix", function(full_mat){
		full_texture = makeTexturefromFloatArr(full_mat.col_major_data);
	}, Float32Array);
	
    initProbeToggleControls();
}

function initProbeToggleControls() {
    window.addEventListener('keydown', function(e) {
        if (e.keyCode === 80) { /* p */
            hideProbes = false;
            if (probeVisualizeUnlit) {
                probeVisualizeUnlit = false;
            } else {
                if (probeVisualizeMode === 'sh')
                    probeVisualizeMode = 'raw';
                else if (probeVisualizeMode === 'raw')
                    probeVisualizeMode = 'sh';
            }
        }
        if (e.keyCode === 85) { /* u */
			if (probeVisualizeUnlit && !hideProbes) {
				hideProbes = true;
			} else {
                probeVisualizeUnlit = true;
                hideProbes = false;
            }
        }
    });
}

function getProbeVisualizeModeString() {
    if (probeVisualizeUnlit) {
        return 'unlit';
    } else {
        return probeVisualizeMode;
    }
}

function createFullscreenVertexArray() {

	var positions = app.createVertexBuffer(PicoGL.FLOAT, 3, new Float32Array([
		-1, -1, 0,
		+3, -1, 0,
		-1, +3, 0
	]));

	var vertexArray = app.createVertexArray()
	.vertexAttributeBuffer(0, positions);

	return vertexArray;

}

function createSphereVertexArray(radius, rings, sectors) {

	var positions = [];

	var R = 1.0 / (rings - 1);
	var S = 1.0 / (sectors - 1);

	var PI = Math.PI;
	var TWO_PI = 2.0 * PI;

	for (var r = 0; r < rings; ++r) {
		for (var s = 0; s < sectors; ++s) {

			var y = Math.sin(-(PI / 2.0) + PI * r * R);
			var x = Math.cos(TWO_PI * s * S) * Math.sin(PI * r * R);
			var z = Math.sin(TWO_PI * s * S) * Math.sin(PI * r * R);

			positions.push(x * radius);
			positions.push(y * radius);
			positions.push(z * radius);

		}
	}

	var indices = [];

	for (var r = 0; r < rings - 1; ++r) {
		for (var s = 0; s < sectors - 1; ++s) {

			var i0 = r * sectors + s;
			var i1 = r * sectors + (s + 1);
			var i2 = (r + 1) * sectors + (s + 1);
			var i3 = (r + 1) * sectors + s;

			indices.push(i2);
			indices.push(i1);
			indices.push(i0);

			indices.push(i3);
			indices.push(i2);
			indices.push(i0);

		}
	}

	var positionBuffer = app.createVertexBuffer(PicoGL.FLOAT, 3, new Float32Array(positions));
	var indexBuffer = app.createIndexBuffer(PicoGL.UNSIGNED_SHORT, 3, new Uint16Array(indices));

	var vertexArray = app.createVertexArray()
	.vertexAttributeBuffer(0, positionBuffer)
	.indexBuffer(indexBuffer);

	return vertexArray;

}

function setupDirectionalLightShadowMapFramebuffer(size) {

	var colorBuffer = app.createTexture2D(size, size, {
		format: PicoGL.RED,
		internalFormat: PicoGL.R32F,
	});

	var depthBuffer = app.createTexture2D(size, size, {
		internalFormat: PicoGL.DEPTH_COMPONENT32F,	
		type: PicoGL.FLOAT,
		compareMode: PicoGL.COMPARE_REF_TO_TEXTURE,
		compareFunc: PicoGL.LEQUAL,
		minFilter: PicoGL.LINEAR,
		magFilter: PicoGL.LINEAR,
	});

	shadowMapFramebuffer = app.createFramebuffer()
	.colorTarget(0, colorBuffer)
	.depthTarget(depthBuffer);

}

var applyDictionaryFramebuffer;
function setupApplyDictFramebuffer(width, height)
{

	var colorBuffer = app.createTexture2D(width, height, {
		internalFormat: PicoGL.RGBA32F,
		minFilter: PicoGL.NEAREST,
		magFilter: PicoGL.NEAREST,
	});

	var depthBuffer = app.createTexture2D(width, height, {
		format: PicoGL.DEPTH_COMPONENT
	});

	applyDictionaryFramebuffer = app.createFramebuffer()
	.colorTarget(0, colorBuffer)
	.depthTarget(depthBuffer); 
	
	
}

var gilightMapFramebuffer = false;

function setupGILightmapFramebuffer(size) {

	var colorBuffer = app.createTexture2D(size, size, {
		internalFormat: PicoGL.RGBA16F,
		minFilter: PicoGL.LINEAR,
		magFilter: PicoGL.LINEAR,
		//wrapS:gl.CLAMP_TO_BORDER,
		//wrapT:gl.CLAMP_TO_BORDER,
	});
	// do we need to set border color -> 0? probs defult.
	// pico gl won't let us? :(

	// we don't need no depth texture.. are we allowed not to have one somehow?
	// not adding it causes it to be undefined...
	var depthBuffer = app.createTexture2D(size, size, {
		format: PicoGL.DEPTH_COMPONENT
	});

	gilightMapFramebuffer = app.createFramebuffer()
	.colorTarget(0, colorBuffer)
	.depthTarget(depthBuffer); 
}

var padDrawCall;
var padFBO;
function setupPaddingFbo(size)
{
	var colorBuffer = app.createTexture2D(size, size, {
		internalFormat: PicoGL.RGBA16F,
		minFilter: PicoGL.LINEAR,
		magFilter: PicoGL.LINEAR,
		//wrapS:gl.CLAMP_TO_BORDER,
		//wrapT:gl.CLAMP_TO_BORDER,
	});
	var depthBuffer = app.createTexture2D(size, size, {
		format: PicoGL.DEPTH_COMPONENT
	});

	padFBO = app.createFramebuffer()
	.colorTarget(0, colorBuffer)
	.depthTarget(depthBuffer);
}



function setupLightmapFramebuffer(size) {
	var colorBuffer = app.createTexture2D(size, size, {
		internalFormat: PicoGL.RGBA16F,
		minFilter: PicoGL.LINEAR,
		magFilter: PicoGL.LINEAR,
		//wrapS:gl.CLAMP_TO_BORDER,
		//wrapT:gl.CLAMP_TO_BORDER,
	});
	// do we need to set border color -> 0? probs defult.
	// pico gl won't let us? :(

	// we don't need no depth texture.. are we allowed not to have one somehow?
	// not adding it causes it to be undefined...
	var depthBuffer = app.createTexture2D(size, size, {
		format: PicoGL.DEPTH_COMPONENT
	});

	lightMapFramebuffer = app.createFramebuffer()
	.colorTarget(0, colorBuffer)
	.depthTarget(depthBuffer); 
}

function setupProbeRadianceFramebuffer() {

    var colorBuffer = app.createTexture2D(num_sh_coefficients, num_probes, {
        internalFormat: PicoGL.RGBA16F,
        minFilter: PicoGL.NEAREST,
        magFilter: PicoGL.NEAREST,
	});
    var depthBuffer = app.createTexture2D(num_sh_coefficients, num_probes, {
        format: PicoGL.DEPTH_COMPONENT
    });
    probeRadianceFramebuffer = app.createFramebuffer()
        .colorTarget(0, colorBuffer)
        .depthTarget(depthBuffer);
}

function setupSceneUniforms() {

	//
	// TODO: Fix all this! I got some weird results when I tried all this before but it should work...
	//

	sceneUniforms = app.createUniformBuffer([
		PicoGL.FLOAT_VEC4 /* 0 - ambient color */   //,
		//PicoGL.FLOAT_VEC4 /* 1 - directional light color */,
		//PicoGL.FLOAT_VEC4 /* 2 - directional light direction */,
		//PicoGL.FLOAT_MAT4 /* 3 - view from world matrix */,
		//PicoGL.FLOAT_MAT4 /* 4 - projection from view matrix */
	])
	.set(0, sceneSettings.ambientColor)
	//.set(1, directionalLight.color)
	//.set(2, directionalLight.direction)
	//.set(3, camera.viewMatrix)
	//.set(4, camera.projectionMatrix)
	.update();

/*
	camera.onViewMatrixChange = function(newValue) {
		sceneUniforms.set(3, newValue).update();
	};

	camera.onProjectionMatrixChange = function(newValue) {
		sceneUniforms.set(4, newValue).update();
	};
*/

}

function createVertexArrayFromMeshInfo(meshInfo) {

	var positions = app.createVertexBuffer(PicoGL.FLOAT, 3, meshInfo.positions);
	var normals   = app.createVertexBuffer(PicoGL.FLOAT, 3, meshInfo.normals);
	var tangents  = app.createVertexBuffer(PicoGL.FLOAT, 4, meshInfo.tangents);
	var texCoords = app.createVertexBuffer(PicoGL.FLOAT, 2, meshInfo.uvs);
	var lightmapCoords = app.createVertexBuffer(PicoGL.FLOAT, 2, meshInfo.uv2s);

	var vertexArray = app.createVertexArray()
	.vertexAttributeBuffer(0, positions)
	.vertexAttributeBuffer(1, normals)
	.vertexAttributeBuffer(2, texCoords)
	.vertexAttributeBuffer(3, tangents)
	.vertexAttributeBuffer(4, lightmapCoords);

	return vertexArray;

}

function createVertexArrayFromMeshInfoUV2(meshInfo) {

    var positions = app.createVertexBuffer(PicoGL.FLOAT, 3, meshInfo.positions);
    var normals   = app.createVertexBuffer(PicoGL.FLOAT, 3, meshInfo.normals);
    var tangents  = app.createVertexBuffer(PicoGL.FLOAT, 4, meshInfo.tangents);
    var texCoords = app.createVertexBuffer(PicoGL.FLOAT, 2, meshInfo.uvs);
    var lightmapCoords = app.createVertexBuffer(PicoGL.FLOAT, 2, meshInfo.uv2s);

    var vertexArray = app.createVertexArray()
        .vertexAttributeBuffer(0, positions)
        .vertexAttributeBuffer(1, normals)
        .vertexAttributeBuffer(2, texCoords)
        .vertexAttributeBuffer(3, tangents)
        .vertexAttributeBuffer(4, lightmapCoords);

    return vertexArray;

}

function createVertexArrayFromMeshInfo(meshInfo) {

    var positions = app.createVertexBuffer(PicoGL.FLOAT, 3, meshInfo.positions);
    var normals   = app.createVertexBuffer(PicoGL.FLOAT, 3, meshInfo.normals);
    var tangents  = app.createVertexBuffer(PicoGL.FLOAT, 4, meshInfo.tangents);
    var texCoords = app.createVertexBuffer(PicoGL.FLOAT, 2, meshInfo.uvs);

    var vertexArray = app.createVertexArray()
        .vertexAttributeBuffer(0, positions)
        .vertexAttributeBuffer(1, normals)
        .vertexAttributeBuffer(2, texCoords)
        .vertexAttributeBuffer(3, tangents);

    return vertexArray;

}



function setupProbeDrawCalls(vertexArray, unlitShader, probeVisualizeSHShader, probeVisualizeRawShader) {

	// We need at least one (x,y,z) pair to render any probes
	if (!probeLocations || probeLocations.length <= 3) {
		return;
	}

	if (probeLocations.length % 3 !== 0) {
		console.error('Probe locations invalid! Number of coordinates is not divisible by 3.');
		return;
	}

	num_probes = probeLocations.length / 3;

	// Set up for instanced drawing at the probe locations
	var translations = app.createVertexBuffer(PicoGL.FLOAT, 3, new Float32Array(probeLocations))
	vertexArray.instanceAttributeBuffer(10, translations);

	probeDrawCall = app.createDrawCall(unlitShader, vertexArray)
	.uniform('u_color', vec3.fromValues(0, 1, 0));

	probeVisualizeSHDrawCall = app.createDrawCall(probeVisualizeSHShader, vertexArray);
	probeVisualizeRawDrawCall = app.createDrawCall(probeVisualizeRawShader, vertexArray);
}

////////////////////////////////////////////////////////////////////////////////

function resize() {

	var w = window.innerWidth;
	var h = window.innerHeight;
	// w = 1920*0.8;
	// h = 1080*0.8;


	app.resize(w, h);
	camera.resize(w, h);

}

////////////////////////////////////////////////////////////////////////////////
// Rendering

function render() {
	var startStamp = new Date().getTime();

	stats.begin();
	picoTimer.start();


	var gl = picoTimer.gl;
	{

		if (settings["rotate_light"]) {
            // Rotate light
            vec3.rotateY(directionalLight.direction, directionalLight.direction, vec3.fromValues(0.0,0.0,0.0), 0.01);
        }


        camera.update();

        debugMoveDynamicObject();

		if (!bakedDirect) {
            renderShadowMap();
        }


		

		if(settings.redraw_global_illumination)
		{
			var lightmap = lightMapFramebuffer.colorTextures[0];
			renderProbeRadiance(relight_uvs_texture, relight_shs_texture, lightmap);
			renderLightmap();
			
			if(compressed)
			{
				render_apply_dictionary();
				render_gi();
			}
			else{
				render_gi_uncompressed();
			}
		}


        if (settings.lightmap_only) {
        	renderSceneTexturedByLightMap();
		} else {
            renderScene();
        }

		var viewProjection = mat4.mul(mat4.create(), camera.projectionMatrix, camera.viewMatrix);
		if (!hideProbes) {
            renderProbes(viewProjection, getProbeVisualizeModeString()); // 'unlit' | 'sh' | 'raw'
        }

		var inverseViewProjection = mat4.invert(mat4.create(), viewProjection);
		renderEnvironment(inverseViewProjection)

		// Call this to get a debug render of the passed in texture
		if(settings.view_lightmap){
			renderTextureToScreen(lightMapFramebuffer.colorTextures[0]);
		}
		
		if(gilightMapFramebuffer && settings.view_gi_lightmap)
		{
			renderTextureToScreen(gilightMapFramebuffer.colorTextures[0]);
		}

		if(gilightMapFramebuffer && settings.view_padded)
		{
			renderTextureToScreen(padFBO.colorTextures[0]);
		}

		if(applyDictionaryFramebuffer && settings.view_trans_pc)
		{
			renderTextureToScreen(applyDictionaryFramebuffer.colorTextures[0]);
		}
		if(settings.view_shadow_map)
		{
			renderTextureToScreen(shadowMapFramebuffer.colorTextures[0]);
		}
	}
	picoTimer.end();
	stats.end();

	if (picoTimer.ready()) {
		gpuTimePanel.update(picoTimer.gpuTime, 35);
	}

	var renderDelta = new Date().getTime() - startStamp;
	setTimeout( function() {
		requestAnimationFrame(render);
	}, 1000 / settings.target_fps - renderDelta-1000/120);

}

function shadowMapNeedsRendering() {

	var lastDirection = shadowMapNeedsRendering.lastDirection || vec3.create();
	var lastMeshCount = shadowMapNeedsRendering.lastMeshCount || 0;

	if (vec3.equals(lastDirection, directionalLight.direction) && lastMeshCount === meshes.length) {

		return false;

	} else {

		shadowMapNeedsRendering.lastDirection = vec3.copy(lastDirection, directionalLight.direction);
		shadowMapNeedsRendering.lastMeshCount = meshes.length;

		return true;

	}


}

function renderShadowMap() {

	if (!directionalLight) return;
	if (!shadowMapNeedsRendering()) return;

	var lightViewProjection = directionalLight.getLightViewProjectionMatrix();

	app.drawFramebuffer(shadowMapFramebuffer)
	.viewport(0, 0, shadowMapSize, shadowMapSize)
	.depthTest()
	.depthFunc(PicoGL.LEQUAL)
	.noBlend()
	.clear();

	for (var i = 0, len = meshes.length; i < len; ++i) {

		var mesh = meshes[i];

		mesh.shadowMapDrawCall
		.uniform('u_world_from_local', mesh.modelMatrix)
		.uniform('u_light_projection_from_world', lightViewProjection)
		.draw();

	}
}

function renderLightmap() {
	var dirLightViewDirection = directionalLight.viewSpaceDirection(camera);
	var lightViewProjection = directionalLight.getLightViewProjectionMatrix();
	var shadowMap = shadowMapFramebuffer.depthTexture;
	var lightMap;
	if(compressed) lightMap = gilightMapFramebuffer.colorTextures[0];
	else lightMap = padFBO.colorTextures[0];

	app.drawFramebuffer(lightMapFramebuffer)
	.viewport(0, 0, lightMapSize, lightMapSize)
	.noDepthTest()
	.noBlend()
	.clearColor(0,0,0)
	.drawBackfaces()
	.clear();

	for (var i = 0, len = staticMeshes.length; i < len; ++i) {
		var mesh = staticMeshes[i];
		if (bakedDirect) {
            mesh.lightmapDrawCall
                .uniform('u_world_from_local', mesh.modelMatrix)
                .uniform('u_view_from_world', camera.viewMatrix)
                .uniform('u_light_projection_from_world', lightViewProjection)
                .texture('u_light_map', lightMap)
                .texture('u_baked_direct', bakedDirect)
                .draw();
		} else {
            mesh.lightmapDrawCall
                .uniform('u_world_from_local', mesh.modelMatrix)
                .uniform('u_view_from_world', camera.viewMatrix)
                .uniform('u_dir_light_color', directionalLight.color)
                .uniform('u_dir_light_view_direction', dirLightViewDirection)
                .uniform('u_light_projection_from_world', lightViewProjection)
                .texture('u_shadow_map', shadowMap)
                .texture('u_light_map', lightMap)
                .draw();
        }
	}
}

function render_apply_dictionary()
{
	if(dict && applyDictionaryFramebuffer && probeRadianceFramebuffer && applyDictDrawCall)
	{
		app.drawFramebuffer(applyDictionaryFramebuffer)
		.viewport(0, 0, 1024, 1)
		.noDepthTest()
		.noBlend()
		.clearColor(0,0,0)
		.clear();

		applyDictDrawCall
		.texture('sigma_v', dict)
		.texture('sh_coeffs', probeRadianceFramebuffer.colorTextures[0])
		.draw();
	}
	
}

function render_gi()
{
	if(applyDictionaryFramebuffer && gilightMapFramebuffer && GIDrawCall)
	{
		app.drawFramebuffer(gilightMapFramebuffer)
		.viewport(0, 0, giLightMapSize, giLightMapSize)
		.noDepthTest()
		.noBlend()
		.clearColor(0,0,0)
		.clear();

		GIDrawCall
		.texture('dictionary', applyDictionaryFramebuffer.colorTextures[0])
		.draw();
	}
	
}

function render_gi_uncompressed()
{
	if(probeRadianceFramebuffer && full_texture && gilightMapFramebuffer)
	{
		app.drawFramebuffer(gilightMapFramebuffer)
		.viewport(0, 0, giLightMapSize, giLightMapSize)
		.noDepthTest()
		.noBlend()
		.clearColor(0,0,0)
		.clear();
		
		GIDrawCall
		.texture('probes_sh_coeffs',probeRadianceFramebuffer.colorTextures[0])
		.texture('rec_sh_coeffs', full_texture).draw();

		app.drawFramebuffer(padFBO)
		.viewport(0, 0, giLightMapSize, giLightMapSize)
		.noDepthTest()
		.noBlend()
		.clearColor(0,0,0)
		.clear();

		padDrawCall.texture('u_texture', gilightMapFramebuffer.colorTextures[0]).draw();
	}
}

function renderScene() {

	var dirLightViewDirection = directionalLight.viewSpaceDirection(camera);
	var lightViewProjection = directionalLight.getLightViewProjectionMatrix();
	var shadowMap = shadowMapFramebuffer.depthTexture;
	//var lightMap = lightMapFramebuffer.colorTextures[0];
	if(compressed)
	{
		if(gilightMapFramebuffer) lightMap = gilightMapFramebuffer.colorTextures[0];
	}
	else{
		if(padFBO) lightMap = padFBO.colorTextures[0];
	}


	app.defaultDrawFramebuffer()
	.defaultViewport()
	.depthTest()
	.depthFunc(PicoGL.LEQUAL)
	.noBlend()
	.clear();

	for (var i = 0, len = meshes.length; i < len; ++i) {
		var mesh = meshes[i];
		if (mesh.isDynamic) {
			mesh.drawCall
				.uniform('u_world_from_local', mesh.modelMatrix)
				.uniform('u_view_from_world', camera.viewMatrix)
				.uniform('u_projection_from_view', camera.projectionMatrix)
				.uniform('u_dir_light_color', directionalLight.color)
				.uniform('u_dir_light_view_direction', dirLightViewDirection)
				.uniform('u_light_projection_from_world', lightViewProjection)
				.texture('u_shadow_map', shadowMap)
                .texture('u_probe_sh_texture', probeRadianceFramebuffer.colorTextures[0])
				.draw();

		} else {
            if (bakedDirect) {
                mesh.drawCall
                    .uniform('u_world_from_local', mesh.modelMatrix)
                    .uniform('u_view_from_world', camera.viewMatrix)
                    .uniform('u_projection_from_view', camera.projectionMatrix)
                    .uniform('u_light_projection_from_world', lightViewProjection)
                    .texture('u_light_map', lightMap)
                    .texture('u_baked_direct', bakedDirect)
                    .draw();
            } else {
                mesh.drawCall
                    .uniform('u_world_from_local', mesh.modelMatrix)
                    .uniform('u_view_from_world', camera.viewMatrix)
                    .uniform('u_projection_from_view', camera.projectionMatrix)
                    .uniform('u_dir_light_color', directionalLight.color)
                    .uniform('u_dir_light_view_direction', dirLightViewDirection)
                    .uniform('u_light_projection_from_world', lightViewProjection)
                    .texture('u_shadow_map', shadowMap)
                    .texture('u_light_map', lightMap)
                    .draw();
            }
		}

	}
}

function renderSceneTexturedByLightMap() {
	if(!gilightMapFramebuffer)return;
    app.defaultDrawFramebuffer()
        .defaultViewport()
        .depthTest()
        .depthFunc(PicoGL.LEQUAL)
        .noBlend()
        .clear();

    for (var i = 0, len = staticMeshes.length; i < len; ++i) {
        var mesh = staticMeshes[i];
        mesh.texturedByLightmapDrawCall
            .uniform('u_world_from_local', mesh.modelMatrix)
            .uniform('u_view_from_world', camera.viewMatrix)
            .uniform('u_projection_from_view', camera.projectionMatrix)
            .texture('u_light_map', padFBO.colorTextures[0])
            .draw();
    }
}


function renderProbes(viewProjection, type) {

    app.defaultDrawFramebuffer()
        .defaultViewport()
        .depthTest()
        .depthFunc(PicoGL.LEQUAL)
        .noBlend();

    switch(type) {
        case 'unlit':
            if (probeDrawCall) {
                probeDrawCall
                    .uniform('u_projection_from_world', viewProjection)
                    .draw();
            }
            break;
        case 'sh':
            if (probeVisualizeSHDrawCall && probeRadianceFramebuffer) {
                probeVisualizeSHDrawCall
                    .uniform('u_projection_from_world', viewProjection)
                    .uniform('u_num_sh_coeffs_to_render', settings['num_sh_coeffs_to_render'])
                    .texture('u_probe_sh_texture', probeRadianceFramebuffer.colorTextures[0])
					.draw();
            }
            break;
        case 'raw':
            if (probeVisualizeRawDrawCall && lightMapFramebuffer && relight_dirs_texture && relight_uvs_texture) {
                probeVisualizeRawDrawCall
                    .uniform('u_projection_from_world', viewProjection)
                    .texture('u_relight_uvs_texture', relight_uvs_texture)
                    .texture('u_relight_dirs_texture', relight_dirs_texture)
					.texture('u_lightmap', lightMapFramebuffer.colorTextures[0])
                    .draw();
            }
            break;
    }


}

function renderEnvironment(inverseViewProjection) {

	if (environmentDrawCall) {

		app.defaultDrawFramebuffer()
		.defaultViewport()
		.depthTest()
		.depthFunc(PicoGL.EQUAL)
		.noBlend();

		environmentDrawCall
		.uniform('u_camera_position', camera.position)
		.uniform('u_world_from_projection', inverseViewProjection)
		.uniform('u_environment_brightness', settings.environment_brightness)
		.draw();

	}

}

function renderTextureToScreen(texture) {

	//
	// NOTE:
	//
	//   This function can be really helpful for debugging!
	//   Just call this whenever and you get the texture on
	//   the screen (just make sure nothing is drawn on top)
	//

	if (!blitTextureDrawCall) {
		return;
	}

	app.defaultDrawFramebuffer()
	.defaultViewport()
	.noDepthTest()
	.noBlend();

	blitTextureDrawCall
	.texture('u_texture', texture)
	.draw();

}

function renderProbeRadiance(relight_uvs_texture, relight_shs_texture, lightmap) {

    if (!probeRadianceDrawCall || !probeRadianceFramebuffer) {
        return;
    }

    app.drawFramebuffer(probeRadianceFramebuffer)
        .viewport(0, 0, num_sh_coefficients, num_probes)
        .noDepthTest()
        .noBlend()
        .clearColor(0,0,0)
        .clear();

    probeRadianceDrawCall
		.uniform('u_num_sh_coeffs_to_render', settings['num_sh_coeffs_to_render'])
        .texture('u_relight_uvs_texture', relight_uvs_texture)
        .texture('u_relight_shs_texture', relight_shs_texture)
		.texture('u_lightmap', lightmap)
        .draw();

}

////////////////////////////////////////////////////////////////////////////////
