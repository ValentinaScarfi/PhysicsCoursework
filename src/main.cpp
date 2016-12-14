#define GLM_ENABLE_EXPERIMENTAL
#include "physics.h"
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <graphics_framework.h>
#include <phys_utils.h>
#include <thread>
#include "main.h"

using namespace std;
using namespace graphics_framework;
using namespace glm;

#define physics_tick 1.0 / 60.0

double xpos = 0.0f;
double ypos = 0.0f;
free_camera free_cam;
target_camera camT;
int Cam;

static vector<unique_ptr<Entity>> ClothParticles;
static vector<cSpring> springList;
static unique_ptr<Entity> floorEnt;

static vector<unique_ptr<Entity>> balls;

int rows = 20;
float stretchConstant = 60.0f;
float shearConstant = 30.0f;
float bendingConstant = 10.0f;
float naturalLength = 1.0f;
float dampingFactor = 100.0f;

unique_ptr<Entity> CreateParticle(int xPos, int yPos, int zPos, double myMass) {
	unique_ptr<Entity> ent(new Entity());
	ent->SetPosition(vec3(xPos, yPos, zPos));
	cPhysics *phys = new cPhysics();
	phys->mass = myMass;
	unique_ptr<Component> physComponent(phys);
	unique_ptr<cShapeRenderer> renderComponent(new cShapeRenderer(cShapeRenderer::SPHERE));
	renderComponent->SetColour(phys::RandomColour());
	ent->AddComponent(physComponent);
	cSphereCollider *coll = new cSphereCollider();
	coll->radius = 0.3f;
	ent->AddComponent(unique_ptr<Component>(coll));
	ent->AddComponent(unique_ptr<Component>(move(renderComponent)));
	return ent;
}

cPhysics *getParticle(int x, int z)
{
	auto p = static_cast<cPhysics *>(ClothParticles[x * rows + z]->GetComponents("Physics")[0]);
	return p;
}

cSpring getSpring(int x, int z)
{
	auto p = springList[x * rows + z];
	return p;
}

void Cloth()
{
	for (int x = 0; x < rows; x++)
	{
		for (int z = 0; z < rows; z++)
		{
			unique_ptr<Entity> particle = CreateParticle(x*1.0, (z + 10)*1.0, -20.0 , 1.0);
			auto p = static_cast<cPhysics *>(particle->GetComponents("Physics")[0]);
			ClothParticles.push_back(move(particle));
		}
	}
}

