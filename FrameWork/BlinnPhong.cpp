#include "BlinnPhong.h"

void LightInfoObject::AddDirection(const glm::vec3& dir)
{
	if (curr_index[DIR_IND] < 0 && curr_index[DIR_IND] != MaxLights)
	{
		const short arr_size = sizeof(glm::vec3);
		*(direction + arr_size * curr_index[DIR_IND]) = dir;
		++curr_index[DIR_IND];
	}

}

void LightInfoObject::AddPosition(const glm::vec3& pos)
{
	if (curr_index[POS_IND] < 0 && curr_index[POS_IND] != MaxLights)
	{
		const short arr_size = sizeof(glm::vec3);
		*(lightPos + arr_size * curr_index[POS_IND]) = pos;
		++curr_index[POS_IND];
	}

}

void LightInfoObject::Create(const glm::vec3& pos, const glm::vec3& dir)
{
	this->AddDirection(dir);
	this->AddPosition(pos);
}