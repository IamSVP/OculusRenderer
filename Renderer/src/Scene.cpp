#include "Scene.h"

void Scene::CreateScene(){

	

	Model* m = new Model("RenderStuff//Obj//sphere.obj", true);
	//m->LoadShaders("RenderStuff//Shaders//StandardShading.vs", "RenderStuff//Shaders//SimpleFragmentShader.fgs");
	//m->InitializeTexture();
	//m->AllocateVertexBuffers();
	//Models.push_back(m);

	/*Model *m = new Model(true);
	m->indexed_vertices.push_back(Vector3f(-1.0f, -1.0f, 0.0f));
	m->indexed_vertices.push_back(Vector3f(1.0f,  -1.0f, 0.0f));
	m->indexed_vertices.push_back(Vector3f( 1.0f,  1.0f, 0.0f));
	m->indexed_vertices.push_back(Vector3f( -1.0f, 1.0f, 0.0f));



	m->indexed_uvs.push_back(Vector2f(1.0f, 1.0f));
	m->indexed_uvs.push_back(Vector2f(0.0f, 1.0f));
	m->indexed_uvs.push_back(Vector2f(0.0f,0.0f));
	m->indexed_uvs.push_back(Vector2f(1.0f, 0.0f));
	
	m->indices.push_back(0);
	m->indices.push_back(1);
	m->indices.push_back(3);

	m->indices.push_back(1);
	m->indices.push_back(2);
	m->indices.push_back(3); */
	m->LoadShaders("RenderStuff//Shaders//SimpleVertexShader.vs", "RenderStuff//Shaders//SimpleFragmentShader.fgs");
	m->InitializeTextures();
	m->AllocateVertexBuffers();
	Models.push_back(m);

	
}

Scene::~Scene(){

	for (int i = 0; i < Models.size(); i++)
		delete Models[i];

}
void Scene::AddModel(const char *ObjPath,bool dynamic){

	

}

void Scene::Render(Matrix4f view, Matrix4f proj, std::unique_ptr<gpu::GPUContext> &ctx){

	for (int i = 0; i < Models.size(); i++){
		Models[i]->RenderModel(view, proj, ctx);
	}
}