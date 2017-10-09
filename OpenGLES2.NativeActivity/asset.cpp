
asset::asset (struct android_app * app, const std::string & fname)
{
	base = AAssetManager_open (app->activity->assetManager, fname.c_str (), AASSET_MODE_STREAMING);
}

asset::~asset ()
{
	AAsset_close (base);
}

uint64_t asset::filesize ()
{
	return AAsset_getLength64 (base);
}

uint64_t asset::pos ()
{
	return filesize () - AAsset_getRemainingLength64 (base);
}

void asset::read (std::vector <char> & data, size_t length)
{
	AAsset_read (base, data.data (), length);
}

void asset::read (std::vector <char> & data)
{
	read (data, data.size ());
}

void asset::seek (size_t pos, whences whence)
{
	switch (whence)
	{
	case whences::set: AAsset_seek64 (base, pos, SEEK_SET);
		break;
	case whences::cur: AAsset_seek64 (base, pos, SEEK_CUR);
		break;
	case whences::end: AAsset_seek64 (base, pos, SEEK_END);
		break;
	default:
		break;
	}
}
