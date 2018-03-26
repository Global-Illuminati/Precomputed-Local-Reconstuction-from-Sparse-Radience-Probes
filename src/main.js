'using strict';

////////////////////////////////////////////////////////////////////////////////

var stats;
var gui;

var settings = {
	target_fps: 60,
	environment_brightness: 1.5,
    num_sh_coeffs_to_render: 16
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

var blitTextureDrawCall;
var environmentDrawCall;

var sceneUniforms;

var shadowMapSize = 4096;
var shadowMapFramebuffer;

var lightMapSize = 1024;
var lightMapFramebuffer;

var probeRadianceFramebuffer;
var probeRadianceDrawCall;

var camera;
var directionalLight;
var meshes = [];

var probeDrawCall;
var probeVisualizeSHDrawCall;
var probeVisualizeRawDrawCall;
var probeLocations = [
	-10, 4,  0,
	+10, 4,  0,
	-10, 14, 0,
	+10, 14, 0
]
var probeVisualizeMode = 'raw';
var probeVisualizeUnlit = true;

var num_probes;
var num_relight_rays;
var num_sh_coefficients;
var relight_uvs;
var relight_uvs_texture;
var relight_shs;
var relight_shs_texture;
var relight_dirs;
var relight_dirs_texture;
var sigma_v_texture;

var u_texture;
var px_map_vao;

var calcGIShader;
var transformPCProbesDrawCall;
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

function loadTexture(imageName, options) {

	if (!options) {

		var options = {};
		options['minFilter'] = PicoGL.LINEAR_MIPMAP_NEAREST;
		options['magFilter'] = PicoGL.LINEAR;
		options['mipmaps'] = true;

	}

	var texture = app.createTexture2D(1, 1, options);
	texture.data(new Uint8Array([200, 200, 200, 256]));

	var image = document.createElement('img');
	image.onload = function() {

		texture.resize(image.width, image.height);
		texture.data(image);

	};
	image.src = 'assets/' + imageName;
	return texture;

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
    image_data = new Float32Array(relight_uvs.reduce( (a,b) => a.concat(b)).map( x => x==-1?-1:x/lightMapSize));
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

function makeTextureFromMatrix1(matrix) {
	var options = {};
	options['minFilter'] = PicoGL.NEAREST;
	options['magFilter'] = PicoGL.NEAREST;
	options['mipmaps'] = false;
	options['format'] = PicoGL.RED;
	options['internalFormat'] = PicoGL.R32F;
	options['type'] = PicoGL.FLOAT;		
	return app.createTexture2D(matrix.col_major_data, matrix.cols, matrix.rows, options);
}

function makeTexture4096fromFloatArr(data) {
	var options = {};
	options['minFilter'] = PicoGL.NEAREST;
	options['magFilter'] = PicoGL.NEAREST;
	options['mipmaps'] = false;
	options['format'] = PicoGL.RGBA;
	options['internalFormat'] = PicoGL.RGBA32F;
	options['type'] = PicoGL.FLOAT;

	// @ROBUSTNESS, spec requires support of only 1024x1024 but if the exist on my laptop maybe it's fine?
	//              if the iphone 4s supports it maybe it should be fine for most pcs?
	// 				also in the long term we'd like to compress the data better anyway, 200meg is a bit much for my taste
	// 				and they get away with 80meg in the paper so we'll probably do what they do anyway
	// 				but for now, here we are.

	var aligned_length = (data.length/4 + 4095) & ~4095;
	console.log(data.length/4);
	image_data = new Float32Array(aligned_length*4);
	console.log('height:',aligned_length>>12,'length:',data.length,'aligned length:',aligned_length);
	image_data.set(data);
	return app.createTexture2D(image_data, 4096, aligned_length >> 12,  options);
}

function makeShader(name, shaderLoaderData) {

	var programData = shaderLoaderData[name];
	var program = app.createProgram(programData.vertexSource, programData.fragmentSource);
	return program;

}

function loadObject(directory, objFilename, mtlFilename, modelMatrix) {

	var objLoader = new OBJLoader();
	var mtlLoader = new MTLLoader();

	var path = 'assets/' + directory;

	objLoader.load(path + objFilename, function(objects) {
		mtlLoader.load(path + mtlFilename, function(materials) {
			objects.forEach(function(object) {

				var material = materials[object.material];
				var diffuseMap  = (material.properties.map_Kd)   ? directory + material.properties.map_Kd   : 'default_diffuse.png';
				var specularMap = (material.properties.map_Ks)   ? directory + material.properties.map_Ks   : 'default_specular.jpg';
				var normalMap   = (material.properties.map_norm) ? directory + material.properties.map_norm : 'default_normal.jpg';

				var vertexArray = createVertexArrayFromMeshInfo(object);

				var drawCall = app.createDrawCall(defaultShader, vertexArray)
				.uniformBlock('SceneUniforms', sceneUniforms)
				.texture('u_diffuse_map', loadTexture(diffuseMap))
				.texture('u_specular_map', loadTexture(specularMap))
				.texture('u_normal_map', loadTexture(normalMap));

				var lightMappingDrawCall = app.createDrawCall(lightMapShader, vertexArray)
				.uniformBlock('SceneUniforms', sceneUniforms)
				.texture('u_diffuse_map', loadTexture(diffuseMap))
				.texture('u_specular_map', loadTexture(specularMap))
				.texture('u_normal_map', loadTexture(normalMap));
				
				var shadowMappingDrawCall = app.createDrawCall(shadowMapShader, vertexArray);

				meshes.push({
					modelMatrix: modelMatrix || mat4.create(),
					drawCall: drawCall,
					shadowMapDrawCall: shadowMappingDrawCall,
					lightmapDrawCall: lightMappingDrawCall
				});

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

	//////////////////////////////////////
	// Basic GL state

	app.clearColor(0, 0, 0, 1);
	app.cullBackfaces();
	app.noBlend();

	//////////////////////////////////////
	// Camera stuff

	var cameraPos = vec3.fromValues(-15, 3, 0);
	var cameraRot = quat.fromEuler(quat.create(), 15, -90, 0);
	camera = new Camera(cameraPos, cameraRot);

	//////////////////////////////////////
	// Scene setup

	directionalLight = new DirectionalLight();
	setupDirectionalLightShadowMapFramebuffer(shadowMapSize);
	setupLightmapFramebuffer(lightMapSize);
	setupGILightmapFramebuffer(lightMapSize);

	setupSceneUniforms();

	var shaderLoader = new ShaderLoader('src/shaders/');
	shaderLoader.addShaderFile('common.glsl');
	shaderLoader.addShaderFile('scene_uniforms.glsl');
	shaderLoader.addShaderFile('mesh_attributes.glsl');
	shaderLoader.addShaderProgram('unlit', 'unlit.vert.glsl', 'unlit.frag.glsl');
	shaderLoader.addShaderProgram('default', 'default.vert.glsl', 'default.frag.glsl');
	shaderLoader.addShaderProgram('default_lm', 'default_lm.vert.glsl', 'default_lm.frag.glsl');
	shaderLoader.addShaderProgram('environment', 'environment.vert.glsl', 'environment.frag.glsl');
	shaderLoader.addShaderProgram('textureBlit', 'screen_space.vert.glsl', 'texture_blit.frag.glsl');
	shaderLoader.addShaderProgram('shadowMapping', 'shadow_mapping.vert.glsl', 'shadow_mapping.frag.glsl');
	shaderLoader.addShaderProgram('lightMapping', 'direct_lightmap.vert.glsl', 'direct_lightmap.frag.glsl');
	shaderLoader.addShaderProgram('calc_gi', 'calc_gi.vert.glsl', 'calc_gi.frag.glsl');
	shaderLoader.addShaderProgram('transform_pc_probes', 'screen_space.vert.glsl', 'transform_pc_probes.frag.glsl');

    shaderLoader.addShaderProgram('probeRadiance', 'probe_radiance.vert.glsl', 'probe_radiance.frag.glsl');
    shaderLoader.addShaderProgram('probeVisualizeSH', 'probe_visualize_sh.vert.glsl', 'probe_visualize_sh.frag.glsl');
    shaderLoader.addShaderProgram('probeVisualizeRaw', 'probe_visualize_raw.vert.glsl', 'probe_visualize_raw.frag.glsl');

	shaderLoader.load(function(data) {

		var fullscreenVertexArray = createFullscreenVertexArray();

		var textureBlitShader = makeShader('textureBlit', data);
		blitTextureDrawCall = app.createDrawCall(textureBlitShader, fullscreenVertexArray);

		var environmentShader = makeShader('environment', data);
		environmentDrawCall = app.createDrawCall(environmentShader, fullscreenVertexArray)
		.texture('u_environment_map', loadTexture('environments/ocean.jpg', {}));

		var probeVertexArray = createSphereVertexArray(0.20, 32, 32);
        var unlitShader = makeShader('unlit', data);
        var probeVisualizeSHShader = makeShader('probeVisualizeSH', data);
        var probeVisualizeRawShader = makeShader('probeVisualizeRaw', data);
        setupProbeDrawCalls(probeVertexArray, unlitShader, probeVisualizeSHShader, probeVisualizeRawShader);

		defaultShader = makeShader('default_lm', data);
		shadowMapShader = makeShader('shadowMapping', data);
		lightMapShader = makeShader('lightMapping', data);
		
		var transformPCProbesShader = makeShader('transform_pc_probes', data);
		calcGIShader = makeShader('calc_gi', data);
		if(px_map_vao) GIDrawCall =app.createDrawCall(calcGIShader, px_map_vao, PicoGL.POINTS);
		
		transformPCProbesDrawCall = app.createDrawCall(transformPCProbesShader,fullscreenVertexArray);

        var probeRadianceShader = makeShader('probeRadiance', data);
        probeRadianceDrawCall = app.createDrawCall(probeRadianceShader, fullscreenVertexArray);
		
		loadObject('sponza/', 'sponza.obj_2xuv', 'sponza.mtl');

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

    var suffix = "_100"; //"";
    var relight_uvs_dir = "assets/precompute/relight_uvs" + suffix + ".dat";
    var relight_shs_dir = "assets/precompute/relight_shs" + suffix + ".dat";
    var relight_directions_dir = "assets/precompute/relight_directions" + suffix + ".dat";


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



	dat_loader.load("assets/precompute/probes.dat", function(value) {
		probeLocations = value.reduce( (a,b) => a.concat(b) );
	});

	var matrix_loader = new MatrixLoader();

	matrix_loader.load("assets/precompute/sigma_v.matrix", function(sigma_v){
		sigma_v_texture = makeTextureFromMatrix1(sigma_v);
		console.log(sigma_v);
		// console.log(sigma_v.width,sigma_v.height);
		setupTransformPCFramebuffer(16,16); // num pc components * num shs 
	}, Float32Array);

	matrix_loader.load("assets/precompute/u.matrix", function(u_mat){
		u_texture = makeTexture4096fromFloatArr(u_mat.col_major_data);
		console.log(u_mat);
	}, Float32Array);

	matrix_loader.load("assets/precompute/receiver_px_map.imatrix", function(px_map_mat){
		px_map_vao = createGIVAO(px_map_mat);
		if(calcGIShader) GIDrawCall = app.createDrawCall(calcGIShader,px_map_vao,PicoGL.POINTS);
	}, Int32Array);

    initProbeToggleControls();
}

function initProbeToggleControls() {
    window.addEventListener('keydown', function(e) {
        if (e.keyCode === 80) { /* p */
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
            probeVisualizeUnlit = true;
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
		internalFormat: PicoGL.R8,
		minFilter: PicoGL.NEAREST,
		magFilter: PicoGL.NEAREST
	});

	var depthBuffer = app.createTexture2D(size, size, {
		format: PicoGL.DEPTH_COMPONENT
	});

	shadowMapFramebuffer = app.createFramebuffer()
	.colorTarget(0, colorBuffer)
	.depthTarget(depthBuffer);

}

var transformPCFramebuffer;
function setupTransformPCFramebuffer(width, height)
{

	var colorBuffer = app.createTexture2D(width, height, {
		internalFormat: PicoGL.RGBA32F,
		minFilter: PicoGL.NEAREST,
		magFilter: PicoGL.NEAREST,
	});

	var depthBuffer = app.createTexture2D(width, height, {
		format: PicoGL.DEPTH_COMPONENT
	});

	transformPCFramebuffer = app.createFramebuffer()
	.colorTarget(0, colorBuffer)
	.depthTarget(depthBuffer); 
	
	
}


var GIlightMapFramebuffer = false;

function setupGILightmapFramebuffer(size) {

	var colorBuffer = app.createTexture2D(size, size, {
		internalFormat: PicoGL.RGBA8,
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



function setupLightmapFramebuffer(size) {

	var colorBuffer = app.createTexture2D(size, size, {
		internalFormat: PicoGL.RGBA8,
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
        internalFormat: PicoGL.RGBA8,
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



function createGIVAO(px_map) {
	var buf_px_map  = app.createVertexBuffer(PicoGL.INT, 2, px_map.col_major_data);
	var vertexArray = app.createVertexArray()
	.vertexIntegerAttributeBuffer(0, buf_px_map);
	return vertexArray;
}

function setupProbeDrawCalls(vertexArray, unlitShader, probeVisualizeSHShader, probeVisualizeRawShader) {

	// We need at least one (x,y,z) pair to render any probes
	if (probeLocations.length <= 3) {
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

	app.resize(w, h);
	camera.resize(w, h);

}

////////////////////////////////////////////////////////////////////////////////
// Rendering

function render() {
	var startStamp = new Date().getTime();

	stats.begin();
	picoTimer.start();
	{
		camera.update();

		renderShadowMap();
		renderLightmap();
		

        var lightmap = lightMapFramebuffer.colorTextures[0];
        renderProbeRadiance(relight_uvs_texture, relight_shs_texture, lightmap);

		render_probe_pc_transfrom();
		render_gi();

		renderScene();

		 var viewProjection = mat4.mul(mat4.create(), camera.projectionMatrix, camera.viewMatrix);
		 renderProbes(viewProjection, getProbeVisualizeModeString()); // 'unlit' | 'sh' | 'raw'

		 var inverseViewProjection = mat4.invert(mat4.create(), viewProjection);
		 renderEnvironment(inverseViewProjection)

		

		// Call this to get a debug render of the passed in texture
		// renderTextureToScreen(lightMapFramebuffer.colorTextures[0]);
		
        if (probeRadianceFramebuffer) {
			 // renderTextureToScreen(probeRadianceFramebuffer.colorTextures[0])
		}
		if(transformPCFramebuffer)
		{
			//renderTextureToScreen(transformPCFramebuffer.colorTextures[0]);
		}
		if(gilightMapFramebuffer)
		{
			// renderTextureToScreen(gilightMapFramebuffer.colorTextures[0]);
		}
		if(u_texture)
		{
			// renderTextureToScreen(u_texture);
		}

	}
	picoTimer.end();
	stats.end();

	if (picoTimer.ready()) {
		gpuTimePanel.update(picoTimer.gpuTime, 35);
	}

	//requestAnimationFrame(render);

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

	app.drawFramebuffer(lightMapFramebuffer)
	.viewport(0, 0, lightMapSize, lightMapSize)
	.noDepthTest()
	.noBlend()
	.clearColor(0,0,0)
	.drawBackfaces()
	.clear();

	for (var i = 0, len = meshes.length; i < len; ++i) {
		var mesh = meshes[i];
		mesh.lightmapDrawCall
		.uniform('u_world_from_local', mesh.modelMatrix)
		.uniform('u_view_from_world', camera.viewMatrix)
		.uniform('u_dir_light_color', directionalLight.color)
		.uniform('u_dir_light_view_direction', dirLightViewDirection)
		.uniform('u_light_projection_from_world', lightViewProjection)
		.texture('u_shadow_map', shadowMap)
		.draw();
	}
}

function render_probe_pc_transfrom()
{
	if(sigma_v_texture && transformPCFramebuffer && probeRadianceFramebuffer && transformPCProbesDrawCall)
	{
		app.drawFramebuffer(transformPCFramebuffer)
		.viewport(0, 0, 16, 16)
		.noDepthTest()
		.noBlend()
		.clearColor(0,0,0)
		.clear();

		transformPCProbesDrawCall
		.texture('sigma_vt', sigma_v_texture)
		.texture('sh_coeffs', probeRadianceFramebuffer.colorTextures[0])
		.draw();
	}
	
}

function render_gi()
{
	if(transformPCFramebuffer && u_texture && gilightMapFramebuffer)
	{
		app.drawFramebuffer(gilightMapFramebuffer)
		.viewport(0, 0, lightMapSize, lightMapSize)
		.noDepthTest()
		.noBlend()
		.clearColor(0,0,0)
		.clear();
		
		GIDrawCall
		.texture('pc_sh_coeffs',transformPCFramebuffer.colorTextures[0])
		.texture('rec_sh_coeffs', u_texture).draw();
	}
	
}

function renderScene() {

	var dirLightViewDirection = directionalLight.viewSpaceDirection(camera);
	var lightViewProjection = directionalLight.getLightViewProjectionMatrix();
	var shadowMap = shadowMapFramebuffer.depthTexture;
	//var lightMap = lightMapFramebuffer.colorTextures[0];
	if(gilightMapFramebuffer) lightMap = gilightMapFramebuffer.colorTextures[0];
	app.defaultDrawFramebuffer()
	.defaultViewport()
	.depthTest()
	.depthFunc(PicoGL.LEQUAL)
	.noBlend()
	.clear();

	for (var i = 0, len = meshes.length; i < len; ++i) {
		var mesh = meshes[i];
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
				console.log('drawing sh');
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
        .texture('u_relight_uvs_texture', relight_uvs_texture)
        .texture('u_relight_shs_texture', relight_shs_texture)
		.texture('u_lightmap', lightmap)
        .draw();

}

////////////////////////////////////////////////////////////////////////////////
