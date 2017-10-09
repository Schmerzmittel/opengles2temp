
object::object ()
{
	tras = glm::mat4 (1);
	rota = glm::mat4 (1);
}

object::~object ()
{
}

void object::translate (float x, float y, float z)
{
	translate (glm::vec3 { x, y, z });
}

void object::translate (const glm::vec3 & pos)
{
	this->pos = pos;
	tras = glm::translate (pos);
}

void object::rotate (float angle, float p, float y, float r)
{
	rotate (angle, { p, y, r });
}

void object::rotate (float angle, const glm::vec3 & col)
{
	rot = col * angle;
	rota = glm::rotate (glm::mat4 (1), angle, col);
}

wavefront::wavefront (const std::string & fname)
{
	load (fname);
}

wavefront::~wavefront ()
{
}

struct wavefront_material
{
	std::string name;
	glm::vec4 ka, kd, ks;
	double ns;
	int illum;
};

void wavefront::load (const std::string & fname)
{
	verts.clear ();
	norms.clear ();
	texts.clear ();
	colos.clear ();
	indis.clear ();

	std::stringstream ss;
	std::vector <char> src = read_asset (fname);
	std::vector <uint16_t> nindices, tindices;
	std::vector <wavefront_material> materials;
	wavefront_material * usemtl = nullptr;

	std::copy (src.begin (), src.end (), std::ostreambuf_iterator <char> (ss));

	auto check_quote = [] (std::istringstream & iss)->bool
	{
		auto buf = iss.rdbuf ();
		auto next = buf->sbumpc ();

		while (next == ' ')
			next = buf->sbumpc ();

		if (next == '#')
			return true;

		buf->sputbackc (next);

		return false;
	};

	auto check_indices = [] (std::vector <uint16_t> & verts, const std::vector <uint16_t> & face)->void
	{
		if (face.size () < 1)
			return;

		if (face.size () % 3 == 0)
		{
			verts.insert (verts.end (), face.begin (), face.end ());
			return;
		}
		else if (face.size () % 4 == 0)
		{
			verts.push_back (face [0]);
			verts.push_back (face [1]);
			verts.push_back (face [2]);
			verts.push_back (face [2]);
			verts.push_back (face [3]);
			verts.push_back (face [0]);
			return;
		}
	};

	while (not ss.eof ())
	{
		std::string line;
		std::getline (ss, line);

		trim (line);

		if (line [0] == '#')
			continue;

		if (line.compare (0, 2, "v ") == 0)
		{
			glm::vec3 v;
			std::istringstream iss (line.substr (2));
			iss >> v.x >> v.y >> v.z;
			verts.push_back (std::move (v));
		}
		else if (line.compare (0, 3, "vt ") == 0)
		{
			glm::vec2 v;
			std::istringstream iss (line.substr (3));
			iss >> v.x >> v.y;
			texts.push_back (std::move (v));
		}
		else if (line.compare (0, 3, "vn ") == 0)
		{
			glm::vec3 v;
			std::istringstream iss (line.substr (3));
			iss >> v.x >> v.y >> v.z;
			norms.push_back (std::move (v));
		}
		else if (line.compare (0, 3, "vp ") == 0)
		{
		}
		else if (line.compare (0, 2, "f ") == 0)
		{
			std::istringstream iss (line.substr (2));
			std::vector <uint16_t> vis, tis, nis;

			if (norms.empty () and texts.empty ())
			{
				uint16_t index;

				while (not iss.eof ())
				{
					iss >> index;
					vis.push_back (index - 1);

					if (check_quote (iss))
						break;
				}
			}
			else if (norms.empty () and not texts.empty ())
			{
				uint16_t vi, ti;
				char c;

				while (not iss.eof ())
				{
					iss >> vi >> c >> ti;
					vis.push_back (vi - 1);
					tis.push_back (ti - 1);

					if (check_quote (iss))
						break;
				}
			}
			else if (not norms.empty () and texts.empty ())
			{
				uint16_t vi, ni;
				char c;

				while (not iss.eof ())
				{
					iss >> vi >> c >> c >> ni;
					vis.push_back (vi - 1);
					nis.push_back (ni - 1);

					if (check_quote (iss))
						break;
				}
			}
			else
			{
				uint16_t vi, ti, ni;
				char c;

				while (not iss.eof ())
				{
					iss >> vi >> c >> ti >> c >> ni;
					vis.push_back (vi - 1);
					tis.push_back (ti - 1);
					nis.push_back (ni - 1);

					if (check_quote (iss))
						break;
				}
			}

			check_indices (indis, vis);
			check_indices (tindices, tis);
			check_indices (nindices, nis);

			if (colos.size () != verts.size ())
				colos.resize (verts.size ());

			if (usemtl != nullptr)
			{
				for (auto & i : vis)
					colos [i] = usemtl->ka;
			}
			else
			{
				for (auto & i : vis)
					colos [i] = glm::vec4 { 1.0f, 1.0f, 1.0f, 1.0f };
			}
		}
		else if (line.compare (0, 7, "usemtl ") == 0)
		{
			auto target = line.substr (7);

			for (size_t i = 0 ; i < materials.size () ; i ++)
			{
				if (materials [i].name == target)
				{
					usemtl = &materials [i];
					break;
				}
			}
		}
		else if (line.compare (0, 7, "mtllib ") == 0)
			materials = load_mtl (line.substr (7));
	}

	std::vector <glm::vec2> tcopy;
	std::vector <glm::vec3> ncopy;

	for (auto & i : tindices)
		tcopy.push_back (texts [i]);

	for (auto & i : nindices)
		ncopy.push_back (norms [i]);

	texts = std::move (tcopy);
	norms = std::move (ncopy);
}

