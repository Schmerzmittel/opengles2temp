
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <utility>
#include <functional>
#include <cassert>
#include <sstream>
#include <chrono>
#include <jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/sensor.h>
#include "android_native_app_glue.h"

#define logv(...) ((void) __android_log_print (ANDROID_LOG_VERBOSE, "net.schmzmtl.has", __VA_ARGS__))
#define logi(...) ((void) __android_log_print (ANDROID_LOG_INFO, "net.schmzmtl.has", __VA_ARGS__))
#define logw(...) ((void) __android_log_print (ANDROID_LOG_WARN, "net.schmzmtl.has", __VA_ARGS__))
#define loge(...) ((void) __android_log_print (ANDROID_LOG_ERROR, "net.schmzmtl.has", __VA_ARGS__))

void log_information (const std::string & log);
void log_warning (const std::string & log);
void log_error (const std::string & log);

double get_time ();

class engine;

class object
{
	friend class engine;

	glm::mat4 tras, rota;
	glm::vec3 pos, rot;

protected:
	std::vector <glm::vec3> verts, norms;
	std::vector <glm::vec4> colos;
	std::vector <glm::vec2> texts;
	std::vector <uint16_t> indis;

	object ();

public:
	object (const object &) = default;
	object (object &&) = default;

	virtual ~object ();

	virtual void load (const std::string & fname) = 0;

	void model (std::function <void (glm::mat4 & mat)> lambda);
	void translate (float x, float y, float z);
	void translate (const glm::vec3 & pos);
	void rotate (float angle, float p, float y, float r);
	void rotate (float angle, const glm::vec3 & col);
};

struct wavefront_material;

class wavefront : public object
{
	std::vector <wavefront_material> load_mtl (const std::string & fname);

public:
	wavefront (const std::string & fname);
	wavefront (const wavefront &) = default;
	wavefront (wavefront &&) = default;
	~wavefront ();

	void load (const std::string & fname);
};

class engine
{
	friend void player_callback (SLAndroidSimpleBufferQueueItf, void *);

	struct android_app * app;

	std::string version;
	std::vector <char> vshader, fshader;
	std::vector <std::shared_ptr <object>> objs;

	glm::vec3 light = { 2.0f, 2.0f, 2.0f }, camera = { -2.0f, 1.8f, -2.0f };
	glm::mat4 view, proj;

	int16_t audio_buffer [2] [256], which_buffer = 0;
	int32_t width, height;

	EGLDisplay display = EGL_NO_DISPLAY;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLContext context = EGL_NO_CONTEXT;

	SLObjectItf slobject = nullptr, outputmix = nullptr, slplayer = nullptr;
	SLEngineItf slengine = nullptr;
	SLPlayItf slplay = nullptr;
	SLAndroidSimpleBufferQueueItf slbufferqueue = nullptr;
	SLVolumeItf slvolume = nullptr;

	int glprogram = 0, glvertex = 0, glfragment = 0;
	int mvphandle = 0, mvmhandle = 0, lgthandle;
	int poshandle = 0, norhandle = 0, colhandle = 0;

	void init_display ();
	void init_audio ();
	void term_display ();
	void term_audio ();

public:
	engine (struct android_app * state) : app (state)
	{
		memset (audio_buffer [0], 0, 512);
		memset (audio_buffer [1], 0, 512);
	}
	~engine ()
	{
		term_display ();
		term_audio ();
	}

	bool animating;
	bool load_shader (const std::vector <char> & src, int type);

	int32_t handle_input (AInputEvent * event);

	void add_object (std::shared_ptr <object> obj);
	void set_shader_sources (const std::vector <char> & vshader, const std::vector <char> & fshader);
	void draw_frame ();
	void handle_cmd (int32_t cmd);
};

std::vector <char> read_asset (const std::string & fname);

void trim (std::string & str);
