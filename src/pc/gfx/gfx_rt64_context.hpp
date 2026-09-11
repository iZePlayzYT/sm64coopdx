#pragma once

#include "rt64/rt64.h"

#include "gfx.h"
#include "gfx_shader.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef _LANGUAGE_C
# define _LANGUAGE_C
#endif
#include <PR/gbi.h>

#include <Windows.h>

extern "C" {
#include "game/area.h"
}

#define RT64_CACHED_MESH_REQUIRED_FRAMES 5
#define RT64_CACHED_MESH_MAX_PER_FRAME  1
#define RT64_CACHED_MESH_EVICT_FRAMES   300 // Idle frames before geometry is released
#define RT64_MAX_LIGHTS                 512
#define RT64_MAX_LEVEL_LIGHTS           128
#define RT64_MAX_LEVELS                 0x8000
#define RT64_MAX_FRAMES_IN_FLIGHT       3
#define RT64_MAX_RENDER_FRAMES          (RT64_MAX_FRAMES_IN_FLIGHT + 3)

static const unsigned int sMinResolutionScale = 1;
static const unsigned int sMaxResolutionScale = 100;

struct ShaderProgramRT64 {
    RT64_COMBINER_DESC cc;
    u64 hash;
    u8 numInputs;
    bool usedTextures[2];
    bool usedFog;
    std::unordered_map<u16, RT64_SHADER *> shaderVariantMap;
    struct Shader *vertexShader = nullptr;
    struct Shader *fragmentShader = nullptr;
    bool hasCustomShader = false;
    std::string customVertexHLSL;
    std::string customFragmentHLSL;
    std::vector<RT64_SHADER_INPUT> customVertexInputs;
    std::atomic<bool> customShaderFailed{false};
};

static inline bool gfx_rt64_program_uses_custom_shader(const ShaderProgramRT64 *prg) {
    return prg->hasCustomShader && !prg->customShaderFailed.load(std::memory_order_relaxed);
}

struct RecordedTexture {
    bool linearFilter;
    u32 cms;
    u32 cmt;
    u64 hash;
    std::string pendingName;
};

struct RecordedMod {
    RT64_MATERIAL *materialMod = nullptr;
    RT64_LIGHT *lightMod = nullptr;
    u64 bumpMapHash = 0;
    u64 normalMapHash = 0;
    u64 specularMapHash = 0;

    ~RecordedMod() {
        delete materialMod;
        delete lightMod;
    }
};

struct AreaLighting {
    RT64_SCENE_DESC sceneDesc;
    RT64_LIGHT lights[RT64_MAX_LEVEL_LIGHTS];
    int lightCount = 0;
};

struct GameInstance {
    RT64_INSTANCE_DESC desc;
    std::vector<RT64_SHADER_UNIFORM_BLOCK> uniformBlocks;
    std::vector<u8> uniformBlockData;

    struct {
        u32 diffuse = 0;
        u32 bump = 0;
        u32 normal = 0;
        u32 specular = 0;
        u32 diffuse2 = 0; // second N64 tile texture (TEXEL1), e.g. Mario's cap emblem
    } textures;

    u64 textureHash = 0;

    void *geoLayout = nullptr;

    struct {
        ShaderProgramRT64 *program = nullptr;
        bool raytrace;
        int filter;
        int hAddr;
        int vAddr;
        bool normalMap;
        bool specularMap;
        bool bumpMap;
    } shader;

    RT64_LIGHT light;
};

struct GameMesh {
    float *vertexBuffer = nullptr;
    u64 rawVertexBufferHash = 0;
    u64 vertexBufferHash = 0;
    u64 positionHash = 0; // Hash of vertex positions only.
    u32 vertexCount = 0;
    u32 vertexStride = 0;
    u32 indexCount = 0;
    bool raytrace = false;
};

struct GameDisplayList {
    std::vector<GameInstance> instances;
    std::vector<GameMesh> meshes;
    Mat4 transform;
    RT64_LIGHT light;
    int drawCount = 0;
};

