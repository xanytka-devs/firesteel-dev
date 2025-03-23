#include "engine/include/app.hpp"
#include "engine/include/entity.hpp"
using namespace Firesteel;

#include <box2d/box2d.h>
#include "engine/include/camera.hpp"

static b2Vec2 v(glm::vec3 tV) {
	return b2Vec2{ tV.x, tV.y };
}

class TwoDExampleApp : public App{
	b2WorldDef world; b2WorldId worldId;
	b2BodyDef ground; b2BodyId groundId;
	b2BodyDef ground1; b2BodyId groundId1;
	b2BodyDef ground2; b2BodyId groundId2;
	b2BodyDef ground3; b2BodyId groundId3;
	b2BodyDef dynam; b2BodyId dynamId;

	Entity quad;
	Shader base;
	Camera camera;
	bool jumped = false;

	float mouseSenstivity = 0.5f;
	void processInput(Window* tWin, float tDeltaTime) {
		if (Keyboard::keyDown(KeyCode::ESCAPE))
			tWin->close();

		//float dx = Mouse::getCursorDX(), dy = Mouse::getCursorDY();
		//if (dx != 0 || dy != 0) {
		//	camera.rot.y += dy * mouseSenstivity;
		//	camera.rot.z += dx * mouseSenstivity;
		//	if (camera.rot.y > 89.0f)
		//		camera.rot.y = 89.0f;
		//	if (camera.rot.y < -89.0f)
		//		camera.rot.y = -89.0f;
		//	if (camera.rot.z >= 360.0f)
		//		camera.rot.z -= 360.0f;
		//	if (camera.rot.z <= -360.0f)
		//		camera.rot.z += 360.0f;
		//	camera.update();
		//}

		float velocity = 2.5f * tDeltaTime;

		if (Keyboard::getKey(KeyCode::A))
			b2Body_ApplyForceToCenter(dynamId, b2Vec2{ -1,0 }, true);
		if (Keyboard::getKey(KeyCode::D))
			b2Body_ApplyForceToCenter(dynamId, b2Vec2{ 1,0 }, true);

		if (Keyboard::getKey(KeyCode::SPACEBAR) && !jumped) {
			jumped = true;
			b2Body_ApplyLinearImpulseToCenter(dynamId, b2Vec2{ 0,18 }, true);
		}
		if (Keyboard::keyUp(KeyCode::SPACEBAR)) jumped = false;

	}

	virtual void onInitialize() override {
		quad = Entity("res\\primitives\\quad.obj");
		base = Shader("res\\mats\\shaders\\base_2d.vs", "res\\mats\\shaders\\base_2d.fs");

		world = b2DefaultWorldDef();
		world.gravity = b2Vec2{0, -1};
		worldId = b2CreateWorld(&world);

		ground = b2DefaultBodyDef();
		ground.position = b2Vec2{ 0.f,-10.f };
		groundId = b2CreateBody(worldId, &ground);
		b2Polygon groundBox = b2MakeBox(3.f, 1.f);
		b2ShapeDef groundShape = b2DefaultShapeDef();
		b2CreatePolygonShape(groundId, &groundShape, &groundBox);

		ground1 = b2DefaultBodyDef();
		ground1.position = b2Vec2{ 5.f,-2.5f };
		groundId1 = b2CreateBody(worldId, &ground1);
		b2Polygon groundBox1 = b2MakeBox(3.f, 1.f);
		b2ShapeDef groundShape1 = b2DefaultShapeDef();
		b2CreatePolygonShape(groundId1, &groundShape1, &groundBox1);

		ground2 = b2DefaultBodyDef();
		ground2.position = b2Vec2{ -5.f,5.f };
		groundId2 = b2CreateBody(worldId, &ground2);
		b2Polygon groundBox2 = b2MakeBox(3.f, 1.f);
		b2ShapeDef groundShape2 = b2DefaultShapeDef();
		b2CreatePolygonShape(groundId2, &groundShape2, &groundBox2);

		ground3 = b2DefaultBodyDef();
		ground3.position = b2Vec2{ 5.f,12.5f };
		groundId3 = b2CreateBody(worldId, &ground3);
		b2Polygon groundBox3 = b2MakeBox(3.f, 1.f);
		b2ShapeDef groundShape3 = b2DefaultShapeDef();
		b2CreatePolygonShape(groundId3, &groundShape3, &groundBox3);

		dynam = b2DefaultBodyDef();
		dynam.type = b2_dynamicBody;
		dynam.position = b2Vec2{ 0.f,1.5f };
		dynamId = b2CreateBody(worldId, &dynam);
		b2Polygon dynamBox = b2MakeBox(1.f, 1.f);
		b2ShapeDef dynamShape = b2DefaultShapeDef();
		dynamShape.density = 1.f;
		dynamShape.friction = 0.3f;
		b2CreatePolygonShape(dynamId, &dynamShape, &dynamBox);

		//camera.isPerspective = false;
		camera.pos = glm::vec3(0, -5, 40);
		camera.rot = glm::vec3(0, 0, 270);
		camera.update();
		glfwSwapInterval(1);
	}

	float fixedTimeStep = 1.f/24.f;
	int subStepCount = 4;

	virtual void onUpdate() override {
		// Handle window size change.
		camera.aspect = window.aspect();
		glViewport(0, 0, static_cast<GLsizei>(window.getWidth()), static_cast<GLsizei>(window.getHeight()));
		//processInput(&window, deltaTime);
		b2World_Step(worldId, fixedTimeStep, subStepCount);
		b2Vec2 pos = b2Body_GetPosition(dynamId);
		b2Rot rot = b2Body_GetRotation(dynamId);
		window.clearBuffers();
		processInput(&window, deltaTime);

		glm::mat4 projection = camera.getProjection(1);
		glm::mat4 view = camera.getView();

		quad.transform.Position = glm::vec3(0, -10, 1);
		quad.transform.Rotation = glm::vec3(0);
		quad.transform.Size = glm::vec3(3, 1, 1);
		base.enable();
		base.setMat4("projection", projection);
		base.setMat4("view", view);
		base.setVec3("viewPos", camera.pos);
		base.setVec3("color", glm::vec3(1));
		quad.draw(&base);

		quad.transform.Position = glm::vec3(5, -2.5f, 1);
		quad.draw(&base);

		quad.transform.Position = glm::vec3(-5, 5.f, 1);
		quad.draw(&base);

		quad.transform.Position = glm::vec3(5, 12.5f, 1);
		quad.draw(&base);

		quad.transform.Position = glm::vec3(pos.x, pos.y, 1);
		quad.transform.Rotation = glm::vec3(rot.c, rot.s, 0);
		camera.pos.y = pos.y;
		quad.transform.Size = glm::vec3(1);
		base.setVec3("color", glm::vec3(1, 0, 0));
		quad.draw(&base);

		if (b2Body_GetPosition(dynamId).y <= -20) {
			b2Body_SetTransform(dynamId, b2Vec2{ 0.f,1.5f }, b2MakeRot(0));
			b2Body_SetLinearVelocity(dynamId, b2Vec2{ 0.f,0.f });
			b2Body_SetAngularVelocity(dynamId, 0);
		}
	}

	virtual void onShutdown() override {
		b2DestroyWorld(worldId);
	}
};

int main() {
	TwoDExampleApp app{};
	int r = app.start("2D Example ", 800, 600, WS_NORMAL);
	LOG_INFO("Shutting down Firesteel App.");
	return r;
}