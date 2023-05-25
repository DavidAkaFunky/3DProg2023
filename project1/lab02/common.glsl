/**
 * common.glsl
 * Common types and functions used for ray tracing.
 */

const float pi = 3.14159265358979;
const float epsilon = 0.001;

struct Ray {
    vec3 o;     // origin
    vec3 d;     // direction - always set with normalized vector
    float t;    // time, for motion blur
};

Ray createRay(vec3 o, vec3 d, float t)
{
    Ray r;
    r.o = o;
    r.d = d;
    r.t = t;
    return r;
}

Ray createRay(vec3 o, vec3 d)
{
    return createRay(o, d, 0.0);
}

vec3 pointOnRay(Ray r, float t)
{
    return r.o + r.d * t;
}

float gSeed = 0.0;

uint baseHash(uvec2 p)
{
    p = 1103515245U * ((p >> 1U) ^ (p.yx));
    uint h32 = 1103515245U * ((p.x) ^ (p.y>>3U));
    return h32 ^ (h32 >> 16);
}

float hash1(inout float seed) {
    uint n = baseHash(floatBitsToUint(vec2(seed += 0.1,seed += 0.1)));
    return float(n) / float(0xffffffffU);
}

vec2 hash2(inout float seed) {
    uint n = baseHash(floatBitsToUint(vec2(seed += 0.1,seed += 0.1)));
    uvec2 rz = uvec2(n, n * 48271U);
    return vec2(rz.xy & uvec2(0x7fffffffU)) / float(0x7fffffff);
}

vec3 hash3(inout float seed)
{
    uint n = baseHash(floatBitsToUint(vec2(seed += 0.1, seed += 0.1)));
    uvec3 rz = uvec3(n, n * 16807U, n * 48271U);
    return vec3(rz & uvec3(0x7fffffffU)) / float(0x7fffffff);
}

