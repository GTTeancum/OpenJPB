/* Windows-only realization of original frontend PNG assets. */
#define COBJMACROS
#include <windows.h>
#include <objbase.h>
#include <wincodec.h>

#include "pc_image_wic.h"

#include <limits.h>

static int pc_utf8_to_wide(
    const char *path, wchar_t *wide_path, int capacity)
{
    int result;

    result = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        path, -1, wide_path, capacity);
    if (result == 0) {
        result = MultiByteToWideChar(
            CP_ACP, 0, path, -1, wide_path, capacity);
    }
    return result != 0;
}

int jpb_PCInspectImageWIC(
    const char *path,
    int *width,
    int *height)
{
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    wchar_t wide_path[1024];
    HRESULT initialize_result;
    HRESULT result;
    UINT image_width = 0;
    UINT image_height = 0;
    int initialized_com = 0;
    int success = 0;

    if (path == NULL || width == NULL || height == NULL ||
        !pc_utf8_to_wide(
            path,
            wide_path,
            (int)(sizeof(wide_path) / sizeof(wide_path[0])))) {
        return 0;
    }
    initialize_result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    initialized_com = initialize_result == S_OK ||
        initialize_result == S_FALSE;
    if (FAILED(initialize_result) &&
        initialize_result != RPC_E_CHANGED_MODE) {
        return 0;
    }
    result = CoCreateInstance(
        &CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory,
        (void **)&factory);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICImagingFactory_CreateDecoderFromFilename(
        factory,
        wide_path,
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICBitmapFrameDecode_GetSize(
        frame, &image_width, &image_height);
    if (FAILED(result) || image_width == 0 || image_height == 0 ||
        image_width > INT_MAX || image_height > INT_MAX) {
        goto cleanup;
    }
    *width = (int)image_width;
    *height = (int)image_height;
    success = 1;

cleanup:
    if (frame != NULL) {
        IWICBitmapFrameDecode_Release(frame);
    }
    if (decoder != NULL) {
        IWICBitmapDecoder_Release(decoder);
    }
    if (factory != NULL) {
        IWICImagingFactory_Release(factory);
    }
    if (initialized_com) {
        CoUninitialize();
    }
    return success;
}

int jpb_PCLoadImageWIC(
    const char *path,
    int width,
    int height,
    uint32_t *pixels,
    int stride_pixels)
{
    IWICImagingFactory *factory = NULL;
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *frame = NULL;
    IWICBitmapScaler *scaler = NULL;
    IWICFormatConverter *converter = NULL;
    wchar_t wide_path[1024];
    HRESULT initialize_result;
    HRESULT result;
    int initialized_com = 0;
    int success = 0;

    if (path == NULL || pixels == NULL ||
        width <= 0 || height <= 0 ||
        stride_pixels < width ||
        stride_pixels > INT_MAX / 4 ||
        height > INT_MAX / (stride_pixels * 4) ||
        !pc_utf8_to_wide(
            path,
            wide_path,
            (int)(sizeof(wide_path) / sizeof(wide_path[0])))) {
        return 0;
    }
    initialize_result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    initialized_com = initialize_result == S_OK ||
        initialize_result == S_FALSE;
    if (FAILED(initialize_result) &&
        initialize_result != RPC_E_CHANGED_MODE) {
        return 0;
    }
    result = CoCreateInstance(
        &CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory,
        (void **)&factory);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICImagingFactory_CreateDecoderFromFilename(
        factory,
        wide_path,
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICImagingFactory_CreateBitmapScaler(factory, &scaler);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICBitmapScaler_Initialize(
        scaler,
        (IWICBitmapSource *)frame,
        (UINT)width,
        (UINT)height,
        WICBitmapInterpolationModeFant);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICImagingFactory_CreateFormatConverter(
        factory, &converter);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICFormatConverter_Initialize(
        converter,
        (IWICBitmapSource *)scaler,
        &GUID_WICPixelFormat32bppBGRA,
        WICBitmapDitherTypeNone,
        NULL,
        0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(result)) {
        goto cleanup;
    }
    result = IWICFormatConverter_CopyPixels(
        converter,
        NULL,
        (UINT)(stride_pixels * 4),
        (UINT)(stride_pixels * height * 4),
        (BYTE *)pixels);
    success = SUCCEEDED(result);

cleanup:
    if (converter != NULL) {
        IWICFormatConverter_Release(converter);
    }
    if (scaler != NULL) {
        IWICBitmapScaler_Release(scaler);
    }
    if (frame != NULL) {
        IWICBitmapFrameDecode_Release(frame);
    }
    if (decoder != NULL) {
        IWICBitmapDecoder_Release(decoder);
    }
    if (factory != NULL) {
        IWICImagingFactory_Release(factory);
    }
    if (initialized_com) {
        CoUninitialize();
    }
    return success;
}
