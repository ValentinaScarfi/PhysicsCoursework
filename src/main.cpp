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

static vector<unique_ptr<Entity>> ClothParticles;
static vector<cSpring> springList;
static unique_ptr<Entity> floorEnt;

static vector<unique_ptr<Entity>> balls;

int rows = 10;
float springConstant = 80.0f;
float naturalLength = 2.0f;

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
			unique_ptr<Entity> particle = CreateParticle((x - 3) * 3.0, 1.0, (z - 5) * 3.0, 1.0);
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

			if (z == (rows - 1))
			{
				if (x + 1 != rows)
				{
					//horizontal spring
					cSpring aa = cSpring(getParticle(x + 1, z), getParticle(x, z), springConstant, naturalLength);
					aa.update(1.0);
					springList.push_back(aa);
				}
			}
			else if (x == (rows - 1))
			{
				if (z + 1 != rows)
				{
					//vertical spring
					cSpring aa = cSpring(getParticle(x, z + 1), getParticle(x, z), springConstant, naturalLength);
					aa.update(1.0);
					springList.push_back(aa);
				}
			}
			else if (x != (rows - 1) && z != (rows - 1))
			{
				////diagonal spring
				cSpring aa = cSpring(getParticle(x + 1, z + 1), getParticle(x, z), springConstant*0.5, (naturalLength * sqrt(2.0)));
				aa.update(1.0);
				springList.push_back(aa);
				//vertical spring
				aa = cSpring(getParticle(x, z + 1), getParticle(x, z), springConstant, naturalLength);
				aa.update(1.0);
				springList.push_back(aa);
				//horizontal spring
				aa = cSpring(getParticle(x + 1, z), getParticle(x, z), springConstant, naturalLength);
				aa.update(1.0);
				springList.push_back(aa);
			}
			if (x > 0 && z < (rows - 1))
			{
			//reverse diagonal spring
			cSpring aa = cSpring(getParticle(x - 1, z + 1), getParticle(x, z), springConstant, (naturalLength * sqrt(2.0)));
			aa.update(1.0);
			springList.push_back(aa);
			}
		}
	}


}



bool update(double delta_time) {
	static double t = 0.0;
	static double accumulator = 0.0;
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

	//getParticle(4, 4)->position = getParticle(4, 4)->prev_position;
	//getParticle(3, 4)->position = getParticle(3, 4)->prev_position;
	//getParticle(2, 4)->position = getParticle(2, 4)->prev_position;
	//getParticle(1, 4)->position = getParticle(1, 4)->prev_position;
	//getParticle(0, 4)->position = getParticle(0, 4)->prev_position;

	phys::Update(delta_time);
	return true;
}

bool load_content() {
	phys::Init();
	Cloth();



	floorEnt = unique_ptr<Entity>(new Entity());
	floorEnt->AddComponent(unique_ptr<Component>(new cPlaneCollider()));

	phys::SetCameraPos(vec3(10.0f, 40.0f, -50.0f));
	phys::SetCameraTarget(vec3(0.0f, 0.0f, 0));
	InitPhysics();
	return true;
}

bool render() {
	for (auto &e : ClothParticles) {
		e->Render();
	}

	/*for (int i = 0; i < ClothParticles.size() - 1; i++)
	{
		auto b = ClothParticles[i]->GetComponents("Physics");
		const auto p = static_cast<cPhysics *>(b[0]);
		phys::DrawLine(ClothParticles[i]->GetPosition(), ClothParticles[i+1]->GetPosition(), false, BLUE);
	}*/
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

