#pragma once

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "ofMain.h"
#include "glm/glm.hpp"

#include "Model.h"
#include "VideoSource.h"
#include "VideoFaceDetector.h"


class CreepyPortrait: public ofBaseApp {

public:
	CreepyPortrait() {}
	~CreepyPortrait() {}

	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	// Application state that can be modified is below:

	// Distance camera is back on the Z axis from the origin.  For the skull model
	// this should be about 650.
	float 		cameraDistance = 650.0;

	// True/false if video and detected faces should be displayed.
	bool 		displayVideo;

	// Number of face detection samples to buffer.
	int 		faceBufferSize;

	// How far away from the camera to assume detected faces lie.
	float 		faceDepth = 10.0;

	// How often to delay between runs of the face detection logic, in seconds.
	float 		faceUpdateDelay;

	// Position of the point light source in world coordinates.
	// Using ofVec4f for compatibility with existing uniform setup; internally cast to glm
	ofVec4f 	lightPosition = ofVec4f(-600, 100, 600, 1);

	// The model(s) to load for rendering.
	std::string model;

	// How long to wait for no detected face before the skull rotates back to 0.
	float 		noFaceResetSeconds;
	// How long to wait before idle wander begins (Phase 7).
	float 		noFaceWanderSeconds = 600.0f;

	// True/false to render the skull (true) or evil jacklantern (false).
	bool 		renderSkullMode = true;

	// True/false if the skull should just rotate around the Y axis.
	bool 		rotateSkull = false;

	// Velocity of the rotating skull in degrees/second.
	float 		rotateSkullVelocity = 5.0;

	// Skull rendering fragment and vertex shader files names.
	std::string	skullFragmentShader;
	std::string	skullVertexShader;

	// Enable or disable normal/bump mapping in the shader.
	bool 		useNormalMapping;

	// Reference to a video source.
	std::shared_ptr<IVideoSource> video;

	// Diagonal field of view of the camera, in degrees.
	float 		videoFOV;

	// Pixel offset to use when rendering video on the screen.
	glm::vec2 	videoOffset = glm::vec2(10, 10);

private:
	void updateCurrentRotation();
	glm::vec2 cameraPointToAngle(const glm::vec2& point);
	glm::vec2 cameraAngleToModelAngle(const glm::vec2& angle, float area);

	// Internal application state:
	ofEasyCam camera;
	float pixelFocalLength;
	float faceLastSeen = 0.0;
	float faceLastUpdate = 0.0;
	float lastUpdate;
	ofImage curtain;
	ofShader shader;
	std::list<Model> models;
	std::list<Model>::iterator currentModel;
	VideoFaceDetector detector;
	glm::vec2 currentRotation;
	glm::vec2 targetRotation;
	glm::vec2 oldRotation;
	bool jawOpen = false;
	bool eyeAnimEnabled = true;
	// Phase 6 - Audio
	enum AudioState { AUDIO_IDLE, AUDIO_TRIGGERED, AUDIO_PLAYING };
	AudioState audioState = AUDIO_IDLE;
	ofSoundPlayer soundPlayer;
	float smoothAmplitude = 0.0f;
	std::vector<std::string> audioClips;

	// Phase 3 - Eye system
	glm::vec2 eyeRotation;
	glm::vec2 eyeTargetRotation;

	// Pupil dilation
	float pupilScale = 1.0f;
	float pupilTargetScale = 1.0f;
	float dilationTimer = 0.0f;
	float dilationDuration = 0.0f;

	// Eye twitch
	float twitchTimer = 0.0f;
	float twitchAmount = 0.0f;
	float twitchDuration = 0.0f;
	bool twitching = false;

	// Eye dart
	float dartTimer = 0.0f;
	float dartHoldTimer = 0.0f;
	glm::vec2 dartOffset;
	bool darting = false;

	// Microsaccade
	float microsaccadeTimer = 0.0f;
	glm::vec2 microsaccadeOffset;
};
