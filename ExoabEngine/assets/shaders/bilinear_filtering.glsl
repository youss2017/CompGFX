
vec4 texture2D_bilinear(in sampler2D t, in vec2 uv, in vec2 textureSize, in vec2 texelSize)
{
    vec4 tl = texture2D(t, uv);
    vec4 tr = texture2D(t, uv + vec2(texelSize.x, 0.0));
    vec4 bl = texture2D(t, uv + vec2(0.0, texelSize.y));
    vec4 br = texture2D(t, uv + vec2(texelSize.x, texelSize.y));
    vec2 f = fract( uv * textureSize );
    vec4 tA = mix( tl, tr, f.x );
    vec4 tB = mix( bl, br, f.x );
    return mix( tA, tB, f.y );
}

/*
vec4 texture2D_bilinear(in sampler2D t, in vec2 uv, in vec2 textureSize, in vec2 texelSize)
{
    vec2 f = fract( uv * textureSize );
    uv += texelSize*0.06;    // <--- precision hack (anything higher breaks it)
    vec4 tl = texture2D(t, uv);
    vec4 tr = texture2D(t, uv + vec2(texelSize.x, 0.0));
    vec4 bl = texture2D(t, uv + vec2(0.0, texelSize.y));
    vec4 br = texture2D(t, uv + vec2(texelSize.x, texelSize.y));
    vec4 tA = mix( tl, tr, f.x );
    vec4 tB = mix( bl, br, f.x );
    return mix( tA, tB, f.y );
}

*/

/*
vec4 texture2D_bilinear(in sampler2D t, in vec2 uv, in vec2 textureSize, in vec2 texelSize)
{
    vec2 f = fract( uv * textureSize );
    uv += ( .5 - f ) * texelSize;    // move uv to texel centre
    vec4 tl = texture2D(t, uv);
    vec4 tr = texture2D(t, uv + vec2(texelSize.x, 0.0));
    vec4 bl = texture2D(t, uv + vec2(0.0, texelSize.y));
    vec4 br = texture2D(t, uv + vec2(texelSize.x, texelSize.y));
    vec4 tA = mix( tl, tr, f.x );
    vec4 tB = mix( bl, br, f.x );
    return mix( tA, tB, f.y );
}
*/