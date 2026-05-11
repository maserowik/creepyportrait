#include "CreepyPortrait.h"

using namespace std;

//--------------------------------------------------------------
void CreepyPortrait::setup(){
	ofSetVerticalSync(true);
	// Make texture coordinates go from 0...1
	ofDisableArbTex();
	// Calculate focal length of video camera (relative to video pixel size).
	pixelFocalLength = sqrt(pow(video->getWidth()/2.0, 2) + pow(video->getHeight()/2.0, 2))/sin(ofDegToRad(videoFOV/2.0));
	// Set up model rotation state
	currentRotation = glm::vec2(0, 0);
	// Set up camera
	camera.setDistance(cameraDistance);
	camera.setTarget(glm::vec3(0, 0, 0));
	camera.disableMouseInput();
	// Set up video face detection buffer.
	detector.setBufferSize(faceBufferSize);
	// Load background
	// Image provided by: http://www.flickr.com/photos/57845051@N00/2884743046/
	// Modern OF: load() replaces deprecated loadImage()
	curtain.load("redcurtain_1024.jpg");
	// Load lighting shader
	shader.load(skullVertexShader, skullFragmentShader);
	// Load models
	if (model == "skull" || model == "all") {
		models.push_back(Model({"models/skull_mesh_1.ply", "models/skull_mesh_2.ply"},
								"models/skull_diffuse_1024.jpg",
								"models/skull_specular_1024.jpg",
								"models/skull_ao_1024.jpg",
								"models/skull_normal_1024.jpg",
								1.0,	// Don't scale the skull.
								30.0,	// Move the skull up to center.
								0.0,	// Don't rotate the skull.
								true,	// Has eyes.
								"eye.png"));
	}
	if (model == "jackevil" || model == "all") {
		models.push_back(Model({"models/jack_evil_mesh_1.ply", "models/jack_evil_mesh_2.ply"},
								"models/jack_evil_diffuse_1024.jpg",
								"models/jack_evil_specular_1024.jpg",
								"models/jack_evil_ilumination_1024.jpg",
								"models/jack_evil_normal_1024.jpg",
								6.5,		// Scale the pumpkin up in size.
								-40.0,		// Move the pumpkin down to center.
								0.0)); 		// Don't rotate the evil pumpkin.
	}
	if (model == "jackhappy" || model == "all") {
		models.push_back(Model({"models/jack_happy_mesh_1.ply", "models/jack_happy_mesh_2.ply"},
								"models/jack_happy_diffuse_1024.jpg",
								"models/jack_happy_specular_1024.jpg",
								"models/jack_happy_ilumination_1024.jpg",
								"models/jack_happy_normal_1024.jpg",
								6.5,		// Scale the pumpkin up in size.
								-40.0,		// Move the pumpkin down to center.
								-4.0));		// Rotate the happy pumpkin to better face center.
	}
	currentModel = begin(models);
	// Prime the time delta for the first update loop run.
	lastUpdate = ofGetElapsedTimef();
	// Phase 6 - Audio setup
	audioClips.push_back("audio/Test_Clip.wav");
	// Add more clips here: audioClips.push_back("audio/growl.wav");
	soundPlayer.load(audioClips[0]);
	soundPlayer.setLoop(false);
	soundPlayer.setVolume(1.0f);
}

//--------------------------------------------------------------
void CreepyPortrait::update(){
	// Update the video source (no-op internally for Raspberry Pi camera).
	video->update();
	// NOTE: Don't try to read the video source here if using the pi camera!
	// See note inside VideoSource.cpp's getPixels() function for the explanation.
}

