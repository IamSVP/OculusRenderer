#ifndef SCENE_H
#define SCENE_H

#include "Model.h"

class Scene{

public:
	Scene(){}
	~Scene();
	void AddModel();
	void CreateScene();
	void AddModel(const char *ObjPath,bool dynamic);
	void Render(Matrix4f view, Matrix4f proj);


	vector<Model*> Models;
};

#endif