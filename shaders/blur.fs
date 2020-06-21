# version 450 core

in vec2 texCoord;

out vec4 fragColor;

uniform sampler2D textureSSAO;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(textureSSAO, 0));
    vec4 result = vec4(0.0);
    for (int x = -3; x < 3; ++x)
    {
        for (int y = -3; y < 3; ++y)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(textureSSAO, texCoord + offset);
        }
    }
    fragColor = result / (7.0 * 7.0);
}