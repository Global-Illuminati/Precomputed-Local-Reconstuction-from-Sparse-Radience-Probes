
function DirectionalLight(direction, color) {

	//this.direction = direction || vec3.fromValues(0.3, -1.0, 5.3);
	//this.direction = direction || vec3.fromValues(0.3, -1.0, 0.3);
	// this.direction = direction || vec3.fromValues(-0.2, -1.0, 0.333);
	this.direction = direction || vec3.fromValues(-0.155185, -0.221726, 0.962681);
	vec3.normalize(this.direction, this.direction);
	var s = 20.0;
	this.color = color || new Float32Array([1.0*s, 0.803*s, 0.43*s]);
	// s = 1.0;
	// this.color = color || new Float32Array([1.0*s, 1.0*s, 1.0*s]);
	
	//this.color = color || new Float32Array([2.0, 2.0, 2.0]);

	//

	this.orthoProjectionSize = 50.0;

	this.lightViewMatrix = mat4.create();
	this.lightProjectionMatrix = mat4.create();
	this.lightViewProjection = mat4.create();

}

DirectionalLight.prototype = {

	constructor: DirectionalLight,

	viewSpaceDirection: function(camera) {

		var inverseRotation = quat.conjugate(quat.create(), camera.orientation);

		var result = vec3.create();
		vec3.transformQuat(result, this.direction, inverseRotation);

		return result;

	},

	getLightViewMatrix: function() {

		// Calculate as a look-at matrix from center to the direction (interpreted as a point)
		var eyePosition = vec3.fromValues(0, 0, 0);
		var up          = vec3.fromValues(0, 1, 0);

		mat4.lookAt(this.lightViewMatrix, eyePosition, this.direction, up);

		return this.lightViewMatrix;

	},

	getLightProjectionMatrix: function() {

		var size = this.orthoProjectionSize / 2.0;
		mat4.ortho(this.lightProjectionMatrix, -size*0.5, size*0.5, -size*0.2, size*0.3, -size*0.5, size*0.5);
		// mat4.ortho(this.lightProjectionMatrix, -size, size, -size, size, -size, size);

		return this.lightProjectionMatrix;

	},

	getLightViewProjectionMatrix: function () {

		var lightViewMatrix = this.getLightViewMatrix();
		var lightProjMatrix = this.getLightProjectionMatrix();
		mat4.multiply(this.lightViewProjection, lightProjMatrix, lightViewMatrix);

		return this.lightViewProjection;

	}

};