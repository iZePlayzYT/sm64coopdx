//
// RT64
//

#ifndef RT64_H_INCLUDED
#define RT64_H_INCLUDED

#include <Windows.h>
#include <stdio.h>
#include <string.h>

// Material constants.
#define RT64_MATERIAL_FILTER_POINT				0
#define RT64_MATERIAL_FILTER_LINEAR				1
#define RT64_MATERIAL_ADDR_WRAP					0
#define RT64_MATERIAL_ADDR_MIRROR				1
#define RT64_MATERIAL_ADDR_CLAMP				2
#define RT64_MATERIAL_CC_SHADER_0				0
#define RT64_MATERIAL_CC_SHADER_INPUT_1			1
#define RT64_MATERIAL_CC_SHADER_INPUT_2			2
#define RT64_MATERIAL_CC_SHADER_INPUT_3			3
#define RT64_MATERIAL_CC_SHADER_INPUT_4			4
#define RT64_MATERIAL_CC_SHADER_TEXEL0			5
#define RT64_MATERIAL_CC_SHADER_TEXEL0A			6
#define RT64_MATERIAL_CC_SHADER_TEXEL1			7

// Color combiner shaders.
#define RT64_CC_SHADER_0						0
#define RT64_CC_SHADER_INPUT_1					1
#define RT64_CC_SHADER_INPUT_2					2
#define RT64_CC_SHADER_INPUT_3					3
#define RT64_CC_SHADER_INPUT_4					4
#define RT64_CC_SHADER_INPUT_5					5
#define RT64_CC_SHADER_INPUT_6					6
#define RT64_CC_SHADER_INPUT_7					7
#define RT64_CC_SHADER_INPUT_8					8
#define RT64_CC_SHADER_TEXEL0					9
#define RT64_CC_SHADER_TEXEL0A					10
#define RT64_CC_SHADER_TEXEL1					11
#define RT64_CC_SHADER_TEXEL1A					12
#define RT64_CC_SHADER_1						13
#define RT64_CC_SHADER_COMBINED					14
#define RT64_CC_SHADER_COMBINEDA				15
#define RT64_CC_SHADER_NOISE					16

// Material attributes.
#define RT64_ATTRIBUTE_NONE							0x0000
#define RT64_ATTRIBUTE_IGNORE_NORMAL_FACTOR			0x0001
#define RT64_ATTRIBUTE_UV_DETAIL_SCALE				0x0002
#define RT64_ATTRIBUTE_REFLECTION_FACTOR			0x0004
#define RT64_ATTRIBUTE_REFLECTION_FRESNEL_FACTOR	0x0008
#define RT64_ATTRIBUTE_REFLECTION_SHINE_FACTOR		0x0010
#define RT64_ATTRIBUTE_REFRACTION_FACTOR			0x0020
#define RT64_ATTRIBUTE_SPECULAR_COLOR				0x0040
#define RT64_ATTRIBUTE_SPECULAR_SHINYNESS			0x0080
#define RT64_ATTRIBUTE_SOLID_ALPHA_MULTIPLIER		0x0100
#define RT64_ATTRIBUTE_SHADOW_ALPHA_MULTIPLIER		0x0200
#define RT64_ATTRIBUTE_DEPTH_BIAS					0x0400
#define RT64_ATTRIBUTE_SHADOW_RAY_BIAS				0x0800
#define RT64_ATTRIBUTE_SELF_LIGHT_COLOR				0x1000
#define RT64_ATTRIBUTE_LIGHT_GROUP_MASK_BITS		0x2000
#define RT64_ATTRIBUTE_DIFFUSE_COLOR_MIX			0x4000
#define RT64_ATTRIBUTE_SPECULAR_TINT				0x8000
#define RT64_ATTRIBUTE_SHADING_MODEL				0x10000
#define RT64_ATTRIBUTE_DIFFUSE_INTENSITY			0x20000
#define RT64_ATTRIBUTE_SPECULAR_FACTOR				0x40000
#define RT64_ATTRIBUTE_SPECULAR_ECCENTRICITY		0x80000
#define RT64_ATTRIBUTE_BUMP_STRENGTH				0x100000
#define RT64_ATTRIBUTE_NORMAL_STRENGTH				0x200000
#define RT64_ATTRIBUTE_REFLECTION_COLOR				0x400000
#define RT64_ATTRIBUTE_SHADOW_ENABLED				0x800000
#define RT64_ATTRIBUTE_SPECULAR_INTENSITY			0x1000000
#define RT64_ATTRIBUTE_SELF_LIGHT_INTENSITY			0x2000000
#define RT64_ATTRIBUTE_SHADOW_CENTER				0x4000000

