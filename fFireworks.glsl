// Fragment shader for fireworks
#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
              //      due to different settings of the default GLSL version
in  vec4 color;
in vec4 fPosition;
out vec4 fColor;

void main() 
{ 
    if (fPosition.y < 0.1) {
	discard;
    }

    fColor = color;
} 