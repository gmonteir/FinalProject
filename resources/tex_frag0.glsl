#version 330 core
uniform sampler2D Texture0;

in vec2 vTexCoord;
in vec3 fragNor;
in vec3 fragPos;
in vec3 eyePos;
out vec4 Outcolor;

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
};

struct DirLight {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
};

vec3 CalcDirLight(DirLight light, vec3 normal)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 ambient  = light.ambient  * vec3(texture(Texture0, vTexCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(Texture0, vTexCoord));
    return (ambient + diffuse);
}  

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragP)
{
    vec3 lightDir = normalize(light.position - fragP);
    float diff = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position - fragP);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    

    vec3 ambient = light.ambient * vec3(texture(Texture0, vTexCoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(Texture0, vTexCoord));
    ambient *= attenuation;
    diffuse *= attenuation;
    return (ambient + diffuse);
} 

void main() {
    vec3 norm = normalize(fragNor);
    vec3 viewDir = normalize(eyePos - fragPos);

    DirLight dirLight = { vec3(1, 1, 1), vec3(0, 0, 0), vec3(1, 1, 1) };

    PointLight pointLights[9] = {
        { vec3(-25, 5, -25), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(-25, 5, 25), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(25, 5, -25), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(25, 5, 25), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(-50, 5, 0), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(0, 5, -50), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(50, 5, 0), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(0, 5, 50), 1.0, 0.0014, 0.000007, vec3(0, 0, 0), vec3(1, 1, 1) },
        { vec3(0, 5, 0), 1.0, 0.35, 0.44, vec3(0, 0, 0), vec3(1, 1, 1) }
    };

    vec3 result = CalcDirLight(dirLight, norm);

    for (int i = 0; i < 9; i++)
    {
        result += CalcPointLight(pointLights[i], norm, fragPos);
    }
    
    Outcolor = vec4(result, 1.0);
}