#version 450

//TODO: sampler for albedo here...

layout( location = 0 ) in vec4 inWorldNormal;
layout( location = 1 ) in vec4 inWorldPosition;
layout( location = 2 ) in vec2 inTexCoord;
layout( location = 3 ) in vec4 inColor;


layout( location = 0 ) out vec4 outNormal;
layout( location = 1 ) out vec4 outPosition;
layout( location = 2 ) out vec4 outAlbedo;

void main()
{
	outNormal = inWorldNormal;
	outPosition = inWorldPosition;
	outAlbedo = inColor;
}