void object::model (std::function <void (glm::mat4 & mat)> lambda)
{
	auto mat = tras * rota;
	lambda (mat);
}

std::vector <wavefront_material> wavefront::load_mtl (const std::string & fname)
{
	std::stringstream ss;
	std::vector <wavefront_material> ret;

	auto data = read_asset (fname);

	std::copy (data.begin (), data.end (), std::ostreambuf_iterator <char> (ss));

	while (not ss.eof ())
	{
		std::string line;
		std::getline (ss, line);

		trim (line);

		if (line [0] == '#')
			continue;

		if (line.compare (0, 7, "newmtl ") == 0)
		{
			wavefront_material mat;
			mat.name = line.substr (7);
			ret.push_back (std::move (mat));
		}
		else if (line.compare (0, 3, "Ka ") == 0)
		{
			auto & thiz = ret.back ();
			std::istringstream iss (line.substr (3));
			iss >> thiz.ka.x >> thiz.ka.y >> thiz.ka.z;

			if (not iss.eof ())
				iss >> thiz.ka.a;
			else
				thiz.ka.a = 1.0f;
		}
		else if (line.compare (0, 3, "Kd ") == 0)
		{
			auto & thiz = ret.back ();
			std::istringstream iss (line.substr (3));
			iss >> thiz.kd.x >> thiz.kd.y >> thiz.kd.z;

			if (not iss.eof ())
				iss >> thiz.kd.a;
			else
				thiz.kd.a = 1.0f;
		}
		else if (line.compare (0, 3, "Ks ") == 0)
		{
			auto & thiz = ret.back ();
			std::istringstream iss (line.substr (3));
			iss >> thiz.ks.x >> thiz.ks.y >> thiz.ks.z;

			if (not iss.eof ())
				iss >> thiz.ks.a;
			else
				thiz.ks.a = 1.0f;
		}
		else if (line.compare (0, 3, "Ns ") == 0)
		{
			std::istringstream iss (line.substr (3));
			iss >> ret.back ().ns;
		}
		else if (line.compare (0, 6, "illum ") == 0)
		{
			std::istringstream iss (line.substr (6));
			iss >> ret.back ().illum;
		}
	}

	return ret;
}
