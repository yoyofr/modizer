#ifndef st_Resources_h_
#define st_Resources_h_

#include "Mesh.h"


struct Resources
{
	Mesh m_mesh;
	uint m_textureHandle;
};


namespace ResourcesUtils
{
	void Load(Resources& resources);
	void Unload(Resources& resources);
}


#endif