// Shading models.
#define RT64_SHADING_MODEL_LAMBERT					0x0
#define RT64_SHADING_MODEL_PHONG					0x1
#define RT64_SHADING_MODEL_BLINN					0x2

// Mesh flags.
#define RT64_MESH_RAYTRACE_ENABLED				0x1
#define RT64_MESH_RAYTRACE_UPDATABLE			0x2
#define RT64_MESH_RAYTRACE_FAST_TRACE			0x4
#define RT64_MESH_RAYTRACE_COMPACT				0x8

// Shader flags.
#define RT64_SHADER_FILTER_POINT				0x0
#define RT64_SHADER_FILTER_LINEAR				0x1
#define RT64_SHADER_ADDRESSING_WRAP				0x0
#define RT64_SHADER_ADDRESSING_MIRROR			0x1
#define RT64_SHADER_ADDRESSING_CLAMP			0x2
#define RT64_SHADER_RASTER_ENABLED				0x1
#define RT64_SHADER_RAYTRACE_ENABLED			0x2
#define RT64_SHADER_NORMAL_MAP_ENABLED			0x4
#define RT64_SHADER_SPECULAR_MAP_ENABLED		0x8
#define RT64_SHADER_BUMP_MAP_ENABLED			0x10

// Instance flags.
#define RT64_INSTANCE_RASTER_BACKGROUND			0x1
#define RT64_INSTANCE_DISABLE_BACKFACE_CULLING	0x2

// Light flags.
#define RT64_LIGHT_GROUP_MASK_ALL				0xFFFFFFFF
#define RT64_LIGHT_GROUP_DEFAULT				0x1
#define RT64_LIGHT_MAX_SAMPLES					128

// Light types.
#define RT64_LIGHT_TYPE_AREA					0x0
#define RT64_LIGHT_TYPE_POINT					0x1

// Light shapes.
#define RT64_LIGHT_SHAPE_CIRCLE					0x0
#define RT64_LIGHT_SHAPE_SQUARE					0x1

// View attributes.
#define RT64_INSPECTOR_PANEL_MATERIAL			0x0
#define RT64_INSPECTOR_PANEL_GEO_LAYOUT			0x1
#define RT64_INSPECTOR_PANEL_COUNT				0x2

#define RT64_UPSCALER_OFF						0x0
#define RT64_UPSCALER_AUTO						0x1
#define RT64_UPSCALER_DLSS						0x2
#define RT64_UPSCALER_FSR						0x3
#define RT64_UPSCALER_XESS						0x4
#define RT64_UPSCALER_MODE_AUTO					0x0
#define RT64_UPSCALER_MODE_ULTRA_PERFORMANCE	0x1
#define RT64_UPSCALER_MODE_PERFORMANCE			0x2
#define RT64_UPSCALER_MODE_BALANCED				0x3
#define RT64_UPSCALER_MODE_QUALITY				0x4
#define RT64_UPSCALER_MODE_ULTRA_QUALITY		0x5
#define RT64_UPSCALER_MODE_NATIVE				0x6

// Texture formats.
#define RT64_TEXTURE_FORMAT_RGBA8				0x1
#define RT64_TEXTURE_FORMAT_DDS					0x2

// Forward declaration of types.
typedef struct RT64_DEVICE RT64_DEVICE;
typedef struct RT64_VIEW RT64_VIEW;
typedef struct RT64_SCENE RT64_SCENE;
typedef struct RT64_INSTANCE RT64_INSTANCE;
typedef struct RT64_MESH RT64_MESH;
typedef struct RT64_TEXTURE RT64_TEXTURE;
typedef struct RT64_SHADER RT64_SHADER;
typedef struct RT64_INSPECTOR RT64_INSPECTOR;

