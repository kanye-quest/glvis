R"(
struct Material {
    vec4 specular;
    float shininess;
};

struct PointLight { 
    vec3 position;
    vec4 diffuse;
    vec4 specular;
};

uniform int numLights; 
uniform PointLight lights[3]; 
uniform vec4 g_ambient;

uniform Material material;

vec4 blinnPhong(in vec3 pos, in vec3 norm, in vec4 color) {
    //calculate ambient
    vec4 lit_color = g_ambient * color;
    for (int i = 0; i < 3; i++)
    {
        if (i >= numLights)
            break;
        vec3 light_dir = normalize(lights[i].position - pos);
        
        //calculate diffuse
        lit_color += lights[i].diffuse * color * max(dot(norm, light_dir), 0.0);

        //calculate specular
        vec3 half_v = normalize(light_dir - pos);
        float specular_factor = max(dot(half_v, norm), 0.0);
        lit_color += lights[i].specular * material.specular * pow(specular_factor, material.shininess);
    }
    return lit_color;
}
)"
