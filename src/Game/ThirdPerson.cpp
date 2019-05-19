#include "ThirdPerson.hpp"
#ifndef ANDROID
    #include <glm/glm.hpp>
    #include <glm/gtc/matrix_transform.hpp>
#else
    #include "../glm/glm.hpp"
    #include "../glm/gtc/matrix_transform.hpp"
#endif


ThirdPerson::ThirdPerson(){

}

void ThirdPerson::update_camera_postion(){
	
	glm::mat4 mat = glm::translate(glm::mat4(1.0),glm::vec3(0,0,0));
	
	glm::mat4 rotated = glm::rotate(mat,glm::radians(90.f),glm::vec3(0,1,0));
	glm::mat4 rotated2 = glm::rotate(rotated,glm::radians(90.f),glm::vec3(0,1,0));
	glm::mat4 rotated3 = glm::rotate(rotated2,glm::radians(90.f),glm::vec3(-1,0,0));

	glm::mat4 translated =glm::translate(glm::mat4(1.0),camera_position);


	this->engine->main_camera.View = translated * rotated3 * glm::inverse(this->mesh->model_matrix);
	//engine->main_camera.cameraPos = camera_position + mesh->location_vector;
	
}
void ThirdPerson::update(){

	std::string log_position = "position " + std::to_string(mesh->location_vector.x) + " " + std::to_string(mesh->location_vector.y) + " " + std::to_string(mesh->location_vector.z);
	//engine->print_debug(log_position,10,0);	

	engine->play_animations = false;
	if(engine->input.W.bIsPressed == true){
		engine->animation_manager.play_animation(mesh->skeletal,"walk",true);
		engine->translate_mesh(mesh,FORWARD,velocity);
	}

	if(engine->input.D.bIsPressed){
		engine->translate_mesh(mesh,RIGTH,velocity);
	}

	if(engine->input.A.bIsPressed){
		engine->translate_mesh(mesh,LEFT,velocity);
	}

	if(engine->input.S.bIsPressed){
		engine->translate_mesh(mesh,BACKWARD,velocity);
	}	

	if(engine->input.Z.bIsPressed){		
	
	}
	if(engine->input.X.bIsPressed){
		
		//this->mesh->model_matrix = glm::rotate(mesh->model_matrix,glm::radians(-15.0f * engine->deltaTime),glm::vec3(0,0,1));
		 		
	}
	if(engine->input.SPACE.bIsPressed){				
		engine->animation_manager.play_animation(mesh->skeletal,"jump",true);
	}

	if(engine->input.right_button_pressed){
		engine->animation_manager.play_animation(mesh->skeletal,"aim",false);
	}
	#ifdef DEVELOPMENT
	if(engine->input.Q.bIsPressed){		
		engine->translate_mesh(mesh,UP,velocity);		
	}
	if(engine->input.E.bIsPressed){
		engine->translate_mesh(mesh,DOWN,velocity);
	}
	#endif
	

	update_camera_postion();
	
	mouse_control(engine->input.yaw, engine->input.pitch);


}

void ThirdPerson::mouse_control(float yaw, float pitch){
	
	float rotation = (yaw * -5.f) * engine->deltaTime;
	if(yaw != 0){
		this->mesh->model_matrix = glm::rotate(mesh->model_matrix,glm::radians(rotation),glm::vec3(0,0,1));
	}
}

