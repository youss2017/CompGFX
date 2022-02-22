#pragma once
#include <memory/Buffer2.hpp>
// immediate mode render for debuging

struct DebugVertex {
	float x, y, z;
};

class DebugRenderer
{
public:
	DebugRenderer(GraphicsContext context);
	void Destroy();
	void DrawPoint(float x, float y, float z);
	void DrawLine(float x, float y, float z, float x2, float y2, float z2);
	void DrawCircle(float x, float y, float z, float r);
	void DrawBox(float x, float y, float z, float x2, float y2, float z2);
	void DrawVertices(DebugVertex* vertices, unsigned int count);
private:
	IBuffer2 m_vertices_buffer;
};
