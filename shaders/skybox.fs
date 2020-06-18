#version 450 core
out vec4 fragColor;

in vec3 localPos;

uniform sampler2D hdr;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec4 color = texture(hdr, uv);
    float e = color.a;
    vec3 envColor = color.rgb;

    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2));

    fragColor = vec4(envColor, 1.0);
    // fragColor = vec4(vec3(exp2(e-128)), 1.0);
    // fragColor = vec4(localPos, 1.0);
}