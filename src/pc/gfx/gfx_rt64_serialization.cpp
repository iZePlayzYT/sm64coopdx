#if defined(_WIN32)

extern "C" {
#include "pc/lua/smlua.h"
#include "pc/fs/fs.h"
#include "pc/mods/mod.h"
#include "pc/lua/utils/smlua_level_utils.h"
#include "level_table.h"
}

#include "gfx_rt64.h"
#include "gfx_rt64_context.hpp"
#include "gfx_rt64_default_config.hpp"
#include "gfx_rt64_serialization.hpp"

s32 DynOS_Builtin_Actor_GetCount(void);
const GeoLayout *DynOS_Builtin_Actor_GetFromIndex(s32 aIndex);
const char *DynOS_Builtin_Actor_GetNameFromIndex(s32 aIndex);

#include <cmath>
#include <cstring>
#include <fstream>
#include <set>
#include <vector>

extern "C" {
#include "engine/math_util.h"
}

static inline std::string gfx_rt64_level_lights_path(void) { return fs_get_write_path("mods/level_lights.lua"); }
static inline std::string gfx_rt64_geo_layout_mods_path(void) { return fs_get_write_path("mods/geo_layout_mods.lua"); }
static inline std::string gfx_rt64_texture_mods_path(void) { return fs_get_write_path("mods/texture_mods.lua"); }

static void gfx_rt64_ensure_config_dir(void) {
    fs_sys_mkdir(fs_get_write_path("mods"));
}

static void gfx_rt64_area_lighting_key_split(u32 key, u32 *outLevelNum, u32 *outAreaIndex) {
    *outLevelNum = key / MAX_AREAS;
    *outAreaIndex = key % MAX_AREAS;
}

//
// Texture names
//

static inline size_t gfx_rt64_string_hash(const u8 *str) {
    size_t h = 0;
    for (const u8 *p = str; *p; p++) {
        h = 31 * h + *p;
    }
    return h;
}

u64 gfx_rt64_texture_name_string_hash(const std::string &name) {
    return gfx_rt64_string_hash((const u8 *)(name.c_str()));
}

u64 gfx_rt64_get_texture_name_hash(const std::string &name) {
    u64 hash = gfx_rt64_texture_name_string_hash(name);
    RT64.texNameMap[hash] = name;
    RT64.nameTexMap[name] = hash;
    return hash;
}

std::string gfx_rt64_texture_mod_name(u64 texHash) {
    if (texHash == 0) { return std::string(); }

    auto it = RT64.texNameMap.find(texHash);
    return (it != RT64.texNameMap.end()) ? it->second : std::string();
}

static bool gfx_rt64_parse_placeholder_texture_name(const std::string &name, u64 *outHash) {
    unsigned long long parsed = 0;

    if ((name.size() == 24) && (sscanf(name.c_str(), "texture_%16llx", &parsed) == 1)) {
        *outHash = (u64)(parsed);
        return true;
    }

    if ((name.size() == 22) && (sscanf(name.c_str(), "solid_%16llx", &parsed) == 1)) {
        *outHash = (u64)(parsed);
        return true;
    }

    return false;
}

//
// Baselines
//

struct ModSnapshot {
    bool hasMaterial = false;
    RT64_MATERIAL material = {};
    bool hasLight = false;
    RT64_LIGHT light = {};
    u64 bumpMapHash = 0;
    u64 normalMapHash = 0;
    u64 specularMapHash = 0;

    bool operator==(const ModSnapshot &o) const {
        if ((hasMaterial != o.hasMaterial) || (hasLight != o.hasLight)) { return false; }
        if ((bumpMapHash != o.bumpMapHash) || (normalMapHash != o.normalMapHash) || (specularMapHash != o.specularMapHash)) { return false; }
        if (hasMaterial && (memcmp(&material, &o.material, sizeof(RT64_MATERIAL)) != 0)) { return false; }
        if (hasLight && (memcmp(&light, &o.light, sizeof(RT64_LIGHT)) != 0)) { return false; }
        return true;
    }
};

static std::map<u32, AreaLighting> sBaselineAreaLighting;
static std::unordered_map<u64, ModSnapshot> sBaselineTexMods;
static std::map<std::string, ModSnapshot> sBaselineGeoLayoutMods;

static ModSnapshot gfx_rt64_snapshot_mod(const RecordedMod *recordedMod) {
    ModSnapshot snapshot;
    if (recordedMod == nullptr) { return snapshot; }

    if (recordedMod->materialMod != nullptr) {
        snapshot.hasMaterial = true;
        snapshot.material = *recordedMod->materialMod;
    }

    if (recordedMod->lightMod != nullptr) {
        snapshot.hasLight = true;
        snapshot.light = *recordedMod->lightMod;
    }

    snapshot.bumpMapHash = recordedMod->bumpMapHash;
    snapshot.normalMapHash = recordedMod->normalMapHash;
    snapshot.specularMapHash = recordedMod->specularMapHash;

    return snapshot;
}

static bool gfx_rt64_area_lighting_matches(const AreaLighting &a, const AreaLighting &b) {
    if (a.lightCount != b.lightCount) { return false; }
    if (memcmp(&a.sceneDesc, &b.sceneDesc, sizeof(RT64_SCENE_DESC)) != 0) { return false; }
    return memcmp(a.lights, b.lights, sizeof(RT64_LIGHT) * (size_t)(a.lightCount)) == 0;
}

//
// Default level lighting
//

static void gfx_rt64_default_area_lighting(AreaLighting &areaLighting) {
    memset(areaLighting.lights, 0, sizeof(areaLighting.lights));
    areaLighting.lightCount = 0;

    // Configure the default area lighting scene description.
    auto &sceneDesc = areaLighting.sceneDesc;
    vec3f_set(sceneDesc.ambientBaseColor, 51.0f, 51.0f, 64.0f);
    vec3f_set(sceneDesc.ambientNoGIColor, 26.0f, 38.0f, 51.0f);
    vec3f_set(sceneDesc.eyeLightDiffuseColor, 26.0f, 26.0f, 26.0f);
    vec3f_set(sceneDesc.eyeLightSpecularColor, 26.0f, 26.0f, 26.0f);
    vec3f_set(sceneDesc.skyDiffuseMultiplier, 1.0f, 1.0f, 1.0f);
    vec3f_set(sceneDesc.skyHSLModifier, 0.0f, 0.0f, 0.0f);
    sceneDesc.skyYawOffset = 0.0f;
    sceneDesc.giDiffuseStrength = 0.7f;
    sceneDesc.giSkyStrength = 0.35f;

    // Configure a default directional sun.
    RT64_LIGHT &light = areaLighting.lights[0];
    vec3f_set(light.position, 100000.0f, 200000.0f, 100000.0f);
    vec3f_set(light.diffuseColor, 204.0f, 191.0f, 166.0f);
    light.intensity = 1.0f;
    light.attenuationRadius = 1e11f;
    light.pointRadius = 5000.0f;
    vec3f_set(light.specularColor, 204.0f, 191.0f, 166.0f);
    light.shadowOffset = 0.0f;
    light.attenuationExponent = 0.0f;
    light.groupBits = RT64_LIGHT_GROUP_DEFAULT;
    areaLighting.lightCount = 1;
}

