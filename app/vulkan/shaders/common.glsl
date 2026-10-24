

// cheap hash, no uniforms/includes needed — just gl_FragCoord as seed
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}


vec2 random01(float seed)
{
    return vec2(hash(vec2(seed, 0.0)), hash(vec2(seed, 1.0)));
}