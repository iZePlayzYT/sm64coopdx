#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "types.h"

struct lua_State;
struct ShaderProgram;
struct GfxRenderingAPI;

struct Rt64Light {
    Vec3f position;
    Vec3f diffuseColor;
    f32 attenuationRadius;
    f32 pointRadius;
    Vec3f specularColor;
    f32 shadowOffset;
    f32 attenuationExponent;
    f32 flickerIntensity;
    u32 groupBits;
    u32 lightType;
    f32 pitch;
    f32 yaw;
    f32 roll;
    f32 scaleX;
    f32 scaleY;
    u32 lightShape;
    u32 apertureEnabled;
    f32 aperturePitch;
    f32 apertureYaw;
    u32 volumetricEnabled;
    f32 volumetricIntensity;
    f32 intensity;
};

struct Rt64SceneDesc {
    Vec3f ambientBaseColor;
    Vec3f ambientNoGIColor;
    Vec3f eyeLightDiffuseColor;
    Vec3f eyeLightSpecularColor;
    Vec3f skyDiffuseMultiplier;
    Vec3f skyHSLModifier;
    f32 skyYawOffset;
    f32 giDiffuseStrength;
    f32 giSkyStrength;
};

#define RT64_LUA_MAX_AREA_LIGHTS 128

struct Rt64AreaLighting {
    struct Rt64SceneDesc scene;
    C_ARRAY struct Rt64Light lights[RT64_LUA_MAX_AREA_LIGHTS];
    s32 lightCount;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct GfxRenderingAPI gfx_rt64_api;

void gfx_rt64_reset_lua_config(void);
bool gfx_rt64_is_ready(void);

void gfx_rt64_lua_register_level_lights(struct lua_State *L, s32 levelNum, s32 areaIndex, s32 tableIndex);
void gfx_rt64_lua_register_texture_mod(struct lua_State *L, const char *name, s32 tableIndex);
void gfx_rt64_lua_register_geo_layout_mod(struct lua_State *L, const char *name, s32 tableIndex);
struct Rt64AreaLighting *gfx_rt64_lua_get_area_lighting(s32 levelNum, s32 areaIndex);

#if defined(_WIN32)

void gfx_rt64_set_xlu_depth_state(bool xluDepthWrite, bool zmodeXlu);
void gfx_rt64_set_fog(u8 fogR, u8 fogG, u8 fogB, s16 fogMul, s16 fogOffset);
void gfx_rt64_set_camera_perspective(f32 fovDegrees, f32 nearDist, f32 farDist, bool canInterpolate);
void gfx_rt64_set_camera_matrix(f32 matrix[4][4]);
void gfx_rt64_draw_triangles_ortho(f32 bufVbo[], size_t bufVboLen, size_t bufVboNumTris, bool doubleSided, u32 uid);
void gfx_rt64_draw_triangles_persp(f32 bufVbo[], size_t bufVboLen, size_t bufVboNumTris, f32 transformAffine[4][4], bool doubleSided, u32 uid);
void gfx_rt64_set_texture_gen(bool enabled, const f32 coeffU[4], const f32 coeffV[4]);
bool gfx_rt64_shader_uses_full_vertex_layout(struct ShaderProgram *prg);
bool gfx_rt64_set_skybox(const u8 *const *tiles, f32 diffuseColor[3]);
bool gfx_rt64_is_active(void);
void gfx_rt64_set_graph_node_mod(void *graphNodeMod);
void gfx_rt64_set_graph_node_root(void *graphNodeRoot);
void gfx_rt64_register_layout_graph_node(void *geoLayout, void *graphNode);
void gfx_rt64_inherit_graph_node_mod(void *originalGraphNode, void *replacementGraphNode);
void *gfx_rt64_build_graph_node_mod(void *graphNode, f32 modelviewMatrix[4][4], u32 uid);
void gfx_rt64_set_material_display_list(const void *displayList);
void gfx_rt64_save_lua_config(void);
void gfx_rt64_toggle_inspector(void);
bool gfx_rt64_inspector_active(void);
bool gfx_rt64_handle_window_message(void *hWnd, u32 message, uintptr_t wParam, intptr_t lParam);
void gfx_rt64_main_loop_iter(void (*runOneGameIter)(void));
void gfx_rt64_acquire_cpu_frame(void);
u64 gfx_rt64_get_generated_frame_count(void);

#else

static inline void gfx_rt64_set_xlu_depth_state(bool xluDepthWrite, bool zmodeXlu) { (void)(xluDepthWrite); (void)(zmodeXlu); }
static inline void gfx_rt64_set_fog(u8 fogR, u8 fogG, u8 fogB, s16 fogMul, s16 fogOffset) { (void)(fogR); (void)(fogG); (void)(fogB); (void)(fogMul); (void)(fogOffset); }
static inline void gfx_rt64_set_camera_perspective(f32 fovDegrees, f32 nearDist, f32 farDist, bool canInterpolate) { (void)(fovDegrees); (void)(nearDist); (void)(farDist); (void)(canInterpolate); }
static inline void gfx_rt64_set_camera_matrix(f32 matrix[4][4]) { (void)(matrix); }
static inline void gfx_rt64_draw_triangles_ortho(f32 bufVbo[], size_t bufVboLen, size_t bufVboNumTris, bool doubleSided, u32 uid) { (void)(bufVbo); (void)(bufVboLen); (void)(bufVboNumTris); (void)(doubleSided); (void)(uid); }
static inline void gfx_rt64_draw_triangles_persp(f32 bufVbo[], size_t bufVboLen, size_t bufVboNumTris, f32 transformAffine[4][4], bool doubleSided, u32 uid) { (void)(bufVbo); (void)(bufVboLen); (void)(bufVboNumTris); (void)(transformAffine); (void)(doubleSided); (void)(uid); }
static inline void gfx_rt64_set_texture_gen(bool enabled, const f32 coeffU[4], const f32 coeffV[4]) { (void)(enabled); (void)(coeffU); (void)(coeffV); }
static inline bool gfx_rt64_shader_uses_full_vertex_layout(struct ShaderProgram *prg) { (void)(prg); return false; }
static inline bool gfx_rt64_set_skybox(const u8 *const *tiles, f32 diffuseColor[3]) { (void)(tiles); (void)(diffuseColor); return false; }
static inline bool gfx_rt64_is_active(void) { return false; }
static inline void gfx_rt64_set_graph_node_mod(void *graphNodeMod) { (void)(graphNodeMod); }
static inline void gfx_rt64_set_graph_node_root(void *graphNodeRoot) { (void)(graphNodeRoot); }
static inline void gfx_rt64_register_layout_graph_node(void *geoLayout, void *graphNode) { (void)(geoLayout); (void)(graphNode); }
static inline void gfx_rt64_inherit_graph_node_mod(void *originalGraphNode, void *replacementGraphNode) { (void)(originalGraphNode); (void)(replacementGraphNode); }
static inline void *gfx_rt64_build_graph_node_mod(void *graphNode, f32 modelviewMatrix[4][4], u32 uid) { (void)(graphNode); (void)(modelviewMatrix); (void)(uid); return NULL; }
static inline void gfx_rt64_set_material_display_list(const void *displayList) { (void)(displayList); }
static inline void gfx_rt64_save_lua_config(void) { }
static inline void gfx_rt64_toggle_inspector(void) { }
static inline bool gfx_rt64_inspector_active(void) { return false; }
static inline bool gfx_rt64_handle_window_message(void *hWnd, u32 message, uintptr_t wParam, intptr_t lParam) { (void)(hWnd); (void)(message); (void)(wParam); (void)(lParam); return false; }
static inline void gfx_rt64_main_loop_iter(void (*runOneGameIter)(void)) { (void)(runOneGameIter); }
static inline void gfx_rt64_acquire_cpu_frame(void) { }
static inline u64 gfx_rt64_get_generated_frame_count(void) { return 0; }

#endif

#ifdef __cplusplus
}
#endif
