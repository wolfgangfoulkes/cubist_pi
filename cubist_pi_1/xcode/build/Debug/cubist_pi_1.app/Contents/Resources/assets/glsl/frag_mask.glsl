#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect	t_masked;
uniform sampler2DRect   t_mask;
uniform float           f_width;
uniform float           f_height;

void main()
{
    vec2 v_coors;
    v_coors.x = gl_TexCoord[0].s * f_width;
    v_coors.y = gl_TexCoord[0].t * f_height;
    
	vec3 v_masked = texture2DRect( t_masked, v_coors ).rgb;
    vec3 v_mask = texture2DRect( t_mask, v_coors ).rgb;
    float alpha = v_mask.b;
    
    gl_FragColor = vec4(v_masked, alpha);
}