static void gfx_rt64_default_level_lights(void) {
    gfx_rt64_default_area_lighting(RT64.defaultAreaLighting);

    for (auto &pair : RT64.levelAreaLighting) {
        gfx_rt64_default_area_lighting(pair.second);
    }
}

static inline void rt64_default_value(const RT64DefaultFloat &v, float *dest) { *dest = v.value; }
static inline void rt64_default_value(const RT64DefaultInt &v, float *dest) { *dest = (float)(v.value); }
static inline void rt64_default_value(const RT64DefaultColor &v, Vec3f *dest) { memcpy(*dest, v.value, sizeof(Vec3f)); }
static inline void rt64_default_value(const RT64DefaultColorMix &v, Vec4f *dest) { memcpy(*dest, v.value, sizeof(Vec4f)); }
static inline void rt64_default_value(const RT64DefaultMask &v, u32 *dest) { *dest = v.value; }

static void gfx_rt64_read_default_light(const RT64DefaultLight &src, RT64_LIGHT *light) {
    memset(light, 0, sizeof(RT64_LIGHT));
    memcpy(light->position, src.position, sizeof(Vec3f));
    memcpy(light->diffuseColor, src.diffuseColor.value, sizeof(Vec3f));
    memcpy(light->specularColor, src.specularColor.value, sizeof(Vec3f));
    light->attenuationRadius = src.attenuationRadius;
    light->pointRadius = src.pointRadius;
    light->shadowOffset = src.shadowOffset;
    light->attenuationExponent = src.attenuationExponent;
    light->flickerIntensity = src.flickerIntensity;
    light->groupBits = src.groupBits;
    light->lightType = src.lightType;
    light->pitch = src.pitch;
    light->yaw = src.yaw;
    light->roll = src.roll;
    light->scaleX = src.scaleX;
    light->scaleY = src.scaleY;
    light->lightShape = src.lightShape;
    light->apertureEnabled = src.apertureEnabled;
    light->aperturePitch = src.aperturePitch;
    light->apertureYaw = src.apertureYaw;
    light->volumetricEnabled = src.volumetricEnabled;
    light->volumetricIntensity = src.volumetricIntensity;
    light->intensity = src.intensity;
}

static void gfx_rt64_read_default_scene(const RT64DefaultScene &src, RT64_SCENE_DESC *scene) {
    memcpy(scene->ambientBaseColor, src.ambientBaseColor.value, sizeof(Vec3f));
    memcpy(scene->ambientNoGIColor, src.ambientNoGIColor.value, sizeof(Vec3f));
    memcpy(scene->eyeLightDiffuseColor, src.eyeLightDiffuseColor.value, sizeof(Vec3f));
    memcpy(scene->eyeLightSpecularColor, src.eyeLightSpecularColor.value, sizeof(Vec3f));
    memcpy(scene->skyDiffuseMultiplier, src.skyDiffuseMultiplier, sizeof(Vec3f));
    memcpy(scene->skyHSLModifier, src.skyHSLModifier, sizeof(Vec3f));
    scene->skyYawOffset = src.skyYawOffset;
    scene->giDiffuseStrength = src.giDiffuseStrength;
    scene->giSkyStrength = src.giSkyStrength;
}

void gfx_rt64_load_level_lights(void) {
    gfx_rt64_default_level_lights();
    sBaselineAreaLighting.clear();

    for (int i = 0; i < gRT64DefaultLevelLightCount; i++) {
        const RT64DefaultArea &defaultArea = gRT64DefaultLevelLights[i];
        if ((defaultArea.levelNum >= (u32)(RT64_MAX_LEVELS)) || (defaultArea.areaIndex >= (u32)(MAX_AREAS))) { continue; }

        AreaLighting &areaLighting = gfx_rt64_get_or_add_area_lighting(defaultArea.levelNum, defaultArea.areaIndex);
        gfx_rt64_read_default_scene(defaultArea.scene, &areaLighting.sceneDesc);
        areaLighting.lightCount = 0;
        for (int j = 0; (j < defaultArea.lightCount) && (areaLighting.lightCount < RT64_MAX_LEVEL_LIGHTS); j++) {
            gfx_rt64_read_default_light(defaultArea.lights[j], &areaLighting.lights[areaLighting.lightCount++]);
        }

        sBaselineAreaLighting[gfx_rt64_area_lighting_key(defaultArea.levelNum, defaultArea.areaIndex)] = areaLighting;
    }
}

static void gfx_rt64_set_level_lights(u32 levelIndex, u32 areaIndex, const std::vector<RT64_LIGHT> *lights, const RT64_SCENE_DESC *sceneDesc) {
    if ((levelIndex >= (u32)(RT64_MAX_LEVELS)) || (areaIndex >= (u32)(MAX_AREAS))) { return; }

    const std::lock_guard<std::mutex> lightingLock(RT64.levelAreaLightingMutex);
    AreaLighting &areaLighting = gfx_rt64_get_or_add_area_lighting(levelIndex, areaIndex);

    if (lights != nullptr) {
        int lightCount = (int)(lights->size());
        if (lightCount > RT64_MAX_LEVEL_LIGHTS) { lightCount = RT64_MAX_LEVEL_LIGHTS; }
        if (lightCount > 0) {
            memcpy(areaLighting.lights, lights->data(), sizeof(RT64_LIGHT) * lightCount);
        }

        areaLighting.lightCount = lightCount;
    }

    if (sceneDesc != nullptr) {
        areaLighting.sceneDesc = (*sceneDesc);
    }

    sBaselineAreaLighting[gfx_rt64_area_lighting_key(levelIndex, areaIndex)] = areaLighting;
}

//
// Recorded mods
//

static void gfx_rt64_reset_recorded_mod(RecordedMod *recordedMod) {
    if (recordedMod->materialMod != nullptr) {
        memset(recordedMod->materialMod, 0, sizeof(RT64_MATERIAL));
        recordedMod->materialMod->enabledAttributes = RT64_ATTRIBUTE_NONE;
    }

    delete recordedMod->lightMod;
    recordedMod->lightMod = nullptr;
    recordedMod->bumpMapHash = 0;
    recordedMod->normalMapHash = 0;
    recordedMod->specularMapHash = 0;
}

