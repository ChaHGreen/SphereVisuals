/* 
File Name: "fshader53.glsl":
           Fragment Shader
*/

#version 150  // YJC: Comment/un-comment this line to resolve compilation errors
              //      due to different settings of the default GLSL version

in  vec4 color;
in  vec4 ePosition;
in vec2 GtexCoord;
in vec2 StexCoord;
in vec2 LattCoord;
out vec4 fColor;

// lighting
uniform bool uUseLighting;

uniform sampler2D texture_2D;
uniform sampler1D texture_1D;

// fog
uniform float linear_start;
uniform float linear_end;
uniform float fog_density;
uniform vec4 fog_color, shadow_color;
uniform int fog_mode;
uniform bool shadow;

uniform int sphereTex;
uniform int tex_mode;
uniform int frame_mode;
uniform int sphereCB;
uniform int lattice_mode;


void main() 
{
    if (uUseLighting)
    {   
    if(!shadow){// Sphere, ground
        vec4 newColor = color;
        if (tex_mode == 1)
        {
             newColor = color * texture(texture_2D, GtexCoord);
        }
        if (sphereTex == 1 || sphereTex == 2 )
        {
            newColor = color * texture(texture_1D, StexCoord.x);
        } 
        if (sphereCB == 1)
        {
            newColor = color * texture(texture_2D, StexCoord);
            if (newColor.g > newColor.r && newColor.g > newColor.b)
            {
                newColor = color * vec4(0.9, 0.1, 0.1, 1.0);
            }
        }
    if ( (lattice_mode == 1 || lattice_mode == 2) 
                && (fract(4 * LattCoord.x) < 0.35 
                &&  fract(4 * LattCoord.y) < 0.35)) discard;

        // fog
        float z = length(ePosition.xyz);
        float fog_factor;
        switch(fog_mode)
        {
            case 0:
                fog_factor = 1;  // No fog
                break;
            case 1:
                fog_factor = (linear_end - z)/(linear_end - linear_start) ;   // Linear fog
                break;
            case 2:
                fog_factor = exp(-(fog_density * z));   // Exp fog
                break;
            case 3:
                fog_factor = exp(-pow((fog_density * z), 2));   // Exp Sqr fog
                break;
        }
        fog_factor = clamp(fog_factor, 0.0, 1.0);
        fColor = mix(fog_color, newColor, fog_factor);}

    }
    else if (shadow && fog_mode == 0)
    {
        fColor = vec4(0.25, 0.25, 0.25, 0.65);
        if ( (lattice_mode == 1 || lattice_mode == 2) 
                && (fract(4 * LattCoord.x) < 0.35 
                &&  fract(4 * LattCoord.y) < 0.35)) discard;
        
    }
    else if( shadow&&fog_mode != 0)
    {
        if ( (lattice_mode == 1 || lattice_mode == 2) 
                && (fract(4 * LattCoord.x) < 0.35 
                &&  fract(4 * LattCoord.y) < 0.35)) discard;
        vec4 shadowColor = vec4(0.25, 0.25, 0.25, 0.65); // Shadow when fog
        float fog_factor = 1.0;
        float z = length(ePosition.xyz);
        switch(fog_mode){
        case 0:
            fog_factor = 1;  // No fog
            break;
        case 1:
            fog_factor = (linear_end - z)/(linear_end - linear_start) ;   // Linear fog
            break;
        case 2:
            fog_factor = exp(-(fog_density * z));   // Exp fog
            break;
        case 3:
            fog_factor = exp(-pow((fog_density * z), 2));   // Exp Sqr fog
            break;
        }
        fog_factor = clamp(fog_factor, 0.0, 1.0);
        fColor = mix(shadowColor, fog_color, fog_factor);
    }
    else
    {
        fColor = color;
    }
} 

