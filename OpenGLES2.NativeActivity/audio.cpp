
#define slinvoke_noargs(obj,func) (* obj)->func (obj)
#define slinvoke(obj,func,...) (* obj)->func (obj, __VA_ARGS__)

void player_callback (SLAndroidSimpleBufferQueueItf queue, void * context)
{
	engine * eng = (engine *) context;

	assert (context != nullptr);
	assert (eng->slbufferqueue == queue);
}

void engine::init_audio ()
{
	assert (not slCreateEngine (&slobject, 0, nullptr, 0, nullptr, nullptr));
	assert (not slinvoke (slobject, Realize, SL_BOOLEAN_FALSE));
	assert (not slinvoke (slobject, GetInterface, SL_IID_ENGINE, &slengine));
	assert (not slinvoke (slengine, CreateOutputMix, &outputmix, 0, 0, 0));
	assert (not slinvoke (outputmix, Realize, SL_BOOLEAN_FALSE));

	SLDataLocator_AndroidSimpleBufferQueue locbufq =
	{
		SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
		2
	};

	SLDataFormat_PCM format =
	{
		SL_DATAFORMAT_PCM,
		2,
		SL_SAMPLINGRATE_44_1,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
		SL_BYTEORDER_LITTLEENDIAN
	};

	SLDataSource src =
	{
		&locbufq,
		&format
	};

	SLDataLocator_OutputMix locoutmix =
	{
		SL_DATALOCATOR_OUTPUTMIX,
		outputmix
	};

	SLDataSink sink =
	{
		&locoutmix,
		nullptr
	};

	const SLInterfaceID ids [] =
	{
		SL_IID_BUFFERQUEUE, SL_IID_VOLUME
	};

	const SLboolean req [] =
	{
		SL_BOOLEAN_TRUE,
		SL_BOOLEAN_TRUE
	};

	assert (not slinvoke (slengine, CreateAudioPlayer, &slplayer, &src, &sink, 2, ids, req));
	assert (not slinvoke (slplayer, Realize, SL_BOOLEAN_FALSE));
	assert (not slinvoke (slplayer, GetInterface, SL_IID_PLAY, &slplay));
	assert (not slinvoke (slplayer, GetInterface, SL_IID_BUFFERQUEUE, &slbufferqueue));
	assert (not slinvoke (slplayer, GetInterface, SL_IID_VOLUME, &slvolume));
	assert (not slinvoke (slbufferqueue, RegisterCallback, player_callback, this));

	// assert (not slinvoke (slplay, SetPlayState, SL_PLAYSTATE_PLAYING));
}

void engine::term_audio ()
{
	if (slplayer != nullptr)
	{
		slinvoke_noargs (slplayer, Destroy);
		slplayer = nullptr;
	}

	if (outputmix != nullptr)
	{
		slinvoke_noargs (outputmix, Destroy);
		outputmix = nullptr;
	}

	if (slobject != nullptr)
	{
		slinvoke_noargs (slobject, Destroy);
		slobject = nullptr;
	}
}