static void gfx_rt64_apply_default_mod(const RT64DefaultMod &defaultMod, RecordedMod *recordedMod) {
    const RT64DefaultMaterial &src = defaultMod.materialMod;

    s32 enabledAttributes = RT64_ATTRIBUTE_NONE;
    #define RT64_MATERIAL_ATTR(flag, field, luaName, kind) if (src.field.set) { enabledAttributes |= (flag); }
    #include "gfx_rt64_material_attributes.inl"
    #undef RT64_MATERIAL_ATTR
    enabledAttributes |= RT64_ATTRIBUTE_SHADING_MODEL;
    if (src.specularTintSet) { enabledAttributes |= RT64_ATTRIBUTE_SPECULAR_TINT; }
    if (src.shadowEnabledSet) { enabledAttributes |= RT64_ATTRIBUTE_SHADOW_ENABLED; }
    if (src.shadowCenterSet) { enabledAttributes |= RT64_ATTRIBUTE_SHADOW_CENTER; }

    if (enabledAttributes != RT64_ATTRIBUTE_NONE) {
        if (recordedMod->materialMod == nullptr) {
            recordedMod->materialMod = new RT64_MATERIAL();
        }

        RT64_MATERIAL *material = recordedMod->materialMod;
        memset(material, 0, sizeof(RT64_MATERIAL));
        material->enabledAttributes = enabledAttributes;

        #define RT64_MATERIAL_ATTR(flag, field, luaName, kind) \
            if (src.field.set) { rt64_default_value(src.field, &material->field); }
        #include "gfx_rt64_material_attributes.inl"
        #undef RT64_MATERIAL_ATTR

        material->shadingModel = src.shadingModelSet;
        if (src.specularTintSet) { material->specularTint = src.specularTint; }
        if (src.shadowEnabledSet) { material->shadowEnabled = src.shadowEnabled; }
        if (src.shadowCenterSet) { material->shadowCenter = src.shadowCenter; }
    }

    if (defaultMod.lightMod.set) {
        delete recordedMod->lightMod;
        recordedMod->lightMod = new RT64_LIGHT();
        gfx_rt64_read_default_light(defaultMod.lightMod, recordedMod->lightMod);
    }

    const std::string defaultRoot = fs_get_write_path("textures/segment2/");
    if ((defaultMod.bumpMap != nullptr) && (defaultMod.bumpMap[0] != '\0')) {
        recordedMod->bumpMapHash = gfx_rt64_get_texture_name_hash(defaultMod.bumpMap);
        gfx_rt64_register_map_texture(defaultMod.bumpMap, defaultRoot.c_str());
    }

    if ((defaultMod.normalMap != nullptr) && (defaultMod.normalMap[0] != '\0')) {
        recordedMod->normalMapHash = gfx_rt64_get_texture_name_hash(defaultMod.normalMap);
        gfx_rt64_register_map_texture(defaultMod.normalMap, defaultRoot.c_str());
    }

    if ((defaultMod.specularMap != nullptr) && (defaultMod.specularMap[0] != '\0')) {
        recordedMod->specularMapHash = gfx_rt64_get_texture_name_hash(defaultMod.specularMap);
        gfx_rt64_register_map_texture(defaultMod.specularMap, defaultRoot.c_str());
    }
}

static void gfx_rt64_set_recorded_mod(RecordedMod *recordedMod, const RT64_MATERIAL *materialMod, const RT64_LIGHT *lightMod, const std::string &bumpMapName, const std::string &normalMapName, const std::string &specularMapName) {
    if (materialMod != nullptr) {
        if (recordedMod->materialMod == nullptr) {
            recordedMod->materialMod = new RT64_MATERIAL();
        }

        (*recordedMod->materialMod) = (*materialMod);
    }

    if (lightMod != nullptr) {
        delete recordedMod->lightMod;
        recordedMod->lightMod = new RT64_LIGHT();
        (*recordedMod->lightMod) = (*lightMod);
    }

    if (!bumpMapName.empty()) {
        recordedMod->bumpMapHash = gfx_rt64_get_texture_name_hash(bumpMapName);
    }

    if (!normalMapName.empty()) {
        recordedMod->normalMapHash = gfx_rt64_get_texture_name_hash(normalMapName);
    }

    if (!specularMapName.empty()) {
        recordedMod->specularMapHash = gfx_rt64_get_texture_name_hash(specularMapName);
    }
}

static bool gfx_rt64_recorded_mod_is_empty(const RecordedMod *recordedMod) {
    if (recordedMod == nullptr) { return true; }
    if ((recordedMod->materialMod != nullptr) && (recordedMod->materialMod->enabledAttributes != RT64_ATTRIBUTE_NONE)) { return false; }
    if (recordedMod->lightMod != nullptr) { return false; }
    return (recordedMod->bumpMapHash == 0) && (recordedMod->normalMapHash == 0) && (recordedMod->specularMapHash == 0);
}

//
// Geo layout mods
//

static RecordedMod *gfx_rt64_bind_geo_layout_mod(const std::string &geoName) {
    auto geoIt = RT64.nameGeoLayoutMap.find(geoName);
    void *geoLayout = (geoIt != RT64.nameGeoLayoutMap.end()) ? geoIt->second : nullptr;

    RecordedMod *recordedMod = nullptr;
    if (geoLayout != nullptr) {
        auto modIt = RT64.geoLayoutMods.find(geoLayout);
        if (modIt != RT64.geoLayoutMods.end()) { recordedMod = modIt->second; }
    } else {
        auto pendingIt = RT64.pendingGeoLayoutMods.find(geoName);
        if (pendingIt != RT64.pendingGeoLayoutMods.end()) { recordedMod = pendingIt->second; }
    }

    if (recordedMod == nullptr) {
        recordedMod = new RecordedMod();
    } else {
        gfx_rt64_reset_recorded_mod(recordedMod);
    }

    if (geoLayout != nullptr) {
        RT64.geoLayoutMods[geoLayout] = recordedMod;
        RT64.nameGeoLayoutMap[geoName] = geoLayout;
    } else {
        RT64.pendingGeoLayoutMods[geoName] = recordedMod;
    }

    return recordedMod;
}

void gfx_rt64_load_geo_layout_mods(void) {
    std::set<RecordedMod *> previousMods;
    for (const auto &pair : RT64.geoLayoutMods) { previousMods.insert(pair.second); }
    for (const auto &pair : RT64.pendingGeoLayoutMods) { previousMods.insert(pair.second); }
    RT64.geoLayoutMods.clear();
    RT64.pendingGeoLayoutMods.clear();

    {
        const std::lock_guard<std::mutex> pickLock(RT64.pickTextureMutex);
        RT64.pickedGeoLayout = nullptr;
        RT64.pickedGeoLayoutMaterial = nullptr;
        RT64.pickedGeoLayoutMod = nullptr;
        RT64.pickedGeoLayoutName.clear();
        RT64.pickedGeoLayoutLightEnabled = false;
        RT64.pickedGeoLayoutOriginCount = 0;
        RT64.publishedGeoLayout = nullptr;

        for (RecordedMod *previousMod : previousMods) { delete previousMod; }
    }

    sBaselineGeoLayoutMods.clear();

    for (s32 i = 0; i < DynOS_Builtin_Actor_GetCount(); i++) {
        void *geoLayout = (void *)(DynOS_Builtin_Actor_GetFromIndex(i));
        const char *name = DynOS_Builtin_Actor_GetNameFromIndex(i);
        RT64.geoLayoutNameMap[geoLayout] = name;
        RT64.nameGeoLayoutMap[name] = geoLayout;
    }

    for (int i = 0; i < gRT64DefaultGeoLayoutModCount; i++) {
        const RT64DefaultMod &defaultMod = gRT64DefaultGeoLayoutMods[i];
        if ((defaultMod.name == nullptr) || (defaultMod.name[0] == '\0')) { continue; }

        RecordedMod *recordedMod = gfx_rt64_bind_geo_layout_mod(defaultMod.name);
        gfx_rt64_apply_default_mod(defaultMod, recordedMod);
        sBaselineGeoLayoutMods[defaultMod.name] = gfx_rt64_snapshot_mod(recordedMod);
    }
}