float rand(vec2 v)
{
    return fract(sin(dot(v.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 toLinear(vec3 c)
{
    return pow(c, vec3(2.2));
}

vec3 toGamma(vec3 c)
{
    return pow(c, vec3(1.0 / 2.2));
}

vec2 randomInUnitDisk(inout float seed) {
    vec2 h = hash2(seed) * vec2(1.0, 6.28318530718);
    float phi = h.y;
    float r = sqrt(h.x);
	return r * vec2(sin(phi), cos(phi));
}

vec3 randomInUnitSphere(inout float seed)
{
    vec3 h = hash3(seed) * vec3(2.0, 6.28318530718, 1.0) - vec3(1.0, 0.0, 0.0);
    float phi = h.y;
    float r = pow(h.z, 1.0/3.0);
	return r * vec3(sqrt(1.0 - h.x * h.x) * vec2(sin(phi), cos(phi)), h.x);
}

vec3 randomUnitVector(inout float seed) //to be used in diffuse reflections with distribution cosine
{
    return(normalize(randomInUnitSphere(seed)));
}

struct Camera
{
    vec3 eye;
    vec3 u, v, n;
    float width, height;
    float lensRadius;
    float planeDist, focusDist;
    float time0, time1;
};

Camera createCamera(
    vec3 eye,
    vec3 at,
    vec3 worldUp,
    float fovy,
    float aspect,
    float aperture,  //diametro em multiplos do pixel size
    float focusDist,  //focal ratio
    float time0,
    float time1)
{
    Camera cam;
    if(aperture == 0.0) cam.focusDist = 1.0; //pinhole camera then focus in on vis plane
    else cam.focusDist = focusDist;
    vec3 w = eye - at;
    cam.planeDist = length(w);
    cam.height = 2.0 * cam.planeDist * tan(fovy * pi / 180.0 * 0.5);
    cam.width = aspect * cam.height;

    cam.lensRadius = aperture * 0.5 * cam.width / iResolution.x;  //aperture ratio * pixel size; (1 pixel=lente raio 0.5)
    cam.eye = eye;
    cam.n = normalize(w);
    cam.u = normalize(cross(worldUp, cam.n));
    cam.v = cross(cam.n, cam.u);
    cam.time0 = time0;
    cam.time1 = time1;
    return cam;
}

Ray getRay(Camera cam, vec2 pixelSample)  //rnd pixelSample viewport coordinates
{
    vec2 ls = cam.lensRadius * randomInUnitDisk(gSeed);  //ls - lens sample for DOF
    float time = cam.time0 + hash1(gSeed) * (cam.time1 - cam.time0);
    
    vec3 aux = vec3(
        cam.width * (pixelSample.x / iResolution.x - 0.5),
		cam.height * (pixelSample.y / iResolution.y - 0.5),
		- cam.planeDist
    );

    vec3 focalPlaneSample = aux * cam.focusDist;

    vec3 eyeOffset = cam.eye + cam.u * ls.x + cam.v * ls.y;

    vec3 rayDirection = normalize(
        cam.u * (focalPlaneSample.x - ls.x) + 
        cam.v * (focalPlaneSample.y - ls.y) + 
        cam.n * focalPlaneSample.z
    );

    return createRay(eyeOffset, normalize(rayDirection), time);
}

// MT_ material type
#define MT_DIFFUSE 0
#define MT_METAL 1
#define MT_DIELECTRIC 2

struct Material
{
    int type;
    vec3 albedo;  //diffuse color
    vec3 specColor;  //the color tint for specular reflections. for metals and opaque dieletrics like coloured glossy plastic
    vec3 emissive; //
    float roughness; // controls roughness for metals. It can be used for rough refractions
    float refIdx; // index of refraction for dielectric
    vec3 refractColor; // absorption for beer's law
};

Material createDiffuseMaterial(vec3 albedo)
{
    Material m;
    m.type = MT_DIFFUSE;
    m.albedo = albedo;
    m.specColor = vec3(0.0);
    m.roughness = 1.0;  //ser usado na iluminação direta
    m.refIdx = 1.0;
    m.refractColor = vec3(0.0);
    m.emissive = vec3(0.0);
    return m;
}

Material createMetalMaterial(vec3 specClr, float roughness)
{
    Material m;
    m.type = MT_METAL;
    m.albedo = vec3(0.0);
    m.specColor = specClr;
    m.roughness = roughness;
    m.emissive = vec3(0.0);
    return m;
}

Material createDielectricMaterial(vec3 refractClr, float refIdx, float roughness)
{
    Material m;
    m.type = MT_DIELECTRIC;
    m.albedo = vec3(0.0);
    m.specColor = vec3(0.04);
    m.refIdx = refIdx;
    m.refractColor = refractClr;  
    m.roughness = roughness;
    m.emissive = vec3(0.0);
    return m;
}

struct HitRecord
{
    vec3 pos;
    vec3 normal;
    float t;            // ray parameter
    Material material;
};


float schlick(float cosine, float refIdx)
{
    // Assuming one of the media is air/vacuum (refIdx = 1.0)
    float r0 = pow((1.0 - refIdx) / (1.0 + refIdx), 2.0);
    return r0 + (1.0 - r0) * pow(1.0 - cosine, 5.0);
}

bool scatter(Ray rIn, HitRecord rec, out vec3 atten, out Ray rScattered)
{
    vec3 shadingNormal = (dot(-rIn.d, rec.normal) > 0.0) ? rec.normal : -rec.normal;
    vec3 bias = epsilon * shadingNormal;

    if(rec.material.type == MT_DIFFUSE)
    {
        rScattered = createRay(rec.pos + epsilon * shadingNormal, normalize(shadingNormal + randomUnitVector(gSeed)), rIn.t);
        atten = rec.material.albedo * max(dot(rScattered.d, shadingNormal), 0.0) / pi;
        return true;
    }
    
    if(rec.material.type == MT_METAL)
    {   
        vec3 reflected = reflect(rIn.d, shadingNormal);
        rScattered = createRay(rec.pos + bias, normalize(reflected + rec.material.roughness * randomInUnitSphere(gSeed)), rIn.t);
        atten = rec.material.specColor;
        return true;
    }

    if(rec.material.type == MT_DIELECTRIC)
    {   
        float cosThetaI = max(dot(-rIn.d, shadingNormal), 0.0); 
        vec3 tangentVec = rIn.d + shadingNormal * cosThetaI;
        float niOverNt;

        if(dot(-rIn.d, rec.normal) < 0.0) //hit inside
        {
            niOverNt = rec.material.refIdx;
            atten = exp(-rec.material.refractColor * rec.t); // Beer's law
        }
        else // hit outside
        {
            niOverNt = 1.0 / rec.material.refIdx;
            atten = vec3(1.0);   
        }

        float sinThetaT = length(tangentVec) * niOverNt;
        float reflectProb;

        if (sinThetaT > 1.0) {
            atten = rec.material.specColor;
            reflectProb = 1.0;  // Total internal reflection
        }
        else
            reflectProb = schlick(sqrt(1.0 - pow(sinThetaT, 2.0)), rec.material.refIdx);  

        if(hash1(gSeed) < reflectProb)  //Reflection
            rScattered = createRay(rec.pos + bias, reflect(rIn.d, shadingNormal), rIn.t);
        else  //Refraction
            rScattered = createRay(rec.pos - bias, refract(rIn.d, shadingNormal, niOverNt), rIn.t);

        return true;
    }
    
    return false;
}

struct Triangle {vec3 a; vec3 b; vec3 c; vec3 normal;};

Triangle createTriangle(vec3 v0, vec3 v1, vec3 v2)
{
    Triangle t;
    t.a = v0; t.b = v1; t.c = v2;
    t.normal = normalize(cross(v1 - v0, v2 - v0));
    return t;
}

bool hit_triangle(Triangle triangle, Ray r, float tmin, float tmax, out HitRecord rec)
{
    float vd = dot(r.d, triangle.normal);

    if (vd == 0.0) return false;

    float d = - dot(triangle.normal, triangle.a);
    float t = - (dot(triangle.normal, r.o) + d) / vd;

    if (t < 0.0 || t < tmin || t > tmax) return false;

    vec3 p = pointOnRay(r, t);
    vec3 c;

    vec3 edge0 = triangle.b - triangle.a;
    vec3 vp0 = p - triangle.a;
    c = cross(edge0, vp0);
    if (dot(triangle.normal, c) < 0.0) return false;

    vec3 edge1 = triangle.c - triangle.b;
    vec3 vp1 = p - triangle.b;
    c = cross(edge1, vp1);
    if (dot(triangle.normal, c) < 0.0) return false;

    vec3 edge2 = triangle.a - triangle.c;
    vec3 vp2 = p - triangle.c;
    c = cross(edge2, vp2);
    if (dot(triangle.normal, c) < 0.0) return false;

    rec.t = t;
    rec.normal = triangle.normal;
    rec.pos = p;
    return true;
}


struct Sphere
{
    vec3 center;
    float radius, sqRadius;
};

Sphere createSphere(vec3 center, float radius)
{
    Sphere s;
    s.center = center;
    s.radius = radius;
    s.sqRadius = radius * radius;
    return s;
}


struct MovingSphere
{
    vec3 center0, center1;
    float radius, sqRadius;
    float time0, time1;
    vec3 velocity;
};

MovingSphere createMovingSphere(vec3 center0, vec3 center1, float radius, float time0, float time1)
{
    MovingSphere s;
    s.center0 = center0;
    s.center1 = center1;
    s.radius = radius;
    s.sqRadius = radius * radius;
    s.time0 = time0;
    s.time1 = time1;
    s.velocity = (center1 - center0) / (time1 - time0);
    return s;
}

vec3 center(MovingSphere mvsphere, float time)
{
    return mvsphere.center0 + (time - mvsphere.time0) * mvsphere.velocity;
}

bool hit_genericSphere(vec3 center, float radius, float sqRadius, Ray r, float tmin, float tmax, out HitRecord rec)
{
    vec3 oc = center - r.o;
    float b = dot(r.d, oc);
    float c = dot(oc, oc) - sqRadius;

    if (c > 0.0 && b <= 0.0) return false;

    float delta = b * b - c;

    if (delta <= 0.0) return false;

    float sqrtDelta = sqrt(delta);

    float t = (c > 0.0) ? b - sqrtDelta : b + sqrtDelta;
    
    if (t < tmax && t > tmin) {
        rec.t = t;
        rec.pos = pointOnRay(r, t);
        rec.normal = normalize(rec.pos - center);
        return true;
    }
    return false;
}

/*
 * The function naming convention changes with these functions to show that they implement a sort of interface for
 * the book's notion of "hittable". E.g. hit_<type>.
 */

bool hit_sphere(Sphere s, Ray r, float tmin, float tmax, out HitRecord rec)
{
    return hit_genericSphere(s.center, s.radius, s.sqRadius, r, tmin, tmax, rec);
}

bool hit_movingSphere(MovingSphere s, Ray r, float tmin, float tmax, out HitRecord rec)
{
    return hit_genericSphere(center(s, r.t), s.radius, s.sqRadius, r, tmin, tmax, rec);;
}

struct pointLight {
    vec3 pos;
    vec3 color;
};

pointLight createPointLight(vec3 pos, vec3 color) 
{
    pointLight l;
    l.pos = pos;
    l.color = color;
    return l;
}