struct GameFrame {
    Mat4 viewMatrix;
    Mat4 invViewMatrix;
    float fovRadians;
    float nearDist;
    float farDist;
    bool canReprojectView = true;
    std::unordered_map<u32, GameDisplayList> displayLists;
    RT64_SCENE_DESC sceneDesc;
    RT64_LIGHT areaLights[RT64_MAX_LEVEL_LIGHTS];
    unsigned int areaLightCount = 0;
    u32 skyTextureKey = 0;
};

struct GPUInstance {
    RT64_INSTANCE *instance = nullptr;
    Mat4 transform;
};

struct GPUMesh {
    RT64_MESH *mesh = nullptr;
    u64 vertexBufferHash = 0;
    u32 vertexCount = 0;
    u32 vertexStride = 0;
    u32 indexCount = 0;
    u64 ownerKey = 0; // The key of the display list that owns this mesh
    u64 positionHash = 0;
    bool raytrace = false;
    bool inUse = false;
    int staticFrames = 0;
    int unusedFrames = 0; // Ticks since this mesh was last submitted
};

struct GPUDisplayList {
    std::vector<GPUInstance> instances;
    int drawCount = 0;
    int idleFrames = 0;
};

struct GPUTexture {
    RT64_TEXTURE *texture = nullptr;
    u64 hash = 0;
};

struct UploadTexture {
    RT64_TEXTURE_DESC desc;
    u32 key;
    u64 hash;
    u64 contentHash;
};

struct InspectorMessage {
    unsigned int message;
    uintptr_t wParam;
    intptr_t lParam;
};

//  Convention of bits for different lights.
//      1   - Directional Tier A
//      2   - Directional Tier B
//      4   - Stage Tier A
//      8   - Stage Tier B
//      16  - Objects Tier A
//      32  - Objects Tier B
//      64  - Particles Tier A
//      128 - Particles Tier B

struct RT64Context {
    // Window data.
    HWND hwnd = NULL;
    bool useVsync = true;

    // Game data.
    RT64_MATERIAL defaultMaterial;
    RT64_TEXTURE *blankTexture = nullptr;
    std::unordered_map<u32, AreaLighting> levelAreaLighting;
    AreaLighting defaultAreaLighting;
    std::mutex levelAreaLightingMutex;
    std::unordered_map<void *, std::string> geoLayoutNameMap;
    std::map<std::string, void *> nameGeoLayoutMap;
    std::unordered_map<void *, RecordedMod *> geoLayoutMods;
    std::map<std::string, RecordedMod *> pendingGeoLayoutMods;
    std::unordered_map<void *, void *> graphNodeGeoLayouts;
    std::unordered_set<void *> graphNodeRootsNamed;
    std::unordered_map<void *, void *> graphNodeModsSyncedGeoLayout;
    std::unordered_map<u64, std::string> texNameMap;
    std::map<std::string, u64> nameTexMap;
    std::unordered_map<u64, RecordedMod *> texMods;
    std::mutex texModsMutex;
    std::unordered_map<u32, u64> textureHashIdMap;
    std::unordered_map<u64, std::string> mapTexturePaths;
    std::unordered_set<u64> mapTexturesLoaded;
    std::unordered_map<u64, std::string> mapTextureNames;
    std::unordered_map<const void *, u64> materialNameHashes;
    const void *materialNameHashDl = nullptr;
    u64 materialNameHashCached = 0;
    std::unordered_map<const void *, u64> materialModNameHashes;
    bool loadedGeoLayoutMods = false;
    bool loadedTexMods = false;
    bool loadedLevelLights = false;

    // Runtime data.
    std::unordered_map<u32, RecordedTexture> textures;
    int postProcessWidth = 0;
    int postProcessHeight = 0;
    std::mutex postProcessMutex;
    std::atomic<bool> postProcessDirty{false};
    std::string postProcessHLSL;
    std::string postProcessOutputName;
    std::vector<std::string> postProcessInputNames;
    std::vector<RT64_SHADER_INPUT> postProcessInputs;
    struct Shader *postProcessShader = nullptr;
    std::vector<RT64_SHADER_UNIFORM_BLOCK> postProcessUniformBlocks;
    std::vector<u8> postProcessUniformData;
    std::unordered_map<u64, ShaderProgramRT64 *> shaderPrograms;
    std::vector<ShaderProgramRT64 *> retiredShaderPrograms;
    std::mutex shaderProgramsMutex;
    std::unordered_map<void *, std::shared_ptr<RecordedMod>> graphNodeMods;