typedef float Vec2f[2];
typedef float Vec3f[3];
typedef float Vec4f[4];
typedef float Mat4[4][4];
typedef int Recti[4]; // X, Y, W, H

typedef struct {
	int diffuseTexIndex;
	int normalTexIndex;
	int specularTexIndex;
	int bumpTexIndex;
	float bumpStrength;
	float normalStrength;
	float ignoreNormalFactor;
	float uvDetailScale;
	float reflectionFactor;
	float reflectionFresnelFactor;
	float reflectionShineFactor;
	float refractionFactor;
	Vec3f specularColor;
	float specularShinyness;
	float solidAlphaMultiplier;
	float shadowAlphaMultiplier;
	float depthBias;
	float shadowRayBias;
	Vec3f selfLightColor;
	unsigned int lightGroupMaskBits;
	Vec3f fogColor;
	Vec4f diffuseColorMix;
	float fogMul;
	float fogOffset;
	unsigned int fogEnabled;
	float lockMask;
	int diffuse2TexIndex;

	// Flag containing all attributes that are actually used by this material.
	int enabledAttributes;

	unsigned int ccRgb1;
	unsigned int ccAlpha1;
	unsigned int ccRgb2;
	unsigned int ccAlpha2;
	unsigned int ccFlags;
	unsigned int ccSamplerIndex;
	unsigned int specularTint;
	unsigned int shadingModel;
	float diffuseIntensity;
	float specularFactor;
	float specularEccentricity;
	unsigned int textureGenEnabled;
	Vec4f textureGenU;
	Vec4f textureGenV;
	Vec3f reflectionColor;
	float specularIntensity;
	float selfLightIntensity;
	unsigned int shadowEnabled;
	unsigned int shadowCenter;
	unsigned int depthWrite;
	unsigned int depthTest;
	unsigned int zmodeXlu;
} RT64_MATERIAL;

// Light
typedef struct {
	Vec3f position;
	Vec3f diffuseColor;
	float attenuationRadius;
	float pointRadius;
	Vec3f specularColor;
	float shadowOffset;
	float attenuationExponent;
	float flickerIntensity;
	unsigned int groupBits;
	unsigned int lightType;
	float pitch, yaw, roll;
	float scaleX, scaleY;
	unsigned int lightShape;
	unsigned int apertureEnabled;
	float aperturePitch, apertureYaw;
	unsigned int volumetricEnabled;
	float volumetricIntensity;
	float intensity;
} RT64_LIGHT;

typedef struct {
	Vec3f ambientBaseColor;
	Vec3f ambientNoGIColor;
	Vec3f eyeLightDiffuseColor;
	Vec3f eyeLightSpecularColor;
	Vec3f skyDiffuseMultiplier;
	Vec3f skyHSLModifier;
	float skyYawOffset;
	float giDiffuseStrength;
	float giSkyStrength;
} RT64_SCENE_DESC;

typedef struct {
	float resolutionScale;
	float motionBlurStrength;
	unsigned int diSamples;
	unsigned int giSamples;
	unsigned int maxLights;
	unsigned int maxReflections;
	unsigned char upscaler;
	unsigned char upscalerMode;
	float upscalerSharpness;
	bool denoiserEnabled;
	float aspectRatio;
} RT64_VIEW_DESC;

#define RT64_MAX_SHADER_UNIFORM_BLOCKS 8

typedef struct {
	unsigned int shaderRegister;
	unsigned int size;
	const void *data;
} RT64_SHADER_UNIFORM_BLOCK;

typedef struct {
	RT64_MESH *mesh;
	Mat4 transform;
	Mat4 previousTransform;
	RT64_TEXTURE *diffuseTexture;
	RT64_TEXTURE *normalTexture;
	RT64_TEXTURE *specularTexture;
	RT64_TEXTURE *diffuse2Texture;
	RT64_TEXTURE *bumpTexture;
	RT64_SHADER *shader;
	RT64_MATERIAL material;
	Recti scissorRect;
	Recti viewportRect;
	unsigned int flags;
	const RT64_SHADER_UNIFORM_BLOCK *shaderUniformBlocks;
	unsigned int shaderUniformBlockCount;
} RT64_INSTANCE_DESC;

typedef struct {
	void *bytes;
	int byteCount;
	int format;
	int width;
	int height;
	int rowPitch;
} RT64_TEXTURE_DESC;

