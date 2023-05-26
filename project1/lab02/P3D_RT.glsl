/**
* ver hash functions em
* https://www.shadertoy.com/view/XlGcRh hash functions GPU
* http://www.jcgt.org/published/0009/03/02/
 */

 #include "./common.glsl"
 #iChannel0 "self"

 // mouse camera control parameters
const float c_minCameraAngle = 0.01;
const float c_maxCameraAngle = 3.14 - 0.01;
const float c_cameraDistance = 20.0;

bool hit_world(Ray r, float tmin, float tmax, out HitRecord rec)
{
    bool hit = false;
    rec.t = tmax;
   
    if (hit_triangle(createTriangle(vec3(-10.0, -0.01, 10.0), vec3(10.0, -0.01, 10.0), vec3(-10.0, -0.01, -10.0)), r, tmin, rec.t, rec))
    {
        hit = true;
        rec.material = createDiffuseMaterial(vec3(0.2));
    }

    if (hit_triangle(createTriangle(vec3(-10.0, -0.01, -10.0), vec3(10.0, -0.01, 10), vec3(10.0, -0.01, -10.0)), r, tmin, rec.t, rec))
    {
        hit = true;
        rec.material = createDiffuseMaterial(vec3(0.2));
    }

    // GREEN SPHERE
    if (hit_sphere(
        createSphere(vec3(-4.0, 1.0, 0.0), 1.0),
        r,
        tmin,
        rec.t,
        rec))
    {
        hit = true;
        rec.material = createDiffuseMaterial(vec3(0.2, 0.95, 0.1));
        //rec.material = createDiffuseMaterial(vec3(0.4, 0.2, 0.1));
    }

    // ORANGE SPHERE
    if (hit_sphere(
        createSphere(vec3(4.0, 1.0, 0.0), 1.0),
        r,
        tmin,
        rec.t,
        rec))
    {
        hit = true;
        rec.material = createMetalMaterial(vec3(0.7, 0.6, 0.5), 0.5);
    }

    // GLASS SPHERE
    if (hit_sphere(
        createSphere(vec3(0.0, 1.0, 0.0), 1.0),
        r,
        tmin,
        rec.t,
        rec))
    {
        hit = true;
        rec.material = createDielectricMaterial(vec3(0.0), 1.333, 0.1);
    }

    // INNER GLASS SPHERE
    if (hit_sphere(
        createSphere(vec3(0.0, 1.0, 0.0), -0.45),
        r,
        tmin,
        rec.t,
        rec))
    {
        hit = true;
        rec.material = createDielectricMaterial(vec3(0.0), 1.333, 0.5);
    }
   
    int numxy = 5;
    
    for(int x = -numxy; x < numxy; ++x)
    {
        for(int y = -numxy; y < numxy; ++y)
        {
            float fx = float(x);
            float fy = float(y);
            float seed = fx + fy / 1000.0;
            vec3 rand1 = hash3(seed);
            vec3 center = vec3(fx + 0.9 * rand1.x, 0.2, fy + 0.9 * rand1.y);
            float chooseMaterial = rand1.z;
            if (distance(center, vec3(4.0, 0.2, 0.0)) > 0.9)
            {
                if (chooseMaterial < 0.3)
                {
                    vec3 center1 = center + vec3(0.0, hash1(gSeed) * 0.5, 0.0);
                    // diffuse
                    if (hit_movingSphere(
                        createMovingSphere(center, center1, 0.2, 0.0, 1.0),
                        r,
                        tmin,
                        rec.t,
                        rec))
                    {
                        hit = true;
                        rec.material = createDiffuseMaterial(hash3(seed) * hash3(seed));
                    }
                }
                else if (chooseMaterial < 0.5)
                {
                    // diffuse
                    if (hit_sphere(
                        createSphere(center, 0.2),
                        r,
                        tmin,
                        rec.t,
                        rec))
                    {
                        hit = true;
                        rec.material = createDiffuseMaterial(hash3(seed) * hash3(seed));
                    }
                }
                else if (chooseMaterial < 0.7)
                {
                    // metal
                    if (hit_sphere(
                        createSphere(center, 0.2),
                        r,
                        tmin,
                        rec.t,
                        rec))
                    {
                        hit = true;
                       // rec.material.type = MT_METAL;
                        rec.material = createMetalMaterial((hash3(seed) + 1.0) * 0.5, 0.0);
                    }
                }
                else if (chooseMaterial < 0.9)
                {
                    // metal
                    if (hit_sphere(
                        createSphere(center, 0.2),
                        r,
                        tmin,
                        rec.t,
                        rec))
                    {
                        hit = true;
                       // rec.material.type = MT_METAL;
                        rec.material = createMetalMaterial((hash3(seed) + 1.0) * 0.5, hash1(seed));
                    }
                }
                else
                {
                    // glass (dielectric)
                    if (hit_sphere(
                        createSphere(center, 0.2),
                        r,
                        tmin,
                        rec.t,
                        rec))
                    {
                        hit = true;
                        rec.material.type = MT_DIELECTRIC;
                        rec.material = createDielectricMaterial(hash3(seed), 1.2, 0.0);
                    }
                }
            }
        }
    }
    return hit;
}

