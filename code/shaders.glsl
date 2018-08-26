@vs mainVS
uniform gl {
	mat4 viewProj;
};

uniform gba {
	mat4 model;
};

in vec4 position;
in vec2 texcoord0;

out vec2 uv;

void main() {
	vec4 pos;
	pos.xy = (mat3(model) * vec3(position.xy, 1.0)).xy;
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