typedef struct {
	unsigned char rgb1[4];
	unsigned char alpha1[4];
	unsigned char rgb2[4];
	unsigned char alpha2[4];
	unsigned char use2Cycle;
	unsigned char optAlpha;
	unsigned char optTextureEdge;
	unsigned char optNoise;
} RT64_COMBINER_DESC;

typedef struct {
	unsigned int location;
	unsigned int size;
	const char *name;
} RT64_SHADER_INPUT;

inline void RT64_ApplyMaterialAttributes(RT64_MATERIAL *dst, RT64_MATERIAL *src) {
	if (src->enabledAttributes & RT64_ATTRIBUTE_IGNORE_NORMAL_FACTOR) {
		dst->ignoreNormalFactor = src->ignoreNormalFactor;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_UV_DETAIL_SCALE) {
		dst->uvDetailScale = src->uvDetailScale;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_REFLECTION_FACTOR) {
		dst->reflectionFactor = src->reflectionFactor;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_REFLECTION_FRESNEL_FACTOR) {
		dst->reflectionFresnelFactor = src->reflectionFresnelFactor;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_REFLECTION_SHINE_FACTOR) {
		dst->reflectionShineFactor = src->reflectionShineFactor;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_REFRACTION_FACTOR) {
		dst->refractionFactor = src->refractionFactor;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SPECULAR_COLOR) {
		memcpy(dst->specularColor, src->specularColor, sizeof(Vec3f));
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SPECULAR_SHINYNESS) {
		dst->specularShinyness = src->specularShinyness;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SOLID_ALPHA_MULTIPLIER) {
		dst->solidAlphaMultiplier = src->solidAlphaMultiplier;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SHADOW_ALPHA_MULTIPLIER) {
		dst->shadowAlphaMultiplier = src->shadowAlphaMultiplier;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_DEPTH_BIAS) {
		dst->depthBias = src->depthBias;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SHADOW_RAY_BIAS) {
		dst->shadowRayBias = src->shadowRayBias;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SELF_LIGHT_COLOR) {
		memcpy(dst->selfLightColor, src->selfLightColor, sizeof(Vec3f));
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_LIGHT_GROUP_MASK_BITS) {
		dst->lightGroupMaskBits = src->lightGroupMaskBits;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_DIFFUSE_COLOR_MIX) {
		memcpy(dst->diffuseColorMix, src->diffuseColorMix, sizeof(Vec4f));
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SPECULAR_TINT) {
		dst->specularTint = src->specularTint;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SHADING_MODEL) {
		dst->shadingModel = src->shadingModel;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_DIFFUSE_INTENSITY) {
		dst->diffuseIntensity = src->diffuseIntensity;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SPECULAR_FACTOR) {
		dst->specularFactor = src->specularFactor;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SPECULAR_ECCENTRICITY) {
		dst->specularEccentricity = src->specularEccentricity;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_BUMP_STRENGTH) {
		dst->bumpStrength = src->bumpStrength;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_NORMAL_STRENGTH) {
		dst->normalStrength = src->normalStrength;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_REFLECTION_COLOR) {
		memcpy(dst->reflectionColor, src->reflectionColor, sizeof(Vec3f));
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SPECULAR_INTENSITY) {
		dst->specularIntensity = src->specularIntensity;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SELF_LIGHT_INTENSITY) {
		dst->selfLightIntensity = src->selfLightIntensity;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SHADOW_ENABLED) {
		dst->shadowEnabled = src->shadowEnabled;
	}

	if (src->enabledAttributes & RT64_ATTRIBUTE_SHADOW_CENTER) {
		dst->shadowCenter = src->shadowCenter;
	}
}

