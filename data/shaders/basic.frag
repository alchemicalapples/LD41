precision mediump float;

varying vec2 v_texcoord;
varying vec3 v_normal;

uniform sampler2D s_texture;
uniform vec3 cam_forward;

void main()
{
    if (dot(v_normal, cam_forward) > 0.0) discard;
    gl_FragColor = texture2D(s_texture, v_texcoord);
}
