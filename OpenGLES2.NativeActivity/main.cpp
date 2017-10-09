
static struct android_app * state;

double get_time ()
{
	auto now = std::chrono::steady_clock::now ().time_since_epoch ();
	return (double) std::chrono::duration_cast <std::chrono::microseconds> (now).count () / 1000000;
}

void trim (std::string & str)
{
	auto first = std::find_if (str.begin (), str.end (), [] (int ch)
	{
		return not std::isspace (ch);
	});

	str.erase (str.begin (), first);

	auto second = std::find_if (str.rbegin (), str.rend (), [] (int ch)
	{
		return not std::isspace (ch);
	}).base ();

	str.erase (second, str.end ());
}

std::vector <char> read_asset (const std::string & fname)
{
	auto asset = AAssetManager_open (state->activity->assetManager, fname.c_str (), AASSET_MODE_STREAMING);
	auto ret = std::vector <char> (AAsset_getLength64 (asset));

	assert (asset);

	AAsset_read (asset, ret.data (), ret.size ());
	AAsset_close (asset);

	return ret;
}

void log_information (const std::string & log)
{
	__android_log_print (ANDROID_LOG_INFO, "net.schmzmtl.opengles2", "%s\n", log.c_str ());
}

void log_warning (const std::string & log)
{
	__android_log_print (ANDROID_LOG_WARN, "net.schmzmtl.opengles2", "%s\n", log.c_str ());
}

void log_error (const std::string & log)
{
	__android_log_print (ANDROID_LOG_ERROR, "net.schmzmtl.opengles2", "%s\n", log.c_str ());
}

void engine::handle_cmd (int32_t cmd)
{
	switch (cmd)
	{
	case APP_CMD_SAVE_STATE:
		break;

	case APP_CMD_INIT_WINDOW:
		if (app->window == nullptr)
			return;

		init_display ();
		init_audio ();

		draw_frame ();
		break;

	case APP_CMD_TERM_WINDOW:
		term_display ();
		term_audio ();
		break;

	case APP_CMD_GAINED_FOCUS:
		break;

	case APP_CMD_LOST_FOCUS:
		animating = false;
		draw_frame ();
		break;

	case APP_CMD_LOW_MEMORY:
		break;

	default:
		break;
	}
}

int32_t engine::handle_input (AInputEvent * event)
{
	if (AInputEvent_getType (event) == AINPUT_EVENT_TYPE_MOTION)
	{
		AMotionEvent_getX (event, 0);
		AMotionEvent_getY (event, 0);

		return 1;
	}

	return 0;
}

static int32_t engine_handle_input (struct android_app * app, AInputEvent * event)
{
	auto eng = (engine *) app->userData;
	return eng->handle_input (event);
}

static void engine_handle_cmd (struct android_app * app, int32_t cmd)
{
	auto eng = (engine *) app->userData;
	eng->handle_cmd (cmd);
}

static void set_orientation ()
{
	auto & vm = state->activity->vm;
	auto env = (JNIEnv *) nullptr;

	vm->AttachCurrentThread (&env, nullptr);

	auto clazz = env->FindClass ("android/app/Activity");
	auto func = env->GetMethodID (clazz, "setRequestedOrientation", "(I)V");

	env->CallVoidMethod (state->activity->clazz, func, 6);
	vm->DetachCurrentThread ();
}

void android_main (struct android_app * state)
{
	auto dir = AAssetManager_openDir (state->activity->assetManager, "");
	auto eng = std::make_shared <engine> (state);

	::state = state;

	auto obj = std::make_shared <wavefront> ("cube.obj");

	eng->add_object (obj);

	state->userData = eng.get ();
	state->onAppCmd = engine_handle_cmd;
	state->onInputEvent = engine_handle_input;

	if (state->savedState != nullptr)
		;

	eng->set_shader_sources (read_asset ("shader.vert"), read_asset ("shader.frag"));
	eng->animating = true;

	set_orientation ();

	while (true)
	{
		int ident, events;
		struct android_poll_source * source;

		while ((ident = ALooper_pollAll (eng->animating ? 0 : -1, nullptr, &events, (void **) &source)) >= 0)
		{
			if (source != nullptr)
				source->process (state, source);

			if (state->destroyRequested != 0)
				goto exit;
		}

		if (eng->animating)
			eng->draw_frame ();
	}

exit:
	eng.reset ();
	AAssetDir_close (dir);
}
