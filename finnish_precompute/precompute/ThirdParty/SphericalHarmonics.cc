/*
* This file is part of ANUChem.
*
*  This file is licensed to You under the Eclipse Public License (EPL);
*  You may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*      http://www.opensource.org/licenses/eclipse-1.0.php
*
* (C) Copyright IBM Corporation 2014.
* (C) Copyright Mahidol University International College 2014.
*/
#include <cstdlib>
#include <cmath>
#include <cassert>

#define PVT(l,m) ((m)+((l)*((l)+1))/2)
#define YVR(l,m) ((m)+(l)+((l)*(l)))

class SphericalHarmonics {
public:
	explicit SphericalHarmonics(int LL);
	~SphericalHarmonics();

	/* Compute an entire set of Y_{l,m}(\theta,\phi) and store in array Y */
	void computeY(const int L, const float z, const float x, const float y, float* const Y);

private:
	float* A;  // coefficients A for P_l^m
	float* B;  // coefficients B for P_l^m
	float* P;
};
SphericalHarmonics::SphericalHarmonics(int L) {
	int sizeP = (L + 1) * (L + 2) / 2;
	P = new float[sizeP];
	A = new float[sizeP];
	B = new float[sizeP];
}

SphericalHarmonics::~SphericalHarmonics() {
	delete[] P;
	delete[] A;
	delete[] B;
}


/* Compute an entire set of Y_{l,m}(\theta,\phi) and store in the array Y */
void computeY(const int L, const float z,
	const float x, const float y, float* const Y) {
	int sizeP = (L + 1) * (L + 2) / 2;
	float *P = new float[sizeP];
	float *A = new float[sizeP];
	float *B = new float[sizeP];
	
	for (int l = 2; l <= L; l++) {
		float ls = l * l;
		float lm1s = (l - 1) * (l - 1);
		for (int m = 0; m < l - 1; m++) {
			float ms = m * m;
			A[PVT(l, m)] = sqrt((4 * ls - 1.) / (ls - ms));
			B[PVT(l, m)] = -sqrt((lm1s - ms) / (4 * lm1s - 1.));
		}
	}

	const float sintheta = vec2(x,y).norm();
	float temp = 0.39894228040143267794;  // = sqrt(0.5/M_PI)
	P[PVT(0, 0)] = temp;

	const float SQRT3 = 1.7320508075688772935;
	P[PVT(1, 0)] = z * SQRT3 * temp;
	const float SQRT3DIV2 = -1.2247448713915890491;
	temp = SQRT3DIV2 * sintheta * temp;
	P[PVT(1, 1)] = temp;

	for (int l = 2; l <= L; l++) {
		for (int m = 0; m < l - 1; m++) {
			P[PVT(l, m)] = A[PVT(l, m)]
				* (z * P[PVT(l - 1, m)]
					+ B[PVT(l, m)] * P[PVT(l - 2, m)]);
		}
		P[PVT(l, l - 1)] = z * sqrt(2 * (l - 1) + 3) * temp;
		temp = -sqrt(1.0 + 0.5 / l) * sintheta * temp;
		P[PVT(l, l)] = temp;
	}

	for (int l = 0; l <= L; l++)
		Y[YVR(l, 0)] = P[PVT(l, 0)] * 0.5 * sqrt(2);

	float c1 = 1.0, c2 = x/sintheta;
	float s1 = 0.0, s2 = -y/sintheta;
	float tc = 2.0 * c2;
	for (int m = 1; m <= L; m++) {
		float s = tc * s1 - s2;
		float c = tc * c1 - c2;
		s2 = s1;
		s1 = s;
		c2 = c1;
		c1 = c;
		for (int l = m; l <= L; l++) {
			Y[YVR(l, -m)] = P[PVT(l, m)] * s;
			Y[YVR(l, m)] = P[PVT(l, m)] * c;
		}
	}
}