// Internal function pointer types.
typedef const char *(*GetLastErrorPtr)();
typedef RT64_DEVICE* (*CreateDevicePtr)(void *hwnd);
typedef void (*DestroyDevicePtr)(RT64_DEVICE* device);
typedef void (*DrawDevicePtr)(RT64_DEVICE *device, int vsyncInterval, float deltaTimeMs);
typedef void (*BeginMeshBatchPtr)(RT64_DEVICE *device);
typedef void (*EndMeshBatchPtr)(RT64_DEVICE *device);
typedef RT64_VIEW* (*CreateViewPtr)(RT64_SCENE* scenePtr);
typedef void (*SetViewPerspectivePtr)(RT64_VIEW *viewPtr, Mat4 viewMatrix, float fovRadians, float nearDist, float farDist, bool canReproject);
typedef void (*SetViewDescriptionPtr)(RT64_VIEW *viewPtr, RT64_VIEW_DESC viewDesc);
typedef void (*GetViewDescriptionPtr)(RT64_VIEW *viewPtr, RT64_VIEW_DESC *outViewDesc);
typedef void (*SetViewSkyPlanePtr)(RT64_VIEW *viewPtr, RT64_TEXTURE *texturePtr);
typedef RT64_INSTANCE* (*GetViewRaytracedInstanceAtPtr)(RT64_VIEW *viewPtr, int x, int y);
typedef bool (*GetViewUpscalerSupportPtr)(RT64_VIEW *viewPtr, char upscaler);
typedef void (*DestroyViewPtr)(RT64_VIEW* viewPtr);
typedef RT64_SCENE* (*CreateScenePtr)(RT64_DEVICE* devicePtr);
typedef void (*SetSceneDescriptionPtr)(RT64_SCENE* scenePtr, RT64_SCENE_DESC sceneDesc);
typedef void (*SetSceneLightsPtr)(RT64_SCENE* scenePtr, RT64_LIGHT* lightArray, int lightCount);
typedef void (*DestroyScenePtr)(RT64_SCENE* scenePtr);
typedef RT64_MESH* (*CreateMeshPtr)(RT64_DEVICE* devicePtr, int flags);
typedef void (*SetMeshPtr)(RT64_MESH* meshPtr, void* vertexArray, int vertexCount, int vertexStride, unsigned int* indexArray, int indexCount);
typedef void (*SetMeshVertexDataPtr)(RT64_MESH* meshPtr, void* vertexArray, int vertexCount, int vertexStride, unsigned int* indexArray, int indexCount);
typedef void *(*BeginMeshVertexUpdatePtr)(RT64_MESH* meshPtr, int vertexCount, int vertexStride, unsigned int* indexArray, int indexCount);
typedef void (*EndMeshVertexUpdatePtr)(RT64_MESH* meshPtr, int vertexCount, int vertexStride, int updateAccelerationStructure);
typedef void (*DestroyMeshPtr)(RT64_MESH* meshPtr);
typedef RT64_SHADER *(*CreateShaderPtr)(RT64_DEVICE *devicePtr, RT64_COMBINER_DESC cc, unsigned int filter, unsigned int hAddr, unsigned int vAddr, int flags);
typedef RT64_SHADER *(*CreateShaderFromSourcePtr)(RT64_DEVICE *devicePtr, RT64_COMBINER_DESC cc, const char *customVertexHLSL, const char *customFragmentHLSL, const RT64_SHADER_INPUT *vertexInputs, unsigned int vertexInputCount, const char *fragmentOutputName, unsigned int filter, unsigned int hAddr, unsigned int vAddr, int flags);
typedef void (*SetPostProcessShaderPtr)(RT64_DEVICE *devicePtr, const char *fragmentHLSL, const RT64_SHADER_INPUT *fragmentInputs, unsigned int fragmentInputCount, const char *fragmentOutputName, int targetWidth, int targetHeight);
typedef void (*SetPostProcessUniformsPtr)(RT64_DEVICE *devicePtr, const RT64_SHADER_UNIFORM_BLOCK *blocks, unsigned int blockCount);
typedef void (*DestroyShaderPtr)(RT64_SHADER *shaderPtr);
typedef RT64_INSTANCE* (*CreateInstancePtr)(RT64_SCENE* scenePtr);
typedef void (*SetInstanceDescriptionPtr)(RT64_INSTANCE* instancePtr, const RT64_INSTANCE_DESC *instanceDesc);
typedef void (*DestroyInstancePtr)(RT64_INSTANCE* instancePtr);
typedef RT64_TEXTURE* (*CreateTexturePtr)(RT64_DEVICE* devicePtr, RT64_TEXTURE_DESC textureDesc);
typedef void (*DestroyTexturePtr)(RT64_TEXTURE* texture);
typedef RT64_INSPECTOR* (*CreateInspectorPtr)(RT64_DEVICE* devicePtr);
typedef bool (*HandleMessageInspectorPtr)(RT64_INSPECTOR* inspectorPtr, UINT msg, WPARAM wParam, LPARAM lParam);
typedef void (*SetSceneInspectorPtr)(RT64_INSPECTOR* inspectorPtr, RT64_SCENE_DESC* sceneDesc);
typedef void (*SetMaterialInspectorPtr)(RT64_INSPECTOR* inspectorPtr, RT64_MATERIAL* material, const char *materialName);
typedef void (*SetLightsInspectorPtr)(RT64_INSPECTOR* inspectorPtr, RT64_LIGHT* lights, int *lightCount, int maxLightCount);
typedef void (*SetGeoLayoutInspectorPtr)(RT64_INSPECTOR* inspectorPtr, RT64_MATERIAL* material, RT64_LIGHT* light, bool* lightEnabled, const Vec3f* origins, int originCount, const char *geoLayoutName);
typedef void (*SetInspectorMaterialDefaultsPtr)(RT64_INSPECTOR *inspectorPtr, const RT64_MATERIAL *defaults);
typedef void (*SetInspectorMapNamesPtr)(RT64_INSPECTOR *inspectorPtr, int panel, const char *bumpMapName, const char *normalMapName, const char *specularMapName);
typedef bool (*GetInspectorMapNamesPtr)(RT64_INSPECTOR *inspectorPtr, int panel, char *outBumpMapName, char *outNormalMapName, char *outSpecularMapName, int bufferSize);
typedef bool (*InspectorWantsMousePtr)(RT64_INSPECTOR* inspectorPtr);
typedef void (*PrintClearInspectorPtr)(RT64_INSPECTOR *inspectorPtr);
typedef void (*PrintMessageInspectorPtr)(RT64_INSPECTOR* inspectorPtr, const char* message);
typedef void (*DestroyInspectorPtr)(RT64_INSPECTOR* inspectorPtr);

