/*
 * Program 3 base code - includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "Texture.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Particle.h"
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define PI 3.141592

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	float theta = 0;
	float phi = 0;
	float radius = 1;
	float x, y, z;
	double clickX, clickY;

	vec3 center;
	vec3 up = vec3(0, 1, 0);
	vec3 eye = vec3(0, 0.5, 0);
	vec3 forward;
	const vec3 speed = vec3(0.5, 0.5, 0.5);

	int lastX = 0;
	int lastY = 0;

	int mater = 1;

	long frames = 0;
	float time;

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog;
	std::shared_ptr<Program> skyProg;
	std::shared_ptr<Program> waterProg;
	std::shared_ptr<Program> specProg;

	// Shape to be used (from  file) - modify to support multiple
	std::shared_ptr<Shape> cube;
	std::shared_ptr<Shape> totem;
	std::shared_ptr<Shape> tree;
	std::shared_ptr<Shape> shack;
	std::shared_ptr<Shape> terrain;
	std::shared_ptr<Shape> plane;
	std::vector<shared_ptr<Shape>> AllShapes;

	// Textures
	shared_ptr<Texture> texture0;
	shared_ptr<Texture> texture1;
	shared_ptr<Texture> texture2;
	unsigned int cubeMapTexture;

	// Skybox faces
	vector<std::string> faces{
		"bluecloud_rt.jpg",
		"bluecloud_lf.jpg",
		"bluecloud_up.jpg",
		"bluecloud_dn.jpg",
		"bluecloud_bk.jpg",
		"bluecloud_ft.jpg"
	};

	vec3 dTrans = vec3(0);
	float dScale = 1.0;

	struct hash_pair {
		template <class T1, class T2>
		size_t operator()(const pair<T1, T2>& p) const
		{
			auto hash1 = hash<T1>{}(p.first);
			auto hash2 = hash<T2>{}(p.second);
			return hash1 ^ hash2;
		}
	};

	vector<vec3> treePoints;
	unordered_map<pair<int, int>, float, hash_pair> heightMap;

	//example data that might be useful when trying to compute bounds on multi-shape
	vec3 gMin;

	bool mouseDown = false;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_M && (action == GLFW_PRESS))
		{
			mater = (mater + 1) % 4;
		}
		if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
		{
			eye += speed * forward;
			if (eye.y < 0)
			{
				eye.y = 0;
			}
			center += speed * forward;
		}
		if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
		{
			eye -= speed * forward;
			if (eye.y < 0)
			{
				eye.y = 0;
			}
			center -= speed * forward;
		}
		if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
		{
			eye += speed * cross(up, forward);
			center += speed * cross(up, forward);
		}
		if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
		{
			eye -= speed * cross(up, forward);
			center -= speed * cross(up, forward);
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY)
	{
		
	}

	void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		if (mouseDown)
		{
			double deltaX = xpos - lastX;
			double deltaY = ypos - lastY;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			theta += deltaX / 500;
			phi += -deltaY / 500;

			if (phi > 1.5)
			{
				phi = 1.5;
			}
			else if (phi < -1.5)
			{
				phi = -1.5;
			}

			lastX += deltaX;
			lastY += deltaY;
		}
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;
		if (action == GLFW_PRESS)
		{
			mouseDown = true;
			glfwGetCursorPos(window, &posX, &posY);
			cout << "Pos X " << posX << " Pos Y " << posY << endl;
		}

		if (action == GLFW_RELEASE)
		{
			mouseDown = false;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.0f, 0.0f, 0.8f, 0.25f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("Texture0");
		prog->addUniform("eyePos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");

		skyProg = make_shared<Program>();
		skyProg->setVerbose(true);
		skyProg->setShaderNames(resourceDirectory + "/cube_vert.glsl", resourceDirectory + "/cube_frag.glsl");
		skyProg->init();
		skyProg->addUniform("P");
		skyProg->addUniform("V");
		skyProg->addUniform("M");
		skyProg->addUniform("skybox");
		skyProg->addAttribute("vertPos");
		skyProg->addAttribute("vertNor");

		specProg = make_shared<Program>();
		specProg->setVerbose(true);
		specProg->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		specProg->init();
		specProg->addUniform("P");
		specProg->addUniform("V");
		specProg->addUniform("M");
		specProg->addUniform("MatAmb");
		specProg->addUniform("MatDif");
		specProg->addUniform("MatSpec");
		specProg->addUniform("shine");
		specProg->addUniform("lightPos");
		specProg->addUniform("eye");
		specProg->addAttribute("vertPos");
		specProg->addAttribute("vertNor");
		specProg->addAttribute("vertTex");

		/*waterProg = make_shared<Program>();
		waterProg->setVerbose(true);
		waterProg->setShaderNames(resourceDirectory + "/water_vert.glsl", resourceDirectory + "/water_frag.glsl");
		waterProg->init();
		waterProg->addUniform("P");
		waterProg->addUniform("V");
		waterProg->addUniform("M");
		waterProg->addUniform("lightPos");
		waterProg->addAttribute("vertPos");
		waterProg->addAttribute("vertNor");*/
	}

	void initTex(const std::string& resourceDirectory)
	{
		texture0 = make_shared<Texture>();
		texture0->setFilename(resourceDirectory + "/crate.jpg");
		texture0->init();
		texture0->setUnit(0);
		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		texture1 = make_shared<Texture>();
		texture1->setFilename(resourceDirectory + "/shacktex.jpg");
		texture1->init();
		texture1->setUnit(1);
		texture1->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

		texture2 = make_shared<Texture>();
		texture2->setFilename(resourceDirectory + "/grass.jpg");
		texture2->init();
		texture2->setUnit(2);
		texture2->setWrapModes(GL_REPEAT, GL_REPEAT);
	}

	void initGeom(const std::string& resourceDirectory)
	{

		//EXAMPLE set up to read one shape from one obj file - convert to read several
		// Initialize mesh
		// Load geometry
 		// Some obj files contain material information.We'll ignore them for this assignment.
 		vector<tinyobj::shape_t> TOshapes;
 		vector<tinyobj::material_t> objMaterials;
 		string errStr;
		genRandPoints(vec2(-6, -6), vec2(-80, -80), vec2(80, 80), 12, 12);

		bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr,
			(resourceDirectory + "/dummy.obj").c_str());

		if (!rc)
		{
			cerr << errStr << endl;
		}
		else
		{
			for (size_t i = 0; i < TOshapes.size(); i++)
			{
				shared_ptr<Shape> curMesh = make_shared<Shape>();;
				curMesh->createShape(TOshapes[i]);
				curMesh->measure();
				curMesh->init();
				AllShapes.push_back(curMesh);
			}

			float dMax = AllShapes[0]->max.x;
			float dMin = AllShapes[0]->min.x;
			vec3 dvMax = AllShapes[0]->max;
			vec3 dvMin = AllShapes[0]->min;
			for (int i = 0; i < AllShapes.size(); i++) {
				if (AllShapes[i]->max.x > AllShapes[i]->max.y && AllShapes[i]->max.x > AllShapes[i]->max.z)
				{
					if (AllShapes[i]->max.x > dMax) {
						dMax = AllShapes[i]->max.x;
						dvMax = AllShapes[i]->max;
					}
					if (AllShapes[i]->min.x < dMin) {
						dMin = AllShapes[i]->min.x;
						dvMin = AllShapes[i]->min;
					}
				}
				else if (AllShapes[i]->max.y > AllShapes[i]->max.x && AllShapes[i]->max.y > AllShapes[i]->max.z)
				{
					if (AllShapes[i]->max.y > dMax) {
						dMax = AllShapes[i]->max.y;
						dvMax = AllShapes[i]->max;
					}
					if (AllShapes[i]->min.y < dMin) {
						dMin = AllShapes[i]->min.y;
						dvMin = AllShapes[i]->min;
					}
				}
				else
				{
					if (AllShapes[i]->max.z > dMax) {
						dMax = AllShapes[i]->max.z;
						dvMax = AllShapes[i]->max;
					}
					if (AllShapes[i]->min.z < dMin) {
						dMin = AllShapes[i]->min.z;
						dvMin = AllShapes[i]->min;
					}
				}
			}
			dScale = 2.0 / (dMax - dMin);
			dTrans = dMin + 0.5f * (dvMax - dvMin);
		}
		
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/cube.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			cube = make_shared<Shape>();
			cube->createShape(TOshapes[0]);
			cube->measure();
			cube->init();
		}

		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/tree.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			tree = make_shared<Shape>();
			tree->createShape(TOshapes[0]);
			tree->measure();
			tree->init();
		}

		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/totem.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			totem = make_shared<Shape>();
			totem->createShape(TOshapes[0]);
			totem->measure();
			totem->init();
		}

		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/shack.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			shack = make_shared<Shape>();
			shack->createShape(TOshapes[0]);
			shack->measure();
			shack->init();
		}

		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/plane.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			plane = make_shared<Shape>();
			plane->createShape(TOshapes[0]);
			plane->measure();
			plane->init();
		}

		rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/terrain.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			terrain = make_shared<Shape>();
			terrain->createShape(TOshapes[0]);
			terrain->tileCoords(8.0);
			terrain->measure();
			terrain->init();
			getHeights(TOshapes[0].mesh.positions);
		}

		cubeMapTexture = createSky(resourceDirectory + "/cracks/", faces);
	}

	void getHeights(vector<float> positions)
	{
		for (size_t i = 0; i < positions.size(); i+=3)
		{
			heightMap[make_pair((int)positions[i], (int)positions[i+2])] = positions[i+1];
		}
	}

	double trunc_decs(double value, std::size_t digits_after_decimal = 0)
	{
		if (digits_after_decimal >= std::numeric_limits<std::intmax_t>::digits10) return value;

		double integral_part;
		const double fractional_part = std::modf(value, std::addressof(integral_part));

		std::intmax_t multiplier = 1;
		while (digits_after_decimal--) multiplier *= 10;

		return integral_part + double(std::intmax_t(fractional_part*multiplier)) / multiplier;
		// or: return integral_part + std::trunc(fractional_part*multiplier) / multiplier ;
	}

	float randomFloat(float mn, float mx)
	{
		return ((float(rand()) / float(RAND_MAX)) * (mx - mn)) + mn;
	}

	void genRandPoints(vec2 point, vec2 bound1, vec2 bound2, float width, float height)
	{
		float x1 = point.x;
		float y1 = point.y;
		float x2 = x1 + width;
		float y2 = y1 + height;
		float bx = bound1.x;
		float by = bound1.y;
		float tx = bound2.x;
		float ty = bound2.y;

		vector<vector<float>> boxes;
		boxes.push_back({ bx,by,x1,y1 });
		boxes.push_back({ x1,by,x2,y1 });
		boxes.push_back({ x2,by,tx,y1 });
		boxes.push_back({ bx,y1,x1,y2 });
		boxes.push_back({ x2,y1,tx,y2 });
		boxes.push_back({ bx,y2,x1,ty });
		boxes.push_back({ x1,y2,x2,ty });
		boxes.push_back({ x2,y2,tx,ty });

		for (size_t i = 0; i < 1000; i++)
		{
			int r = rand() % boxes.size();
			vector<float> box = boxes[r];
			float x = randomFloat(box[0], box[2]);
			float z = randomFloat(box[1], box[3]);

			treePoints.push_back(vec3(x, 0, z));
		}
	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
    }

	void setMaterial(std::shared_ptr<Program> prog, int i) {
		switch (i) {
		case 0:
			glUniform3f(prog->getUniform("MatAmb"), 0.19125, 0.0735, 0.0225);
			glUniform3f(prog->getUniform("MatDif"), 0.7038, 0.27048, 0.0828);
			glUniform3f(prog->getUniform("MatSpec"), 0.256777, 0.137622, 0.086014);
			glUniform1f(prog->getUniform("shine"), 12.8);
			break;
		case 1:
			glUniform3f(prog->getUniform("MatAmb"), 0.329412, 0.223529, 0.027451);
			glUniform3f(prog->getUniform("MatDif"), 0.780392, 0.568627, 0.113725);
			glUniform3f(prog->getUniform("MatSpec"), 0.992157, 0.941176, 0.807843);
			glUniform1f(prog->getUniform("shine"), 27.8974);
			break;
		case 2:
			glUniform3f(prog->getUniform("MatAmb"), 0.1, 0.18725, 0.1745);
			glUniform3f(prog->getUniform("MatDif"), 0.396, 0.74151, 0.69102);
			glUniform3f(prog->getUniform("MatSpec"), 0.297254, 0.30829, 0.306678);
			glUniform1f(prog->getUniform("shine"), 12.8);
			break;
		case 3:
			glUniform3f(prog->getUniform("MatAmb"), 0.2, 0.2, 0.2);
			glUniform3f(prog->getUniform("MatDif"), 0.1, 0.35, 0.1);
			glUniform3f(prog->getUniform("MatSpec"), 0.45, 0.55, 0.45);
			glUniform1f(prog->getUniform("shine"), 0.25);
			break;
		}
	}

	unsigned int createSky(string dir, vector<string> faces)
	{
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false);
		for (GLuint i = 0; i < faces.size(); i++)
		{
			unsigned char* data = stbi_load((dir+faces[i]).c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			}
			else
			{
				cout << "failed to load: " << (dir+faces[i]).c_str() << endl;
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		cout << " creating cube map any errors : " << glGetError() << endl;
		return textureID;
	}
	
	void render() {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		time = frames / 60.0f;

		x = radius*cos(phi)*cos(theta);
		y = radius*sin(phi);
		z = radius*cos(phi)*sin(theta);
		center = eye + vec3(x, y, z);
		forward = normalize(center - eye);

		shared_ptr<Shape> leftArm = AllShapes[10];
		shared_ptr<Shape> leftElbow = AllShapes[9];
		shared_ptr<Shape> leftFore = AllShapes[8];
		shared_ptr<Shape> leftWrist = AllShapes[7];
		shared_ptr<Shape> leftHand = AllShapes[6];

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		//auto View = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 10000.0f);

		// View is global translation along negative z for now
		//View->pushMatrix();
		//View->loadIdentity();

		// Draw skybox
		skyProg->bind();
		glUniformMatrix4fv(skyProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glDepthFunc(GL_LEQUAL);
		glUniformMatrix4fv(skyProg->getUniform("V"), 1, GL_FALSE, value_ptr(lookAt(eye, center, up)));

		Model->pushMatrix();
			Model->loadIdentity();
			Model->translate(eye);
			Model->scale(vec3(110, 110, 110));

			glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
			setModel(skyProg, Model);
			cube->draw(skyProg);
		Model->popMatrix();

		glDepthFunc(GL_LESS);
		skyProg->unbind();
		
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(lookAt(eye, center, up)));
		glUniform3f(prog->getUniform("eyePos"), eye.x, eye.y, eye.z);
		//setMaterial(prog, mater);

		// draw stuff
		Model->pushMatrix();
			Model->loadIdentity();

			// draw ground
			Model->pushMatrix();
				Model->translate(vec3(0, -3, 0));
				texture2->bind(prog->getUniform("Texture0"));
				//Model->translate(vec3(-10, -4.5, 0));
				//Model->scale(vec3(500.0, 500.0, 500.0));
				//setMaterial(prog, 2);
				setModel(prog, Model);
				terrain->draw(prog);
				texture2->unbind();
			Model->popMatrix();

			Model->pushMatrix();
				//Model->translate(vec3(5.0, 0.0, -20.0));

				// draw trees
				Model->pushMatrix();
					Model->scale(vec3(0.6, 0.6, 0.6));
					//setMaterial(prog, 0);
					texture0->bind(prog->getUniform("Texture0"));

					for (size_t i = 0; i < 1000; i++)
					{
						Model->pushMatrix();
							vec3 p = treePoints[i];
							Model->translate(vec3(p.x, heightMap[make_pair((int)p.x, (int)p.z)] - 3.5, p.z));
							setModel(prog, Model);
							tree->draw(prog);
						Model->popMatrix();
					}

					texture0->unbind();
				Model->popMatrix();

				// draw shack
				Model->pushMatrix();
					Model->translate(vec3(0, heightMap[make_pair(0, 0)] - 3, 0));
					Model->rotate(PI / 2.0, vec3(0, 1, 0));
					//Model->rotate(0.174533, vec3(0, 0, 1));
					Model->scale(vec3(0.03, 0.03, 0.03));
					//setMaterial(prog, 1);
					texture1->bind(prog->getUniform("Texture0"));
					setModel(prog, Model);
					shack->draw(prog);
					texture1->unbind();
				Model->popMatrix();
			Model->popMatrix();
		Model->popMatrix();

		prog->unbind();

		specProg->bind();
		glUniformMatrix4fv(specProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(specProg->getUniform("V"), 1, GL_FALSE, value_ptr(lookAt(eye, center, up)));
		glUniform3f(specProg->getUniform("lightPos"), 1.0, 1.0, 1.0);
		glUniform3f(specProg->getUniform("eye"), eye.x, eye.y, eye.z);

		Model->pushMatrix();
			Model->loadIdentity();

			// draw ground
			Model->pushMatrix();
				Model->translate(vec3(-5, heightMap[make_pair(-3, -3)] - 3, -5));
				setModel(specProg, Model);
				setMaterial(specProg, mater);
				totem->draw(specProg);
			Model->popMatrix();

			for (int i = 0; i < AllShapes.size(); i++) {
				Model->pushMatrix();
					Model->translate(vec3(0, 0.f, -5));
					Model->rotate(radians(-90.f), vec3(1, 0, 0));
					Model->rotate(radians(-90.f), vec3(0, 0, 1));
					Model->scale(dScale);
					Model->translate(-1.0f*dTrans);
					setModel(specProg, Model);
					AllShapes[i]->draw(specProg);
				Model->popMatrix();
			}

			/*Model->pushMatrix();
				Model->translate(vec3(0, 0.f, -5));
				Model->rotate(radians(-90.f), vec3(1, 0, 0));
				Model->rotate(radians(-90.f), vec3(0, 0, 1));
				Model->scale(dScale);
				Model->translate(-1.0f*dTrans);
				Model->rotate(sinf(time), vec3(0, 0, 1));
				setModel(specProg, Model);
				leftArm->draw(specProg);
				leftElbow->draw(specProg);
				Model->pushMatrix();
					setModel(specProg, Model);
					leftFore->draw(specProg);
					Model->pushMatrix();
						setModel(specProg, Model);
						leftWrist->draw(specProg);
						Model->pushMatrix();
							setModel(specProg, Model);
							leftHand->draw(specProg);
						Model->popMatrix();
					Model->popMatrix();
				Model->popMatrix();
			Model->popMatrix();

			for (int i = 11; i < AllShapes.size(); i++) {
				Model->pushMatrix();
					Model->translate(vec3(0, 0.f, -5));
					Model->rotate(radians(-90.f), vec3(1, 0, 0));
					Model->rotate(radians(-90.f), vec3(0, 0, 1));
					Model->scale(dScale);
					Model->translate(-1.0f*dTrans);
					setModel(specProg, Model);
					AllShapes[i]->draw(specProg);
				Model->popMatrix();
			}*/

		Model->popMatrix();
		specProg->unbind();

		//waterProg->bind();
		//
		//glUniformMatrix4fv(waterProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		//glUniformMatrix4fv(waterProg->getUniform("V"), 1, GL_FALSE, value_ptr(lookAt(eye, center, up)));
		//glUniform3f(waterProg->getUniform("eyePos"), eye.x, eye.y, eye.z);

		//for (int i = 0; i < 4; ++i) {
		//	float amplitude = 0.5f / (i + 1);
		//	glUniform1f(waterProg->getUniform("amplitude[" + to_string(i) + "]"), amplitude);

		//	float wavelength = 8 * 3.14159 / (i + 1);
		//	glUniform1f(waterProg->getUniform("wavelength[" + to_string(i) + "]"), wavelength);

		//	float speed = 1.0f + 2*i;
		//	glUniform1f(waterProg->getUniform("speed[" + to_string(i) + "]"), speed);

		//	float angle = randomFloat(-3.14159/3, 3.14159/3);
		//	glUniform2f(waterProg->getUniform("direction[" + to_string(i) + "]"), cos(angle), sin(angle));
		//}

		//glUniform1i(waterProg->getUniform("numWaves"), 4);
		//glUniform1f(waterProg->getUniform("waterHeight"), 10);
		//glUniform1f(waterProg->getUniform("time"), frames / 60.0);

		//// draw stuff
		//Model->pushMatrix();
		//	Model->loadIdentity();

		//	// draw ground
		//	Model->pushMatrix();
		//		Model->translate(vec3(0, 10, 0));
		//		setModel(waterProg, Model);
		//		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		//		plane->draw(waterProg);
		//	Model->popMatrix();

		//Model->popMatrix();
		//waterProg->unbind();

		Projection->popMatrix();
		////View->popMatrix();
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);
	application->initTex(resourceDir);

	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
		
		application->frames++;
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
