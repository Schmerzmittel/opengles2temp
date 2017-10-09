
EGLint configs [] =
{
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_RENDERABLE_TYPE, /* EGL_OPENGL_ES3_BIT_KHR */ EGL_OPENGL_ES2_BIT,
	EGL_CLIENT_APIS, EGL_OPENGL_API,
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_DEPTH_SIZE, 16,
	EGL_STENCIL_SIZE, 8,
	EGL_NONE
}, attribs [] =
{
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

void engine::init_display ()
{
	EGLint format;
	EGLint numConfigs;
	EGLConfig config;

	assert (display = eglGetDisplay (EGL_DEFAULT_DISPLAY));

	assert (eglInitialize (display, 0, 0));
	assert (eglChooseConfig (display, configs, &config, 1, &numConfigs));
	assert (eglGetConfigAttrib (display, config, EGL_NATIVE_VISUAL_ID, &format));

	ANativeWindow_setBuffersGeometry (app->window, 0, 0, format);

	surface = eglCreateWindowSurface (display, config, app->window, nullptr);
	context = eglCreateContext (display, config, nullptr, attribs);

	assert (eglMakeCurrent (display, surface, surface, context));

	eglQuerySurface (display, surface, EGL_WIDTH, &width);
	eglQuerySurface (display, surface, EGL_HEIGHT, &height);

	version = (char *) glGetString (GL_VERSION);

	assert (glprogram = glCreateProgram ());

	assert (load_shader (vshader, GL_VERTEX_SHADER));
	assert (load_shader (fshader, GL_FRAGMENT_SHADER));

	glAttachShader (glprogram, glvertex);
	glAttachShader (glprogram, glfragment);

	glBindAttribLocation (glprogram, 0, "pos");
	glBindAttribLocation (glprogram, 1, "nor");
	glBindAttribLocation (glprogram, 2, "col");

	glLinkProgram (glprogram);

	GLint stat;

	glGetProgramiv (glprogram, GL_LINK_STATUS, &stat);

	assert (stat);

	mvphandle = glGetUniformLocation (glprogram, "view_perspective");
	mvmhandle = glGetUniformLocation (glprogram, "model");
	lgthandle = glGetUniformLocation (glprogram, "light_position");
	poshandle = glGetAttribLocation (glprogram, "position");
	norhandle = glGetAttribLocation (glprogram, "normal");
	colhandle = glGetAttribLocation (glprogram, "color");

	glUseProgram (glprogram);

	glEnableVertexAttribArray (poshandle);
	glEnableVertexAttribArray (norhandle);
	glEnableVertexAttribArray (colhandle);

	glViewport (0, 0, width, height);
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);

	proj = glm::perspective (glm::radians (60.0f), (float) width / height, 0.01f, 100.0f);
	view = glm::lookAt (camera, glm::vec3 { 0.0f, 0.0f, 0.0f }, glm::vec3 { 0.0f, 1.0f, 0.0f });
}

void engine::draw_frame ()
{
	if (display == nullptr)
		return;

	auto mvp = proj * view;

	light = glm::vec3 { 2.0f * cos (get_time ()), 2.0f, 2.0f * sin (get_time ()) };

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);

	glUniformMatrix4fv (mvphandle, 1, GL_FALSE, glm::value_ptr (mvp));
	glUniform3fv (lgthandle, 1, glm::value_ptr (light));

	for (auto & obj : objs)
	{
		obj->model ([=] (glm::mat4 & mat)->void
		{
			glUniformMatrix4fv (mvmhandle, 1, GL_FALSE, glm::value_ptr (mat));
		});

		auto size = obj->indis.size ();

		glVertexAttribPointer (poshandle, 3, GL_FLOAT, GL_FALSE, 0, obj->verts.data ());
		glVertexAttribPointer (norhandle, 3, GL_FLOAT, GL_FALSE, 0, obj->norms.data ());
		glVertexAttribPointer (colhandle, 4, GL_FLOAT, GL_FALSE, 0, obj->colos.data ());

		glDrawElements (GL_TRIANGLES, obj->indis.size (), GL_UNSIGNED_SHORT, obj->indis.data ());
	}
	
	eglSwapBuffers (display, surface);
}

void engine::term_display ()
{
	glDisableVertexAttribArray (poshandle);
	glDisableVertexAttribArray (norhandle);
	glDisableVertexAttribArray (colhandle);

	if (glvertex != 0)
	{
		glDeleteShader (glvertex);
		glvertex = 0;
	}

	if (glfragment != 0)
	{
		glDeleteShader (glfragment);
		glfragment = 0;
	}

	if (glprogram != 0)
	{
		glDeleteProgram (glprogram);
		glprogram = 0;
	}

	if (display != EGL_NO_DISPLAY)
	{
		eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (context != EGL_NO_CONTEXT)
			eglDestroyContext (display, context);

		if (surface != EGL_NO_SURFACE)
			eglDestroySurface (display, surface);

		eglTerminate (display);
	}

	animating = false;
	display = EGL_NO_DISPLAY;
	context = EGL_NO_CONTEXT;
	surface = EGL_NO_SURFACE;
}

void engine::set_shader_sources (const std::vector <char> & vshader, const std::vector <char> & fshader)
{
	this->vshader = vshader;
	this->fshader = fshader;
}

void engine::add_object (std::shared_ptr <object> obj)
{
	objs.push_back (obj);
}

bool engine::load_shader (const std::vector <char> & data, int type)
{
	auto * handle = &glvertex;

	if (type == GL_FRAGMENT_SHADER)
		handle = &glfragment;

	(* handle) = glCreateShader (type);

	if (not handle)
		return false;

	auto src = const_cast <GLchar *> (data.data ());
	auto len = static_cast <GLint> (data.size ());

	glShaderSource (* handle, 1, &src, &len);
	glCompileShader (* handle);

	auto stat = static_cast <GLint> (1);

	glGetShaderiv (* handle, GL_COMPILE_STATUS, &stat);

	if (not stat)
	{
		glGetShaderiv (* handle, GL_INFO_LOG_LENGTH, &stat);

		std::string str;
		str.resize (stat + 1);

		if (stat)
		{
			glGetShaderInfoLog (* handle, stat + 1, &stat, const_cast <GLchar *> (str.data ()));
			log_error (str);
		}
		else
			log_error ("Unknown shader error.");

		glDeleteShader (* handle);

		* handle = 0;

		return false;
	}

	return true;
}
