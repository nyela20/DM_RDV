#include <tuple>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cmath>


struct vec3 {

    //float x=0, y=0, z=0;
	float x, y, z;
          float& operator[](const int i)       { return i==0 ? x : (1==i ? y : z); }
    const float& operator[](const int i) const { return i==0 ? x : (1==i ? y : z); }
    vec3  operator*(const float v) const { return {x*v, y*v, z*v};       }
    float operator*(const vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    vec3  operator+(const vec3& v) const { return {x+v.x, y+v.y, z+v.z}; }
    vec3  operator-(const vec3& v) const { return {x-v.x, y-v.y, z-v.z}; }
    vec3  operator-()              const { return {-x, -y, -z};          }
    float norm() const { return std::sqrt(x*x+y*y+z*z); }
    vec3 normalized() const { return (*this)*(1.f/norm()); }
};

vec3 cross(const vec3 v1, const vec3 v2) {
    return { v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x };
}

struct Material {
	/*float refractive_index = 1;
	float albedo[4] = { 2,0,0,0 };
	vec3 diffuse_color = { 0,0,0 };
	float specular_exponent = 0;*/

	float refractive_index;
	float albedo[4] ;
	vec3 diffuse_color ;
	float specular_exponent ;
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;
    
    Sphere(const vec3& c, float r, const Material& m) : center(c), radius(r), material(m) {}
};

constexpr Material      ivory = {1.0, {0.9,  0.5, 0.1, 0.0}, {0.4, 0.4, 0.3},   50.};
constexpr Material      white = {1.0, {0.9,  0.5, 0.0, 0.0}, {1.0, 1.0, 1.0},   50.};
constexpr Material      black = {1.0, {0.9,  0.5, 0.0, 0.0}, {0.0, 0.0, 0.0},   50.};
constexpr Material      brown = {1.0, {0.4,  0.0, 0.0, 0.0}, {0.5, 0.0, 0.0},   50.};
constexpr Material      glass = {1.5, {0.0,  0.9, 0.1, 0.8}, {0.6, 0.7, 0.8},  125.};
constexpr Material      red_rubber = {1.0, {1.4,  0.3, 0.0, 0.0}, {0.3, 0.1, 0.1},   10.};
constexpr Material      mirror = {1.0, {0.0, 16.0, 0.8, 0.0}, {1.0, 1.0, 1.0}, 1425.};


std::vector<Sphere> spheres = {
    {{0,    -1,   -16}, 3,      white},
    {{0, 2.5, -15}, 2,      white},
    {{0, 3, -13.425}, 0.5,      brown},
    {{0, 2, -13.425}, 0.5,      brown},
    {{0, 5, -14}, 1.5, white},
    {{-0.5, 4.5, -11}, 0.125, black},
    {{0.5, 4.5, -11}, 0.125, black}
};

struct Light {
    vec3 position;
    vec3 intensity;
};

constexpr Light lights[] = {
    {vec3{-20, 20,  20}, vec3{3., 3., 3.}},
    {vec3{ 30, 50, -25}, vec3{3., 3., 3.}},
    {vec3{ 30, 20,  30}, vec3{3., 3., 3.}}
};

void init(){
    float dVert = 0.01f;
    float dHori = 0.05f;
    float dZ    = 0.005f;
    int nbS = 30;
    //hands
    for(int i = 0; i < nbS; i++) {
        Sphere s1 = {{ 2.f + i * dHori, 2.5f + i * dVert, -15.f - i * dZ}, 0.1f, black};
        Sphere s2 = {{-2.f - i * dHori, 2.5f + i * dVert, -15.f - i * dZ}, 0.1f, black};
        spheres.push_back(s1);
        spheres.push_back(s2);
    }
    Sphere leftHand  = {{ 2.5f + nbS * dHori, 2.5f + nbS * dVert, -15.f }, 0.5f, ivory};
    Sphere rightHand = {{-2.5f - nbS * dHori, 2.5f + nbS * dVert, -15.f }, 0.5f, ivory};
    spheres.push_back(leftHand);
    spheres.push_back(rightHand);
    
    int nbs = 10;
    float dVert2 = 0.003f;
    float dHori2 = 0.03f;
    //smile
    for(int j = 0; j< nbs; j++) {
        Sphere s1 = {{ 0.f + j * dHori2, 3.7f + j * dVert2, -11.f }, 0.1f, red_rubber};
        Sphere s2 = {{ 0.f - j * dHori2, 3.7f + j * dVert2, -11.f }, 0.1f, red_rubber};
        spheres.push_back(s1);
        spheres.push_back(s2);
    }
}

//Attenuation of light
constexpr float kC = 1.0;  // constant
constexpr float kL = 0.04; // lineaire
constexpr float kQ = 0.0008; // quadratic

vec3 reflect(const vec3 &I, const vec3 &N) {
    return I - N*2.f*(I*N);
}

vec3 refract(const vec3 &I, const vec3 &N, const float eta_t, const float eta_i=1.f) { // Snell's law
    float cosi = - std::max(-1.f, std::min(1.f, I*N));
    if (cosi<0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
    float eta = eta_i / eta_t;
    float k = 1 - eta*eta*(1 - cosi*cosi);
    return k<0 ? vec3{1,0,0} : I*eta + N*(eta*cosi - std::sqrt(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}

std::tuple<bool,float> ray_sphere_intersect(const vec3 &orig, const vec3 &dir, const Sphere &s) { // ret value is a pair [intersection found, distance]
    vec3 L = s.center - orig;
    float tca = L*dir;
    float d2 = L*L - tca*tca;
    if (d2 > s.radius*s.radius) return {false, 0};
    float thc = std::sqrt(s.radius*s.radius - d2);
    float t0 = tca-thc, t1 = tca+thc;
    if (t0>.001) return {true, t0};  // offset the original point by .001 to avoid occlusion by the object itself
    if (t1>.001) return {true, t1};
    return {false, 0};
}

std::tuple<bool,vec3,vec3,Material> scene_intersect(const vec3 &orig, const vec3 &dir) {
    vec3 pt, N;
    //Material material;
	Material material = { 1,{ 2,0,0,0 },{ 0,0,0 },0 };

    float nearest_dist = 1e10;
    if (std::abs(dir.y)>.001) { // intersect the ray with the checkerboard, avoid division by zero
        float d = -(orig.y+4)/dir.y; // the checkerboard plane has equation y = -4
        vec3 p = orig + dir*d;
        if (d>.001 && d<nearest_dist && std::abs(p.x)<10 && p.z<-10 && p.z>-30) {
            nearest_dist = d;
            pt = p;
            N = {0,1,0};
            material.diffuse_color = (int(.5f*pt.x+1000) + int(.5f*pt.z)) & 1 ? vec3{.3f, .3f, .3f} : vec3{.0f, .0f, .0f};
        }
    }

    for (const Sphere &s : spheres) { // intersect the ray with all spheres
       // auto  [intersection, d] = ray_sphere_intersect(orig, dir, s);
		auto tuplevalue = ray_sphere_intersect(orig, dir, s);
		auto intersection =std::get<0>(tuplevalue);
		auto d = std::get<1>(tuplevalue);
        if (!intersection || d > nearest_dist) continue;
        nearest_dist = d;
        pt = orig + dir*nearest_dist;
        N = (pt - s.center).normalized();
        material = s.material;
    }
    return { nearest_dist<1000, pt, N, material };
}

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const int depth=0) {
    //auto [hit, point, N, material] = scene_intersect(orig, dir);
	auto tuplevalue = scene_intersect(orig, dir);
	auto hit = std::get<0>(tuplevalue);
	auto point = std::get<1>(tuplevalue);
	auto N = std::get<2>(tuplevalue);
	auto material = std::get<3>(tuplevalue);
    if (depth>4 || !hit)
        return {0.5f, 0.5f, 0.5f}; // background color

    vec3 reflect_dir = reflect(dir, N).normalized();
    vec3 refract_dir = refract(dir, N, material.refractive_index).normalized();
    vec3 reflect_color = cast_ray(point, reflect_dir, depth + 1);
    vec3 refract_color = cast_ray(point, refract_dir, depth + 1);
    vec3 ambient_light = {0.2f, 0.2f, 0.2f};
    vec3 ambient_component = {1.0f * ambient_light.x, 1.0f * ambient_light.y, 1.0f * ambient_light.z};


    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (const Light &light : lights) { // checking if the point lies in the shadow of the light
        vec3 light_dir = (light.position - point).normalized();
        float light_distance = (light.position - point).norm();
        float attenuation = 1.0 / (kC + kL * light_distance + kQ * light_distance * light_distance);
        const float dShadow = 1e-3;// to avoid self-shadowing
        auto shadow_ray_orig = (light_dir*N) < 0 ? point - N*dShadow : point + N*dShadow;
        //auto [hit, shadow_pt, trashnrm, trashmat] = scene_intersect(point, light_dir);
		auto tuplevalue = scene_intersect(shadow_ray_orig, light_dir);
		auto hit = std::get<0>(tuplevalue);
		auto shadow_pt = std::get<1>(tuplevalue);
		auto trashnrm = std::get<2>(tuplevalue);
		auto trashmat = std::get<3>(tuplevalue);
        if (hit && (shadow_pt-shadow_ray_orig).norm() < light_distance) continue;
        diffuse_light_intensity  += std::max(0.f, light_dir*N) * attenuation * light.intensity.x;
        vec3 reflect_dir = reflect(-light_dir, N);
        specular_light_intensity += std::pow(std::max(0.f, reflect_dir.normalized()*dir.normalized()), material.specular_exponent) * attenuation * light.intensity.x;
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + ambient_component + vec3{1., 1., 1.}*specular_light_intensity * material.albedo[1] + reflect_color*material.albedo[2] + refract_color*material.albedo[3];
}

int main() {
    constexpr int   width  = 1024;
    constexpr int   height = 768;
    constexpr float fov    = 1.05; // 60 degrees field of view in radians
    std::vector<vec3> framebuffer(width*height);
    init();
#pragma omp parallel for
    for (int pix = 0; pix<width*height; pix++) { // actual rendering loop
        float dir_x =  (pix%width + 0.5) -  width/2.;
        float dir_y = -(pix/width + 0.5) + height/2.; // this flips the image at the same time
        float dir_z = -height/(2.*tan(fov/2.));
        framebuffer[pix] = cast_ray(vec3{0,0,0}, vec3{dir_x, dir_y, dir_z}.normalized());
    }

    std::ofstream ofs("./out.ppm", std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (vec3 &color : framebuffer) {
        float max = std::max(1.f, std::max(color[0], std::max(color[1], color[2])));
        for (int chan : {0,1,2})
            ofs << (char)(255 *  color[chan]/max);
    }
    return 0;
}
