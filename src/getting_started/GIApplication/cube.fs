#version 330 core
out vec4 FragColor;

const float PI = 3.1415926535f;
uniform int bands;

struct Material {
    //sampler2D diffuse;
    //sampler2D specular; 
    vec3 objectColor;
    float shininess;
}; 

struct Light {
    vec3 position;  
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
	
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;
  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;
//-------------------------------
uniform sampler3D coeff3DMapr[4];
uniform sampler3D coeff3DMapg[4];
uniform sampler3D coeff3DMapb[4];
//-------------------------------
vec3 coeffs[16];
vec3 TexCoord3D;

uniform samplerCube depthMap;
uniform float far_plane;

uniform bool useShadow;
uniform bool show_directLight;
uniform bool show_indirectLight;

float ShadowCalculation(vec3 fragPos)
{
    vec3 lightPos = light.position;
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    // ise the fragment to light vector to sample from the depth map    
    float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;        
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    
    
    return shadow;
}

float CalculateSphericalHarmonic(int l, int m, float x, float y, float z)
{
	float result = 0;
	if (l == 0 && m == 0)
	{
		result = 0.5f * sqrt(1.0f / PI);
	}
	else if (l == 1)
	{
		if (m == -1) result = 0.5f * sqrt(3.f / PI) * y;
		else if (m == 0) result = sqrt(3.f / (4.f * PI)) * z;
		else if (m == 1) result = 0.5f * sqrt(3.f / PI) * x;
	}
	else if (l == 2)
	{
		if (m == 0) result = 0.25f * sqrt(5.f / PI) * (-x * x - y * y + 2 * z * z);
		else if (m == 1) result = 0.5f * sqrt(15.f / PI) * x * z;
		else if (m == -1) result = 0.5f * sqrt(15.f / PI) * y * z;
		else if (m == 2) result = 0.25f * sqrt(15.f / PI) * (x * x - y * y);
		else if (m == -2) result = 0.5f * sqrt(15.f / PI) * x * y;
	}
	else if (l == 3)
	{
		if (m == -3) result = 0.25f * sqrt(35.f / (2.f * PI)) * (3 * x * x - y * y) * y;
		else if (m == -2) result = 0.5f * sqrt(105.f / PI) * x * y * z;
		else if (m == -1) result = 0.25f * sqrt(21.f / (2.f * PI)) * y * (4 * z * z - x * x - y * y);
		else if (m == 0) result = 0.25f * sqrt(7.f / PI) * z * (2 * z * z - 3 * x * x - 3 * y * y);
		else if (m == 1) result = 0.25f * sqrt(21.f / (2.f * PI)) * x * (4 * z * z - x * x - y * y);
		else if (m == 2) result = 0.25f * sqrt(105.f / (PI)) * (x * x - y * y) * z;
		else if (m == 3) result = 0.25f * sqrt(35.f / (2.f*PI)) * (x * x - 3 * y * y) * x;
	}
	else if (l == 4)
	{
		if (m == -4) result = 0.75f * sqrt(35.f / (PI)) * x * y *(x * x - y * y);
		else if (m == -3) result = 0.75f * sqrt(35.f / (2.f*PI)) * (3*x*x-y*y)*y*z;
		else if (m == -2) result = 0.75f * sqrt(5.f / (PI)) * x * y * (7 * z * z - 1.f);
		else if (m == -1) result = 0.75f * sqrt(5.f / (2.f*PI)) * y * z * (7 * z * z - 3.f);
		else if (m == 0) result = (3.f/16.f) * sqrt(1.f / (PI)) * (35 * z * z * z * z - 30*z*z + 3.f);
		else if (m == 1) result = 0.75f * sqrt(5.f / (2.f*PI)) * x*z*(7*z*z - 3.f);
		else if (m == 2) result = (3.f/8.f) * sqrt(5.f / (PI)) * (x * x - y * y) * (7*z*z-1.f);
		else if (m == 3) result = 0.75f * sqrt(35.f / (2.f * PI)) * (x * x - 3 * y * y) * x*z;
		else if (m == 4) result = (3.f / 16.f) * sqrt(35.f / (PI)) * (x*x*(x * x - 3 * y * y)-y*y*(3*x*x-y*y));
	}
	return result;
}
void main()
{
        vec3 norm = normalize(Normal);

        // ambient
        vec3 ambient = light.ambient * material.objectColor;
        vec3 result = ambient;
  	    if(show_directLight)
        {
            // diffuse 
            vec3 lightDir = normalize(light.position - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = light.diffuse * diff * material.objectColor;  
    
            // specular
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);  
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
            if(dot(norm, lightDir)<=0.0f) spec=0.0f;
            vec3 specular = light.specular * spec * material.objectColor;  
        

            // attenuation
            float distance    = length(light.position - FragPos);
            float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

            ambient  *= attenuation;  
            diffuse  *= attenuation;
            specular *= attenuation;   

            float shadow = useShadow ? ShadowCalculation(FragPos) : 0.0;
            result = ambient + (1.0 - shadow)*(diffuse + specular);
        }
        
    if(show_indirectLight)
    {
        float x_min=-9.5f,x_max=9.5f,y_min=0.5f,y_max=9.5f,z_min=-9.5f,z_max=9.5f;
        TexCoord3D = vec3((FragPos.x-x_min)/(x_max-x_min),(FragPos.y-y_min)/(y_max-y_min),(FragPos.z-z_min)/(z_max-z_min));
        //TexCoord3D = vec3((FragPos.x-x_min)/(x_max-x_min),(FragPos.z-z_min)/(z_max-z_min),(FragPos.y-y_min)/(y_max-y_min));
        //TexCoord3D = vec3((FragPos.y-y_min)/(y_max-y_min),(FragPos.x-x_min)/(x_max-x_min),(FragPos.z-z_min)/(z_max-z_min));
        //TexCoord3D = vec3((FragPos.z-z_min)/(z_max-z_min),(FragPos.x-x_min)/(x_max-x_min),(FragPos.y-y_min)/(y_max-y_min));
        //TexCoord3D = vec3((FragPos.y-y_min)/(y_max-y_min),(FragPos.z-z_min)/(z_max-z_min),(FragPos.x-x_min)/(x_max-x_min));
        //TexCoord3D = vec3((FragPos.z-z_min)/(z_max-z_min),(FragPos.y-y_min)/(y_max-y_min),(FragPos.x-x_min)/(x_max-x_min));
        TexCoord3D.x=clamp(TexCoord3D.x,0.0f,1.0f);
	    TexCoord3D.y=clamp(TexCoord3D.y,0.0f,1.0f);
	    TexCoord3D.z=clamp(TexCoord3D.z,0.0f,1.0f);
        //-----------------------
        if(bands==4)
        {
            for(int i=0;i<4;i++)
            {
                for(int j=0;j<4;j++)
                {
                    coeffs[4*i+j].r = texture(coeff3DMapr[i], TexCoord3D)[j];
                    coeffs[4*i+j].g = texture(coeff3DMapg[i], TexCoord3D)[j];
                    coeffs[4*i+j].b = texture(coeff3DMapb[i], TexCoord3D)[j];
                }
            }
        }
        else if(bands==2)
        {
            for(int j=0;j<4;j++)
            {
                  coeffs[j].r = texture(coeff3DMapr[0], TexCoord3D)[j];
                  coeffs[j].g = texture(coeff3DMapg[0], TexCoord3D)[j];
                  coeffs[j].b = texture(coeff3DMapb[0], TexCoord3D)[j];
            }
        }
        //-----------------------
        vec3 sampleDir = normalize(Normal);
        vec3 shColor=vec3(0.0);
        for (int l = 0; l < bands; l++)
			for (int m = -l; m <= l; m++)
			{
				int j = l * (l + 1) + m;
                vec3 tmp=coeffs[j] * CalculateSphericalHarmonic(l, m, sampleDir.x, sampleDir.y, sampleDir.z);
//				tmp.r=clamp(tmp.r,0.0f,1.0f);
//				tmp.g=clamp(tmp.g,0.0f,1.0f);
//				tmp.b=clamp(tmp.b,0.0f,1.0f);
				shColor +=tmp;
//				shColor.r=clamp(shColor.r,0.0f,1.0f);
//				shColor.g=clamp(shColor.g,0.0f,1.0f);
//				shColor.b=clamp(shColor.b,0.0f,1.0f);
			}
       result += shColor * material.objectColor;
    }
   
   // HDR tonemapping and gamma correct
   result = result / (result + vec3(1.0));
   //result = pow(result, vec3(1.0/2.2)); 
   FragColor = vec4(result, 1.0);
}