#pragma once
#include "Physics.h"

class Controller 
{
	enum 
	{
		W = 0, A, S, D
	};

	enum 
	{
		KEYBOARD = 0,
	};

	bool keys[4];
public:
	Controller() = default;
	void Update();
};