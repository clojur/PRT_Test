#version 330 core
out vec4 FragColor;
uniform vec3 lightColor;
void main()
{
    vec3 Color = lightColor / (lightColor + vec3(1.0));
    //Color = pow(Color, vec3(1.0/2.2)); 
    FragColor = vec4(Color, 1.0); // set alle 4 vector values to 1.0
}