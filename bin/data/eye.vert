#version 150
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 textureMatrix;
in vec4 position;
in vec2 texcoord;
out vec2 texCoordVarying;
void main(){
    texCoordVarying = (textureMatrix * vec4(texcoord, 0.0, 1.0)).xy;
    gl_Position = modelViewProjectionMatrix * position;
}
