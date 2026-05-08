#include "Model.h"

using namespace std;

bool Model::useNormalMapping = false;

Model::Model(initializer_list<string> meshNames,
			 const string& diffuseName,
			 const string& specularName,
		 	 const string& ambientName,
		 	 const string& normalName,
		 	 float uniformScale,
			 float translateY,
			 float rotateY,
			 bool hasEyes,
			 const string& eyeTexName) {
	// Load meshes
	meshes.resize(meshNames.size());
	int i = 0;
	for (auto name : meshNames) {
		meshes[i].load(name);
		i += 1;
	}
	// Load textures - modern OF uses load() instead of deprecated loadImage()
	diffuse.load(diffuseName);
	specular.load(specularName);
	if (useNormalMapping) {
		ambient.load(ambientName);
		normal.load(normalName);
		// Generate tangent values for normal mapping
		transform(begin(meshes), end(meshes), back_inserter(tangents), Model::generateTangents);
		// Build VBOs once here — they are reused every frame in draw()
		vbos.resize(meshes.size());
		for (unsigned int i = 0; i < meshes.size(); ++i) {
			vbos[i].setMesh(meshes[i], GL_STATIC_DRAW);
		}
	}
	// Save scaling and translation values
	this->uniformScale = uniformScale;
	this->translateY = translateY;
	this->rotateY = rotateY;
	this->hasEyes = hasEyes;
	if (hasEyes && !eyeTexName.empty()) {
		eyeShader.load("eye.vert", "eye.frag");
		eyeTexture.load(eyeTexName);
		float s = 42.0f;
		leftEyeQuad.clear();
		leftEyeQuad.addVertex(glm::vec3(-s,-s,0));
		leftEyeQuad.addVertex(glm::vec3( s,-s,0));
		leftEyeQuad.addVertex(glm::vec3( s, s,0));
		leftEyeQuad.addVertex(glm::vec3(-s, s,0));
		leftEyeQuad.addTexCoord(glm::vec2(0,1));
		leftEyeQuad.addTexCoord(glm::vec2(1,1));
		leftEyeQuad.addTexCoord(glm::vec2(1,0));
		leftEyeQuad.addTexCoord(glm::vec2(0,0));
		leftEyeQuad.addIndex(0); leftEyeQuad.addIndex(1); leftEyeQuad.addIndex(2);
		leftEyeQuad.addIndex(0); leftEyeQuad.addIndex(2); leftEyeQuad.addIndex(3);
		rightEyeQuad = leftEyeQuad;
	}
}

