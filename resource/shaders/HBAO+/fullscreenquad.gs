#version 150

layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

in vec2 texCoord[3];

out vec2 outTexCoord;

void main()
{
	for (int i = 0; i < 3; i++) {
                outTexCoord = texCoord[i];
		gl_Layer = gl_PrimitiveIDIn;
		gl_PrimitiveID = gl_PrimitiveIDIn;
		gl_Position = gl_in[i].gl_Position;
		EmitVertex();
	}
}