// Stores all the function pointers used in the RT64 library.
typedef struct {
	HMODULE handle;
	GetLastErrorPtr GetLastError;
	CreateDevicePtr CreateDevice;
	DestroyDevicePtr DestroyDevice;
#ifndef RT64_MINIMAL
	DrawDevicePtr DrawDevice;
	BeginMeshBatchPtr BeginMeshBatch;
	EndMeshBatchPtr EndMeshBatch;
	CreateViewPtr CreateView;
	SetViewPerspectivePtr SetViewPerspective;
	SetViewDescriptionPtr SetViewDescription;
	GetViewDescriptionPtr GetViewDescription;
	SetViewSkyPlanePtr SetViewSkyPlane;
	GetViewRaytracedInstanceAtPtr GetViewRaytracedInstanceAt;
	GetViewUpscalerSupportPtr GetViewUpscalerSupport;
	DestroyViewPtr DestroyView;
	CreateScenePtr CreateScene;
	SetSceneDescriptionPtr SetSceneDescription;
	SetSceneLightsPtr SetSceneLights;
	DestroyScenePtr DestroyScene;
	CreateMeshPtr CreateMesh;
	SetMeshPtr SetMesh;
	SetMeshVertexDataPtr SetMeshVertexData;
	BeginMeshVertexUpdatePtr BeginMeshVertexUpdate;
	EndMeshVertexUpdatePtr EndMeshVertexUpdate;
	DestroyMeshPtr DestroyMesh;
	CreateShaderPtr CreateShader;
	CreateShaderFromSourcePtr CreateShaderFromSource;
	SetPostProcessShaderPtr SetPostProcessShader;
	SetPostProcessUniformsPtr SetPostProcessUniforms;
	DestroyShaderPtr DestroyShader;
	CreateInstancePtr CreateInstance;
	SetInstanceDescriptionPtr SetInstanceDescription;
	DestroyInstancePtr DestroyInstance;
	CreateTexturePtr CreateTexture;
	DestroyTexturePtr DestroyTexture;
	CreateInspectorPtr CreateInspector;
	HandleMessageInspectorPtr HandleMessageInspector;
	PrintClearInspectorPtr PrintClearInspector;
	PrintMessageInspectorPtr PrintMessageInspector;
	SetSceneInspectorPtr SetSceneInspector;
	SetMaterialInspectorPtr SetMaterialInspector;
	SetLightsInspectorPtr SetLightsInspector;
	SetGeoLayoutInspectorPtr SetGeoLayoutInspector;
	SetInspectorMaterialDefaultsPtr SetInspectorMaterialDefaults;
	SetInspectorMapNamesPtr SetInspectorMapNames;
	GetInspectorMapNamesPtr GetInspectorMapNames;
	InspectorWantsMousePtr InspectorWantsMouse;
	DestroyInspectorPtr DestroyInspector;
#endif
} RT64_LIBRARY;


