#include "CreepyPortrait.h"
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <ctime>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>

using namespace std;

//--------------------------------------------------------------
void CreepyPortrait::setup(){
    appStartTime = ofGetElapsedTimef();
	ofSetVerticalSync(true);
	// Make texture coordinates go from 0...1
	ofDisableArbTex();
	// Calculate focal length of video camera (relative to video pixel size).
	pixelFocalLength = sqrt(pow(video->getWidth()/2.0, 2) + pow(video->getHeight()/2.0, 2))/sin(ofDegToRad(videoFOV/2.0));
	ofLogWarning("CreepyPortrait::setup") << "webcam opened:"
		<< " resolution=" << video->getWidth() << "x" << video->getHeight()
		<< " FOV=" << videoFOV;
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
	ofLogWarning("CreepyPortrait::setup") << "background loaded: redcurtain_1024.jpg";
	// Load lighting shader
	shader.load(skullVertexShader, skullFragmentShader);
	ofLogWarning("CreepyPortrait::setup") << "shader loaded: " << skullVertexShader << " + " << skullFragmentShader;
	// Load models
	if (model == "skull" || model == "all") {
		models.push_back(Model({"models/skull_mesh_1.ply", "models/skull_mesh_2.ply"},
								"models/skull_diffuse_1024.jpg",
								"models/skull_specular_1024.jpg",
								"models/skull_ao_1024.jpg",
								"models/skull_normal_1024.jpg",
								0.85,	// Skull scaled down 15% to prevent jaw clipping.
								30.0,	// Move the skull up to center.
								0.0,	// Don't rotate the skull.
								true,	// Has eyes.
								"eye.png"));
		ofLogWarning("CreepyPortrait::setup") << "model loaded: skull";
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
		ofLogWarning("CreepyPortrait::setup") << "model loaded: jackevil";
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
		ofLogWarning("CreepyPortrait::setup") << "model loaded: jackhappy";
	}
	// Parallel name list - must match models.push_back order above
	modelNames.clear();
	modelNames.push_back("skull");
	modelNames.push_back("jack_evil");
	modelNames.push_back("jack_happy");
	currentModelIndex = 0;
	currentModel = begin(models);
	// Prime the time delta for the first update loop run.
	lastUpdate = ofGetElapsedTimef();
	// Phase 10 - Scan audio/ dir and load all .wav files
	{
		ofDirectory audioDir("audio");
		audioDir.allowExt("wav");
		audioDir.listDir();
		for (auto& f : audioDir.getFiles()) {
			audioClips.push_back(f.path());
		}
		if (audioClips.empty()) {
			audioClips.push_back("audio/Test_Clip.wav");
		}
		std::sort(audioClips.begin(), audioClips.end());
		ofLogWarning("CreepyPortrait::setup") << "audio clips found: " << audioClips.size();
		for (auto& c : audioClips) ofLogWarning("CreepyPortrait::setup") << "  " << c;
	}
	soundPlayer.load(audioClips[0]);
	currentClipName = ofFilePath::getFileName(audioClips[0]);
	soundPlayer.setLoop(false);
	soundPlayer.setVolume(1.0f);
	audioRepeatDelay = ofRandom(8.0f, 20.0f);
	audioRepeatTimer = 0.0f;
	audioWanderTimer = ofRandom(15.0f, 45.0f);
	// Phase 7 - Seed blink timer
	dilationTimer = ofRandom(2.0f, 8.0f);
	// Phase 7 - Seed microsaccade timer
	microsaccadeTimer = ofRandom(0.5f, 2.0f);
	// Phase 7 - Seed dart timer
	dartTimer = 3.0f;
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
	// Phase 8 - Float Drift
	ofTranslate(wanderDrift.x, wanderDrift.y, 0);
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

    // System info overlay (toggle with 'i')
    if (showSysInfo) {
        std::string info = getSysInfoString();
        // Count lines for background rect height
        int lineCount = 0;
        for (char ch : info) if (ch == 0x0a) lineCount++;
        int boxX = ofGetWidth() - 462;
        int boxY = 14;
        int boxW = 458;
        int boxH = lineCount * 14 + 16;
        // Draw background rect via raw GL - bypasses OF blend state entirely
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, ofGetWidth(), ofGetHeight(), 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
        glRecti(boxX, boxY, boxX + boxW, boxY + boxH);
        glEnable(GL_BLEND);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        ofPushStyle();
        ofEnableAlphaBlending();
        ofSetColor(200, 255, 200, 255);
        ofDrawBitmapString(info, boxX + 6, boxY + 14);
        ofPopStyle();
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
		currentRotation.x = sin(time * 0.37f + 1.3f) * 12.0f + sin(time * 0.13f + 0.7f) * 5.0f;
		currentRotation.y = sin(time * 0.29f + 2.1f) * 20.0f + sin(time * 0.07f + 1.5f) * 8.0f;
		// Phase 8 - Float Drift during wander
		wanderDrift.x = sin(time * 0.11f + 0.5f) * 80.0f + sin(time * 0.07f + 1.2f) * 30.0f;
		wanderDrift.y = sin(time * 0.09f + 2.3f) * 40.0f + sin(time * 0.05f + 0.8f) * 20.0f;
	} else {
		// Phase 8 - Lerp drift back to center when face present
		wanderDrift = glm::mix(wanderDrift, glm::vec2(0.0f, 0.0f), 0.05f);
	}
	// Phase 7 - Eye pulsate + blink
	if (eyeAnimEnabled) {
		// Pulsate
		pupilScale = 1.4f + sin(time * 1.8f) * 0.08f;
		// Blink - three phases: close, hold, open
		dilationTimer -= delta;
		if (dilationTimer <= 0.0f && !twitching) {
			twitching = true;
			twitchTimer = 0.0f;
			twitchDuration = 0.6f;
			dilationTimer = ofRandom(2.0f, 8.0f);
		}
		if (twitching) {
			twitchTimer += delta;
			float t = twitchTimer / twitchDuration;
			if (t < 0.35f) {
				twitchAmount = t / 0.35f;
			} else if (t < 0.65f) {
				twitchAmount = 1.0f;
			} else {
				twitchAmount = 1.0f - ((t - 0.65f) / 0.35f);
			}
			if (twitchTimer >= twitchDuration) {
				twitching = false;
				twitchTimer = 0.0f;
				twitchAmount = 0.0f;
			}
		}
		// Microsaccade - tiny random UV jitter
		microsaccadeTimer -= delta;
		if (microsaccadeTimer <= 0.0f) {
			microsaccadeOffset += glm::vec2(ofRandom(-0.015f, 0.015f), ofRandom(-0.015f, 0.015f));
			microsaccadeOffset = glm::clamp(microsaccadeOffset, glm::vec2(-0.02f, -0.02f), glm::vec2(0.02f, 0.02f));
			microsaccadeTimer = ofRandom(0.5f, 2.0f);
		}
		microsaccadeOffset = glm::mix(microsaccadeOffset, glm::vec2(0.0f, 0.0f), 0.03f);
		// Dart - quick snap to random position, hold, return
		dartTimer -= delta;
		if (dartTimer <= 0.0f && !darting) {
			dartOffset = glm::vec2(ofRandom(-0.08f, 0.08f), ofRandom(-0.08f, 0.08f));
			darting = true;
			dartHoldTimer = ofRandom(0.3f, 0.7f);
			dartTimer = 3.0f;
		}
		if (darting) {
			dartHoldTimer -= delta;
			if (dartHoldTimer <= 0.0f) {
				darting = false;
			}
		} else {
			dartOffset = glm::mix(dartOffset, glm::vec2(0.0f, 0.0f), 0.1f);
		}
		// Combine microsaccade and dart into final offset
		glm::vec2 finalOffset = microsaccadeOffset + dartOffset;
		microsaccadeOffset = glm::clamp(finalOffset, glm::vec2(-0.05f, -0.05f), glm::vec2(0.05f, 0.05f));
	}

	// j key - jaw toggle independent of audio (only when sound not playing)
	if (!soundPlayer.isPlaying()) {
		if (jawOpen) currentModel->jawAngle = ofLerp(currentModel->jawAngle, 25.0f, 0.12f);
		else currentModel->jawAngle = ofLerp(currentModel->jawAngle, 0.0f, 0.12f);
	}
	// Phase 10 - Audio state machine (random clips, replay, wander audio)
	bool faceNow = detector.isFaceDetected();
	// Phase 13 - Auto LED state based on face detection
	{
		std::string ledState = faceNow ? "active" : ((!rotateSkull && (time - faceLastSeen) > noFaceWanderSeconds) ? "ember" : "fade");
		if (ledState != lastLedState) {
			std::ofstream lf(ofToDataPath("led_state.txt"));
			if (lf) { lf << ledState; }
			lastLedState = ledState;
		}
	}
	if (!audioClips.empty()) {
		bool wandering = (!rotateSkull && (time - faceLastSeen) > noFaceWanderSeconds);
		if (faceNow && audioState == AUDIO_IDLE) {
			// Face appeared - pick and play a random clip
			int idx = (int)ofRandom(0, (float)audioClips.size());
			idx = ofClamp(idx, 0, (int)audioClips.size() - 1);
			soundPlayer.load(audioClips[idx]);
			currentClipName = ofFilePath::getFileName(audioClips[idx]);
			soundPlayer.play();
			ofLogWarning("CreepyPortrait") << "audio playing: " << audioClips[idx];
			audioState = AUDIO_TRIGGERED;
			audioRepeatTimer = 0.0f;
			audioRepeatDelay = ofRandom(8.0f, 20.0f);
		} else if (!faceNow && !wandering && audioState != AUDIO_IDLE) {
			// Face lost (not wandering) - stop audio
			soundPlayer.stop();
			audioState = AUDIO_IDLE;
		} else if (audioState == AUDIO_TRIGGERED && soundPlayer.isPlaying()) {
			audioState = AUDIO_PLAYING;
		} else if (audioState == AUDIO_PLAYING && !soundPlayer.isPlaying()) {
			// Clip finished - enter waiting state
			audioState = AUDIO_WAITING;
			audioRepeatTimer = 0.0f;
			audioRepeatDelay = ofRandom(8.0f, 20.0f);
		} else if (faceNow && audioState == AUDIO_WAITING) {
			// Face still present - count down to replay
			audioRepeatTimer += delta;
			if (audioRepeatTimer >= audioRepeatDelay) {
				int idx = (int)ofRandom(0, (float)audioClips.size());
				idx = ofClamp(idx, 0, (int)audioClips.size() - 1);
				soundPlayer.load(audioClips[idx]);
				currentClipName = ofFilePath::getFileName(audioClips[idx]);
				soundPlayer.play();
				ofLogWarning("CreepyPortrait") << "audio replay: " << audioClips[idx];
				audioState = AUDIO_TRIGGERED;
				audioRepeatTimer = 0.0f;
				audioRepeatDelay = ofRandom(8.0f, 20.0f);
			}
		}
		// Wander audio - occasional random clip during idle wander
		if (wandering && audioState == AUDIO_IDLE) {
			audioWanderTimer -= delta;
			if (audioWanderTimer <= 0.0f) {
				int idx = (int)ofRandom(0, (float)audioClips.size());
				idx = ofClamp(idx, 0, (int)audioClips.size() - 1);
				soundPlayer.load(audioClips[idx]);
				currentClipName = ofFilePath::getFileName(audioClips[idx]);
				soundPlayer.play();
				ofLogWarning("CreepyPortrait") << "audio wander: " << audioClips[idx];
				audioState = AUDIO_TRIGGERED;
				audioWanderTimer = ofRandom(15.0f, 45.0f);
			}
		} else if (!wandering) {
			audioWanderTimer = ofRandom(15.0f, 45.0f);
		}
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
	float targetJaw = ofMap(smoothAmplitude, 0.0f, jawThreshold, 0.0f, 25.0f, true);
	currentModel->jawAngle = ofLerp(currentModel->jawAngle, targetJaw, jawLerp);
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
	if (key == 'i') {
        showSysInfo = !showSysInfo;
        ofLogWarning("CreepyPortrait") << "key i: sysinfo " << (showSysInfo ? "ON" : "OFF");
    }
    else if (key == 'v') {
		displayVideo = !displayVideo;
		ofLogWarning("CreepyPortrait") << "key v: video overlay " << (displayVideo ? "ON" : "OFF");
	}
	else if (key == 'r') {
		rotateSkull = !rotateSkull;
		ofLogWarning("CreepyPortrait") << "key r: rotate " << (rotateSkull ? "ON" : "OFF");
		if (rotateSkull) {
			faceLastSeen = ofGetElapsedTimef(); // exit wander when r activates
		}
	}
	else if (key == 'm') {
		currentModel++;
		if (currentModel == end(models)) {
			currentModel = begin(models);
			currentModelIndex = 0;
		}
	}
	else if (key == 'c') {
		targetRotation = glm::vec2(0, 0);
		currentRotation = glm::vec2(0, 0);
		faceLastSeen = ofGetElapsedTimef(); // Phase 7 - exit wander mode
		rotateSkull = false; // stop r rotation mode
		ofLogWarning("CreepyPortrait") << "key c: center + wander reset";
	}
	else if (key == '[') {
		jawThreshold = max(0.005f, jawThreshold - 0.005f);
		ofLogWarning("CreepyPortrait") << "key [: jawThreshold " << jawThreshold;
	}
	else if (key == ']') {
		jawThreshold = min(0.1f, jawThreshold + 0.005f);
		ofLogWarning("CreepyPortrait") << "key ]: jawThreshold " << jawThreshold;
	}
	else if (key == '-') {
		jawLerp = max(0.01f, jawLerp - 0.01f);
		ofLogWarning("CreepyPortrait") << "key -: jawLerp " << jawLerp;
	}
	else if (key == '=') {
		jawLerp = min(0.5f, jawLerp + 0.01f);
		ofLogWarning("CreepyPortrait") << "key =: jawLerp " << jawLerp;
	}
	else if (key == 'j') {
		// j key - toggle jaw open/closed only, no audio
		jawOpen = !jawOpen;
		ofLogWarning("CreepyPortrait") << "key j: jaw " << (jawOpen ? "OPEN" : "CLOSED");
	}
	else if (key == 's') {
		// s key - play a random clip
		if (!audioClips.empty()) {
			int idx = (int)ofRandom(0, (float)audioClips.size());
			idx = ofClamp(idx, 0, (int)audioClips.size() - 1);
			soundPlayer.load(audioClips[idx]);
			currentClipName = ofFilePath::getFileName(audioClips[idx]);
			soundPlayer.play();
			ofLogWarning("CreepyPortrait") << "audio s-key: " << audioClips[idx];
		}
	}
	else if (key == 'w') {
		// w key - toggle wander via explicit flag
		forceWander = !forceWander;
		ofLogWarning("CreepyPortrait") << "key w: wander " << (forceWander ? "ON" : "OFF");
		if (forceWander) {
			faceLastSeen = -99999.0f;
		} else {
			faceLastSeen = ofGetElapsedTimef();
		}
	}
	else if (key == 'e') {
		// e key - toggle eye animation on/off
		eyeAnimEnabled = !eyeAnimEnabled;
		ofLogWarning("CreepyPortrait") << "key e: eye anim " << (eyeAnimEnabled ? "ON" : "OFF");
	}
	else if (key == 'l') {
		// l key - cycle LED candle state: ember -> active -> fade -> ember
		std::string states[] = {"ember", "active", "fade"};
		std::string current = "ember";
		std::ifstream rf(ofToDataPath("led_state.txt"));
		if (rf.good()) std::getline(rf, current);
		rf.close();
		int idx = 0;
		for (int i = 0; i < 3; i++) { if (states[i] == current) { idx = (i + 1) % 3; break; } }
		std::ofstream wf(ofToDataPath("led_state.txt"));
		wf << states[idx];
		wf.close();
		ofLogWarning("CreepyPortrait") << "key l: LED state -> " << states[idx];
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

//--------------------------------------------------------------
std::string CreepyPortrait::getSysInfoString() {
    std::ostringstream out;

    // ── Date / time ──────────────────────────────────────────────────────────────────────────────────────────────────────
    time_t now = time(nullptr);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d  %H:%M:%S", localtime(&now));
    out << "Date/Time    : " << timebuf << "\n";

    // App uptime
    float appSecs = ofGetElapsedTimef() - appStartTime;
    int ah  = (int)(appSecs / 3600);
    int am  = (int)(appSecs / 60) % 60;
    int as_ = (int)appSecs % 60;
    char appbuf[32];
    snprintf(appbuf, sizeof(appbuf), "%02dh %02dm %02ds", ah, am, as_);
    out << "App uptime   : " << appbuf << "\n";

    // System uptime + boot time
    {
        std::ifstream f("/proc/uptime");
        double uptimeSecs = 0;
        if (f >> uptimeSecs) {
            time_t bootEpoch = now - (time_t)uptimeSecs;
            char bootbuf[64];
            strftime(bootbuf, sizeof(bootbuf), "%Y-%m-%d %H:%M", localtime(&bootEpoch));
            int uh = (int)(uptimeSecs / 3600);
            int um = (int)(uptimeSecs / 60) % 60;
            char upbuf[32];
            snprintf(upbuf, sizeof(upbuf), "%02dh %02dm", uh, um);
            out << "System up    : " << upbuf << "  (booted " << bootbuf << ")\n";
        }
    }

    // Hostname
    {
        char hbuf[256] = {};
        if (gethostname(hbuf, sizeof(hbuf)) == 0)
            out << "Hostname     : " << hbuf << "\n";
    }

    // IP addresses (all active non-loopback interfaces)
    {
        struct ifaddrs *ifap = nullptr;
        if (getifaddrs(&ifap) == 0) {
            for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr) continue;
                if (!(ifa->ifa_flags & IFF_UP)) continue;
                std::string iname(ifa->ifa_name);
                if (iname == "lo") continue;
                if (ifa->ifa_addr->sa_family == AF_INET) {
                    char ipbuf[INET_ADDRSTRLEN] = {};
                    inet_ntop(AF_INET,
                        &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr,
                        ipbuf, sizeof(ipbuf));
                    { std::string lbl = "IP (" + iname + ")"; while ((int)lbl.size() < 12) lbl += " "; out << lbl << " : " << ipbuf << "\n"; }
                }
            }
            // MAC addresses via sysfs
            for (struct ifaddrs *ifa = ifap; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr) continue;
                if (!(ifa->ifa_flags & IFF_UP)) continue;
                std::string iname(ifa->ifa_name);
                if (iname == "lo") continue;
                if (ifa->ifa_addr->sa_family == AF_PACKET) {
                    std::ifstream mf("/sys/class/net/" + iname + "/address");
                    std::string mac;
                    if (std::getline(mf, mac))
                        { std::string lbl = "MAC (" + iname + ")"; while ((int)lbl.size() < 12) lbl += " "; out << lbl << " : " << mac << "\n"; }
                }
            }
            freeifaddrs(ifap);
        }
    }

    // Kernel
    {
        std::ifstream f("/proc/version");
        std::string line;
        if (std::getline(f, line)) {
            if (line.size() > 58) line = line.substr(0, 58) + "...";
            out << "Kernel       : " << line << "\n";
        }
    }

    // CPU model
    {
        std::ifstream f("/proc/cpuinfo");
        std::string line;
        bool found = false;
        while (std::getline(f, line) && !found) {
            if (line.rfind("Model name", 0) == 0 || line.rfind("Model", 0) == 0) {
                auto colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string cpu = line.substr(colon + 2);
                    if (cpu.size() > 42) cpu = cpu.substr(0, 42) + "...";
                    out << "CPU model    : " << cpu << "\n";
                    found = true;
                }
            }
        }
    }

    // CPU frequency
    {
        std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
        long freqKhz = 0;
        if (f >> freqKhz) {
            char fbuf[32];
            snprintf(fbuf, sizeof(fbuf), "%.0f MHz", freqKhz / 1000.0f);
            out << "CPU freq     : " << fbuf << "\n";
        }
    }

    // CPU temperature
    {
        std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
        int millideg = 0;
        if (f >> millideg) {
            float c = millideg / 1000.0f;
            char tbuf[64];
            snprintf(tbuf, sizeof(tbuf), "%.1f C  (%.1f F)", c, c * 9.0f / 5.0f + 32.0f);
            out << "CPU temp     : " << tbuf << "\n";
        }
    }

    // RAM
    {
        std::ifstream f("/proc/meminfo");
        std::string line;
        long totalKB = 0, availKB = 0;
        while (std::getline(f, line)) {
            if (line.rfind("MemTotal:",     0) == 0) sscanf(line.c_str(), "MemTotal: %ld",     &totalKB);
            if (line.rfind("MemAvailable:", 0) == 0) sscanf(line.c_str(), "MemAvailable: %ld", &availKB);
        }
        if (totalKB > 0) {
            float usedMB  = (totalKB - availKB) / 1024.0f;
            float totalMB = totalKB / 1024.0f;
            char rbuf[64];
            snprintf(rbuf, sizeof(rbuf), "%.0f MB used / %.0f MB total  (%.0f%% used)",
                     usedMB, totalMB, 100.0f * usedMB / totalMB);
            out << "RAM          : " << rbuf << "\n";
        }
    }

    // Disk
    {
        struct statvfs sv;
        if (statvfs("/", &sv) == 0) {
            float totalGB = (float)(sv.f_blocks * sv.f_frsize) / (1024.0f*1024*1024);
            float freeGB  = (float)(sv.f_bfree  * sv.f_frsize) / (1024.0f*1024*1024);
            float usedGB  = totalGB - freeGB;
            char dbuf[64];
            snprintf(dbuf, sizeof(dbuf), "%.1f GB used / %.1f GB total  (%.0f%% used)",
                     usedGB, totalGB, 100.0f * usedGB / totalGB);
            out << "Disk /       : " << dbuf << "\n";
        }
    }

    // ── App stats ─────────────────────────────────────────────────────────────────────────────────────────────────────────────────
    out << "\n";

    // FPS
    {
        char fpsbuf[32];
        snprintf(fpsbuf, sizeof(fpsbuf), "%.1f", ofGetFrameRate());
        out << "FPS          : " << fpsbuf << "\n";
    }

    // Current model name
    if (currentModelIndex >= 0 && currentModelIndex < (int)modelNames.size())
        out << "Model        : " << modelNames[currentModelIndex] << "\n";

    // Rotation angles
    {
        char rotbuf[64];
        snprintf(rotbuf, sizeof(rotbuf), "x=%.1f  y=%.1f", currentRotation.x, currentRotation.y);
        out << "Rotation     : " << rotbuf << "\n";
    }

    // Face state
    {
        float sinceface = ofGetElapsedTimef() - faceLastSeen;
        char fbuf[64];
        if (sinceface < 1.0f)
            snprintf(fbuf, sizeof(fbuf), "YES");
        else
            snprintf(fbuf, sizeof(fbuf), "no  (%.0fs ago)", sinceface);
        out << "Face         : " << fbuf << "\n";
    }

    // Candle LED state
    {
        const std::string ledPath = ofToDataPath("led_state.txt");
        struct stat st;
        if (stat(ledPath.c_str(), &st) != 0) {
            out << "Candle       : not running\n";
        } else {
            std::ifstream f(ledPath);
            std::string state;
            std::getline(f, state);
            if (state.empty()) state = "unknown";
            time_t fileAge = time(nullptr) - st.st_mtime;
            if (fileAge > 10)
                state += " (stale)";
            out << "Candle       : " << state << "\n";
        }
    }
    // Current WAV
    {
        if (soundPlayer.isPlaying()) {
            out << "WAV          : " << currentClipName << "\n";
        } else {
            if (currentClipName == "none")
                out << "WAV          : none\n";
            else
                out << "WAV          : last: " << currentClipName << "\n";
        }
    }

    return out.str();
}
