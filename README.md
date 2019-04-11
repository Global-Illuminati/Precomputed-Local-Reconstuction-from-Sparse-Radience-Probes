# Precomputed-Local-Reconstuction-from-Sparse-Radience-Probes


Implementation of: https://users.aalto.fi/~silvena4/Projects/RTGI/index.html



This is part of a bachelor project for evaluation of different techniques for real time global illuimination in web browsers. 

This project will not likely be updated any further. Compared to the paper most things are implemented however Clustered TSVD is not. There's (limited) support for dynamic objects. The dynamic objects only changes the direct light transportation but is of course affected by the indirect light (through spatial interpolation of the probes radiance fields followed by convolution with the rotated cosine lobe). 

For higher quality results the lightmaping issues must be fixed and additionally windowing should be applied to the spherical harmonics.

Compared to the paper we've changed how the receivers are interpolated. Instead of each probe having a fixed support radius we use a radius per receiver. This allows all receivers to interpolate between roughly the same number of probes which reduces the average needed and thus reduce the memory footprint, and improves precompute- and real-time performance. We've not seen this change to cause any loss of quality but a closer investigation into the subject is needed for anything conclusive. 

For detailed information please see our thesis which can be found ![here](https://github.com/Global-Illuminati/Real-Time-Global-Illumination-in-Web-Browsers)

Here's a screenshot from the application where a scene is illuminated by a small patch of direct light.
![Results](Images/image.png?raw=true "Screenshot from the application")
