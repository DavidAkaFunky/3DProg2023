# Ray Tracer - Whitted/Distributed Naive Ray Tracer 

## How to test all funcionalities

### Anti-Aliasing

- Set the number of `spp > 0` in p3f file.

### Soft Shadows

- In p3f file, light is defined as `l pos_x pos_y pos_z width height spl color_R color_G color_B`;
- `width`: length in X axis;
- `height`: lenght in Z axis;
- `spl`: number of samples in the area light (ignored if `spp > 0`). If set to `0`, light is calculated as a point light in the specified position.

### Depth of Field

- Set `aperture > 0` in p3f file.

### Fuzzy Reflections

- Set material's last parameter to be higher than `0`.
