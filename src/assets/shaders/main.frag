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

in vec2 v_texCoord;

out vec4 color;

void main()
{
    vec2 random = vec2(random(v_texCoord.xy), random(v_texCoord.yx)) * 2.0 - 1.0;
    
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

//     float multiplier = float(smpl.x > threshold - width / 2.0 && smpl.x < threshold + width / 2.0);
// if (multiplier == 0.0) {
//     discard;
// }
//     color =  vec4(vec3(smpl.x, smpl.y, smpl.z) * multiplier, 1.0);
    
    color = vec4(vec3(smpl.x, smpl.y, smpl.z), 1.0);
}
)""
