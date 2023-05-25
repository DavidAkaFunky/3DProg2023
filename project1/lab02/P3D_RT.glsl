/**
* ver hash functions em
* https://www.shadertoy.com/view/XlGcRh hash functions GPU
* http://www.jcgt.org/published/0009/03/02/
 */

 #include "./common.glsl"
 #iChannel0 "self"

 // mouse camera control parameters
const float c_minCameraAngle = 0.01f;
const float c_maxCameraAngle = (3.14 - 0.01f);
const vec3 c_cameraAt = vec3(0.0f, 0.0f, 20.0f);
const float c_cameraDistance = 20.0f;
 
void GetCameraVectors(out vec3 cameraPos, out vec3 cameraFwd, out vec3 cameraUp, out vec3 cameraRight)
{
    // if the mouse is at (0,0) it hasn't been moved yet, so use a default camera setup
    vec2 mouse = iMouse.xy;
    if (dot(mouse, vec2(1.0f, 1.0f)) == 0.0f)
    {
        cameraPos = vec3(0.0f, 0.0f, -c_cameraDistance);
        cameraFwd = vec3(0.0f, 0.0f, 1.0f);
        cameraUp = vec3(0.0f, 1.0f, 0.0f);
        cameraRight = vec3(1.0f, 0.0f, 0.0f);
        return;
    }
     
    // otherwise use the mouse position to calculate camera position and orientation
     
    float angleX = -mouse.x * 16.0f / float(iResolution.x);
    float angleY = mix(c_minCameraAngle, c_maxCameraAngle, mouse.y / float(iResolution.y));
     
    cameraPos.x = sin(angleX) * sin(angleY) * c_cameraDistance;
    cameraPos.y = -cos(angleY) * c_cameraDistance;
    cameraPos.z = cos(angleX) * sin(angleY) * c_cameraDistance;
     
    cameraPos += c_cameraAt;
     
    cameraFwd = normalize(c_cameraAt - cameraPos);
    cameraRight = normalize(cross(vec3(0.0f, 1.0f, 0.0f), cameraFwd));
    cameraUp = normalize(cross(cameraFwd, cameraRight));   
}

