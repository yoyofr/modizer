#include "Resources.h"
#include "TextureUtils.h"
#include <assert.h>


void ResourcesUtils::Load(Resources& resources)
{
	/*NSString* textureFilename = [[NSString alloc] initWithFormat:@"%@/texturebg.png", [[NSBundle mainBundle] resourcePath]];
	UIImage* texture = [[UIImage alloc] initWithContentsOfFile:textureFilename];
	resources.m_textureHandle = TextureUtils::Create(texture);
	//assert(resources.m_textureHandle != 0);
	[texture autorelease];
	[textureFilename autorelease];
*/
	/*NSString* meshFilename = [[NSString alloc] initWithFormat:@"%@/Car.obj", [[NSBundle mainBundle] resourcePath]];
	const bool ok = MeshUtils::Load(resources.m_mesh, [meshFilename cStringUsingEncoding:NSASCIIStringEncoding]);
	assert(ok);
	[meshFilename release];*/
}


void ResourcesUtils::Unload(Resources& resources)
{
	// TODO:
}

