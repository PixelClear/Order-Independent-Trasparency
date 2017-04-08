
uniform mat4 MVP;
uniform mat4 WorldMatrix;


vec3 WorldPos;
out vec4 in_color;
vec3 in_normal;
 

vec3 EyeWorldPos = vec3(0.0,0.0,-5.0);
vec3 light_direction = vec3(125.0,125.0,0.0);
vec3 light_color = vec3(0.5,0.5,1.0);

float ambient_intensity = 0.1;
float diffuse_intensity = 0.5;
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
                                                                                            
    return (AmbientColor + SpecularColor);                  
}

void main()
{
	WorldPos = (WorldMatrix * gl_Vertex).xyz;
	in_normal= (WorldMatrix * vec4(gl_Normal, 0.0)).xyz;
	in_normal = normalize(in_normal);
	in_color = CalcLightInternal(light_color, light_direction, in_normal);
	gl_Position = MVP * gl_Vertex;
}
