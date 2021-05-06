#version 330 core
out vec4 FragColor;

const float PI = 3.1415926535f;
uniform int bands;

struct Material {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    //vec3 objectColor;
    //float shininess;
}; 

struct Light {
    vec3 position;  
    vec3 color;
    //vec3 ambient;
    //vec3 diffuse;
    //vec3 specular;
    //float constant;
    //float linear;
    //float quadratic;
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
// PBR
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{

    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   
// ----------------------------------------------------------------------------
void main()
{
        vec3 result = vec3(0.0);
  	    if(show_directLight)
        {
            vec3 Lo = vec3(0.0);
            // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
            // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
            vec3 F0 = vec3(0.04); 
            F0 = mix(F0, material.albedo, material.metallic);
            // calculate per-light radiance
            vec3 N = normalize(Normal);
            vec3 V = normalize(viewPos - FragPos);
            vec3 L = normalize(light.position - FragPos);
            vec3 H = normalize(V + L);
            float distance = length(light.position - FragPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = light.color * attenuation;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, material.roughness);   
            float G   = GeometrySmith(N, V, L, material.roughness);    
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
            vec3 nominator    = NDF * G * F;
            float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001; // 0.001 to prevent divide by zero.
            vec3 specular = nominator / denominator;
        
            // kS is equal to Fresnel
            vec3 kS = F;
            // for energy conservation, the diffuse and specular light can't
            // be above 1.0 (unless the surface emits light); to preserve this
            // relationship the diffuse component (kD) should equal 1.0 - kS.
            vec3 kD = vec3(1.0) - kS;
            // multiply kD by the inverse metalness such that only non-metals 
            // have diffuse lighting, or a linear blend if partly metal (pure metals
            // have no diffuse light).
            kD *= 1.0 - material.metallic;	                
            
            // scale light by NdotL
            float NdotL = max(dot(N, L), 0.0);        

            // add to outgoing radiance Lo
            Lo += (kD * material.albedo / PI + specular) * radiance * NdotL; 
            float shadow = useShadow ? ShadowCalculation(FragPos) : 0.0;
            result += (1.0 - shadow)*Lo;
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
        result += shColor * material.albedo/PI;
    }
   result = result / (result + vec3(1.0f));//
   //result = pow(result, vec3(1.0f / 2.2f));//
   FragColor = vec4(result, 1.0);
}