//--------------------------------------------------------------
void CreepyPortrait::draw(){
	// Update current model rotation in the draw routine, NOT the update routine.
	// See note inside VideoSource.cpp's getPixels() function for the explanation.
	updateCurrentRotation();
	// Draw background
	curtain.draw(0,0,ofGetWidth(),ofGetHeight());
	// Enable depth buffer
	ofEnableDepthTest();
	// Set up the camera
	camera.begin();
	// Save transformation state
	ofPushMatrix();
	// Rotate model - modern OF uses ofRotateDeg() instead of deprecated ofRotateX/Y()
	ofRotateDeg(currentRotation.x, 1, 0, 0);
	ofRotateDeg(currentRotation.y, 0, 1, 0);
	// Set up the shader for rendering.
	shader.begin();
	// Transform light position from world space to camera (view) space.
	// getCurrentViewMatrix() is the reliable way inside a camera.begin() block in OF 0.12+
	glm::vec4 lightPos(lightPosition.x, lightPosition.y, lightPosition.z, lightPosition.w);
	glm::vec4 lightCameraPosition = camera.getModelViewMatrix() * lightPos;
	shader.setUniform4f("lightCameraPosition", lightCameraPosition.x, lightCameraPosition.y, lightCameraPosition.z, 1.0f);
	// Draw the current model.
	currentModel->draw(shader, eyeRotation, twitchAmount, microsaccadeOffset, pupilScale);
	// Reset all the modified rendering state.
	shader.end();
	ofPopMatrix();
	camera.end();
	ofDisableDepthTest();
	// Draw video and detected face rectangle if enabled.
	if (displayVideo) {
		video->draw(videoOffset.x, videoOffset.y, video->getWidth(), video->getHeight());
		if (detector.isFaceDetected()) {
			ofPushStyle();
			ofSetColor(0, 255, 0);
			ofNoFill();
			auto currentFace = detector.getDetectedFace();
			// Modern OF: ofDrawRectangle() replaces deprecated ofRect()
			ofDrawRectangle(currentFace.x + videoOffset.x, currentFace.y + videoOffset.y,
							currentFace.width, currentFace.height);
			ofPopStyle();
		}
	}
}

void CreepyPortrait::updateCurrentRotation() {
	float time = ofGetElapsedTimef();
	float delta = time - lastUpdate;
	lastUpdate = time;
	if (rotateSkull) {
		// Rotate the skull around the Y axis.  Don't do any face detection.
		// Good for testing the shaders.
		currentRotation.y += rotateSkullVelocity*delta;
		if (currentRotation.y > 360.0) {
			currentRotation.y -= 360.0;
		}
	}
	else {
		// Update face detection
		if (video->isFrameNew() && (time - faceLastUpdate >= faceUpdateDelay)) {
			// Update face detection with a new video frame.
			detector.updateFrame(video->getPixels());
			// Update target skull rotation
			if (detector.isFaceDetected()) {
				// Phase 6 - snap to center instantly if returning from wander
				if ((time - faceLastSeen) > noFaceWanderSeconds) {
					currentRotation = glm::vec2(0, 0);
					targetRotation = glm::vec2(0, 0);
					oldRotation = glm::vec2(0, 0);
				}
				faceLastSeen = time;
				auto currentFace = detector.getDetectedFace();
				// Use glm::vec2 for position math
				glm::vec2 faceCenter(currentFace.getCenter().x, currentFace.getCenter().y);
				auto angle = cameraAngleToModelAngle(cameraPointToAngle(faceCenter), currentFace.getArea());
				// Flip x axis because video is mirrored
				angle.x *= -1;
				oldRotation = targetRotation;
				// Rotate head up-down around x axis
				targetRotation.x = angle.y;
				// Rotate head left-right around y axis
				targetRotation.y = angle.x;
			}
			else if ((time - faceLastSeen) > noFaceResetSeconds && (time - faceLastSeen) <= noFaceWanderSeconds) {
				// Lost the face, go back to center.
				oldRotation = targetRotation;
				targetRotation.x = 0;
				targetRotation.y = 0;
			}
			// else don't move the head
			// Set last update to the current time.  Because face detection on the pi
			// can take a long time (500-800ms+) we should read the time again.
			faceLastUpdate = ofGetElapsedTimef();
		}
		// Animate moving from old to target skull rotation in the time it takes to wait for
		// the next face detection.
		else if (targetRotation != currentRotation && (time - faceLastSeen) <= noFaceWanderSeconds) {
			float position = ofClamp((time - faceLastUpdate)/faceUpdateDelay, 0, 1);
			// Modern OF: use glm::mix() instead of deprecated ofVec2f::getInterpolated()
			currentRotation = glm::mix(oldRotation, targetRotation, position);
		}
	}
	// Phase 7 - Idle wander when no face for noFaceWanderSeconds
	if (!rotateSkull && (time - faceLastSeen) > noFaceWanderSeconds) {
		currentRotation.x = sin(time * 0.37f + 1.3f) * 28.0f + sin(time * 0.13f + 0.7f) * 12.0f;
		currentRotation.y = sin(time * 0.29f + 2.1f) * 40.0f + sin(time * 0.07f + 1.5f) * 20.0f;
	}
	// j key - jaw toggle independent of audio (only when sound not playing)
	if (!soundPlayer.isPlaying()) {
		if (jawOpen) currentModel->jawAngle = ofLerp(currentModel->jawAngle, 25.0f, 0.12f);
		else currentModel->jawAngle = ofLerp(currentModel->jawAngle, 0.0f, 0.12f);
	}
	// Phase 6 - Audio state machine
	bool faceNow = detector.isFaceDetected();
	if (faceNow && audioState == AUDIO_IDLE) {
		// Face appeared - play clip (loaded once at setup)
		soundPlayer.play();
		audioState = AUDIO_TRIGGERED;
	} else if (!faceNow && audioState != AUDIO_IDLE) {
		// Face lost - stop audio
		soundPlayer.stop();
		audioState = AUDIO_IDLE;
	} else if (audioState == AUDIO_TRIGGERED && soundPlayer.isPlaying()) {
		audioState = AUDIO_PLAYING;
	} else if (audioState == AUDIO_PLAYING && !soundPlayer.isPlaying()) {
		// Clip finished - idle until next face detection cycle
		audioState = AUDIO_IDLE;
	}
	// Phase 6 - Jaw driven by spectrum amplitude
	ofSoundUpdate();
	float rawAmplitude = 0.0f;
	if (soundPlayer.isPlaying()) {
		int nBands = 64;
		float* spectrum = ofSoundGetSpectrum(nBands);
		if (spectrum != nullptr) {
			for (int i = 0; i < nBands; i++) rawAmplitude += spectrum[i];
			rawAmplitude /= nBands;
		}
	}
	smoothAmplitude = ofLerp(smoothAmplitude, rawAmplitude, 0.15f);
	float targetJaw = ofMap(smoothAmplitude, 0.0f, 0.03f, 0.0f, 25.0f, true);
	currentModel->jawAngle = ofLerp(currentModel->jawAngle, targetJaw, 0.08f);
}

