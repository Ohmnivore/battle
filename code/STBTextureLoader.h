#pragma once
#include "Gfx/TextureLoaderBase.h"
#include "IO/private/ioRequests.h"

namespace Oryol {

	class STBTextureLoader : public TextureLoaderBase {
		OryolClassDecl(STBTextureLoader);
	public:
		/// constructor without success-callback
		STBTextureLoader(const TextureSetup& setup);
		/// constructor with success callback
		STBTextureLoader(const TextureSetup& setup, LoadedFunc onLoaded);
		/// destructor
		~STBTextureLoader();
		/// start loading, return a resource id
		virtual Id Start() override;
		/// continue loading, return resource state (Pending, Valid, Failed)
		virtual ResourceState::Code Continue() override;
		/// cancel the load process
		virtual void Cancel() override;

	private:
		/// convert gliml context attrs into a TextureSetup object
		TextureSetup buildSetup(const TextureSetup& blueprint,
			const int width, const int height, const int numComponents,
			const uint8_t* data, const int size);

		Id resId;
		Ptr<IORead> ioRequest;
	};

} // namespace Oryol