bool hit_world(Ray r, float tmin, float tmax, out HitRecord rec)
{
    bool hit = false;
    rec.t = tmax;
   
    if(hit_triangle(createTriangle(vec3(-10.0, -0.01, 10.0), vec3(10.0, -0.01, 10.0), vec3(-10.0, -0.01, -10.0)), r, tmin, rec.t, rec))
    {
        hit = true;
        rec.material = createDiffuseMaterial(vec3(0.2));
    }

    if(hit_triangle(createTriangle(vec3(-10.0, -0.01, -10.0), vec3(10.0, -0.01, 10), vec3(10.0, -0.01, -10.0)), r, tmin, rec.t, rec))
    {
        hit = true;
        rec.material = createDiffuseMaterial(vec3(0.2));
    }

    // GREEN SPHERE
    if(hit_sphere(
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
    if(hit_sphere(
        createSphere(vec3(4.0, 1.0, 0.0), 1.0),
        r,
        tmin,
        rec.t,
        rec))
    {
        hit = true;
        rec.material = createMetalMaterial(vec3(0.7, 0.6, 0.5), 0.0);
    }

    // GLASS SPHERE
    if(hit_sphere(
        createSphere(vec3(0.0, 1.0, 0.0), 1.0),
        r,
        tmin,
        rec.t,
        rec))
    {
        hit = true;
        rec.material = createDielectricMaterial(vec3(0.0), 1.333, 0.0);
    }

    // INNER GLASS SPHERE
    if(hit_sphere(
        createSphere(vec3(0.0, 1.0, 0.0), -0.45),
        r,
        tmin,
        rec.t,
        rec))
    {
        hit = true;
        rec.material = createDielectricMaterial(vec3(0.0), 1.333, 0.0);
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
            if(distance(center, vec3(4.0, 0.2, 0.0)) > 0.9)
            {
                if(chooseMaterial < 0.3)
                {
                    vec3 center1 = center + vec3(0.0, hash1(gSeed) * 0.5, 0.0);
                    // diffuse
                    if(hit_movingSphere(
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
                else if(chooseMaterial < 0.5)
                {
                    // diffuse
                    if(hit_sphere(
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
                else if(chooseMaterial < 0.7)
                {
                    // metal
                    if(hit_sphere(
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
                else if(chooseMaterial < 0.9)
                {
                    // metal
                    if(hit_sphere(
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
                    if(hit_sphere(
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

vec3 directLighting(pointLight pl, Ray r, HitRecord rec){
    HitRecord dummy;

    vec3 shadingNormal = dot(-r.d, rec.normal) > 0.0 ? rec.normal : -rec.normal;

    vec3 lightDir = normalize(pl.pos - rec.pos);
    Ray shadowRay = createRay(rec.pos + epsilon * shadingNormal, lightDir);
    if (hit_world(shadowRay, 0.0, length(pl.pos - rec.pos), dummy))
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

    return pl.color * (diffColor + specCol);
}

#define MAX_BOUNCES 10

vec3 rayColor(Ray r)
{
    HitRecord rec;
    vec3 col = vec3(0.0);
    vec3 throughput = vec3(1.0f, 1.0f, 1.0f);
    
    pointLight lights[3];
    lights[0] = createPointLight(vec3(-10.0, 15.0, 0.0), vec3(1.0, 1.0, 1.0));
    lights[1] = createPointLight(vec3(8.0, 15.0, 3.0), vec3(1.0, 1.0, 1.0));
    lights[2] = createPointLight(vec3(1.0, 15.0, -9.0), vec3(1.0, 1.0, 1.0));

    for(int i = 0; i < MAX_BOUNCES; ++i)
    {
        if(hit_world(r, 0.0, 10000.0, rec))
        {
            //calculate direct lighting with 3 white point lights:
            {
                for (int i = 0; i < lights.length(); ++i)
                    col += directLighting(lights[i], r, rec) * throughput;
            }
           
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

Camera createMainCamera(vec2 mouse){
    vec3 camPos = vec3(mouse.x * 10.0, mouse.y * 5.0, 8.0);
    vec3 camTarget = vec3(0.0, 0.0, -1.0);
    float fovy = 60.0;
    float aperture = 0.0;
    float distToFocus = 1.0;
    float time0 = 0.0;
    float time1 = 1.0;
    Camera cam = createCamera(
       camPos,
       camTarget,
       vec3(0.0, 1.0, 0.0),    // world up vector
       fovy,
       iResolution.x / iResolution.y,
       aperture,
       distToFocus,
       time0,
       time1);
    return cam;
}

// Camera createAltCamera(vec2 mouse){
//     float fovy = 60.0;
//     vec2 jitter = randomUnitVector(gSeed).xy - 0.5f;

//     vec3 cameraPos, cameraFwd, cameraUp, cameraRight;
//     GetCameraVectors(cameraPos, cameraFwd, cameraUp, cameraRight); 
//     vec3 rayDir;
    
//     // calculate a screen position from -1 to +1 on each axis
//     vec2 uvJittered = (gl_FragCoord.xy + jitter)/iResolution.xy;
//     vec2 screen = uvJittered * 2.0f - 1.0f;
    
//     // adjust for aspect ratio
//     float aspectRatio = iResolution.x / iResolution.y;
//     screen.y /= aspectRatio;
            
//     // make a ray direction based on camera orientation and field of view angle
//     float cameraDistance = tan(fovy * 0.5f * 3.14 / 180.0f);       
//     rayDir = vec3(screen, cameraDistance);
//     rayDir = normalize(mat3(cameraRight, cameraUp, cameraFwd) * rayDir);
//     
// }

void main()
{
    gSeed = float(baseHash(floatBitsToUint(gl_FragCoord.xy))) / float(0xffffffffU) + iTime;

    vec2 mouse = iMouse.xy / iResolution.xy;
    mouse.x = mouse.x * 2.0 - 1.0;

    Camera cam = createMainCamera(mouse);
    // Camera altCam = createAltCamera(mouse);    

    //usa-se o 4 canal de cor para guardar o numero de samples e não o iFrame pois quando se mexe o rato faz-se reset

    vec4 prev = texture(iChannel0, gl_FragCoord.xy / iResolution.xy);
    vec3 prevLinear = toLinear(prev.xyz);  

    vec2 ps = gl_FragCoord.xy + hash2(gSeed);
    //vec2 ps = gl_FragCoord.xy;
    vec3 color = rayColor(getRay(cam, ps));

    if(iMouseButton.x != 0.0 || iMouseButton.y != 0.0)
    {
        gl_FragColor = vec4(toGamma(color), 1.0);  //samples number reset = 1
        return;
    }
    if(prev.w > MAX_SAMPLES)   
    {
        gl_FragColor = prev;
        return;
    }

    float w = prev.w + 1.0;
    color = mix(prevLinear, color, 1.0/w);
    gl_FragColor = vec4(toGamma(color), w);
}