void updateCloth()
{
	springList.clear();

	for (int x = 0; x < rows; x++)
	{
		for (int z = 0; z < rows; z++)
		{
			//structural and shear springs
			if (z == (rows - 1))
			{
				if (x + 1 != rows)
				{
					//horizontal structural spring
					cSpring aa = cSpring(getParticle(x + 1, z), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
					aa.update(1.0);
					springList.push_back(aa);

				}
			}
			else if (x == (rows - 1))
			{
				if (z + 1 != rows)
				{
					//vertical structural spring
					cSpring aa = cSpring(getParticle(x, z + 1), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
					aa.update(1.0);
					springList.push_back(aa);
				}
			}
			else if (x != (rows - 1) && z != (rows - 1))
			{

				//diagonal shear spring
				cSpring aa = cSpring(getParticle(x + 1, z + 1), getParticle(x, z), shearConstant, (naturalLength * sqrt(2.0)), dampingFactor, GREEN);
				aa.update(1.0);
				springList.push_back(aa);
				//vertical structural spring
				aa = cSpring(getParticle(x, z + 1), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
				aa.update(1.0);
				springList.push_back(aa);
				//horizontal structural spring
				aa = cSpring(getParticle(x + 1, z), getParticle(x, z), stretchConstant, naturalLength, dampingFactor, BLUE);
				aa.update(1.0);
				springList.push_back(aa);
			}
			if (x > 0 && z < (rows - 1))
			{
			//reverse diagonal shear spring
			cSpring aa = cSpring(getParticle(x - 1, z + 1), getParticle(x, z), shearConstant, (naturalLength * sqrt(2.0)), dampingFactor, GREEN);
			aa.update(1.0);
			springList.push_back(aa);
			}

			//Bending springs
			if (z % 2 == 0 || z == 0)
			{
				if (z + 2 < rows)
				{
					//vertical bending spring
					cSpring aa = cSpring(getParticle(x, z + 2), getParticle(x, z), bendingConstant, naturalLength * 2, dampingFactor, RED);
					aa.update(1.0);
					springList.push_back(aa);
				}
			}

			if (x % 2 == 0 || x == 0)
			{
				if (x + 2 < rows)
				{
					//horizontal bending spring
					cSpring	aa = cSpring(getParticle(x + 2, z), getParticle(x, z), bendingConstant, naturalLength * 2.0, dampingFactor, ORANGE);
					aa.update(1.0);
					springList.push_back(aa);
				}
			}

			if (z % 2 == 0 && x % 2 == 0)
			{
				if (x + 2 < rows && z + 2 < rows)
				{
					//diagonal shear spring
					cSpring aa = cSpring(getParticle(x + 2, z + 2), getParticle(x, z), 50.0, (naturalLength * sqrt(2.0)) * 2, dampingFactor, AQUA);
					aa.update(1.0);
					springList.push_back(aa);
				}
			}

		}
	}
}

void fixTopRow()
{
	int z = rows - 1;
	int x = 0;
	while (x < rows)
	{
		getParticle(x, z)->position = getParticle(x, z)->prev_position;
		getParticle(x, z)->fixed = true;
		x++;
	}
}

bool update(float delta_time) {
	static float t = 0.0;
	static float accumulator = 0.0;
	accumulator += delta_time;

	while (accumulator > physics_tick) {
		UpdatePhysics(t, physics_tick);
		accumulator -= physics_tick;
		t += physics_tick;
	}

	for (auto &e : ClothParticles) {
		e->Update(delta_time);
	}

	updateCloth();
	//fixTopRow();

	if (glfwGetKey(renderer::get_window(), GLFW_KEY_F))
	{
		Cam = 1;
	}

	//while freecam is in use
	if (Cam == 1)
	{
		//Mouse cursor is disabled and used to rotate freecam
		glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		//Get mouse position for freecam
		double deltax, deltay;
		glfwGetCursorPos(renderer::get_window(), &deltax, &deltay);
		double tempx = deltax;
		double tempy = deltay;
		deltax -= xpos;
		deltay -= ypos;
		double ratio_width = (double)renderer::get_screen_width() / (double)renderer::get_screen_height();
		double ratio_height = 1.0 / ratio_width;
		deltax *= ratio_width * delta_time / 10;
		deltay *= -ratio_height * delta_time / 10;

		//Rotate freecam based on the mouse coordinates
		free_cam.rotate(deltax, deltay);

		// set last cursor pos
		xpos = tempx;
		ypos = tempy;

		//Movement (freecam)
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_W))
		{
			free_cam.move(vec3(0.0, 0.0, 10.0) * delta_time);
		}
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_S))
		{
			free_cam.move(vec3(0.0, 0.0, -10.0) * delta_time);
		}
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_A))
		{
			free_cam.move(vec3(-10.0, 0.0, 0.0) * delta_time);
		}
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_D))
		{
			free_cam.move(vec3(10.0, 0.0, 0.0) * delta_time);
		}

		if (glfwGetKey(renderer::get_window(), GLFW_KEY_UP))
		{
			free_cam.move(vec3(0.0, 10.0, 0.0) * delta_time);
		}
		if (glfwGetKey(renderer::get_window(), GLFW_KEY_DOWN))
		{
			free_cam.move(vec3(0.0, -10.0, 0.0) * delta_time);
		}
		//Update freecam
		free_cam.update(delta_time);

		

		phys::SetCameraTarget(free_cam.get_target());
		phys::SetCameraPos(free_cam.get_position());
	}

	phys::Update(delta_time);
	return true;
}

bool load_content() {
	phys::Init();
	Cloth();
	floorEnt = unique_ptr<Entity>(new Entity());

	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));

	free_cam.set_target(vec3(5.0, -2.0, 0.0));
	free_cam.set_position(vec3(10.0f, 40.0f, -50.0f));

	phys::SetCameraPos(vec3(10.0f, 40.0f, -50.0f));
	phys::SetCameraTarget(vec3(0.0f, 0.0f, 0));
	InitPhysics();
	return true;
}

bool render() {
	for (auto &e : ClothParticles) {
		e->Render();
	}

	phys::DrawScene();

	for (auto &e : springList) {
		e.Render();
	}
	return true;
}

void main() {
	// Create application
	app application;
	// Set load content, update and render methods
	application.set_load_content(load_content);
	application.set_update(update);
	application.set_render(render);
	// Run application
	application.run();
}

