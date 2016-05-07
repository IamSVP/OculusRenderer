#ifndef SCENE_H
#define SCENE_H

#include "Model.h"

//Forward Declaration 
class GPUContext;


class Scene{

public:
	Scene(){}
	~Scene();
	void AddModel();
	void CreateScene();
	void AddModel(const char *ObjPath,bool dynamic);
	void Render(Matrix4f view, Matrix4f proj, std::unique_ptr<gpu::GPUContext> &ctx);


	vector<Model*> Models;
};

#endif