static void gfx_rt64_set_geo_layout_mod(const std::string &geoName, const RT64_MATERIAL *materialMod, const RT64_LIGHT *lightMod, const std::string &bumpMapName, const std::string &normalMapName, const std::string &specularMapName) {
    if (geoName.empty()) { return; }

    RecordedMod *recordedMod = gfx_rt64_bind_geo_layout_mod(geoName);
    gfx_rt64_set_recorded_mod(recordedMod, materialMod, lightMod, bumpMapName, normalMapName, specularMapName);

    sBaselineGeoLayoutMods[geoName] = gfx_rt64_snapshot_mod(recordedMod);

    gfx_rt64_invalidate_mod_configs();
}

//
// Texture mods
//

static RecordedMod *gfx_rt64_bind_texture_mod(const std::string &texName, u64 *outTexHash) {
    u64 texHash;
    if (gfx_rt64_parse_placeholder_texture_name(texName, &texHash)) {
        RT64.texNameMap[texHash] = texName;
        RT64.nameTexMap[texName] = texHash;
    } else {
        texHash = gfx_rt64_get_texture_name_hash(texName);
    }

    RecordedMod *&texMod = RT64.texMods[texHash];
    if (texMod == nullptr) {
        texMod = new RecordedMod();
    } else {
        gfx_rt64_reset_recorded_mod(texMod);
    }

    (*outTexHash) = texHash;
    return texMod;
}

void gfx_rt64_load_texture_mods(void) {
    for (auto &pair : RT64.texMods) { gfx_rt64_reset_recorded_mod(pair.second); }
    sBaselineTexMods.clear();

    for (int i = 0; i < gRT64DefaultTextureModCount; i++) {
        const RT64DefaultMod &defaultMod = gRT64DefaultTextureMods[i];
        if ((defaultMod.name == nullptr) || (defaultMod.name[0] == '\0')) { continue; }

        u64 texHash = 0;
        RecordedMod *texMod = gfx_rt64_bind_texture_mod(defaultMod.name, &texHash);
        gfx_rt64_apply_default_mod(defaultMod, texMod);

        sBaselineTexMods[texHash] = gfx_rt64_snapshot_mod(texMod);
    }
}

static void gfx_rt64_set_texture_mod(const std::string &texName, const RT64_MATERIAL *materialMod, const RT64_LIGHT *lightMod, const std::string &bumpMapName, const std::string &normalMapName, const std::string &specularMapName) {
    if (texName.empty()) { return; }

    const std::lock_guard<std::mutex> texModsLock(RT64.texModsMutex);
    u64 texHash = 0;
    RecordedMod *texMod = gfx_rt64_bind_texture_mod(texName, &texHash);
    gfx_rt64_set_recorded_mod(texMod, materialMod, lightMod, bumpMapName, normalMapName, specularMapName);

    sBaselineTexMods[texHash] = gfx_rt64_snapshot_mod(texMod);
}

//
// Lua readers
//

