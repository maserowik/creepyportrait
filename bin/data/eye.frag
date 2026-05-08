#version 150
uniform sampler2D eyeTex;
uniform float pupilScale;
uniform vec2 pupilOffset;
in vec2 texCoordVarying;
out vec4 fragColor;
void main(){
    if (!gl_FrontFacing) discard;
    vec2 centered = texCoordVarying - vec2(0.5, 0.5);
    centered.y *= pupilScale;
    centered += pupilOffset;
    vec2 uv = centered + vec2(0.5, 0.5);
    vec4 color = texture(eyeTex, uv);
    float brightness = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    if (brightness < 0.05) discard;
    fragColor = color;
}