    struct TickLight {
        RT64_LIGHT light;
        Mat4 transform;
    };
    std::unordered_map<u32, TickLight> tickLights;
    u32 tickLightsTimestamp = 0;

    // Render thread.
    RT64_LIBRARY lib;
    RT64_DEVICE *device = nullptr;
    RT64_SCENE *scene = nullptr;
    RT64_VIEW *view = nullptr;
    ShaderProgramRT64 *lastShaderProgram = nullptr;
    u16 lastShaderVariantKey = 0;
    RT64_SHADER *lastShaderVariant = nullptr;
    std::thread *renderThread = nullptr;
    RT64_INSPECTOR *renderInspector = nullptr;
    std::vector<std::string> renderInspectorMessages;
    std::mutex renderInspectorMutex;
    std::queue<InspectorMessage> inspectorMessageQueue;
    std::mutex inspectorMessageQueueMutex;
    GameFrame frames[RT64_MAX_RENDER_FRAMES];
    int cpuFrameIndex = 0;
    bool cpuFrameAcquired = false;
    std::deque<int> pendingFrameIndices;
    int gpuFrameIndex = -1;
    int barrierFrameIndex = -1;
    std::unordered_map<u32, GPUDisplayList> gpuDisplayLists;
    std::unordered_map<u64, GPUMesh> gpuStaticMeshes;
    std::unordered_set<u64> prevTickMeshHashes;
    std::unordered_set<u64> curTickMeshHashes;
    std::unordered_multimap<u64, GPUMesh> gpuDynamicRtMeshes;
    std::unordered_multimap<u64, GPUMesh> gpuDynamicRasterMeshes;
    std::unordered_map<u32, GPUTexture> gpuTextures;
    std::unordered_map<u64, RT64_TEXTURE *> hashToTexture;
    std::mutex renderFrameIndexMutex;
    std::condition_variable renderFrameCV;
    std::queue<UploadTexture> textureUploadQueue;
    std::mutex textureUploadQueueMutex;
    RT64_VIEW_DESC renderViewDesc;
    bool renderViewDescChanged = false;
    RT64_VIEW_DESC inspectorViewDesc = {};
    bool inspectorViewDescValid = false;
    std::mutex renderViewDescMutex;
    bool textureGenEnabled = false;
    Vec4f textureGenU = { 0.0f, 0.0f, 0.0f, 0.0f };
    Vec4f textureGenV = { 0.0f, 0.0f, 0.0f, 0.0f };
    RT64_LIGHT renderLights[RT64_MAX_LIGHTS];
    unsigned int renderLightCount = 0;
    unsigned int staticMeshesDrawn = 0;
    unsigned int dynamicMeshesDrawn = 0;
    unsigned int meshesCreated = 0;
    unsigned int meshesDestroyed = 0;
    std::atomic<bool> renderThreadRunning;
    std::atomic<bool> renderInspectorActive;
    unsigned int indexTriangleList[MAX_BUFFERED_MODEL_SPACE * 3];

    // Ray picking data.
    bool pickTexture = false;
    bool pickTextureHighlight = false;
    u64 pickTextureHash = 0;
    bool pickGeoLayout = false;
    bool pickGeoLayoutHighlight = false;
    int pickCursorX = 0;
    int pickCursorY = 0;
    void *pickedGeoLayout = nullptr;
    RT64_MATERIAL *pickedGeoLayoutMaterial = nullptr;
    RecordedMod *pickedGeoLayoutMod = nullptr;
    std::string pickedGeoLayoutName;
    RT64_LIGHT pickedGeoLayoutLight = {};
    bool pickedGeoLayoutLightEnabled = false;
    static const int sMaxPickedGeoLayoutOrigins = 32;
    Vec3f pickedGeoLayoutOrigins[sMaxPickedGeoLayoutOrigins] = {};
    int pickedGeoLayoutOriginCount = 0;
    Vec3f buildingGeoLayoutOrigins[sMaxPickedGeoLayoutOrigins] = {};
    int buildingGeoLayoutOriginCount = 0;
    u32 geoLayoutOriginsTimestamp = 0;
    void *publishedGeoLayout = nullptr;
    std::mutex pickTextureMutex;

