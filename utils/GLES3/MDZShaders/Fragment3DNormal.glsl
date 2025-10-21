#version 300 es
precision mediump float;

in vec4 v_color;
in vec3 v_normal;
in vec3 v_fragPos;
in vec3 v_lightPos;

uniform vec3 u_lightColor;
uniform vec3 u_viewPos;

out vec4 outColor;

void main()
{
    // ambient
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * u_lightColor;
    
    // diffuse
    vec3 norm = normalize(v_normal);
    vec3 lightDir = normalize(v_lightPos - v_fragPos);
    float diff = pow(max(dot(norm, lightDir), 0.0),2.0);
    vec3 diffuse = diff * u_lightColor * 0.5;
    
    //specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(-v_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * u_lightColor;
    
    //vec3 result = (ambient + diffuse + specular) * v_color.rgb;
    vec3 result = (ambient + diffuse ) * v_color.rgb + specular;
    //result = v_color.rgb;
    outColor = vec4(result, 1.0);
}
