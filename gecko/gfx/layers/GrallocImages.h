/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef GRALLOCIMAGES_H
#define GRALLOCIMAGES_H

#ifdef MOZ_WIDGET_GONK

#include "ImageLayers.h"
#include "ImageContainer.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/layers/AtomicRefCountedWithFinalize.h"
#include "mozilla/layers/FenceUtils.h"
#include "mozilla/layers/LayersSurfaces.h"

#include <ui/GraphicBuffer.h>

namespace mozilla {
namespace layers {

class TextureClient;

already_AddRefed<gfx::DataSourceSurface>
GetDataSourceSurfaceFrom(android::sp<android::GraphicBuffer>& aGraphicBuffer,
                         gfx::IntSize aSize,
                         const layers::PlanarYCbCrData& aYcbcrData);

/**
 * The YUV format supported by Android HAL
 *
 * 4:2:0 - CbCr width and height is half that of Y.
 *
 * This format assumes
 * - an even width
 * - an even height
 * - a horizontal stride multiple of 16 pixels
 * - a vertical stride equal to the height
 *
 * y_size = stride * height
 * c_size = ALIGN(stride/2, 16) * height/2
 * size = y_size + c_size * 2
 * cr_offset = y_size
 * cb_offset = y_size + c_size
 *
 * The Image that is rendered is the picture region defined by
 * mPicX, mPicY and mPicSize. The size of the rendered image is
 * mPicSize, not mYSize or mCbCrSize.
 */
class GrallocImage : public RecyclingPlanarYCbCrImage
{
  typedef PlanarYCbCrData Data;
  static int32_t sColorIdMap[];
public:
  GrallocImage();

  virtual ~GrallocImage();

  virtual bool CopyData(const Data& aData) override;

  /**
   * This makes a copy of the data buffers, in order to support functioning
   * in all different layer managers.
   */
  virtual bool SetData(const Data& aData);

  using RecyclingPlanarYCbCrImage::AdoptData;
  /**
   *  Share the SurfaceDescriptor without making the copy, in order
   *  to support functioning in all different layer managers.
   */
  void AdoptData(TextureClient* aGraphicBuffer, const gfx::IntSize& aSize);

  /*
   * The color formats defined here is copied from vendors' header file(
   * typically under the directory of gralloc. The reason that we don't
   * include the header directly are:
   * 1. To include vendor's color format header, we need to add additional
   *    including path for each users of GrallocImage, which need a lot of
   *    modification, and will introduce lots of poriting efforts with future
   *    version of gecko.
   * 2. The vendor's color format header may not be able to be compiled
   *    during build process of gecko. This is because the vendor's header
   *    usually leveraged from their AOSP version, which can be compiled
   *    well during AOSP build process, however it may be failed due to
   *    different compiling condition(i.e. warning condition/level).
   */
#if defined(PRODUCT_MANUFACTURER_QUALCOMM)
  enum {
    /* OEM specific HAL formats */
    HAL_PIXEL_FORMAT_RGBA_5551              = 6,
    HAL_PIXEL_FORMAT_RGBA_4444              = 7,
    HAL_PIXEL_FORMAT_NV12_ENCODEABLE        = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS     = 0x7FA30C04,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED     = 0x7FA30C03,
    HAL_PIXEL_FORMAT_YCbCr_420_SP           = 0x109,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x7FA30C01,
    HAL_PIXEL_FORMAT_YCrCb_422_SP           = 0x10B,
    HAL_PIXEL_FORMAT_R_8                    = 0x10D,
    HAL_PIXEL_FORMAT_RG_88                  = 0x10E,
    HAL_PIXEL_FORMAT_YCbCr_444_SP           = 0x10F,
    HAL_PIXEL_FORMAT_YCrCb_444_SP           = 0x110,
    HAL_PIXEL_FORMAT_YCrCb_422_I            = 0x111,
    HAL_PIXEL_FORMAT_BGRX_8888              = 0x112,
    HAL_PIXEL_FORMAT_NV21_ZSL               = 0x113,
    HAL_PIXEL_FORMAT_INTERLACE              = 0x180,
    //v4l2_fourcc('Y', 'U', 'Y', 'L'). 24 bpp YUYV 4:2:2 10 bit per component
    HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT      = 0x4C595559,
    //v4l2_fourcc('Y', 'B', 'W', 'C'). 10 bit per component. This compressed
    //format reduces the memory access bandwidth
    HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT_COMPRESSED = 0x43574259,

