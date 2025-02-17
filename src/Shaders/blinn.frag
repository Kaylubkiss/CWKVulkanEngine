#version 450 


const int maxLights = 1;


layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 viewDir;
layout(location = 3) in vec3 normal;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

//baked for now.

const vec4 I =              { 0.6f, 0.6f, 0.6f, 1.0f };     /*  intensity of each light source */
const vec4 ambientAlbedo =  { 0.6f, 0.6f, 0.6f, 1.0f };     /*  ambient albedo of material */
const vec4 diffuseAlbedo =  { 0.6f, 0.6f, 0.6f, 1.0f };     /*  diffuse albedo of material */

const vec4 specularAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };     /*  specular albedo of material */

const vec4 ambient = I * ambientAlbedo;
const vec4 diffuse = I * diffuseAlbedo;
const vec4 specular = I * specularAlbedo;
const int specularPower = 10;                              /*  specular power of material */


void main()
{
	outColor = texture(texSampler, fragTexCoord);	
	outColor.a = 1;

//    vec4 V = normalize(vec4(viewDir,0));
//    
//    vec2 texCoords = fragTexCoord;
//
//    vec4 Clight = vec4(texture(texSampler, texCoords).rgb, 1.0);
//
//    vec4 N = normalize(vec4(normal, 0.0f));
//
//
//    vec4 diffuse_accum = vec4(0,0,0,0);
//    vec4 specular_accum = vec4(0,0,0,0);
//    
//
//	 vec4 diffuse_portion = vec4(0.0f, 0.0f, 0.0f, 0.0f);
//    
//     vec4 specular_portion = vec4(0.0f, 0.0f, 0.0f, 0.0f);
//           
//     vec4 L = normalize(vec4(lightDir[0], 0));
//          
//
//     float m_dot_L = dot(N, L);
//
//            //////        equation portions  /////////
//    
//     if (m_dot_L > 0.0f)
//     { 
//        diffuse_portion = diffuse * m_dot_L; 
//     }
//
//      /// Use halfway vector to calculate specular light/////
//      vec4 H = normalize(L + V);
//      
//      float N_dot_H = dot(N, H);
//      
//      if (N_dot_H > 0.0f)
//      {
//          specular_portion = 
//          specular * pow(N_dot_H, specularPower);
//      }
//
//      diffuse_accum += diffuse_portion;
//      specular_accum += specular_portion;
//        
//      outColor = (ambient + diffuse_accum + specular_accum) * Clight;
//
//      outColor.a = 1;
}														