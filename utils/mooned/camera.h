
#pragma once

struct Camera
{
	glm::vec3 origin;
	glm::vec3 angles;

	Camera()
		: origin( 0.0f, -1000.0f, 1000.0f )
		, angles( 45.0f, 0.0f, 0.0f )
	{
	}

};