    // Matrices.
    Mat4 identityTransform;

    // Rendering state.
    int instancesDrawn = 0;
    int currentTile = 0;
    u32 currentTextureIds[2] = { 0, 0 };
    ShaderProgramRT64 *shaderProgram = nullptr;
    bool background = false;
    bool xluDepthWrite = false;
    bool zmodeXlu = false;
    bool depthTest = false;
    bool frameEnded = false;
    Vec3f fogColor;
    Recti scissorRect;
    Recti viewportRect;
    s16 fogMul;
    s16 fogOffset;
    RecordedMod *graphNodeMod;
    void *graphNodeGeoLayout = nullptr;
    const void *materialDisplayList = nullptr;
    GameDisplayList *cachedDisplayList = nullptr;
    u32 cachedDisplayListUid = 0;
    u32 skyTextureKey = 0;
    Vec3f skyDiffuseMultiplier = { 1.0f, 1.0f, 1.0f };
    std::unordered_map<u64, u32> stitchedSkyTextureKeys;

    // Timing.
    LARGE_INTEGER frequency;
    std::atomic<bool> pauseMode;
};

extern RT64Context RT64;

static inline u32 gfx_rt64_area_lighting_key(unsigned int levelNum, unsigned int areaIndex) {
    return (levelNum * MAX_AREAS) + areaIndex;
}

static inline const AreaLighting &gfx_rt64_get_area_lighting(unsigned int levelNum, unsigned int areaIndex) {
    auto it = RT64.levelAreaLighting.find(gfx_rt64_area_lighting_key(levelNum, areaIndex));
    return (it != RT64.levelAreaLighting.end()) ? it->second : RT64.defaultAreaLighting;
}

static inline AreaLighting &gfx_rt64_get_or_add_area_lighting(unsigned int levelNum, unsigned int areaIndex) {
    const u32 key = gfx_rt64_area_lighting_key(levelNum, areaIndex);
    auto it = RT64.levelAreaLighting.find(key);
    if (it != RT64.levelAreaLighting.end()) {
        return it->second;
    }

    return RT64.levelAreaLighting.emplace(key, RT64.defaultAreaLighting).first->second;
}

void gfx_rt64_load_mod_configs(void);
void gfx_rt64_invalidate_mod_configs(void);

struct ColorCombiner;
struct FramePass;
struct Mod;
struct ShaderProgram;

LARGE_INTEGER gfx_rt64_profile_marker(void);
LARGE_INTEGER gfx_rt64_profile_delta(LARGE_INTEGER start, LARGE_INTEGER end);

u32 gfx_rt64_new_texture(const char *name);
void gfx_rt64_upload_texture(u32 textureKey, const u8 *rgba32Buf, s32 width, s32 height);
u64 gfx_rt64_material_vanilla_name_hash(void);
u64 gfx_rt64_material_mod_name_hash(void);
u32 gfx_rt64_map_texture_key(u64 nameHash);
u32 gfx_rt64_stitch_skybox_texture(const Texture *const *tiles);
std::string gfx_rt64_mod_texture_root(struct Mod *mod);

void gfx_rt64_destroy_all_shaders(void);
void gfx_rt64_collect_uniform_blocks(struct Shader *const *shaders, int shaderCount, std::vector<RT64_SHADER_UNIFORM_BLOCK> &blocks, std::vector<u8> &data);
void gfx_rt64_capture_post_process_uniforms(void);
void gfx_rt64_render_thread(void);
void gfx_rt64_destroy_gpu_mesh(GPUMesh &mesh);
void gfx_rt64_publish_picked_geo_layout(void);
unsigned int gfx_rt64_clamp_percent(long long percent, unsigned int minPercent, unsigned int maxPercent);
void gfx_rt64_adopt_inspector_view_desc(const RT64_VIEW_DESC &desc);
void gfx_rt64_sync_inspector_map_names(int panel, RecordedMod *mod);
int gfx_rt64_get_level_index(void);
int gfx_rt64_get_area_index(void);
bool gfx_rt64_use_vsync(void);
