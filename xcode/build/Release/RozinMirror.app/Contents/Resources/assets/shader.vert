#version 150

uniform mat4        ciModelViewProjection;
uniform mat4        ciProjectionMatrix, ciViewMatrix;
uniform mat3        ciNormalMatrix;

in vec4             ciPosition;
in vec2             ciTexCoord0;
in vec3             ciNormal;
in vec4             ciColor;
in mat4             vInstanceMatrix; // per-instance position variable
out highp vec2      TexCoord;
out lowp vec4       Color;
out highp vec3      Normal;

void main( void )
{
//    gl_Position    = ciModelViewProjection * vInstanceMatrix * ciPosition;
    gl_Position     = ciProjectionMatrix * ciViewMatrix * vInstanceMatrix * ciPosition;
    Color           = ciColor;
    TexCoord        = ciTexCoord0;
    Normal          = vec3( ciViewMatrix * vInstanceMatrix * vec4( ciNormal, 0 ) );
}

