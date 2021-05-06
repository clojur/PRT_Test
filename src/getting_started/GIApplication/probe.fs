#version 330 core

const float PI = 3.1415926535f;
uniform int bands;

out vec4 FragColor;
in vec3 FragPos;

uniform vec3 coeffs[16];
uniform vec3 probePos;

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
	vec3 sampleDir = normalize(FragPos-probePos);
    vec3 shColor=vec3(0.0f);
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
    //FragColor = vec4(shColor, 1.0);
	// HDR tonemapping and gamma correct
	vec3 result = shColor;
    result = result / (result + vec3(1.0));
    //result = pow(result, vec3(1.0/2.2)); 
    FragColor = vec4(result, 1.0);
}