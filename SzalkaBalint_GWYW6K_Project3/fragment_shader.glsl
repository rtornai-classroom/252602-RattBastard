#version 330 core

in  vec3 FragPos;
in  vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform int  lightOn;

void main()
{
    vec3 objectColor = vec3(1.0, 1.0, 1.0);
    float ambientStrength = 0.10;
    vec3  ambient = ambientStrength * objectColor;
    vec3 result = ambient;

    if (lightOn == 1) {
        vec3 norm     = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);

        float dist        = length(lightPos - FragPos);
        float attenuation = 1.0 / (1.0 + 0.04 * dist + 0.003 * dist * dist);

        float diff    = max(dot(norm, lightDir), 0.0);
        vec3  diffuse = diff * lightColor * objectColor;

        vec3  viewDir  = normalize(viewPos - FragPos);
        vec3  halfDir  = normalize(lightDir + viewDir);
        float spec     = pow(max(dot(norm, halfDir), 0.0), 48.0);
        vec3  specular = 0.35 * spec * lightColor;

        result += (diffuse + specular) * attenuation;
    }

    FragColor = vec4(result, 1.0);
}
