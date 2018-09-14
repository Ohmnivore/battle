#pragma once
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_FAILURE_STRINGS
#include "stb/stb_image.h"

#include "IO/IO.h"
#include "Gfx/Gfx.h"
#include "Gfx/private/gfxResourceContainer.h"

#include "STBTextureLoader.h"

namespace Oryol {

	//------------------------------------------------------------------------------
	STBTextureLoader::STBTextureLoader(const TextureSetup& setup_) :
	TextureLoaderBase(setup_) {
		// empty
	}

	//------------------------------------------------------------------------------
	STBTextureLoader::STBTextureLoader(const TextureSetup& setup_, LoadedFunc loadedFunc_) :
	TextureLoaderBase(setup_, loadedFunc_) {
		// empty
	}

	//------------------------------------------------------------------------------
	STBTextureLoader::~STBTextureLoader() {
		o_assert_dbg(!this->ioRequest);
	}

	//------------------------------------------------------------------------------
	void
	STBTextureLoader::Cancel() {
		if (this->ioRequest) {
			this->ioRequest->Cancelled = true;
			this->ioRequest = nullptr;
		}
	}

	//------------------------------------------------------------------------------
	Id
	STBTextureLoader::Start() {
		this->resId = Gfx::resource()->prepareAsync(this->setup);
		this->ioRequest = IO::LoadFile(setup.Locator.Location());
		return this->resId;
	}

	//------------------------------------------------------------------------------
	ResourceState::Code
	STBTextureLoader::Continue() {
		o_assert_dbg(this->resId.IsValid());
		o_assert_dbg(this->ioRequest.isValid());

		ResourceState::Code result = ResourceState::Pending;

		if (this->ioRequest->Handled) {
			if (IOStatus::OK == this->ioRequest->Status) {
				// yeah, IO is done, let gliml parse the texture data
				// and create the texture resource
				const uint8_t* data = this->ioRequest->Data.Data();
				const int numBytes = this->ioRequest->Data.Size();

				int width, height, numComponents;
				unsigned char* imgData = stbi_load_from_memory(data, numBytes, &width, &height, &numComponents, 0);

				if (imgData != NULL) {
					const int size = width * height * numComponents;
					TextureSetup texSetup = this->buildSetup(this->setup, width, height, numComponents, imgData, size);

					// call the Loaded callback if defined, this
					// gives the app a chance to look at the
					// setup object, and possibly modify it
					if (this->onLoaded) {
						this->onLoaded(texSetup);
					}

					// NOTE: the prepared texture resource might have already been
					// destroyed at this point, if this happens, initAsync will
					// silently fail and return ResourceState::InvalidState
					// (the same for failedAsync)
					result = Gfx::resource()->initAsync(this->resId, texSetup, imgData, size);
				}
				else {
					result = Gfx::resource()->failedAsync(this->resId);
				}
			}
			else {
				// IO had failed
				result = Gfx::resource()->failedAsync(this->resId);
			}
			this->ioRequest = nullptr;
		}
		return result;
	}

	//------------------------------------------------------------------------------
	TextureSetup
	STBTextureLoader::buildSetup(const TextureSetup& blueprint,
		const int width, const int height, const int numComponents,
		const uint8_t* data, const int size)
	{
		PixelFormat::Code pixelFormat = PixelFormat::InvalidPixelFormat;
		if (numComponents == 3)
			pixelFormat = PixelFormat::RGB8;
		else if (numComponents == 4)
			pixelFormat = PixelFormat::RGBA8;

		o_assert(PixelFormat::InvalidPixelFormat != pixelFormat);

		TextureSetup newSetup;
		newSetup = TextureSetup::FromPixelData2D(width, height, 1, pixelFormat, this->setup);

		// setup mipmap offsets
		newSetup.ImageData.Offsets[0][0] = 0;
		newSetup.ImageData.Sizes[0][0] = size;

		return newSetup;
	}

} // namespace Oryol