// Define RT64_DEBUG for loading the debug DLL.
inline RT64_LIBRARY RT64_LoadLibrary() {
	RT64_LIBRARY lib;

#if defined(RT64_MINIMAL)
	lib.handle = LoadLibrary(TEXT("rt64libm.dll"));
#elif defined(RT64_DEBUG)
	lib.handle = LoadLibrary(TEXT("rt64libd.dll"));
#else
	lib.handle = LoadLibrary(TEXT("rt64lib.dll"));
#endif

	if (lib.handle != 0) {
		lib.GetLastError = (GetLastErrorPtr)(GetProcAddress(lib.handle, "RT64_GetLastError"));
		lib.CreateDevice = (CreateDevicePtr)(GetProcAddress(lib.handle, "RT64_CreateDevice"));
		lib.DestroyDevice = (DestroyDevicePtr)(GetProcAddress(lib.handle, "RT64_DestroyDevice"));

#ifndef RT64_MINIMAL
		lib.DrawDevice = (DrawDevicePtr)(GetProcAddress(lib.handle, "RT64_DrawDevice"));
		lib.BeginMeshBatch = (BeginMeshBatchPtr)(GetProcAddress(lib.handle, "RT64_BeginMeshBatch"));
		lib.EndMeshBatch = (EndMeshBatchPtr)(GetProcAddress(lib.handle, "RT64_EndMeshBatch"));
		lib.CreateView = (CreateViewPtr)(GetProcAddress(lib.handle, "RT64_CreateView"));
		lib.SetViewPerspective = (SetViewPerspectivePtr)(GetProcAddress(lib.handle, "RT64_SetViewPerspective"));
		lib.SetViewDescription = (SetViewDescriptionPtr)(GetProcAddress(lib.handle, "RT64_SetViewDescription"));
		lib.GetViewDescription = (GetViewDescriptionPtr)(GetProcAddress(lib.handle, "RT64_GetViewDescription"));
		lib.SetViewSkyPlane = (SetViewSkyPlanePtr)(GetProcAddress(lib.handle, "RT64_SetViewSkyPlane"));
		lib.GetViewRaytracedInstanceAt = (GetViewRaytracedInstanceAtPtr)(GetProcAddress(lib.handle, "RT64_GetViewRaytracedInstanceAt"));
		lib.GetViewUpscalerSupport = (GetViewUpscalerSupportPtr)(GetProcAddress(lib.handle, "RT64_GetViewUpscalerSupport"));
		lib.DestroyView = (DestroyViewPtr)(GetProcAddress(lib.handle, "RT64_DestroyView"));
		lib.CreateScene = (CreateScenePtr)(GetProcAddress(lib.handle, "RT64_CreateScene"));
		lib.SetSceneDescription = (SetSceneDescriptionPtr)(GetProcAddress(lib.handle, "RT64_SetSceneDescription"));
		lib.SetSceneLights = (SetSceneLightsPtr)(GetProcAddress(lib.handle, "RT64_SetSceneLights"));
		lib.DestroyScene = (DestroyScenePtr)(GetProcAddress(lib.handle, "RT64_DestroyScene"));
		lib.CreateMesh = (CreateMeshPtr)(GetProcAddress(lib.handle, "RT64_CreateMesh"));
		lib.SetMesh = (SetMeshPtr)(GetProcAddress(lib.handle, "RT64_SetMesh"));
		lib.SetMeshVertexData = (SetMeshVertexDataPtr)(GetProcAddress(lib.handle, "RT64_SetMeshVertexData"));
		lib.BeginMeshVertexUpdate = (BeginMeshVertexUpdatePtr)(GetProcAddress(lib.handle, "RT64_BeginMeshVertexUpdate"));
		lib.EndMeshVertexUpdate = (EndMeshVertexUpdatePtr)(GetProcAddress(lib.handle, "RT64_EndMeshVertexUpdate"));
		lib.DestroyMesh = (DestroyMeshPtr)(GetProcAddress(lib.handle, "RT64_DestroyMesh"));
		lib.CreateShader = (CreateShaderPtr)(GetProcAddress(lib.handle, "RT64_CreateShader"));
		lib.CreateShaderFromSource = (CreateShaderFromSourcePtr)(GetProcAddress(lib.handle, "RT64_CreateShaderFromSource"));
		lib.SetPostProcessShader = (SetPostProcessShaderPtr)(GetProcAddress(lib.handle, "RT64_SetPostProcessShader"));
		lib.SetPostProcessUniforms = (SetPostProcessUniformsPtr)(GetProcAddress(lib.handle, "RT64_SetPostProcessUniforms"));
		lib.DestroyShader = (DestroyShaderPtr)(GetProcAddress(lib.handle, "RT64_DestroyShader"));
		lib.CreateInstance = (CreateInstancePtr)(GetProcAddress(lib.handle, "RT64_CreateInstance"));
		lib.SetInstanceDescription = (SetInstanceDescriptionPtr)(GetProcAddress(lib.handle, "RT64_SetInstanceDescription"));
		lib.DestroyInstance = (DestroyInstancePtr)(GetProcAddress(lib.handle, "RT64_DestroyInstance"));
		lib.CreateTexture = (CreateTexturePtr)(GetProcAddress(lib.handle, "RT64_CreateTexture"));
		lib.DestroyTexture = (DestroyTexturePtr)(GetProcAddress(lib.handle, "RT64_DestroyTexture"));
		lib.CreateInspector = (CreateInspectorPtr)(GetProcAddress(lib.handle, "RT64_CreateInspector"));
		lib.HandleMessageInspector = (HandleMessageInspectorPtr)(GetProcAddress(lib.handle, "RT64_HandleMessageInspector"));
		lib.SetSceneInspector = (SetSceneInspectorPtr)(GetProcAddress(lib.handle, "RT64_SetSceneInspector"));
		lib.SetMaterialInspector = (SetMaterialInspectorPtr)(GetProcAddress(lib.handle, "RT64_SetMaterialInspector"));
		lib.SetLightsInspector = (SetLightsInspectorPtr)(GetProcAddress(lib.handle, "RT64_SetLightsInspector"));
		lib.SetGeoLayoutInspector = (SetGeoLayoutInspectorPtr)(GetProcAddress(lib.handle, "RT64_SetGeoLayoutInspector"));
		lib.SetInspectorMaterialDefaults = (SetInspectorMaterialDefaultsPtr)(GetProcAddress(lib.handle, "RT64_SetInspectorMaterialDefaults"));
		lib.SetInspectorMapNames = (SetInspectorMapNamesPtr)(GetProcAddress(lib.handle, "RT64_SetInspectorMapNames"));
		lib.GetInspectorMapNames = (GetInspectorMapNamesPtr)(GetProcAddress(lib.handle, "RT64_GetInspectorMapNames"));
		lib.InspectorWantsMouse = (InspectorWantsMousePtr)(GetProcAddress(lib.handle, "RT64_InspectorWantsMouse"));
		lib.PrintClearInspector = (PrintClearInspectorPtr)(GetProcAddress(lib.handle, "RT64_PrintClearInspector"));
		lib.PrintMessageInspector = (PrintMessageInspectorPtr)(GetProcAddress(lib.handle, "RT64_PrintMessageInspector"));
		lib.DestroyInspector = (DestroyInspectorPtr)(GetProcAddress(lib.handle, "RT64_DestroyInspector"));
#endif
	}
	else {
		char errorMessage[256];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, sizeof(errorMessage), NULL);
		fprintf(stderr, "Error when loading library: %s\n", errorMessage);
	}

	return lib;
}

inline void RT64_UnloadLibrary(RT64_LIBRARY lib) {
	FreeLibrary(lib.handle);
}

#endif