glm::vec2 CreepyPortrait::cameraPointToAngle(const glm::vec2& point) {
	// Make the center of the video 0, 0.
	int x = point.x - (video->getWidth()/2);
	int y = point.y - (video->getHeight()/2);
	// Find the angle to the point.
	float xAngle = asin(x/pixelFocalLength);
	float yAngle = asin(y/pixelFocalLength);
	return glm::vec2(ofRadToDeg(xAngle), ofRadToDeg(yAngle));
}

glm::vec2 CreepyPortrait::cameraAngleToModelAngle(const glm::vec2& angle, float area) {
	// Determine what angle the model should face to look at a point along the specified camera angle.
	// This angle is not the same as the camera angle because the model is 'behind' the camera.
	// TODO: Scale distance based on detected face distance to better approximate face distance.
	float distance = faceDepth;
 	return glm::vec2(angle.x * 2.0, angle.y * 4.0);
}

//--------------------------------------------------------------
void CreepyPortrait::keyPressed(int key){
	if (key == 'v') {
		displayVideo = !displayVideo;
	}
	else if (key == 'r') {
		rotateSkull = !rotateSkull;
	}
	else if (key == 'm') {
		currentModel++;
		if (currentModel == end(models)) {
			currentModel = begin(models);
		}
	}
	else if (key == 'c') {
		targetRotation = glm::vec2(0, 0);
		currentRotation = glm::vec2(0, 0);
		faceLastSeen = ofGetElapsedTimef(); // Phase 7 - exit wander mode
	}
	else if (key == 'j') {
		// j key - toggle jaw open/closed only, no audio
		jawOpen = !jawOpen;
	}
	else if (key == 's') {
		// s key - play sound only, no jaw
		soundPlayer.play();
	}
}

//--------------------------------------------------------------
void CreepyPortrait::keyReleased(int key){

}

//--------------------------------------------------------------
void CreepyPortrait::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void CreepyPortrait::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void CreepyPortrait::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void CreepyPortrait::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void CreepyPortrait::windowResized(int w, int h){

}

//--------------------------------------------------------------
void CreepyPortrait::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void CreepyPortrait::dragEvent(ofDragInfo dragInfo){ 

}

// This function is useful for saving a model to a .ply mesh.
// The Raspberry Pi doesn't support the ofxAssimpModelLoader add on
// yet so you need to save meshes on your PC and load them on the Pi.
// #include "ofxAssimpModelLoader.h"
// #include <sstream>
// void CreepyPortrait::save_model(ofxAssimpModelLoader& model, const string& prefix) {
// 	cout << "bongos " << model.getMeshCount() << endl;
// 	for (unsigned int i = 0; i < model.getMeshCount(); ++i) {
// 		stringstream filename;
// 		filename << prefix << i << ".ply";
// 		model.getMesh(i).save(filename.str(), false);
// 	}
// }