static bool gfx_rt64_lua_get_table_field(lua_State *L, int index, const char *key, const char *context) {
    lua_getfield(L, index, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    if (!lua_istable(L, -1)) {
        LOG_LUA_LINE("%s: field '%s' must be a table, got %s", context, key, luaL_typename(L, -1));
        lua_pop(L, 1);
        return false;
    }

    return true;
}

static bool gfx_rt64_lua_get_number_field(lua_State *L, int index, const char *key, float *outValue, const char *context) {
    lua_getfield(L, index, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    if (!lua_isnumber(L, -1)) {
        LOG_LUA_LINE("%s: field '%s' must be a number, got %s", context, key, luaL_typename(L, -1));
        lua_pop(L, 1);
        return false;
    }

    (*outValue) = (float)(lua_tonumber(L, -1));
    lua_pop(L, 1);
    return true;
}

static bool gfx_rt64_lua_get_mask_field(lua_State *L, int index, const char *key, u32 *outValue, const char *context) {
    lua_getfield(L, index, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return false;
    }

    if (!lua_isnumber(L, -1)) {
        LOG_LUA_LINE("%s: field '%s' must be a number, got %s", context, key, luaL_typename(L, -1));
        lua_pop(L, 1);
        return false;
    }

    (*outValue) = (u32)((s64)(lua_tonumber(L, -1)));
    lua_pop(L, 1);
    return true;
}

static std::string gfx_rt64_lua_get_string_field(lua_State *L, int index, const char *key, const char *context) {
    lua_getfield(L, index, key);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return std::string();
    }

    if (lua_type(L, -1) != LUA_TSTRING) {
        LOG_LUA_LINE("%s: field '%s' must be a string, got %s", context, key, luaL_typename(L, -1));
        lua_pop(L, 1);
        return std::string();
    }

    std::string value = lua_tostring(L, -1);
    lua_pop(L, 1);
    return value;
}

static bool gfx_rt64_lua_get_vector_field(lua_State *L, int index, const char *key, float *outValues, int count, const char *context) {
    if (!gfx_rt64_lua_get_table_field(L, index, key, context)) { return false; }

    const int table = lua_gettop(L);
    static const char *components[] = { "x", "y", "z", "w" };
    bool byComponent = false;
    {
        lua_getfield(L, table, "x");
        byComponent = !lua_isnil(L, -1);
        lua_pop(L, 1);
    }

    for (int i = 0; i < count; i++) {
        if (byComponent) {
            lua_getfield(L, table, components[i]);
        } else {
            lua_rawgeti(L, table, i + 1);
        }

        if (!lua_isnumber(L, -1)) {
            LOG_LUA_LINE("%s: field '%s' needs %d numbers", context, key, count);
            lua_pop(L, 2);
            return false;
        }

        outValues[i] = (float)(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return true;
}

static bool gfx_rt64_lua_get_vector3_field(lua_State *L, int index, const char *key, Vec3f *outValue, const char *context) {
    return gfx_rt64_lua_get_vector_field(L, index, key, *outValue, 3, context);
}

static void gfx_rt64_lua_read_light(lua_State *L, int index, RT64_LIGHT *light, const char *context) {
    memset(light, 0, sizeof(RT64_LIGHT));
    light->attenuationRadius = 1e11f;
    light->pointRadius = 5000.0f;
    light->groupBits = RT64_LIGHT_GROUP_DEFAULT;
    light->lightType = RT64_LIGHT_TYPE_AREA;
    light->scaleX = 500.0f;
    light->scaleY = 500.0f;
    light->lightShape = RT64_LIGHT_SHAPE_CIRCLE;
    light->volumetricIntensity = 1.0f;
    light->intensity = 1.0f;

    gfx_rt64_lua_get_vector3_field(L, index, "position", &light->position, context);
    gfx_rt64_lua_get_vector3_field(L, index, "diffuseColor", &light->diffuseColor, context);

    vec3f_copy(light->specularColor, light->diffuseColor);
    gfx_rt64_lua_get_vector3_field(L, index, "specularColor", &light->specularColor, context);

    gfx_rt64_lua_get_number_field(L, index, "intensity", &light->intensity, context);
    gfx_rt64_lua_get_number_field(L, index, "attenuationRadius", &light->attenuationRadius, context);
    gfx_rt64_lua_get_number_field(L, index, "pointRadius", &light->pointRadius, context);
    gfx_rt64_lua_get_number_field(L, index, "shadowOffset", &light->shadowOffset, context);
    gfx_rt64_lua_get_number_field(L, index, "attenuationExponent", &light->attenuationExponent, context);
    gfx_rt64_lua_get_number_field(L, index, "flickerIntensity", &light->flickerIntensity, context);

    gfx_rt64_lua_get_mask_field(L, index, "groupBits", &light->groupBits, context);

    // Everything below only matters for a "point" light (RT64_LIGHT_TYPE_POINT) - an "area" light,
    // the default and the only kind before this, ignores all of it.
    const std::string type = gfx_rt64_lua_get_string_field(L, index, "type", context);
    if (type == "point") {
        light->lightType = RT64_LIGHT_TYPE_POINT;
    } else if (!type.empty() && (type != "area")) {
        LOG_LUA_LINE("%s: unknown light type '%s', expected 'area' or 'point'", context, type.c_str());
    }

    gfx_rt64_lua_get_number_field(L, index, "pitch", &light->pitch, context);
    gfx_rt64_lua_get_number_field(L, index, "yaw", &light->yaw, context);
    gfx_rt64_lua_get_number_field(L, index, "roll", &light->roll, context);
    gfx_rt64_lua_get_number_field(L, index, "scaleX", &light->scaleX, context);
    gfx_rt64_lua_get_number_field(L, index, "scaleY", &light->scaleY, context);

    const std::string shape = gfx_rt64_lua_get_string_field(L, index, "shape", context);
    if (shape == "square") {
        light->lightShape = RT64_LIGHT_SHAPE_SQUARE;
    } else if (!shape.empty() && (shape != "circle")) {
        LOG_LUA_LINE("%s: unknown light shape '%s', expected 'circle' or 'square'", context, shape.c_str());
    }

    const bool hasAperturePitch = gfx_rt64_lua_get_number_field(L, index, "aperturePitch", &light->aperturePitch, context);
    const bool hasApertureYaw = gfx_rt64_lua_get_number_field(L, index, "apertureYaw", &light->apertureYaw, context);
    light->apertureEnabled = (hasAperturePitch || hasApertureYaw) ? 1 : 0;
    light->volumetricEnabled = gfx_rt64_lua_get_number_field(L, index, "volumetricIntensity", &light->volumetricIntensity, context) ? 1 : 0;
}

static void gfx_rt64_lua_read_scene_description(lua_State *L, int index, RT64_SCENE_DESC *sceneDesc, const char *context) {
    gfx_rt64_lua_get_vector3_field(L, index, "ambientBaseColor", &sceneDesc->ambientBaseColor, context);
    gfx_rt64_lua_get_vector3_field(L, index, "ambientNoGIColor", &sceneDesc->ambientNoGIColor, context);
    gfx_rt64_lua_get_vector3_field(L, index, "eyeLightDiffuseColor", &sceneDesc->eyeLightDiffuseColor, context);
    gfx_rt64_lua_get_vector3_field(L, index, "eyeLightSpecularColor", &sceneDesc->eyeLightSpecularColor, context);
    gfx_rt64_lua_get_vector3_field(L, index, "skyDiffuseMultiplier", &sceneDesc->skyDiffuseMultiplier, context);
    gfx_rt64_lua_get_vector3_field(L, index, "skyHSLModifier", &sceneDesc->skyHSLModifier, context);
    gfx_rt64_lua_get_number_field(L, index, "skyYawOffset", &sceneDesc->skyYawOffset, context);
    gfx_rt64_lua_get_number_field(L, index, "giDiffuseStrength", &sceneDesc->giDiffuseStrength, context);
    gfx_rt64_lua_get_number_field(L, index, "giSkyStrength", &sceneDesc->giSkyStrength, context);
}

static void gfx_rt64_lua_read_bool_attribute(lua_State *L, int index, const char *key, u32 *outValue, RT64_MATERIAL *materialMod, s32 attribute) {
    lua_getfield(L, index, key);
    if (!lua_isnil(L, -1)) {
        *outValue = lua_toboolean(L, -1) ? 1u : 0u;
        materialMod->enabledAttributes |= attribute;
    }
    lua_pop(L, 1);
}

static void gfx_rt64_lua_read_material_mod(lua_State *L, int index, RT64_MATERIAL *materialMod, const char *context) {
    memset(materialMod, 0, sizeof(RT64_MATERIAL));
    materialMod->enabledAttributes = RT64_ATTRIBUTE_NONE;

    #define RT64_MATERIAL_ATTR(flag, field, luaName, kind) RT64_READ_##kind(luaName, flag, field)
    #define RT64_READ_FLOAT(name, flag, field) \
        if (gfx_rt64_lua_get_number_field(L, index, name, &materialMod->field, context)) { \
            materialMod->enabledAttributes |= flag; \
        }
    #define RT64_READ_INT(name, flag, field) RT64_READ_FLOAT(name, flag, field)
    #define RT64_READ_COLOR3(name, flag, field) \
        if (gfx_rt64_lua_get_vector3_field(L, index, name, &materialMod->field, context)) { \
            materialMod->enabledAttributes |= flag; \
        }
    #define RT64_READ_MASK(name, flag, field) \
        if (gfx_rt64_lua_get_mask_field(L, index, name, &materialMod->field, context)) { \
            materialMod->enabledAttributes |= flag; \
        }
    #define RT64_READ_COLOR_MIX(name, flag, field) \
        { \
            if (gfx_rt64_lua_get_vector_field(L, index, name, materialMod->field, 4, context)) { \
                materialMod->enabledAttributes |= flag; \
            } \
        }
    #include "gfx_rt64_material_attributes.inl"
    #undef RT64_READ_COLOR_MIX
    #undef RT64_READ_MASK
    #undef RT64_READ_COLOR3
    #undef RT64_READ_INT
    #undef RT64_READ_FLOAT
    #undef RT64_MATERIAL_ATTR

    const std::string shadingModel = gfx_rt64_lua_get_string_field(L, index, "shadingModel", context);
    if (!shadingModel.empty()) {
        if (shadingModel == "lambert") {
            materialMod->shadingModel = RT64_SHADING_MODEL_LAMBERT;
            materialMod->enabledAttributes |= RT64_ATTRIBUTE_SHADING_MODEL;
        } else if (shadingModel == "phong") {
            materialMod->shadingModel = RT64_SHADING_MODEL_PHONG;
            materialMod->enabledAttributes |= RT64_ATTRIBUTE_SHADING_MODEL;
        } else if (shadingModel == "blinn") {
            materialMod->shadingModel = RT64_SHADING_MODEL_BLINN;
            materialMod->enabledAttributes |= RT64_ATTRIBUTE_SHADING_MODEL;
        } else {
            LOG_LUA_LINE("%s: unknown shading model '%s', expected 'lambert', 'phong' or 'blinn'", context, shadingModel.c_str());
        }
    }

    gfx_rt64_lua_read_bool_attribute(L, index, "specularTint", &materialMod->specularTint, materialMod, RT64_ATTRIBUTE_SPECULAR_TINT);
    gfx_rt64_lua_read_bool_attribute(L, index, "shadowEnabled", &materialMod->shadowEnabled, materialMod, RT64_ATTRIBUTE_SHADOW_ENABLED);
    gfx_rt64_lua_read_bool_attribute(L, index, "shadowCenter", &materialMod->shadowCenter, materialMod, RT64_ATTRIBUTE_SHADOW_CENTER);
}

static void gfx_rt64_lua_read_recorded_mod(lua_State *L, int index, RT64_MATERIAL *materialMod, bool *outHasMaterial, RT64_LIGHT *lightMod, bool *outHasLight, std::string *outBumpMapName, std::string *outNormalMapName, std::string *outSpecularMapName, const char *context) {
    (*outHasMaterial) = false;
    (*outHasLight) = false;

    if (gfx_rt64_lua_get_table_field(L, index, "materialMod", context)) {
        gfx_rt64_lua_read_material_mod(L, lua_gettop(L), materialMod, context);
        (*outHasMaterial) = true;
        lua_pop(L, 1);
    }

    if (gfx_rt64_lua_get_table_field(L, index, "lightMod", context)) {
        gfx_rt64_lua_read_light(L, lua_gettop(L), lightMod, context);
        (*outHasLight) = true;
        lua_pop(L, 1);
    }

    (*outBumpMapName) = gfx_rt64_lua_get_string_field(L, index, "bumpMap", context);
    (*outNormalMapName) = gfx_rt64_lua_get_string_field(L, index, "normalMap", context);
    (*outSpecularMapName) = gfx_rt64_lua_get_string_field(L, index, "specularMap", context);

    const std::string modRoot = gfx_rt64_mod_texture_root((gLuaLoadingMod != nullptr) ? gLuaLoadingMod : gLuaActiveMod);
    const char *preferredRoot = modRoot.empty() ? nullptr : modRoot.c_str();
    if (!outNormalMapName->empty()) {
        gfx_rt64_register_map_texture(outNormalMapName->c_str(), preferredRoot);
    }

    if (!outSpecularMapName->empty()) {
        gfx_rt64_register_map_texture(outSpecularMapName->c_str(), preferredRoot);
    }
}

void gfx_rt64_lua_register_level_lights(lua_State *L, int levelNum, int areaIndex, int tableIndex) {
    static const char *const context = "gfx_rt64_set_level_lights";

    if ((levelNum < 0) || (levelNum >= RT64_MAX_LEVELS)) {
        LOG_LUA_LINE("%s: Level %d is out of range", context, levelNum);
        return;
    }

    const bool vanillaSlot = (levelNum >= LEVEL_MIN) && (levelNum < LEVEL_COUNT);
    if (!vanillaSlot && (smlua_level_util_get_info((s16)(levelNum)) == NULL)) {
        LOG_LUA_LINE("%s: Level %d is not a vanilla level or a currently registered custom level - use the value level_register() returned for this level, not a guessed number", context, levelNum);
        return;
    }

    if ((areaIndex < 0) || (areaIndex >= MAX_AREAS)) {
        LOG_LUA_LINE("%s: Area %d is out of range", context, areaIndex);
        return;
    }

    RT64_SCENE_DESC sceneDesc;
    {
        const std::lock_guard<std::mutex> lightingLock(RT64.levelAreaLightingMutex);
        sceneDesc = gfx_rt64_get_area_lighting(levelNum, areaIndex).sceneDesc;
    }

    bool hasScene = false;
    if (gfx_rt64_lua_get_table_field(L, tableIndex, "scene", context)) {
        gfx_rt64_lua_read_scene_description(L, lua_gettop(L), &sceneDesc, context);
        hasScene = true;
        lua_pop(L, 1);
    }

    std::vector<RT64_LIGHT> lights;
    bool hasLights = false;
    if (gfx_rt64_lua_get_table_field(L, tableIndex, "lights", context)) {
        hasLights = true;
        const int lightsTable = lua_gettop(L);
        const int lightCount = (int)(lua_rawlen(L, lightsTable));
        if (lightCount > RT64_MAX_LEVEL_LIGHTS) {
            LOG_LUA_LINE("%s: Level %d area %d has %d lights, only the first %d are used", context, levelNum, areaIndex, lightCount, RT64_MAX_LEVEL_LIGHTS);
        }

        for (int i = 1; i <= lightCount; i++) {
            if ((int)(lights.size()) >= RT64_MAX_LEVEL_LIGHTS) { break; }

            lua_rawgeti(L, lightsTable, i);
            if (lua_istable(L, -1)) {
                RT64_LIGHT light;
                gfx_rt64_lua_read_light(L, lua_gettop(L), &light, context);
                lights.push_back(light);
            } else {
                LOG_LUA_LINE("%s: Light %d must be a table, got %s", context, i, luaL_typename(L, -1));
            }

            lua_pop(L, 1);
        }

        lua_pop(L, 1);
    }

    gfx_rt64_set_level_lights(levelNum, areaIndex, hasLights ? &lights : nullptr, hasScene ? &sceneDesc : nullptr);
}

void gfx_rt64_lua_register_texture_mod(lua_State *L, const char *name, int tableIndex) {
    static const char *const context = "gfx_rt64_set_texture_mod";

    RT64_MATERIAL materialMod;
    RT64_LIGHT lightMod;
    bool hasMaterial = false, hasLight = false;
    std::string bumpMapName, normalMapName, specularMapName;
    gfx_rt64_lua_read_recorded_mod(L, tableIndex, &materialMod, &hasMaterial, &lightMod, &hasLight, &bumpMapName, &normalMapName, &specularMapName, context);

    gfx_rt64_set_texture_mod(name, hasMaterial ? &materialMod : nullptr, hasLight ? &lightMod : nullptr, bumpMapName, normalMapName, specularMapName);
}

void gfx_rt64_lua_register_geo_layout_mod(lua_State *L, const char *name, int tableIndex) {
    static const char *const context = "gfx_rt64_set_geo_layout_mod";

    gfx_rt64_load_mod_configs();

    RT64_MATERIAL materialMod;
    RT64_LIGHT lightMod;
    bool hasMaterial = false, hasLight = false;
    std::string bumpMapName, normalMapName, specularMapName;
    gfx_rt64_lua_read_recorded_mod(L, tableIndex, &materialMod, &hasMaterial, &lightMod, &hasLight, &bumpMapName, &normalMapName, &specularMapName, context);

    gfx_rt64_set_geo_layout_mod(name, hasMaterial ? &materialMod : nullptr, hasLight ? &lightMod : nullptr, bumpMapName, normalMapName, specularMapName);
}

//
// Lua writers
//

static std::string gfx_rt64_lua_float(float value) {
    char buffer[64];
    for (int digits = 1; digits < 9; digits++) {
        snprintf(buffer, sizeof(buffer), "%.*g", digits, value);
        if (strtof(buffer, nullptr) == value) { break; }
    }

    return std::string(buffer);
}

static std::string gfx_rt64_lua_vector3(const Vec3f &v) {
    return "{ " + gfx_rt64_lua_float(v[0]) + ", " + gfx_rt64_lua_float(v[1]) + ", " + gfx_rt64_lua_float(v[2]) + " }";
}

static std::string gfx_rt64_lua_color_component(float value) {
    const s32 clamped = (s32)(lroundf(value));
    return std::to_string((clamped < 0) ? 0 : ((clamped > 255) ? 255 : clamped));
}

static std::string gfx_rt64_lua_color3(const Vec3f &v) {
    return "{ " + gfx_rt64_lua_color_component(v[0]) + ", " + gfx_rt64_lua_color_component(v[1]) + ", " +
        gfx_rt64_lua_color_component(v[2]) + " }";
}

static std::string gfx_rt64_lua_color_mix(const Vec4f &v) {
    return "{ " + gfx_rt64_lua_color_component(v[0]) + ", " + gfx_rt64_lua_color_component(v[1]) + ", " +
        gfx_rt64_lua_color_component(v[2]) + ", " + gfx_rt64_lua_float(v[3]) + " }";
}

static std::string gfx_rt64_lua_int(float value) {
    return std::to_string((s32)(lroundf(value)));
}

static std::string gfx_rt64_lua_string(const std::string &value) {
    std::string out = "\"";
    for (char c : value) {
        if ((c == '"') || (c == '\\')) { out += '\\'; }
        out += c;
    }

    return out + "\"";
}

static void gfx_rt64_lua_field(std::ofstream &o, int depth, const char *name, const std::string &value) {
    o << std::string((size_t)(depth), '\t') << name << " = " << value << ",\n";
}

static void gfx_rt64_lua_write_light(std::ofstream &o, int depth, const RT64_LIGHT &light) {
    gfx_rt64_lua_field(o, depth, "position", gfx_rt64_lua_vector3(light.position));
    gfx_rt64_lua_field(o, depth, "diffuseColor", gfx_rt64_lua_color3(light.diffuseColor));
    gfx_rt64_lua_field(o, depth, "specularColor", gfx_rt64_lua_color3(light.specularColor));
    gfx_rt64_lua_field(o, depth, "attenuationRadius", gfx_rt64_lua_float(light.attenuationRadius));
    gfx_rt64_lua_field(o, depth, "pointRadius", gfx_rt64_lua_float(light.pointRadius));
    gfx_rt64_lua_field(o, depth, "shadowOffset", gfx_rt64_lua_float(light.shadowOffset));
    gfx_rt64_lua_field(o, depth, "attenuationExponent", gfx_rt64_lua_float(light.attenuationExponent));
    gfx_rt64_lua_field(o, depth, "flickerIntensity", gfx_rt64_lua_float(light.flickerIntensity));
    gfx_rt64_lua_field(o, depth, "groupBits", std::to_string((s32)(light.groupBits)));

    if (light.intensity != 1.0f) {
        gfx_rt64_lua_field(o, depth, "intensity", gfx_rt64_lua_float(light.intensity));
    }

    if (light.lightType == RT64_LIGHT_TYPE_POINT) {
        gfx_rt64_lua_field(o, depth, "type", gfx_rt64_lua_string("point"));
        gfx_rt64_lua_field(o, depth, "pitch", gfx_rt64_lua_float(light.pitch));
        gfx_rt64_lua_field(o, depth, "yaw", gfx_rt64_lua_float(light.yaw));
        gfx_rt64_lua_field(o, depth, "roll", gfx_rt64_lua_float(light.roll));
        gfx_rt64_lua_field(o, depth, "scaleX", gfx_rt64_lua_float(light.scaleX));
        gfx_rt64_lua_field(o, depth, "scaleY", gfx_rt64_lua_float(light.scaleY));
        gfx_rt64_lua_field(o, depth, "shape", gfx_rt64_lua_string((light.lightShape == RT64_LIGHT_SHAPE_SQUARE) ? "square" : "circle"));

        if (light.apertureEnabled) {
            gfx_rt64_lua_field(o, depth, "aperturePitch", gfx_rt64_lua_float(light.aperturePitch));
            gfx_rt64_lua_field(o, depth, "apertureYaw", gfx_rt64_lua_float(light.apertureYaw));
        }

        if (light.volumetricEnabled) {
            gfx_rt64_lua_field(o, depth, "volumetricIntensity", gfx_rt64_lua_float(light.volumetricIntensity));
        }
    }
}

static void gfx_rt64_lua_write_material_mod(std::ofstream &o, int depth, const RT64_MATERIAL &material) {
    #define RT64_MATERIAL_ATTR(flag, field, luaName, kind) RT64_WRITE_##kind(flag, luaName, field)
    #define RT64_WRITE_FLOAT(flag, name, field) \
        if (material.enabledAttributes & flag) { gfx_rt64_lua_field(o, depth, name, gfx_rt64_lua_float(material.field)); }
    #define RT64_WRITE_INT(flag, name, field) \
        if (material.enabledAttributes & flag) { gfx_rt64_lua_field(o, depth, name, gfx_rt64_lua_int(material.field)); }
    #define RT64_WRITE_COLOR3(flag, name, field) \
        if (material.enabledAttributes & flag) { gfx_rt64_lua_field(o, depth, name, gfx_rt64_lua_color3(material.field)); }
    #define RT64_WRITE_MASK(flag, name, field) \
        if (material.enabledAttributes & flag) { gfx_rt64_lua_field(o, depth, name, std::to_string((s32)(material.field))); }
    #define RT64_WRITE_COLOR_MIX(flag, name, field) \
        if (material.enabledAttributes & flag) { gfx_rt64_lua_field(o, depth, name, gfx_rt64_lua_color_mix(material.field)); }
    #include "gfx_rt64_material_attributes.inl"
    #undef RT64_WRITE_COLOR_MIX
    #undef RT64_WRITE_MASK
    #undef RT64_WRITE_COLOR3
    #undef RT64_WRITE_INT
    #undef RT64_WRITE_FLOAT
    #undef RT64_MATERIAL_ATTR

    if (material.enabledAttributes & RT64_ATTRIBUTE_SPECULAR_TINT) {
        gfx_rt64_lua_field(o, depth, "specularTint", (material.specularTint != 0) ? "true" : "false");
    }

    if (material.enabledAttributes & RT64_ATTRIBUTE_SHADOW_ENABLED) {
        gfx_rt64_lua_field(o, depth, "shadowEnabled", (material.shadowEnabled != 0) ? "true" : "false");
    }

    if (material.enabledAttributes & RT64_ATTRIBUTE_SHADOW_CENTER) {
        gfx_rt64_lua_field(o, depth, "shadowCenter", (material.shadowCenter != 0) ? "true" : "false");
    }

    if (material.enabledAttributes & RT64_ATTRIBUTE_SHADING_MODEL) {
        const char *shadingModel =
            (material.shadingModel == RT64_SHADING_MODEL_LAMBERT) ? "lambert" :
            (material.shadingModel == RT64_SHADING_MODEL_PHONG) ? "phong" : "blinn";
        gfx_rt64_lua_field(o, depth, "shadingModel", gfx_rt64_lua_string(shadingModel));
    }
}

static void gfx_rt64_lua_write_recorded_mod(std::ofstream &o, int depth, const RecordedMod *recordedMod) {
    const std::string indent((size_t)(depth), '\t');

    if ((recordedMod->materialMod != nullptr) && (recordedMod->materialMod->enabledAttributes != RT64_ATTRIBUTE_NONE)) {
        o << indent << "materialMod = {\n";
        gfx_rt64_lua_write_material_mod(o, depth + 1, *recordedMod->materialMod);
        o << indent << "},\n";
    }

    if (recordedMod->lightMod != nullptr) {
        o << indent << "lightMod = {\n";
        gfx_rt64_lua_write_light(o, depth + 1, *recordedMod->lightMod);
        o << indent << "},\n";
    }

    const std::string bumpName = gfx_rt64_texture_mod_name(recordedMod->bumpMapHash);
    if (!bumpName.empty()) {
        gfx_rt64_lua_field(o, depth, "bumpMap", gfx_rt64_lua_string(bumpName));
    }

    const std::string normName = gfx_rt64_texture_mod_name(recordedMod->normalMapHash);
    if (!normName.empty()) {
        gfx_rt64_lua_field(o, depth, "normalMap", gfx_rt64_lua_string(normName));
    }

    const std::string specName = gfx_rt64_texture_mod_name(recordedMod->specularMapHash);
    if (!specName.empty()) {
        gfx_rt64_lua_field(o, depth, "specularMap", gfx_rt64_lua_string(specName));
    }
}

static bool gfx_rt64_lua_open(std::ofstream &o, const std::string &path) {
    gfx_rt64_ensure_config_dir();
    o.open(path);
    if (!o.is_open()) {
        fprintf(stderr, "Unable to save %s.\n", path.c_str());
        return false;
    }
    return true;
}

static void gfx_rt64_lua_close(std::ofstream &o, const std::string &path) {
    o.flush();
    if (o.bad()) {
        fprintf(stderr, "Error when saving %s.\n", path.c_str());
    } else {
        fprintf(stderr, "Saved %s.\n", path.c_str());
    }
}

void gfx_rt64_save_level_lights(void) {
    const std::string path = gfx_rt64_level_lights_path();
    std::ofstream o;
    if (!gfx_rt64_lua_open(o, path)) { return; }

    std::map<u32, const AreaLighting *> sortedAreas;
    for (const auto &pair : RT64.levelAreaLighting) {
        sortedAreas[pair.first] = &pair.second;
    }

    for (const auto &pair : sortedAreas) {
        u32 levelNum, areaIndex;
        gfx_rt64_area_lighting_key_split(pair.first, &levelNum, &areaIndex);
        const AreaLighting &areaLighting = *pair.second;

        auto baselineIt = sBaselineAreaLighting.find(pair.first);
        const AreaLighting &baseline = (baselineIt != sBaselineAreaLighting.end()) ? baselineIt->second : RT64.defaultAreaLighting;
        if (gfx_rt64_area_lighting_matches(areaLighting, baseline)) { continue; }

        o << "gfx_rt64_set_level_lights(" << levelNum << ", " << areaIndex << ", {\n";

        o << "\tscene = {\n";
        const RT64_SCENE_DESC &scene = areaLighting.sceneDesc;
        gfx_rt64_lua_field(o, 2, "ambientBaseColor", gfx_rt64_lua_color3(scene.ambientBaseColor));
        gfx_rt64_lua_field(o, 2, "ambientNoGIColor", gfx_rt64_lua_color3(scene.ambientNoGIColor));
        gfx_rt64_lua_field(o, 2, "eyeLightDiffuseColor", gfx_rt64_lua_color3(scene.eyeLightDiffuseColor));
        gfx_rt64_lua_field(o, 2, "eyeLightSpecularColor", gfx_rt64_lua_color3(scene.eyeLightSpecularColor));
        gfx_rt64_lua_field(o, 2, "skyDiffuseMultiplier", gfx_rt64_lua_vector3(scene.skyDiffuseMultiplier));
        gfx_rt64_lua_field(o, 2, "skyHSLModifier", gfx_rt64_lua_vector3(scene.skyHSLModifier));
        gfx_rt64_lua_field(o, 2, "skyYawOffset", gfx_rt64_lua_float(scene.skyYawOffset));
        gfx_rt64_lua_field(o, 2, "giDiffuseStrength", gfx_rt64_lua_float(scene.giDiffuseStrength));
        gfx_rt64_lua_field(o, 2, "giSkyStrength", gfx_rt64_lua_float(scene.giSkyStrength));
        o << "\t},\n";

        o << "\tlights = {\n";
        for (int i = 0; i < areaLighting.lightCount; i++) {
            o << "\t\t{\n";
            gfx_rt64_lua_write_light(o, 3, areaLighting.lights[i]);
            o << "\t\t},\n";
        }
        o << "\t},\n";

        o << "})\n\n";
    }

    gfx_rt64_lua_close(o, path);
}

void gfx_rt64_save_geo_layout_mods(void) {
    const std::string path = gfx_rt64_geo_layout_mods_path();
    std::ofstream o;
    if (!gfx_rt64_lua_open(o, path)) { return; }

    std::set<std::string> savedNames;
    auto saveEntry = [&](const std::string &geoName, RecordedMod *geoMod) {
        if (gfx_rt64_recorded_mod_is_empty(geoMod)) { return; }
        if (!savedNames.insert(geoName).second) { return; }

        auto baselineIt = sBaselineGeoLayoutMods.find(geoName);
        if ((baselineIt != sBaselineGeoLayoutMods.end()) && (baselineIt->second == gfx_rt64_snapshot_mod(geoMod))) { return; }

        o << "gfx_rt64_set_geo_layout_mod(" << gfx_rt64_lua_string(geoName) << ", {\n";
        gfx_rt64_lua_write_recorded_mod(o, 1, geoMod);
        o << "})\n\n";
    };

    for (const auto &pair : RT64.nameGeoLayoutMap) {
        auto it = RT64.geoLayoutMods.find(pair.second);
        if (it != RT64.geoLayoutMods.end()) {
            saveEntry(pair.first, it->second);
        }
    }

    for (const auto &pair : RT64.pendingGeoLayoutMods) {
        saveEntry(pair.first, pair.second);
    }

    gfx_rt64_lua_close(o, path);
}

void gfx_rt64_save_texture_mods(void) {
    const std::string path = gfx_rt64_texture_mods_path();
    std::ofstream o;
    if (!gfx_rt64_lua_open(o, path)) { return; }

    for (const auto &pair : RT64.nameTexMap) {
        const std::string texName = pair.first;
        u64 texHash = pair.second;
        auto it = RT64.texMods.find(texHash);
        if (it == RT64.texMods.end()) { continue; }

        RecordedMod *texMod = it->second;
        const ModSnapshot snapshot = gfx_rt64_snapshot_mod(texMod);
        if (gfx_rt64_recorded_mod_is_empty(texMod)) { continue; }

        auto baselineIt = sBaselineTexMods.find(texHash);
        if ((baselineIt != sBaselineTexMods.end()) && (baselineIt->second == snapshot)) { continue; }

        o << "gfx_rt64_set_texture_mod(" << gfx_rt64_lua_string(texName) << ", {\n";
        gfx_rt64_lua_write_recorded_mod(o, 1, texMod);
        o << "})\n\n";
    }

    gfx_rt64_lua_close(o, path);
}

#endif