    //Khronos ASTC formats
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_4x4_KHR    = 0x93B0,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_5x4_KHR    = 0x93B1,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_5x5_KHR    = 0x93B2,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_6x5_KHR    = 0x93B3,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_6x6_KHR    = 0x93B4,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_8x5_KHR    = 0x93B5,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_8x6_KHR    = 0x93B6,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_8x8_KHR    = 0x93B7,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x5_KHR   = 0x93B8,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x6_KHR   = 0x93B9,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x8_KHR   = 0x93BA,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_10x10_KHR  = 0x93BB,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_12x10_KHR  = 0x93BC,
    HAL_PIXEL_FORMAT_COMPRESSED_RGBA_ASTC_12x12_KHR  = 0x93BD,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR    = 0x93D0,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR    = 0x93D1,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR    = 0x93D2,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR    = 0x93D3,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR    = 0x93D4,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR    = 0x93D5,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR    = 0x93D6,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR    = 0x93D7,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR   = 0x93D8,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR   = 0x93D9,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR   = 0x93DA,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR  = 0x93DB,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR  = 0x93DC,
    HAL_PIXEL_FORMAT_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR  = 0x93DD,

    // To be compatible with old GrallocImage enum
    HAL_PIXEL_FORMAT_YCbCr_422_P            = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_P            = 0x103,
  };

#elif defined(PRODUCT_MANUFACTURER_SPRD)
  /* From vendor/sprd/modules/libgpu/gralloc/utgard/gralloc_ext_sprd.h */
  enum
  {
    /* OEM specific HAL formats */
    HAL_PIXEL_FORMAT_YCbCr_420_P  = 0x13,
    HAL_PIXEL_FORMAT_YCbCr_420_SP = 0x15, /*OMX_COLOR_FormatYUV420SemiPlanar*/
    HAL_PIXEL_FORMAT_YCrCb_422_SP = 0x1B,
    HAL_PIXEL_FORMAT_YCrCb_420_P  = 0x1C,

    // To be compatible with old GrallocImage enum
    HAL_PIXEL_FORMAT_YCbCr_422_P            = 0x102,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x10A,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED     = 0x7FA30C03,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS     = 0x7FA30C04,
  };

#elif defined(PRODUCT_MANUFACTURER_MTK)
  /* From vendor/mediatek/proprietary/hardware/gralloc_extra/include/graphics_mtk_defs.h */
  enum
  {
    /* OEM specific HAL formats */
    HAL_PIXEL_FORMAT_I420                 = 0x32315659 + 0x10,          /// MTK I420
    HAL_PIXEL_FORMAT_YUV_PRIVATE          = 0x32315659 + 0x20,          /// I420 or NV12_BLK or NV12_BLK_FCM
    HAL_PIXEL_FORMAT_YUV_PRIVATE_10BIT    = 0x32315659 + 0x30,          /// I420 or NV12_BLK or NV12_BLK_FCM - 10bit

    // To be compatible with old GrallocImage enum
    HAL_PIXEL_FORMAT_YCbCr_422_P            = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_P            = 0x103,
    HAL_PIXEL_FORMAT_YCbCr_420_SP           = 0x109,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x10A,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED     = 0x7FA30C03,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS     = 0x7FA30C04,
  };

#else
  // old enum, for emulator and other unspecific devices.
  // From [android 4.0.4]/hardware/msm7k/libgralloc-qsd8k/gralloc_priv.h
  enum {
    /* OEM specific HAL formats */
    HAL_PIXEL_FORMAT_YCbCr_422_P            = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_P            = 0x103,
    HAL_PIXEL_FORMAT_YCbCr_420_SP           = 0x109,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO    = 0x10A,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED     = 0x7FA30C03,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS     = 0x7FA30C04,
  };

#endif
  // From lollipop-release:
  //   hardware/intel/img/hwcomposer/include/pvr/hal/hal_public.h
  enum {
    HAL_PIXEL_FORMAT_NV12_VED               = 0x7FA00E00,
    HAL_PIXEL_FORMAT_NV12_VEDT              = 0x7FA00F00,
  };

  enum {
    GRALLOC_SW_UAGE = android::GraphicBuffer::USAGE_SOFTWARE_MASK,
  };

  virtual already_AddRefed<gfx::SourceSurface> GetAsSourceSurface() override;

  android::sp<android::GraphicBuffer> GetGraphicBuffer() const;

  void* GetNativeBuffer();

  virtual bool IsValid() { return !!mTextureClient; }

  virtual TextureClient* GetTextureClient(CompositableClient* aClient) override;

  virtual GrallocImage* AsGrallocImage() override
  {
    return this;
  }

  virtual uint8_t* GetBuffer()
  {
    return static_cast<uint8_t*>(GetNativeBuffer());
  }

  int GetUsage()
  {
    return (static_cast<ANativeWindowBuffer*>(GetNativeBuffer()))->usage;
  }

  static int GetOmxFormat(int aFormat)
  {
    uint32_t omxFormat = 0;

    for (int i = 0; sColorIdMap[i]; i += 2) {
      if (sColorIdMap[i] == aFormat) {
        omxFormat = sColorIdMap[i + 1];
        break;
      }
    }

    return omxFormat;
  }

private:
  RefPtr<TextureClient> mTextureClient;
};

} // namespace layers
} // namespace mozilla

#endif

#endif /* GRALLOCIMAGES_H */
