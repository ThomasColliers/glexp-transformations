#version 430

precision mediump float;

out vec4 gl_FragColor;
uniform sampler2D textureUnit;
smooth in vec2 vVaryingTexCoord;

void main(void){
    gl_FragColor = vec4(texture(textureUnit, vVaryingTexCoord));
}
