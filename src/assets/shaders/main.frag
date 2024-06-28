R""(
#version 410 core
precision highp float;

)""
#include "common.glsl"
R""(

uniform float focusAmount;
uniform float blurSize;
uniform sampler2D tex;
uniform vec3[5] colors;
uniform int colorIndex;
uniform float time;

in vec2 v_texCoord;

out vec4 color;

void main()
{
    vec2 random = vec2(random(v_texCoord.xy + fract(time)), random(v_texCoord.yx + fract(time))) * 2.0 - 1.0;
    
    vec4 smpl = texture(tex, vec2(v_texCoord.x, 1.0 - v_texCoord.y) + random * blurSize * (1.0 - focusAmount));

    int closestIndex = 0;
    float closestDist = distance(smpl.xyz, colors[closestIndex]);

    for (int i = 1; i < 5; i++) {
        float dist = distance(smpl.xyz, colors[i]);
        if (dist < closestDist) {
            closestIndex = i;
            closestDist = dist;
        }
    }

    if (closestIndex != colorIndex) {
        discard;
    }

    color = vec4(vec3(smpl.x, smpl.y, smpl.z), 1.0);
}
)""
