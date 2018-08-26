@vs mainVS
uniform params {
    mat4 mvp;
};

in vec4 position;
in vec2 texcoord0;
out vec2 uv;

void main() {
    gl_Position = mvp * position;
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
