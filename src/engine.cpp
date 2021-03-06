#include "engine.h"
#include <sstream>
#include <iostream>
#include <fstream>

#include "collision.h"

#include <thread>

#include "Game/game.hpp"

#ifdef  LINUX
#include <unistd.h>
#endif //  LINUX



#include "Multiplayer/connectivity.hpp"




Engine::Engine()
{

#ifdef VULKAN
	renderer.engine = this;
#endif
	//setup_components();
}


#ifdef ANDROID
Engine::Engine(android_app *pApp)
{
	renderer.app = pApp;
	this->pAndroid_app = pApp;
}
#endif

void Engine::setup_components()
{
	/* MapManager* map_manager = new MapManager();
	map_manager->name = "MapManager";
	components.push_back( (EngineComponent*)map_manager );

	for (EngineComponent* component : this->components) {
		component->engine = this;
	} */
}

EngineComponent* Engine::component_by_name(const char* name)
{
	for (EngineComponent* component : components) {
		if (component->name == name)
			return component;
	}
	return nullptr;
}

void Engine::draw_loading_screen()
{
#if defined(ES2) || defined(ANDROID)
	glClearColor(0.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	this->linear_meshes.clear();
	this->meshes.clear();
	GUI *loading = new GUI(this);
	loading->update_engine_meshes();
	EMesh *mesh = meshes[0];
	renderer.load_shaders(mesh);
	renderer.create_buffer(mesh);
	renderer.load_mesh_texture(mesh);
	glUseProgram(mesh->shader_program);
	renderer.activate_vertex_attributes(mesh);
	update_mvp(mesh);
	renderer.draw_gui(mesh);
	window_manager.swap_buffers();
	meshes.clear();
	linear_meshes.clear();
	delete loading;
#endif
}





void Engine::init()
{
	window_manager.engine = this;
	animation_manager.engine = this;
	mesh_manager.engine = this;
	maps.engine = this;
	textures_manager.engine = this;		
	maps.engine = this;
	renderer.engine = this;
	

#ifdef LINUX
	window_manager.create_window();
#endif
#ifdef ANDROID
	window_manager.create_window(pAndroid_app);
#endif

#ifdef GLFW
	configure_window_callback();
#endif // GLFW


#ifdef LINUX
	audio_manager.init();
	audio_manager.play();
#endif
	
	#if defined (LINUX)
		draw_loading_screen();
	#endif


#ifdef VULKAN
		
	renderer.run(&vkdata);
	renderer.vulkan_device = this->vulkan_device;
	mesh_manager.vulkan_device = this->vulkan_device;
#endif

	

	std::string map_path = assets.path("Maps/map01.map");

	this->meshes.clear();
	game = new Game();
	game->engine = this;
	
	auto time_load_map = std::chrono::high_resolution_clock::now();
	maps.load_file_map(map_path);
	calculate_time("---->map to cpu memory", time_load_map);

	EMesh* player_mesh = this->meshes[maps.player_id];
	game->init();
	game->player->mesh = player_mesh;

	
#if defined (ES2) || defined (OPENGL)
	TIME(renderer.create_buffers(this, linear_meshes), "vertices to GPU")

#endif // "ES2"

	if (meshes.size() > 0) {		
		TIME(textures_manager.load_textures_to_cpu_memory(linear_meshes), "texture to CPU")
	}
	

#if defined(DEVELOPMENT)  && ( defined(ES2) || defined (OPENGL) ) //gizmos helpers
//	SkeletalManager::create_bones_vertices(this);
//	Collision::create_collision_helper_vertices(this);
#endif

	ready_to_game = true;

	auto tStart = std::chrono::high_resolution_clock::now();

	#ifdef VULKAN
		renderer.meshes = meshes;
		renderer.VulkanConfig();		
		
		renderer.create_meshes_graphics_pipeline();

		for (auto mesh : linear_meshes)
		{
			renderer.load_mesh(mesh);
			renderer.update_descriptor_set(mesh);
		}

		renderer.create_sync_objects();
		//create_command_buffer
		renderer.create_command_buffer(meshes);
		
		
	#endif

#ifdef DX11
	renderer.init();
#endif // DX11

#if defined(ES2) || defined(ANDROID) || defined (OPENGL)
	renderer.init_gl();
	renderer.load_shaders(linear_meshes);
	//renderer.load_textures(maps.same_textures);
	auto texture_time = std::chrono::high_resolution_clock::now();
	renderer.load_textures(linear_meshes);//to cpu
	calculate_time("texture to GPU memory", texture_time);
#endif

#ifdef DEVELOPMENT
	calculate_time("total init",tStart);
#endif



	//init_collision_engine();
}

void Engine::loop_data()
{
#ifdef DEVELOPMENT
	//print_fps();
#endif

	get_time();
	main_camera.cameraSpeed = main_camera.velocity * deltaTime;

	game->update();

	//distance_object_from_camera();

	Objects::update_positions(this, tranlation_update);

	animation_manager.play_animations(this);
}


void Engine::es2_draw_frame()
{

#if defined(ES2) || defined(ANDROID) || defined (OPENGL)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (EMesh *mesh : meshes)
	{

		renderer.use_shader_from(mesh);
		renderer.activate_vertex_attributes(mesh);

		if (!mesh->bIsGUI)
		{

			update_mvp(mesh);
			renderer.draw(mesh);
		}else{

			renderer.update_mvp(mesh);
			renderer.draw_gui(mesh);
		}
	}

	#ifdef LINUX
	for (Model *model : models_to_draw)
	{
		glUseProgram(model->shader_program);
		renderer.activate_vertex_attributes(model->mesh);
		update_mvp(model->mesh);
		renderer.draw(model->mesh);
	}
	#endif

#ifdef DEVELOPMENT
	if (draw_gizmos)
	{
		renderer.draw_gizmos(this,colliders_draw_mesh,helpers);
	}

#endif

#endif
}

void Engine::main_loop()
{
#ifdef LINUX
	gettimeofday(&t1, &tz);
#endif
	while (!window_manager.window_should_close())
	{

		window_manager.check_events();

		input.update_input(this);

		loop_data();

		auto tStart = std::chrono::high_resolution_clock::now();

#if defined VULKAN || defined (DX11) 
		renderer.draw_frame();
#endif

#if defined(ES2) || defined(ANDROID) || defined (OPENGL)
		es2_draw_frame();
#endif

#ifdef DEVELOPMENT
		frames++;
		calculate_fps(tStart);
#endif

#ifndef DX11
		window_manager.swap_buffers();
#endif // !DX11

		tranlation_update.movements.clear();
	}

	delete game;

#ifdef VULKAN
	renderer.finish();
//window manager clear ?
#endif
}

void Engine::update_mvp(EMesh* mesh)
{

	glm::mat4 mat = glm::mat4(1.0);
	if (mesh->bIsGUI)
	{
		//TODO: 3d gui
		if (input.Z.bIsPressed)
		{
			mat = translate(mat, vec3(-0.5, -0.5, 0)) * scale(mat, vec3(0.1, 0.1, 1));
			mat = rotate(mat, radians(90.f), vec3(1, 0, 0));
			mat = rotate(mat, radians(90.f), vec3(0, 0, 1));
			mat = translate(mat, vec3(0, 0, -100));

			mat = main_camera.Projection * main_camera.View * mat;
		}

		//loading screen
		if (loading)
		{
			mat = mat4(1.0);
			mat = rotate(mat, radians(180.f), vec3(1, 0, 0)) * scale(mat, vec3(0.3, 0.3, 1));
			loading = false;
			mesh->MVP = mat;
		}
	}
	else
	{

		mat = main_camera.Projection * main_camera.View * mesh->model_matrix;
	}

	if (!mesh->bIsGUI)
	{
		mesh->MVP = mat;
	}
#if defined(ES2) || defined(ANDROID) || defined (OPENGL)
	if (mesh->type == MESH_TYPE_SKINNED)
	{
		mesh->ubo.proj = main_camera.Projection;
		mesh->ubo.view = main_camera.View;
		mesh->ubo.model = mesh->model_matrix;
	}

	renderer.update_mvp(mesh);
#endif

}

void Engine::delete_meshes()
{
	for (auto mesh : meshes)
	{
		delete mesh;
	}
}

#ifdef DEVELOPMENT
void Engine::print_vector(glm::vec3 vector){
	std::cout << vector.x << " " << vector.y << " " << vector.z << std::endl;
}

void Engine::print_debug(const std::string text, int8_t posx, int8_t posy)
{
	printf("%c[%i;%iH", 0x1B, posx, posy);
	printf(text.c_str());
}
void Engine::print_fps()
{
	print_debug("", 0, 15);
	printf("FPS: ");
	printf("%i", last_fps);
	printf(" Frames: %i", frames);
	printf(" Frame time: %f", frame_time);
}

void Engine::calculate_fps(TTime tStart)
{
#ifdef LINUX
		auto tEnd = std::chrono::high_resolution_clock::now();
		auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
		frame_time = (float)tDiff / 1000.0f;

		fps += (float)tDiff;
		if (fps > 1000.0f)
		{
			last_fps = static_cast<uint32_t>((float)frames * (1000.0f / fps));
			fps = 0;
			frames = 0;
		}

		if (++num_frames % 60 == 0)
		{
			gettimeofday(&t2, &tz);
			//float dt  =  t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6;
			//cout << "fps: " << num_frames / dt << endl;
			//print_debug("",0,15);
			//printf("FPS: ");
			//float fps = num_frames / dt;
			//std::cout << fps << std::endl;

			num_frames = 0;
			t1 = t2;
		}
	#ifdef ANDROID
		LIMIT_FPS = 32;
	#endif
		usleep(1000 * LIMIT_FPS);

#endif // LINUX
}

void Engine::calculate_time(std::string text, TTime tStart)
{
	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
	float elapsed_time = (float)tDiff / 1000.0f;

	std::cout << "Loading " << text << " time: " << elapsed_time << std::endl;
}
#endif

float Engine::get_time()
{

	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	deltaTime = time - lastFrame;
	lastFrame = time;

	return time;
}



void Engine::configure_window_callback()
{
#if defined (GLFW)
	window = window_manager.get_window();
	glfwSetWindowUserPointer(window, this);

	glfwSetFramebufferSizeCallback(window, window_manager.framebufferResizeCallback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
	glfwSetKeyCallback(window, input.key_callback);
	glfwSetCursorPosCallback(window, input.mouse_callback);
	glfwSetMouseButtonCallback(window, input.mouse_button_callback);
	glfwSetScrollCallback(window, input.scroll_callback);
#endif
}

void Engine::load_and_assing_location(std::string path, glm::vec3 location)
{
#ifdef VULKAN
	EMesh *model = new EMesh(vulkan_device); //vulkan device for create vertex buffers
	model->texture.format = VK_FORMAT_R8G8B8A8_UNORM;
#else
	EMesh *model = new EMesh();
#endif
#ifdef ANDROID
	mesh_manager.load_mode_gltf_android(model, path.c_str(), pAndroid_app->activity->assetManager);
#else
	mesh_manager.load_model_gltf(model, path.c_str());
#endif

	glm::mat4 model_matrix = glm::mat4(1.0f);
	model_matrix = glm::translate(model_matrix, location);
	model->location_vector = location;
	model->model_matrix = model_matrix;
	meshes.push_back(model);
	linear_meshes.push_back(model);
}

void Engine::load_and_assing_location(struct MapDataToLoad data)
{
	std::string path = assets.path(data.model_path);
	vec3 location = data.location;

#ifdef VULKAN
	EMesh *model = new EMesh(vulkan_device); //vulkan device for create vertex buffers
	model->texture.format = VK_FORMAT_R8G8B8A8_UNORM;
#else
	EMesh *model = new EMesh();
#endif
#ifdef ANDROID
	mesh_manager.load_mode_gltf_android(model, path.c_str(), pAndroid_app->activity->assetManager);
#else
	mesh_manager.load_model_gltf(model, path.c_str());
#endif

	glm::mat4 model_matrix = glm::mat4(1.0f);
	model_matrix = glm::translate(model_matrix, location);
	model->location_vector = location;
	model->model_matrix = model_matrix;
	if (data.type != MESH_LOD)
	{
		meshes.push_back(model);
	}
	model->texture_path = assets.path(data.texture_path);
	linear_meshes.push_back(model);
}


void Engine::translate_mesh(EMesh *mesh, uint direction, float value)
{

	vec3 direction_vector;
	switch (direction)
	{
	case FORWARD:
		direction_vector = vec3(0, -1, 0);
		break;
	case BACKWARD:
		direction_vector = vec3(0, 1, 0);
		break;
	case LEFT:
		direction_vector = vec3(1, 0, 0);
		break;
	case RIGTH:
		direction_vector = vec3(-1, 0, 0);
		break;
	case UP:
		direction_vector = vec3(0, 0, -1);
		break;
	case DOWN:
		direction_vector = vec3(0, 0, 1);
		break;
	}

	Movement movement = {direction_vector, value};

	Collider collider = mesh->collider;
	if (collider.collision)
	{
		if (collider.negative_x)
		{
			collider.can_move_positive_x = false;
		}

		if (collider.positive_x)
		{
			collider.can_move_negative_x = false;
		}
		if (collider.positive_y)
		{
			collider.can_move_negative_y = false;
		}
		if (collider.negative_y)
		{
			collider.can_move_positive_y = false;
		}
	}

	switch (direction)
	{
	case FORWARD:
		if (collider.can_move_positive_y)
		{
			Objects::translate(this, mesh, movement);
		}
		break;
	case BACKWARD:
		if (collider.can_move_negative_y)
		{
			Objects::translate(this, mesh, movement);
		}
		break;
	case LEFT:
		if (collider.can_move_negative_x)
		{
			Objects::translate(this, mesh, movement);
		}
		break;
	case RIGTH:
		if (collider.can_move_positive_x)
		{
			Objects::translate(this, mesh, movement);
		}
		break;

	default:
		Objects::translate(this, mesh, movement);
		break;
	}
}

void Engine::distance_object_from_camera()
{
	vec3 camera_position = main_camera.cameraPos;
	//int mesh_id = 9;
	EMesh *mesh = mesh_manager.mesh_by_name("pole");
	EMesh* lod = mesh_manager.mesh_by_name("poleLOD3");
	if(!lod || !mesh)
		return;
	mesh->lod1 = lod;
	vec3 object_position = mesh->location_vector;

	bool erased = false;
	float distance;
	if (!erased)
	{
		distance = glm::distance(camera_position, object_position);

#ifdef ES2
		//std::cout << "distance: " << distance <<std::endl;
		if (distance > 15)
		{
			meshes[12] = linear_meshes[13];
			erased = true;
			//std::cout << "erased" << std::endl;
		}
		if (distance < 15)
		{
			meshes[12] = linear_meshes[12];
			erased = true;
			//std::cout << "erased" << std::endl;
		}
#endif
	}
}

void Engine::init_collision_engine()
{

	std::thread col_thread(Collision::update_collision_engine, (Engine *)this);
	col_thread.detach();
}


void Engine::update_render_size()
{
	main_camera.update_projection_matrix();
#if defined(ES2) || defined (ANDROID) || defined (OPENGL)
	glViewport(0, 0, main_camera.screen_width, main_camera.screen_height);
#endif

#ifdef  ES2
	if (ready_to_game)
	{
		if (game)
		{
			if (game->gui)
				game->gui->update_elements_mvp();
		}
	}
#endif //  ES2
}


