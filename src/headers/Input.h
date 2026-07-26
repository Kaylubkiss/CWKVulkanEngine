#ifndef INPUT_HPP
#define INPUT_HPP

#include <stack>

class Camera;

enum class InputResult
{
	IRESULT_NONE = 0,
	IRESULT_APP_EXIT,
	IRESULT_TOGGLEUIACTIVE_TRUE,
	IRESULT_TOGGLEUIACTIVE_FALSE,
};

void MoveCamera( Camera& camera, float dt );

#endif