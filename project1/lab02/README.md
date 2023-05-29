# Ray Tracer - GLSL Path Tracer

## How to test all funcionalities

### Soft Shadows

- In line 15 of `P3D_RT.glsl`, set `softShadows = true`.

### Depth of Field

- In main, set the camera aperture value higher than `0.0`.

### Fuzzy Reflections / Refractions

- Set roughness parameter to a value higher than `0.0` in methods `createDielectricMaterial` and `createMetalMaterial`.

### Moving Camera

- Inside the `main` function (line 280 of `P3D_RT.glsl`), set `orbitCam = true`.