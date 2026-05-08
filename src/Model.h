#pragma once

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <string>
#include <vector>

#include "ofMain.h"

class Model {

public:
	Model(std::initializer_list<std::string> meshNames,
		  const std::string& diffuseName,
		  const std::string& specularName,
		  const std::string& ambientName,
		  const std::string& normalName,
		  float uniformScale,
		  float translateY,
		  float rotateY,
		  bool hasEyes = false,
		  const std::string& eyeTexName = "");
	void draw(ofShader& shader, glm::vec2 eyeRot = glm::vec2(0,0), float twitchAmt = 0.0f, glm::vec2 microsaccade = glm::vec2(0,0), float pupilScale = 1.0f);
	static bool useNormalMapping;
	float jawAngle = 0.0f;
	
private:
	static std::vector<float> generateTangents(ofMesh& mesh);

	float uniformScale;
	float translateY;
	float rotateY;
	bool hasEyes = false;
	ofShader eyeShader;
	ofImage eyeTexture;
	ofMesh leftEyeQuad;
	ofMesh rightEyeQuad;
	ofImage diffuse;
	ofImage specular;
	ofImage ambient;
	ofImage normal;
	std::vector<ofMesh> meshes;
	std::vector<std::vector<float>> tangents;
	std::vector<ofVbo> vbos;        // built once in constructor, reused every frame
	GLint cachedTangentLoc = -2;    // -2 = not yet queried; -1 = queried but not found; >=0 = valid location

};