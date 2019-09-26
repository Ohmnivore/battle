@vs mainVS
uniform gl {
    mat4 viewProj;
};

uniform gba {
    vec2 size;
    mat4 model;
};

in vec4 position;
in vec2 texcoord0;

out vec2 uv;

void main() {
    // Workaround for https://github.com/floooh/oryol/issues/308
    mat3 model2D = mat3(model);

    vec4 pos;
    pos.xy = (model2D * vec3(position.xy * size, 1.0)).xy;
    pos.z = position.z;
    pos.w = position.w;

    gl_Position = viewProj * pos;
    uv = texcoord0;
}
@end

@fs mainFS
uniform sampler2D tex;

in vec2 uv;

out vec4 fragColor;

void main() {
    fragColor = texture(tex, uv);
}
@end

@program MainShader mainVS mainFS




@fs paletteFS
uniform sampler2D tex;
uniform sampler2D paletteTex;

uniform palette1 {
    float start1;
    float end1;
    float offset1;
};
uniform palette2 {
    float start2;
    float end2;
    float offset2;
};

in vec2 uv;

out vec4 fragColor;

void main() {
    vec4 indexColor = texture(tex, uv);

    float paletteIndex = indexColor.r;
    if (paletteIndex >= start1 && paletteIndex <= end1)
    {
        paletteIndex = start1 + mod(paletteIndex + offset1 - start1, end1 - start1);
    }
    else if (paletteIndex >= start2 && paletteIndex < end2)
    {
        paletteIndex = start2 + mod(paletteIndex + offset2 - start2, end2 - start2);
    }

    vec4 paletteColor = texture(paletteTex, vec2(paletteIndex, 0.5));
    paletteColor.a *= indexColor.a;
    fragColor = paletteColor;
}
@end

@program PaletteShader mainVS paletteFS




@vs screenQuadVS
in vec4 position;
in vec2 texcoord0;

out vec2 uv;

void main() {
    gl_Position = position;
    uv = vec2(texcoord0.x, 1.0 - texcoord0.y);
}
@end

@fs screenQuadFS
uniform sampler2D tex;

in vec2 uv;

out vec4 fragColor;

void main() {
    fragColor = texture(tex, uv);
}
@end

@program ScreenQuadShader screenQuadVS screenQuadFS