vec3 directLighting(vec3 pos, vec3 color, Ray r, HitRecord rec){
    HitRecord dummy;

    vec3 shadingNormal = dot(-r.d, rec.normal) > 0.0 ? rec.normal : -rec.normal;

    vec3 lightDir = normalize(pos - rec.pos);
    Ray shadowRay = createRay(rec.pos + epsilon * shadingNormal, lightDir);
    if (hit_world(shadowRay, 0.0, length(pos - rec.pos), dummy))
        return vec3(0.0);

    Material material = rec.material;

    vec3 diffColor = vec3(0.0);
    float lightNormalDotProd = dot(lightDir, shadingNormal);
    if (lightNormalDotProd > 0.0) {
        diffColor = material.albedo * lightNormalDotProd;
    }

    vec3 specCol = vec3(0.0);
    float shininess = 4.0/(pow(material.roughness, 4.0) + epsilon) - 2.0;
    vec3 halfwayVec = normalize(lightDir - r.d);
    float halfNormalDotProd = dot(halfwayVec, shadingNormal);

    if (halfNormalDotProd > 0.0) {
        //specCol = vec3(1.0);
        specCol = material.specColor * pow(halfNormalDotProd, shininess);
    }

    return color * (diffColor + specCol);
}

vec3 directLighting(pointLight pl, Ray r, HitRecord rec){
    return directLighting(pl.pos, pl.color, r, rec);
}

vec3 directLighting(areaLight al, Ray r, HitRecord rec){
    vec3 color = vec3(0.0);
    for (int i = 0; i < al.numSamples; i++){
        color += directLighting(al.pos[i], al.color, r, rec);
    }
    return color / float(al.numSamples);
}

#define MAX_BOUNCES 10

vec3 rayColor(Ray r)
{
    HitRecord rec;
    vec3 col = vec3(0.0);
    vec3 throughput = vec3(1.0f, 1.0f, 1.0f);
    
    pointLight pointLights[3];
    pointLights[0] = createPointLight(vec3(-10.0, 15.0, 0.0), vec3(1.0, 1.0, 1.0));
    pointLights[1] = createPointLight(vec3(8.0, 15.0, 3.0), vec3(1.0, 1.0, 1.0));
    pointLights[2] = createPointLight(vec3(1.0, 15.0, -9.0), vec3(1.0, 1.0, 1.0));

    // areaLight areaLights[3];
    // areaLights[0] = createAreaLight(vec3(-10.0, 15.0, 0.0), 3.0, 3.0, vec3(1.0, 1.0, 1.0));
    // areaLights[1] = createAreaLight(vec3(8.0, 15.0, 3.0), 3.0, 3.0, vec3(1.0, 1.0, 1.0));
    // areaLights[2] = createAreaLight(vec3(1.0, 15.0, -9.0), 3.0, 3.0, vec3(1.0, 1.0, 1.0));

    for(int i = 0; i < MAX_BOUNCES; ++i)
    {
        if (hit_world(r, 0.0, 10000.0, rec))
        {
            //calculate direct lighting with 3 white point lights:
            for (int i = 0; i < pointLights.length(); ++i)
                col += directLighting(pointLights[i], r, rec) * throughput;

            // for (int i = 0; i < areaLights.length(); ++i)
            //     col += directLighting(areaLights[i], r, rec) * throughput;
           
            //calculate secondary ray and update throughput
            Ray scatterRay;
            vec3 atten;
            if (scatter(r, rec, atten, scatterRay)) //scatterRay is the secondary ray
            {
                throughput *= atten;
                r = scatterRay;
            }
        }
        else  //background
        {
            float t = 0.8 * (r.d.y + 1.0);
            col += throughput * mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
            break;
        }
    }
    return col;
}

#define MAX_SAMPLES 10000.0

void main()
{
    bool orbitCam = false;
    gSeed = float(baseHash(floatBitsToUint(gl_FragCoord.xy))) / float(0xffffffffU) + iTime;

    vec2 mouse = iMouse.xy / iResolution.xy;
    mouse.x = mouse.x * 2.0 - 1.0;

    vec3 camPos;
    if (orbitCam){
        float angleX = -mouse.x * 16.0f;
        float angleY = mix(c_minCameraAngle, c_maxCameraAngle, mouse.y);
        
        camPos = vec3(
            sin(angleX) * sin(angleY) * c_cameraDistance,
            -cos(angleY) * c_cameraDistance,
            cos(angleX) * sin(angleY) * c_cameraDistance
        );
    } else {
        camPos = vec3(mouse.x * 10.0, mouse.y * 5.0, 8.0);
    }
    vec3 camUp = vec3(0.0, 1.0, 0.0);
    vec3 camTarget = vec3(0.0, 0.0, -1.0);
    float fovy = 60.0;
    float aperture = 10.0;
    float distToFocus = 1.0;
    float time0 = 0.0;
    float time1 = 1.0;
    Camera cam = createCamera(
       camPos,
       camTarget,
       camUp,    // world up vector
       fovy,
       iResolution.x / iResolution.y,
       aperture,
       distToFocus,
       time0,
       time1
    );

    //usa-se o 4 canal de cor para guardar o numero de samples e não o iFrame pois quando se mexe o rato faz-se reset

    vec4 prev = texture(iChannel0, gl_FragCoord.xy / iResolution.xy);
    vec3 prevLinear = toLinear(prev.xyz);  

    vec2 ps = gl_FragCoord.xy + hash2(gSeed); 
    //vec2 ps = gl_FragCoord.xy;
    vec3 color = rayColor(getRay(cam, ps));

    if (iMouseButton.x != 0.0 || iMouseButton.y != 0.0)
    {
        gl_FragColor = vec4(toGamma(color), 1.0);  //samples number reset = 1
        return;
    }
    if (prev.w > MAX_SAMPLES)   
    {
        gl_FragColor = prev;
        return;
    }

    float w = prev.w + 1.0;
    color = mix(prevLinear, color, 1.0/w);
    gl_FragColor = vec4(toGamma(color), w);
}
