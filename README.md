# Precomputed-Local-Reconstuction-from-Sparse-Radience-Probes


implementation of: https://users.aalto.fi/~silvena4/Projects/RTGI/index.html




## RealTime:

- [x] Render Lightmap:
	- Input: Scene
	- Output lightmap

- [ ]  Render Probes Spherical Harmonics:
	- Input: 100\*16 shs_coeffs, ~80\*100 relight uvs, lightmap
	- Output: 16 floats for each probe (4 rgba32f textures)

- [ ]  Transform to principal component basis (apply SIGMA_VT matrix)
	- Input: 16 floats for each probe (~80), U matrix as a texture probably (~80 * ~16)
	- Output: 16 floats for each principal component (~16)

- [ ]  Calculate GI (apply U matrix):
	- Input: PC-probes 16: probes, 16 components for each, SIGMA_VT Matrix as 16\*16 components for each pixel, quite sparse maybe store with indirection? dependent texture lookups are quite slow though?
	- Output: GI_lightmap 

- [x]  Render Scene with GI:
	- Input: Scene, GI_lightmap
  - Output: Pretty, pretty scene


