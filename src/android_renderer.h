#ifndef _Android_Renderer_H_
#define _Android_Renderer_H_

#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <errno.h>
#include <cassert>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
//#include <GLES3/gl3ext.h>

//#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>


#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <chrono>

const char vertex_src [] =
"                                        \
   attribute vec3        position;       \
   varying mediump vec2  pos;            \
   uniform vec4          offset;         \
   uniform mat4          MVP;            \
                                         \
   void main()                           \
   {                                     \
      gl_Position = MVP * vec4(position,1.0);   \
      pos = position.xy;                 \
   }                                     \
";
 
 
const char fragment_src [] =
"                                                      \
   varying mediump vec2    pos;                        \
   uniform mediump float   phase;                      \
                                                       \
   void  main()                                        \
   {                                                   \
      gl_FragColor  =  vec4( 1., 0.9, 0.7, 1.0 ) *     \
        cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y)   \
             + atan(pos.y,pos.x) - phase );            \
   }                                                   \
";

void
print_shader_info_log (
   GLuint  shader      // handle to the shader
)
{
   GLint  length;
 
   glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &length );
 
   if ( length ) {
      char* buffer  =  new char [ length ];
      glGetShaderInfoLog ( shader , length , NULL , buffer );
      LOGW("shader info");
      //cout << "shader info: " <<  buffer << flush;
      delete [] buffer;
 
      GLint success;
      glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
      if ( success != GL_TRUE )   exit ( 1 );
   }
}

GLuint
load_shader (
   const char  *shader_source,
   GLenum       type
)
{
   GLuint  shader = glCreateShader( type );
 
   glShaderSource  ( shader , 1 , &shader_source , NULL );
   glCompileShader ( shader );
 
    print_shader_info_log ( shader );
 
   return shader;
}

const float vertexArray[] = {
   0.0,  0.5,  0.0,
  -0.5,  0.0,  0.0,
   0.0, -0.5,  0.0,
   0.5,  0.0,  0.0,
   0.0,  0.5,  0.0 
};
 
 
extern NativeWindowType createNativeWindow(void);
static EGLint const attribute_list[] = {
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_RED_SIZE, 8,
      EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
      EGL_NONE
};
static const EGLint GiveMeGLES2[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };

GLint
   phase_loc,
   offset_loc,
   position_loc,
   mvp_loc;

using namespace glm;
class Renderer{
public:
    AAssetManager* asset_manager;

    Renderer(android_app *pApp){
        app = pApp;
        asset_manager = pApp->activity->assetManager;
        LOGW("Initialiazing");
        init();

        

    };
    void render(){
        //LOGW("rendering");
        //glViewport(0,0,800,600);

        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlush();

      static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        glm::mat4 model = glm::mat4(1.0);
        glm::mat4 Projection = glm::perspective(glm::radians(45.f), 768.f/1280.f, 0.01f, 1000.f);
        glm::mat4 view = glm::lookAt(vec3(0,15,0),vec3(0,0,0),vec3(0,0,1));

      model = glm::rotate(model, time * glm::radians(12.f), glm::vec3(0.0f, 1.0f, 1.0f));

        mat4 mvp = Projection * view * model;

        glUniformMatrix4fv(mvp_loc,1,GL_FALSE,&mvp[0][0]);



      
         glVertexAttribPointer ( position_loc, 3, GL_FLOAT, false, 0, vertexArray );
            glEnableVertexAttribArray ( position_loc );
            glDrawArrays ( GL_TRIANGLE_STRIP, 0, 5 );
        
        eglSwapBuffers(display, surface);
    };
private:
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    NativeWindowType native_window;
    EGLint num_config;
    struct android_app * app;

    GLuint vertexbuffer;
    GLuint VertexArrayID;

     GLfloat vVertices[9] = {0.0f, 0.5f, 0.0f,
                               -0.5f, -0.5f, 0.0f,
                               0.5f, -0.5f, 0.0f};
                               
    void init(){

        
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

        
        eglInitialize(display, NULL, NULL);

        /* get an appropriate EGL frame buffer configuration */
        eglChooseConfig(display, attribute_list, &config, 1, &num_config);

       
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, GiveMeGLES2);       

        
        surface = eglCreateWindowSurface(display, config, app->window, NULL);

     
        eglMakeCurrent(display, surface, surface, context);  



        LOGW("Loading shaders........................");

       

        GLuint vertexShader   = load_shader ( vertex_src , GL_VERTEX_SHADER  );     // load vertex shader
        GLuint fragmentShader = load_shader ( fragment_src , GL_FRAGMENT_SHADER );  // load fragment shader
        
         LOGW("Shaders loaded");
        GLuint shaderProgram  = glCreateProgram ();                 // create program object
        glAttachShader ( shaderProgram, vertexShader );             // and attach both...
        glAttachShader ( shaderProgram, fragmentShader );           // ... shaders to it
        
        glLinkProgram ( shaderProgram );    // link the program
        glUseProgram  ( shaderProgram );    // and select it for usage
        
        //// now get the locations (kind of handle) of the shaders variables
        position_loc  = glGetAttribLocation  ( shaderProgram , "position" );
        phase_loc     = glGetUniformLocation ( shaderProgram , "phase"    );
        offset_loc    = glGetUniformLocation ( shaderProgram , "offset"   ); 
        mvp_loc         = glGetUniformLocation( shaderProgram , "MVP");


      LOGW("Loading shaders........................");


        AAsset* file = AAssetManager_open(asset_manager,"openme.txt", AASSET_MODE_BUFFER);

        size_t file_length = AAsset_getLength(file);
        char* fileContent = new char[file_length+1];

        AAsset_read(file, fileContent,file_length);

        __android_log_print(ANDROID_LOG_WARN,"native-activity","%s",fileContent);

      
    };

};

#endif