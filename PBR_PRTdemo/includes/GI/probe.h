#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <ctime>
using namespace std;
const float PI = 3.1415926535f;
extern bool probe_useShadow;
// PBR
// ----------------------------------------------------------------------------
float DistributionGGX(glm::vec3 N, glm::vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(glm::dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;

	return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{

	float r = (roughness + 1.0f);
	float k = (r * r) / 8.0f;

	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(glm::vec3 N, glm::vec3 V, glm::vec3 L, float roughness)
{
	float NdotV = max(glm::dot(N, V), 0.0f);
	float NdotL = max(glm::dot(N, L), 0.0f);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
glm::vec3 fresnelSchlick(float cosTheta, glm::vec3 F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}
// ----------------------------------------------------------------------------
glm::vec3 fresnelSchlickRoughness(float cosTheta, glm::vec3 F0, float roughness)
{
	return F0 + (max(glm::vec3(1.0 - roughness), F0) - F0) * pow(1.0f - cosTheta, 5.0f);
}
// ----------------------------------------------------------------------------
class Material {
public:
	glm::vec3 albedo;
	float metallic;
	float roughness;
	float ao;
	Material() {}
	Material(glm::vec3 objectColor, float metal , float rough , float a) :
		albedo(objectColor), metallic(metal), roughness(rough), ao(a) {}
};

class Light {
public:
	glm::vec3 position;
	//glm::vec3 ambient;
	//glm::vec3 diffuse;
	//glm::vec3 specular;
	glm::vec3 color;

	//float constant;
	//float linear;
	//float quadratic;
	Light() {}
	Light(glm::vec3 pos, glm::vec3 col):position(pos), color(col){}
	/*Light(glm::vec3 pos, glm::vec3 ambi, glm::vec3 diff, glm::vec3 specu, float cons, float lin, float quad)
		:position(pos), ambient(ambi), diffuse(diff), specular(specu), constant(cons), linear(lin), quadratic(quad) {}*/
};

class TrianglePlane
{
public:
	glm::vec3 v0, v1, v2;
	glm::vec3 normal;
	Material material;
	TrianglePlane() {}
	TrianglePlane(glm::vec3 vertex0, glm::vec3 vertex1, glm::vec3 vertex2, glm::vec3 norm, Material plane_material)
		:v0(vertex0), v1(vertex1), v2(vertex2), normal(norm), material(plane_material) {}
};

glm::vec3 getFragColor(glm::vec3& FragPos, glm::vec3& viewPos, glm::vec3& Normal, Material& material, Light& light, std::vector<TrianglePlane>& triangleplanes);

class Ray
{
public:
	glm::vec3 position;
	glm::vec3 direction;
	float theta;
	float phi;
	std::vector<float> sh_functions;
	Ray(glm::vec3 pos, glm::vec3 dir):position(pos), direction(dir)
	{

	}
	Ray(glm::vec3 pos, glm::vec3 dir, float atheta, float aphi) :position(pos), direction(dir), theta(atheta), phi(aphi)
	{

	}
	int linearIntersectTriangle(TrianglePlane& trianglePlane, glm::vec3& intersection, float& t) const
	{
		glm::vec3& v0 = trianglePlane.v0;
		glm::vec3& v1 = trianglePlane.v1;
		glm::vec3& v2 = trianglePlane.v2;

		float u, v, det;
		glm::vec3 K,M,T;
		glm::vec3 e1 = v1 - v0;
		glm::vec3 e2 = v2 - v0;
		T = position - v0;
		M = glm::cross(direction,e2);
		det = glm::dot(M,e1);
		//If the line is perpendicular to the normal of triangle.
		if (det > -0.00001f && det < 0.00001f)
		{
			if ((glm::dot(glm::cross(e1, e2), T) > -0.00001f)&&(glm::dot(glm::cross(e1, e2), T) < 0.00001f))
				return 1; //The line is on the plane.
			else return 2;//The line is parallel to the plane.
		}
		K = glm::cross(T, e1);
		t = glm::dot(K, e2) / det;
		u = glm::dot(M,T) / det;
		v = glm::dot(K, direction) / det;
		if (((u<0.0f)||(v<0.0f))||((u+v)>1.0f||t< 0.00001f))
			return 3; //disjoint

		glm::vec3&& Q = position + t * direction;
		intersection = Q;
		return 0;//intersecting
	}
	int linearIntersectTriangle(TrianglePlane& trianglePlane) const
	{
		glm::vec3 intersection;
		float t;
		return linearIntersectTriangle(trianglePlane, intersection, t);
	}
};

glm::vec3 getFragColor(glm::vec3& FragPos,glm::vec3& viewPos, glm::vec3& Normal, Material& material, Light& light, std::vector<TrianglePlane>& trianglePlanes)
{
	glm::vec3 result = glm::vec3(0.0f);
	//---------------------------
	glm::vec3 Lo = glm::vec3(0.0f);
	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
	glm::vec3 F0 = glm::vec3(0.04f);
	F0 = glm::mix(F0, material.albedo, material.metallic);
	// calculate per-light radiance
	glm::vec3 N = glm::normalize(Normal);
	glm::vec3 V = glm::normalize(viewPos - FragPos);
	glm::vec3 L = glm::normalize(light.position - FragPos);
	glm::vec3 H = glm::normalize(V + L);
	float distance = glm::length(light.position - FragPos);
	float attenuation = 1.0 / (distance * distance);
	glm::vec3 radiance = light.color * attenuation;

	// Cook-Torrance BRDF
	float NDF = DistributionGGX(N, H, material.roughness);
	float G = GeometrySmith(N, V, L, material.roughness);
	glm::vec3 F = fresnelSchlick(max(glm::dot(H, V), 0.0f), F0);

	glm::vec3 nominator = NDF * G * F;
	float denominator = 4 * max(glm::dot(N, V), 0.0f) * max(glm::dot(N, L), 0.0f) + 0.001f; // 0.001 to prevent divide by zero.
	glm::vec3 specular = nominator / denominator;

	// kS is equal to Fresnel
	glm::vec3 kS = F;
	// for energy conservation, the diffuse and specular light can't
	// be above 1.0 (unless the surface emits light); to preserve this
	// relationship the diffuse component (kD) should equal 1.0 - kS.
	glm::vec3 kD = glm::vec3(1.0) - kS;
	// multiply kD by the inverse metalness such that only non-metals 
	// have diffuse lighting, or a linear blend if partly metal (pure metals
	// have no diffuse light).
	kD *= 1.0 - material.metallic;

	// scale light by NdotL
	float NdotL = max(dot(N, L), 0.0f);

	// add to outgoing radiance Lo
	Lo += (kD * material.albedo / PI + specular) * radiance * NdotL;
	result += Lo;

	// Considering the shadow
	if (probe_useShadow)
	{
		Ray ray(FragPos, glm::normalize(light.position - FragPos));
		float t_min = (light.position - FragPos).x / glm::normalize(light.position - FragPos).x;
		for (TrianglePlane& trianglePlane : trianglePlanes)
		{
			glm::vec3 intersection;
			float t;
			int interType = ray.linearIntersectTriangle(trianglePlane, intersection, t);
			if (interType == 0 && t < t_min*0.99f)
			{
				glm::vec3 FragColor = result*0.1f;
				return FragColor;
			}
		}
	}
	
	glm::vec3 FragColor = result;

	return FragColor;
}

class Surfel
{
public:
	glm::vec3 position;
	glm::vec3 probePosition;
	glm::vec3 normal;
	Material surfel_material;
	glm::vec3 color;
	Surfel(){}
	Surfel(glm::vec3 pos, glm::vec3 probePos, glm::vec3 norm, Material& material, Light& light, std::vector<TrianglePlane>& triangleplanes) :position(pos), probePosition(probePos), normal(norm)
	{
		surfel_material = material;
		color = getFragColor(position, probePosition, normal, material, light, triangleplanes);
	}

	Surfel(glm::vec3 pos, glm::vec3 probePos, TrianglePlane& trianglePlane, Light& light, std::vector<TrianglePlane>& triangleplanes) :position(pos), probePosition(probePos), normal(trianglePlane.normal)
	{
		surfel_material = trianglePlane.material;
		color = getFragColor(position, probePosition, normal, trianglePlane.material, light, triangleplanes);
	}
	
	void SetColor(Light& light, vector<TrianglePlane>& triangleplanes)
	{
		color = getFragColor(position, probePosition, normal, surfel_material, light, triangleplanes);
	}
};

class Probe
{
public:
	glm::vec3 position;
	std::vector<Ray> rays;
	std::vector<Surfel> surfels;
	std::vector<glm::vec3> irradiances;
	
	Probe(glm::vec3 pos) :position(pos)
	{
		GenerateSampleRays();
	}

	Probe(glm::vec3 pos, std::vector<TrianglePlane>& trianglePlanes,Light& light) :position(pos)
	{
		GenerateSampleRays();
		GenerateSurfels(trianglePlanes,light);
		CalculateIrradiance();
	}

	float Random()
	{
		float random = (float)(rand() % 1000) / 1000.0f;
		return(random);
	}
	void GenerateSampleRays(int N=10)
	{
		rays.clear();
		srand(time(NULL));
		for (int i = 0; i < N; i++)
		{
			for (int j = 0; j < N; j++)
			{
				float a = (((float)i) + Random()) / (float)N;
				float b = (((float)j) + Random()) / (float)N;
				float theta = 2 * acos(sqrt(1 - a));
				float phi = 2 * PI * b;
				float x = sin(theta) * cos(phi);
				float y = sin(theta) * sin(phi);
				float z = cos(theta);
				glm::vec3 dir = glm::vec3(x, y, z);
				Ray ray(position, dir, theta, phi);
				rays.push_back(ray);
			}
		}
	};
	
	////生成均匀分布的方向
	//std::vector<glm::vec3> genAverDir(int number=100) {
	//	//srand(time(NULL));
	//	float rangeTheta = PI;
	//	std::vector<glm::vec3> dirs;
	//	//球面坐标系
	//	float thetaInterval = rangeTheta / number;
	//	for (int i = 0; i <= number; i++) {
	//		float theta = i * thetaInterval;
	//		int m = std::max(floor(number * sin(theta)), 1.0f);
	//		float fiInterval = 2 * PI / m;
	//		for (int j = 0; j < m; j++) {
	//			float fi = j * fiInterval;
	//			//对theta和fi加入小随机扰动
	//			//theta += theta * parameter.dirBlur * getUnitRand();
	//			//fi += fi * parameter.dirBlur * getUnitRand();
	//			//生成随机方向
	//			glm::vec3 dir = glm::vec3(sin(theta) * sin(fi), cos(theta), sin(theta) * cos(fi));
	//			dirs.push_back(dir);
	//		}
	//	}
	//	return dirs;
	//}
	//void GenerateRays(int number = 100)
	//{
	//	//srand(time(NULL));
	//	float rangeTheta = PI;
	//	//球面坐标系
	//	float thetaInterval = rangeTheta / number;
	//	for (int i = 0; i <= number; i++) {
	//		float theta = i * thetaInterval;
	//		int m = std::max(floor(number * sin(theta)), 1.0f);
	//		float fiInterval = 2 * PI / m;
	//		for (int j = 0; j < m; j++) {
	//			float fi = j * fiInterval;
	//			//对theta和fi加入小随机扰动
	//			//theta += theta * parameter.dirBlur * getUnitRand();
	//			//fi += fi * parameter.dirBlur * getUnitRand();
	//			//生成随机方向
	//			glm::vec3 dir = glm::vec3(sin(theta) * sin(fi), cos(theta), sin(theta) * cos(fi));
	//			Ray ray(position, dir,theta,fi);
	//			rays.push_back(ray);
	//		}
	//	}
	//}
	void GenerateSurfels(std::vector<TrianglePlane>& trianglePlanes, Light& light)
	{
		surfels.clear();
		for (Ray& ray : rays)
		{
			Surfel surfel;
			float t=-0.0001f;
			for (TrianglePlane& trianglePlane : trianglePlanes)
			{
				glm::vec3 intersection;
				float t_tmp;
				int interType = ray.linearIntersectTriangle(trianglePlane, intersection, t_tmp);
				if (interType == 0)
				{
					Surfel surfel_tmp(intersection, position, trianglePlane, light, trianglePlanes);
					if (t < 0.0f) { t = t_tmp; surfel = surfel_tmp; }
					else
					{
						if (t_tmp < t) { t = t_tmp; surfel = surfel_tmp; }
					}
				}
			}
			if (t > 0.0f)
			{
				surfels.push_back(surfel);
			}
		}
	}

	void CalculateIrradiance()
	{
		irradiances.clear();
		for (int i = 0; i < rays.size(); i++)
		{
			glm::vec3 irradiance = glm::vec3(0.0f);
			int count = 0;
			for (int j = 0; j < rays.size(); j++)
			{
				float costmp = max(glm::dot(rays[i].direction, rays[j].direction), 0.0f);
				
				if (costmp > 0.0f)
				{
					//float sintmp = sqrtf(1 - costmp * costmp);
					irradiance += costmp*surfels[i].color;//*sintmp
					count++;
				}
			}
			irradiance = irradiance * (PI*PI / (float)count);
			irradiances.push_back(irradiance);
		}
	}

};

std::vector<TrianglePlane> GetTrianglePlanes(int wall_num, float wallVertices[], glm::vec3 wall_material_objectColor[],
	glm::vec3 wallPositions[], float wallAngle[], glm::vec3 wallAxis[], 
	int cube_num, float cubeVertices[], glm::vec3 cube_material_objectColor[], glm::vec3 cubePositions[],Material& generalMaterial)
{
	
	glm::vec3 vertexTmp[36];
	glm::vec3 normalTmp[12];
	for (int i = 0; i < 36; i++)
	{
		vertexTmp[i] = glm::vec3(wallVertices[i * 8], wallVertices[i * 8 + 1], wallVertices[i * 8 + 2]);
		if (i % 3 == 0)
			normalTmp[i / 3] = glm::vec3(wallVertices[i * 8 + 3], wallVertices[i * 8 + 4], wallVertices[i * 8 + 5]);
	}
	std::vector<TrianglePlane> localWallTrianglePlanes(12);
	for (int i = 0; i < 12; i++)
	{
		localWallTrianglePlanes[i].v0 = vertexTmp[i * 3];
		localWallTrianglePlanes[i].v1 = vertexTmp[i * 3 + 1];
		localWallTrianglePlanes[i].v2 = vertexTmp[i * 3 + 2];
		localWallTrianglePlanes[i].normal = normalTmp[i];
	}

	std::vector<TrianglePlane> wallTrianglePlanes(12 * wall_num);
	for (int i = 0; i < wall_num; i++)
	{
		glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
		model = glm::translate(model, wallPositions[i]);
		model = glm::rotate(model, glm::radians(wallAngle[i]), wallAxis[i]);
		
		for (int j = 0; j < 12; j++)
		{
			wallTrianglePlanes[i * 12 + j].v0 = glm::vec3(model * glm::vec4(localWallTrianglePlanes[j].v0, 1.0));
			wallTrianglePlanes[i * 12 + j].v1 = glm::vec3(model * glm::vec4(localWallTrianglePlanes[j].v1, 1.0));
			wallTrianglePlanes[i * 12 + j].v2 = glm::vec3(model * glm::vec4(localWallTrianglePlanes[j].v2, 1.0));
			wallTrianglePlanes[i * 12 + j].normal = glm::mat3(glm::transpose(glm::inverse(model)))*localWallTrianglePlanes[j].normal;
			wallTrianglePlanes[i * 12 + j].material = Material(wall_material_objectColor[i],generalMaterial.metallic, generalMaterial.roughness, generalMaterial.ao);
		}
	}

	//--------------------------------------------------------------------
	for (int i = 0; i < 36; i++)
	{
		vertexTmp[i] = glm::vec3(cubeVertices[i * 8], cubeVertices[i * 8 + 1], cubeVertices[i * 8 + 2]);
		if (i % 3 == 0)
			normalTmp[i / 3] = glm::vec3(cubeVertices[i * 8 + 3], cubeVertices[i * 8 + 4], cubeVertices[i * 8 + 5]);
	}
	std::vector<TrianglePlane> localcubeTrianglePlanes(12);
	for (int i = 0; i < 12; i++)
	{
		localcubeTrianglePlanes[i].v0 = vertexTmp[i * 3];
		localcubeTrianglePlanes[i].v1 = vertexTmp[i * 3 + 1];
		localcubeTrianglePlanes[i].v2 = vertexTmp[i * 3 + 2];
		localcubeTrianglePlanes[i].normal = normalTmp[i];
	}
	std::vector<TrianglePlane> cubeTrianglePlanes(12 * cube_num);
	for (int i = 0; i < cube_num; i++)
	{
		glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
		model = glm::translate(model, cubePositions[i]);

		for (int j = 0; j < 12; j++)
		{
			cubeTrianglePlanes[i * 12 + j].v0 = glm::vec3(model * glm::vec4(localcubeTrianglePlanes[j].v0, 1.0));
			cubeTrianglePlanes[i * 12 + j].v1 = glm::vec3(model * glm::vec4(localcubeTrianglePlanes[j].v1, 1.0));
			cubeTrianglePlanes[i * 12 + j].v2 = glm::vec3(model * glm::vec4(localcubeTrianglePlanes[j].v2, 1.0));
			cubeTrianglePlanes[i * 12 + j].normal = localcubeTrianglePlanes[j].normal;
			cubeTrianglePlanes[i * 12 + j].material = Material(cube_material_objectColor[i], generalMaterial.metallic, generalMaterial.roughness, generalMaterial.ao);
		}
	}
	std::vector<TrianglePlane> trianglePlanes;
	for (auto& trianglePlane : wallTrianglePlanes)
	{
		trianglePlanes.push_back(trianglePlane);
	}
	for (auto& trianglePlane : cubeTrianglePlanes)
	{
		trianglePlanes.push_back(trianglePlane);
	}
	return trianglePlanes;
}

std::vector<Probe> GenerateProbes(std::vector<glm::vec3>& probePositions, std::vector<TrianglePlane>& trianglePlanes, Light& light)
{
	std::vector<Probe> probes;
	for (glm::vec3& probePosition : probePositions)
	{
		Probe probe(probePosition, trianglePlanes, light);
		probes.push_back(probe);
	}
	
	return probes;
}

// Spherical Harmonics----------------------------------------------------------------------------
float DoubleFactorial(int n)
{
	if (n <= 1)
		return(1);
	else
		return(n * DoubleFactorial(n - 2));
}
float Legendre(int l, int m, float x)
{
	float result;
	if (l == m + 1)
		result = x * (2 * m + 1) * Legendre(m, m,x);
	else if (l == m)
		result = pow(-1, (float)m) * DoubleFactorial(2 * m-1) * pow((1.0f-x * x), (float)m / 2.0f);
	else
		result = (x * (2 * l-1) * Legendre(l - 1, m,x) - (l + m-1) * Legendre(l - 2, m,x)) / (float)(l - m);
	return(result);
}
int factorial(int n)
{
	if (n <= 1) return 1;
	return std::tgamma(n + 1);
}
float K(int l, int m)
{
	float num = (2 * l + 1) * factorial(l - std::abs(m));
	float denom = 4 * PI * factorial(l + std::abs(m));
	float result = sqrt(num / denom);
	return(result);
}
float SphericalHarmonic(int l, int m, float theta, float phi)
{
	float result;
	if (m > 0)
		result = sqrt(2.0f) * K(l, m) * cos(m * phi) * Legendre(l, m, cos(theta));
	else if (m < 0)
		result = sqrt(2.0f) * K(l, m) * sin(-m * phi) * Legendre(l, -m, cos(theta));
	else
		result = K(l, m) * Legendre(l, 0, cos(theta));
	return(result);
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

float CalculateSphericalHarmonic(int l, int m, float theta, float phi)
{
	float x = sin(theta) * cos(phi);
	float y = sin(theta) * sin(phi);
	float z = cos(theta);
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
	else
	{
		result = CalculateSphericalHarmonic(l, m, x, y, z);
	}
	return result;
}

//float CalculateSphericalHarmonic(int l, int m, float x, float y, float z)
//{
//	float result;
//	if (l == 0 && m == 0)
//	{
//		result = 0.5f * sqrt(1.0f / PI);
//	}
//	else if (l == 1)
//	{
//		if (m == -1) result = -0.5f * sqrt(3.f / PI) * y;
//		else if (m == 0) result = sqrt(3.f / (4.f * PI)) * z;
//		else if (m == 1) result = -0.5f * sqrt(3.f / PI) * x;
//	}
//	else if (l == 2)
//	{
//		if (m == 0) result = 0.25f * sqrt(5.f / PI) * (-x * x - y * y + 2 * z * z);
//		else if (m == 1) result = -0.5f * sqrt(15.f / PI) * x * z;
//		else if (m == -1) result = -0.5f * sqrt(15.f / PI) * y * z;
//		else if (m == 2) result = 0.25f * sqrt(15.f / PI) * (x * x - y * y);
//		else if (m == -2) result = 0.5f * sqrt(15.f / PI) * x * y;
//	}
//	return result;
//}
void PrecomputeSHFunctions(Probe& probe, int bands)
{
	for (Ray& ray : probe.rays)
	{
		ray.sh_functions.resize(bands * bands);
		for (int l = 0; l < bands; l++)
			for (int m = -l; m <= l; m++)
			{
				int j = l * (l + 1) + m;
				ray.sh_functions[j] = CalculateSphericalHarmonic(l, m, ray.theta, ray.phi);
			}
	}
}

glm::vec3* ProjectRadianceFunctionSH(Probe& probe, int bands)
{
	glm::vec3* coeffs=new glm::vec3[bands * bands];
	PrecomputeSHFunctions(probe, bands);
	for (int i = 0; i < bands * bands; i++)
	{
		coeffs[i].r = 0.0f;
		coeffs[i].g = 0.0f;
		coeffs[i].b = 0.0f;
	}
	int i = 0;
	for (Surfel& surfel : probe.surfels)
	{
		for (int j = 0; j < bands * bands; j++)
		{
			float sh_function = probe.rays[i].sh_functions[j];
			coeffs[j].r += (surfel.color.r * sh_function);
			coeffs[j].g += (surfel.color.g * sh_function);
			coeffs[j].b += (surfel.color.b * sh_function);
		}
		i++;
	}
	float weight = 4.0f * PI;
	float scale = weight / probe.surfels.size();
	for (int i = 0; i < bands * bands; i++)
	{
		coeffs[i].r *= scale;
		coeffs[i].g *= scale;
		coeffs[i].b *= scale;
	}
	return coeffs;
}
glm::vec3* ProjectIrradianceFunctionSH(Probe& probe, int bands)
{
	glm::vec3* coeffs = new glm::vec3[bands * bands];
	PrecomputeSHFunctions(probe, bands);
	for (int i = 0; i < bands * bands; i++)
	{
		coeffs[i].r = 0.0f;
		coeffs[i].g = 0.0f;
		coeffs[i].b = 0.0f;
	}
	int i = 0;
	for (glm::vec3& irradience : probe.irradiances)
	{
		for (int j = 0; j < bands * bands; j++)
		{
			float sh_function = probe.rays[i].sh_functions[j];
			coeffs[j].r += (irradience.r * sh_function);
			coeffs[j].g += (irradience.g * sh_function);
			coeffs[j].b += (irradience.b * sh_function);
		}
		i++;
	}
	float weight = 4.0f * PI;
	float scale = weight / probe.irradiances.size();
	for (int i = 0; i < bands * bands; i++)
	{
		coeffs[i].r *= scale;
		coeffs[i].g *= scale;
		coeffs[i].b *= scale;
	}
	return coeffs;
}
int probe_is_inCube(Probe& probe, int cube_num, glm::vec3 cubePositions[], float cube_length)
{
	glm::vec3 pos = probe.position;
	for (int i = 0; i < cube_num; i++)
	{
		if (std::fabsf(cubePositions[i].x - pos.x) < cube_length / 2.0f
			&& std::fabsf(cubePositions[i].y - pos.y) < cube_length / 2.0f
			&& std::fabsf(cubePositions[i].z - pos.z) < cube_length / 2.0f) return i; //return Cube id 
	}
	return -1;//probe is not in Cubes
}
void improve_probesInCubes(std::vector<Probe>& probes, int cube_num, glm::vec3 cubePositions[], std::vector<TrianglePlane>& trianglePlanes, int wall_num, Light& light)
{
	for (int i = 0; i < probes.size(); i++)
	{
		int id = probe_is_inCube(probes[i], cube_num, cubePositions, 2.0f);
		if (id != -1)
		{
			std::vector<TrianglePlane> trianglePlanes_tmp;
			for (int j = 0; j < trianglePlanes.size(); j++)
			{
				if (!(j >= (12 * wall_num + id * 12) && j < (12 * wall_num + (id + 1) * 12)))
				{
					trianglePlanes_tmp.push_back(trianglePlanes[j]);
				}
			}
			/*trianglePlanes_tmp=GetTrianglePlanes(wall_num, wallVertices, wall_material_objectColor, wallPositions, wallAngle, wallAxis,
				0, cubeVertices, cube_material_objectColor, cubePositions);*/
			probes[i] = Probe(probes[i].position, trianglePlanes_tmp, light);
		}
	}
}