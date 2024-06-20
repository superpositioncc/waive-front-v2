R""(
#version 410 core
precision highp float;

uniform float threshold;
uniform float width;
uniform sampler2D tex;

in vec2 v_texCoord;

out vec4 color;

void main()
{
    vec4 smpl = texture(tex, vec2(v_texCoord.x, 1.0 - v_texCoord.y));

    float multiplier = float(smpl.x > threshold - width / 2.0 && smpl.x < threshold + width / 2.0);
if (multiplier == 0.0) {
    discard;
}
    color =  vec4(vec3(smpl.x, smpl.y, smpl.z) * multiplier, 1.0);
}
)""
