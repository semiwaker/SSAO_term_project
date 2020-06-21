#version 450 core
out vec4 FragColor;
in vec3 localPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}
vec3 blur(vec2 uv)
{
    vec2 texelSize = 1.0 / vec2(textureSize(equirectangularMap, 0));
    vec4 result = vec4(0.0);
    for (int x = -3; x < 3; ++x)
    {
        for (int y = -3; y < 3; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(equirectangularMap, uv + offset);
        }
    }
    return result.rgb;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec3 color = blur(uv);

    FragColor = vec4(color, 1.0);
}