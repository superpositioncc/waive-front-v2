/*
WAIVE-FRONT
Copyright (C) 2024  Bram Bogaerts, Superposition

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file main.frag
 * @brief The main fragment shader for the WAIVE-FRONT -- the meat and potatoes of the program.
 *
 */

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
