// Vertex shader for fireworks
#version 150

in  vec3 vVelocity;
in  vec4 vPosition;
in  vec4 vColor;
out vec4 color;
out vec4 fPosition;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform float t;

void main() 
{
    fPosition = vPosition;
    float X, Y, Z;
    float a = -0.00000049;

    fPosition.x = vPosition.x + 0.001 * vVelocity.x * t;
    fPosition.y = vPosition.y + 0.001 * vVelocity.y * t + 0.5 * a * t * t;
    fPosition.z = vPosition.z + 0.001 * vVelocity.z * t;

    //Position = vec4(X, Y, Z, 1.0);
    gl_Position = Projection * ModelView * fPosition;
    
    color = vColor;
} 

