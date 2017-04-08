#version 430 core 

layout (early_fragment_tests) in;

layout (binding = 0 , r32ui) uniform uimage2D head_pointer_image;
layout (binding = 1, rgba32ui) uniform writeonly uimageBuffer list_buffer;
layout (binding = 0, offset = 0) uniform atomic_uint list_counter;
layout (location = 0) out vec4 color;

uniform float alpha_factor;

in vec3 WorldPos;
in vec3 in_normal;
flat in vec3 light_color;

vec3 EyeWorldPos = vec3(0.0,0.0,-5.0);
vec3 light_direction = vec3(125.0,125.0,0.0);

float ambient_intensity = 0.9;
float diffuse_intensity = 0.7;
float SpecularPower = 32.0;
float SpecularIntensity = 1.0;

vec4 CalcLightInternal(vec3 light_color1, vec3 light_direction1, vec3 Normal)                                                  
{                                                                                           
    vec4 AmbientColor = vec4(light_color1 * ambient_intensity, 1.0f);
    float DiffuseFactor = dot(Normal, light_direction1);                                     
                                                                                            
    vec4 DiffuseColor  = vec4(0, 0, 0, 0);                                                  
    vec4 SpecularColor = vec4(0, 0, 0, 0);                                                  
                                                                                            
    if (DiffuseFactor > 0) {                                                                
        DiffuseColor = vec4(light_color1 * diffuse_intensity * DiffuseFactor, 1.0f);    
                                                                                            
        vec3 VertexToEye = normalize(EyeWorldPos - WorldPos);                             
        vec3 LightReflect = normalize(reflect(light_direction1, Normal));                     
        float SpecularFactor = dot(VertexToEye, LightReflect);                                      
        if (SpecularFactor > 0) {                                                           
            SpecularFactor = pow(SpecularFactor, SpecularPower);                               
            SpecularColor = vec4(light_color1, 1.0f) * SpecularIntensity * SpecularFactor;                         
        }                                                                                   
    }                                                                                       
                                                                                            
    return (AmbientColor+SpecularColor);                  
}

void main()                                                                                 
{      
       //This is next pointer of linked list
       uint index;                  
	   uint old_head;
	   uvec4 item;
	   vec4 frag_color;
	   vec4 modulator;

	   index = atomicCounterIncrement(list_counter);
	   
	   old_head = imageAtomicExchange(head_pointer_image, ivec2(gl_FragCoord.xy), uint(index));

       vec3 normal = normalize(in_normal);                                                         
       frag_color = CalcLightInternal(light_color, light_direction, normal);  
	   modulator = vec4(frag_color.rgb,alpha_factor);
	  	      
	   item.x = old_head;
	   item.y = packUnorm4x8(modulator);
	   item.z = floatBitsToUint(gl_FragCoord.z);
	   item.w = 0;
	   
	   imageStore(list_buffer,int(index), item);

	   color = frag_color;                                             
}