void Model::draw(ofShader& shader, glm::vec2 eyeRot, float twitchAmt, glm::vec2 microsaccade, float pupilScale) {
	// Scale and translate the model into position.
	// Modern OF: use ofRotateDeg() instead of deprecated ofRotateY()
	ofRotateDeg(rotateY, 0, 1, 0);
	ofScale(uniformScale, uniformScale, uniformScale);
	ofTranslate(0, translateY, 0);
	// Set shader inputs and draw meshes.
	// Modern OF: pass .getTexture() explicitly — passing ofImage directly is deprecated
	shader.setUniformTexture("diffuseTex", diffuse.getTexture(), 1);
	shader.setUniformTexture("specularTex", specular.getTexture(), 2);
	if (useNormalMapping) {
		shader.setUniformTexture("ambientTex", ambient.getTexture(), 3);
		shader.setUniformTexture("normalTex", normal.getTexture(), 4);
	} else {
		// Bind diffuse as ambient fallback so lighting_gl's ambientTex sampler
		// always reads something defined (avoids undefined behaviour on that path).
		shader.setUniformTexture("ambientTex", diffuse.getTexture(), 3);
	}
	for (unsigned int i = 0; i < meshes.size(); ++i) {
		if (useNormalMapping) {
			// First draw only: query the tangent attribute location from the live
			// shader and upload tangent data into each VBO. After this the tangent
			// data lives entirely on the GPU and setAttributeData is never called again.
			if (cachedTangentLoc == -2) {
				cachedTangentLoc = shader.getAttributeLocation("tangent");
				if (cachedTangentLoc >= 0) {
					for (unsigned int j = 0; j < vbos.size(); ++j) {
						vbos[j].setAttributeData(cachedTangentLoc,
												 tangents[j].data(),
												 4,                             // 4 floats per vertex (xyzw)
												 (int)(tangents[j].size() / 4), // vertex count
												 GL_STATIC_DRAW,
												 sizeof(float) * 4);
					}
				}
			}
			// Every frame: just draw — no data upload.
			if (i == 1) {
				ofPushMatrix();
				ofTranslate(0, -70, 0);
				ofRotateXDeg(jawAngle);
				ofTranslate(0, 70, 0);
				vbos[i].drawElements(GL_TRIANGLES, meshes[i].getNumIndices());
				ofPopMatrix();
			} else {
				vbos[i].drawElements(GL_TRIANGLES, meshes[i].getNumIndices());
			}
		} else {
			if (i == 1) {
				ofPushMatrix();
				ofTranslate(0, -70, 0);
				ofRotateXDeg(jawAngle);
				ofTranslate(0, 70, 0);
				meshes[i].drawFaces();
				ofPopMatrix();
			} else {
				meshes[i].drawFaces();
			}
		}
	}
	if (hasEyes) {
		eyeShader.begin();
		eyeShader.setUniformTexture("eyeTex", eyeTexture.getTexture(), 0);
		eyeShader.setUniform1f("pupilScale", pupilScale);
		eyeShader.setUniform2f("pupilOffset", 0.0f, 0.0f);
		ofPushMatrix();
		ofTranslate(-68, 24, 190);
		leftEyeQuad.draw();
		ofPopMatrix();
		ofPushMatrix();
		ofTranslate(82, 20, 190);
		rightEyeQuad.draw();
		ofPopMatrix();
		eyeShader.end();
	}
}

vector<float> Model::generateTangents(ofMesh& mesh) {
	// Tangent generation adapted from algorithm at http://www.terathon.com/code/tangent.html
	auto vertices = mesh.getVertices();
	auto normals = mesh.getNormals();
	auto texcoords = mesh.getTexCoords();
	auto triangles = mesh.getIndices();

	vector<float> tangents(vertices.size()*4);

	// Modern OF uses glm::vec3 instead of deprecated ofVec3f
	vector<glm::vec3> tan1(vertices.size(), glm::vec3(0,0,0));
	vector<glm::vec3> tan2(vertices.size(), glm::vec3(0,0,0));

	for (unsigned int i = 0; i < triangles.size(); i += 3) {
		long i1 = triangles[i];
		long i2 = triangles[i+1];
		long i3 = triangles[i+2];

		const glm::vec3& v1 = vertices[i1];
		const glm::vec3& v2 = vertices[i2];
		const glm::vec3& v3 = vertices[i3];

		const glm::vec2& w1 = texcoords[i1];
		const glm::vec2& w2 = texcoords[i2];
		const glm::vec2& w3 = texcoords[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.0F / (s1 * t2 - s2 * t1);
		glm::vec3 sdir((t2 * x1 - t1 * x2) * r, 
					 (t2 * y1 - t1 * y2) * r,
					 (t2 * z1 - t1 * z2) * r);
		glm::vec3 tdir((s1 * x2 - s2 * x1) * r,
					 (s1 * y2 - s2 * y1) * r,
					 (s1 * z2 - s2 * z1) * r);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	for (unsigned int i = 0; i < vertices.size(); ++i) {
		glm::vec3& n = normals[i];
		glm::vec3& t = tan1[i];

		// Gram-Schmidt orthogonalize
		auto tangent = glm::normalize(t - n * glm::dot(n, t));
		// Calculate handedness
		float w = (glm::dot(glm::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;

		tangents[i*4]   = tangent.x;
		tangents[i*4+1] = tangent.y;
		tangents[i*4+2] = tangent.z;
		tangents[i*4+3] = w;
	}

	return tangents;
}