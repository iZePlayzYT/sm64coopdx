#if defined(_WIN32)

extern "C" {
#include "pc/configfile.h"
#include "pc/fs/fs.h"
#include "pc/platform.h"
#include "game/area.h"
#include "game/game_init.h"
#include "game/level_update.h"
#include "game/rendering_graph_node.h"
#include "goddard/gd_math.h"
#include "pc/mods/mod.h"
#include "pc/mods/mods.h"
#include "gfx_shader.h"
}

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <set>
#include <stdint.h>
#include <windows.h>

#include <SDL3/SDL.h>

#include <stb/stb_image.h>
#include "pc/utils/xxhash64.h"

#include "gfx_rendering_api.h"
#include "gfx_rt64.h"
#include "gfx_rt64_context.hpp"
#include "gfx_rt64_serialization.hpp"
#include "gfx_pc.h"

extern "C" {
#include "engine/math_util.h"
#include "game/object_helpers.h"
}

RT64Context RT64;

LARGE_INTEGER gfx_rt64_profile_marker(void) {
    LARGE_INTEGER marker;
    QueryPerformanceCounter(&marker);
    return marker;
}

LARGE_INTEGER gfx_rt64_profile_delta(LARGE_INTEGER start, LARGE_INTEGER end) {
    LARGE_INTEGER delta;
    delta.QuadPart = end.QuadPart - start.QuadPart;
    delta.QuadPart *= 1000000;
    delta.QuadPart /= RT64.frequency.QuadPart;
    return delta;
}

static void gfx_rt64_point_light_basis(f32 pitchDegrees, f32 yawDegrees, f32 rollDegrees, VEC_OUT Vec3f outForward, VEC_OUT Vec3f outRight, VEC_OUT Vec3f outUp) {
    const f32 pitch = degrees_to_radians(pitchDegrees);
    const f32 yaw = degrees_to_radians(yawDegrees);
    const f32 roll = degrees_to_radians(rollDegrees);

    const f32 sinYaw = sinf(yaw), cosYaw = cosf(yaw);
    Vec3f forwardYawed = { sinYaw, 0.0f, cosYaw };
    Vec3f rightYawed = { cosYaw, 0.0f, -sinYaw };
    Vec3f worldUp = { 0.0f, 1.0f, 0.0f };

    const f32 sinPitch = sinf(pitch), cosPitch = cosf(pitch);
    Vec3f upPitched;
    vec3f_combine(outForward, forwardYawed, worldUp, cosPitch, sinPitch);
    vec3f_combine(upPitched, forwardYawed, worldUp, -sinPitch, cosPitch);

    const f32 sinRoll = sinf(roll), cosRoll = cosf(roll);
    vec3f_combine(outRight, rightYawed, upPitched, cosRoll, sinRoll);
    vec3f_combine(outUp, rightYawed, upPitched, -sinRoll, cosRoll);
}

static void gfx_rt64_point_light_angles(Vec3f forward, Vec3f right, f32 *outPitch, f32 *outYaw, f32 *outRoll) {
    *outPitch = radians_to_degrees(asinf(fmaxf(fminf(forward[1], 1.0f), -1.0f)));
    *outYaw = radians_to_degrees(atan2f(forward[0], forward[2]));

    Vec3f rightYawed, upPitched, unusedForward;
    gfx_rt64_point_light_basis(*outPitch, *outYaw, 0.0f, unusedForward, rightYawed, upPitched);

    const f32 rollSin = vec3f_dot(right, upPitched);
    const f32 rollCos = vec3f_dot(right, rightYawed);
    *outRoll = radians_to_degrees(atan2f(rollSin, rollCos));
}

static void gfx_rt64_destroy_shader_program(ShaderProgramRT64 *prg) {
    for (auto &variant : prg->shaderVariantMap) {
        if (variant.second != nullptr) {
            RT64.lib.DestroyShader(variant.second);
        }
    }

    gfx_destroy_shader(prg->vertexShader);
    gfx_destroy_shader(prg->fragmentShader);
    delete prg;
}

void gfx_rt64_destroy_all_shaders(void) {
    const std::lock_guard<std::mutex> lock(RT64.shaderProgramsMutex);
    for (auto &pair : RT64.shaderPrograms) {
        gfx_rt64_destroy_shader_program(pair.second);
    }
    RT64.shaderPrograms.clear();

    for (ShaderProgramRT64 *prg : RT64.retiredShaderPrograms) {
        gfx_rt64_destroy_shader_program(prg);
    }
    RT64.retiredShaderPrograms.clear();

    RT64.shaderProgram = nullptr;

    RT64.lastShaderProgram = nullptr;
    RT64.lastShaderVariant = nullptr;
}

void gfx_rt64_capture_post_process_uniforms(void) {
    const std::lock_guard<std::mutex> lock(RT64.postProcessMutex);
    RT64.postProcessUniformBlocks.clear();
    RT64.postProcessUniformData.clear();

    if (RT64.postProcessShader == nullptr) { return; }

    for (int i = 0; i < RT64.postProcessShader->uniformBlockCount; i++) {
        const struct ShaderUniformBlock *block = &RT64.postProcessShader->uniformBlocks[i];
        if ((block->size == 0) || (block->buffer == nullptr)) { continue; }

        // As above - past the last register there is nowhere to put it, so say so rather than
        // letting the shader read zeroes and leaving no sign of why.
        if (block->location >= RT64_MAX_SHADER_UNIFORM_BLOCKS) {
            static bool sReported = false;
            if (!sReported) {
                sReported = true;
                fprintf(stderr, "RT64: the post process shader declares more uniform blocks than there are constant buffer registers for (%d). The ones past that read as zero.\n",
                    RT64_MAX_SHADER_UNIFORM_BLOCKS);
            }
            continue;
        }

        RT64_SHADER_UNIFORM_BLOCK entry;
        entry.shaderRegister = block->location;
        entry.size = block->size;
        entry.data = nullptr;
        RT64.postProcessUniformBlocks.push_back(entry);
        RT64.postProcessUniformData.insert(RT64.postProcessUniformData.end(), block->buffer, block->buffer + block->size);
    }

    size_t dataOffset = 0;
    for (RT64_SHADER_UNIFORM_BLOCK &entry : RT64.postProcessUniformBlocks) {
        entry.data = RT64.postProcessUniformData.data() + dataOffset;
        dataOffset += entry.size;
    }
}

static u16 gfx_rt64_shader_variant_key(bool raytrace, int filter, int hAddr, int vAddr, bool normalMap, bool specularMap, bool bumpMap) {
    u16 key = 0, fact = 1;
    key += raytrace ? fact : 0; fact *= 2;
    key += filter * fact; fact *= 2;
    key += hAddr * fact; fact *= 3;
    key += vAddr * fact; fact *= 3;
    key += normalMap ? fact : 0; fact *= 2;
    key += specularMap ? fact : 0; fact *= 2;
    key += bumpMap ? fact : 0; fact *= 2;
    return key;
}

static RT64_SHADER *gfx_rt64_render_thread_load_shader_variant(ShaderProgramRT64 *shaderProgram, bool raytrace, int filter, int hAddr, int vAddr, bool normalMap, bool specularMap, bool bumpMap) {
    const u16 variantKey = gfx_rt64_shader_variant_key(raytrace, filter, hAddr, vAddr, normalMap, specularMap, bumpMap);
    if ((RT64.lastShaderVariant != nullptr) && (RT64.lastShaderProgram == shaderProgram) && (RT64.lastShaderVariantKey == variantKey)) {
        return RT64.lastShaderVariant;
    }

    if (shaderProgram->shaderVariantMap[variantKey] == nullptr) {
        int flags = raytrace ? RT64_SHADER_RAYTRACE_ENABLED : RT64_SHADER_RASTER_ENABLED;
        if (normalMap) {
            flags |= RT64_SHADER_NORMAL_MAP_ENABLED;
        }

        if (specularMap) {
            flags |= RT64_SHADER_SPECULAR_MAP_ENABLED;
        }

        if (bumpMap) {
            flags |= RT64_SHADER_BUMP_MAP_ENABLED;
        }

        const bool useCustomSource = shaderProgram->hasCustomShader &&
                                     !shaderProgram->customShaderFailed.load(std::memory_order_relaxed);
        if (useCustomSource) {
            const char *fragmentOutputName = (shaderProgram->fragmentShader != nullptr) ? shaderProgram->fragmentShader->shaderOutputs[0].name : nullptr;
            RT64_SHADER *customShader = RT64.lib.CreateShaderFromSource(RT64.device, shaderProgram->cc, shaderProgram->customVertexHLSL.c_str(), shaderProgram->customFragmentHLSL.c_str(), shaderProgram->customVertexInputs.data(), (unsigned int)(shaderProgram->customVertexInputs.size()), fragmentOutputName, filter, hAddr, vAddr, flags);

            if (customShader == nullptr) {
                shaderProgram->customShaderFailed.store(true, std::memory_order_relaxed);
                const char *rt64Error = (RT64.lib.GetLastError != nullptr) ? RT64.lib.GetLastError() : nullptr;
                fprintf(stderr, "RT64: could not build a shader from custom source, using the built-in shader instead. %s\n",
                    (rt64Error != nullptr) ? rt64Error : "No error message was reported.");
                customShader = RT64.lib.CreateShader(RT64.device, shaderProgram->cc, filter, hAddr, vAddr, flags);
            }

            shaderProgram->shaderVariantMap[variantKey] = customShader;
        } else {
            shaderProgram->shaderVariantMap[variantKey] = RT64.lib.CreateShader(RT64.device, shaderProgram->cc, filter, hAddr, vAddr, flags);
        }
    }

    RT64_SHADER *shader = shaderProgram->shaderVariantMap[variantKey];
    RT64.lastShaderProgram = shaderProgram;
    RT64.lastShaderVariantKey = variantKey;
    RT64.lastShaderVariant = shader;
    return shader;
}

static void gfx_rt64_render_thread_apply_post_process_shader(void) {
    if (RT64.device == nullptr) { return; }

    {
        const std::lock_guard<std::mutex> lock(RT64.postProcessMutex);
        RT64.lib.SetPostProcessUniforms(RT64.device,
            RT64.postProcessUniformBlocks.empty() ? nullptr : RT64.postProcessUniformBlocks.data(),
            (unsigned int)(RT64.postProcessUniformBlocks.size()));
    }

    if (!RT64.postProcessDirty.load(std::memory_order_acquire)) { return; }

    const std::lock_guard<std::mutex> lock(RT64.postProcessMutex);
    if (RT64.postProcessHLSL.empty()) {
        RT64.lib.SetPostProcessShader(RT64.device, nullptr, nullptr, 0, nullptr, 0, 0);
    } else {
        RT64.lib.SetPostProcessShader(RT64.device, RT64.postProcessHLSL.c_str(),
            RT64.postProcessInputs.data(), (unsigned int)(RT64.postProcessInputs.size()),
            RT64.postProcessOutputName.c_str(), RT64.postProcessWidth, RT64.postProcessHeight);
    }

    RT64.postProcessDirty.store(false, std::memory_order_release);
}

void gfx_rt64_destroy_gpu_mesh(GPUMesh &mesh) {
    if (mesh.mesh != nullptr) {
        RT64.lib.DestroyMesh(mesh.mesh);
        mesh.mesh = nullptr;
        RT64.meshesDestroyed++;
    }
}

static RT64_TEXTURE *gfx_rt64_render_thread_find_texture(u32 textureKey) {
    if (textureKey == 0) {
        return RT64.blankTexture;
    }

    auto texIt = RT64.gpuTextures.find(textureKey);
    return ((texIt != RT64.gpuTextures.end()) && (texIt->second.texture != nullptr)) ? texIt->second.texture : RT64.blankTexture;
}

static inline void gfx_rt64_render_thread_add_light(RT64_LIGHT srcLight, const Mat4 &transformSrc) {
    if (RT64.renderLightCount >= RT64_MAX_LIGHTS) {
        return;
    }

    Mat4 transform;
    memcpy(transform, transformSrc, sizeof(Mat4));

    auto &dstLight = RT64.renderLights[RT64.renderLightCount++];
    dstLight = srcLight;

    linear_mtxf_mul_vec3f(transform, dstLight.position, srcLight.position);
    vec3f_add(dstLight.position, transform[3]);

    Vec3f scaleVector;
    linear_mtxf_mul_vec3f(transform, scaleVector, gVec3fOne);
    f32 scale = vec3f_length(scaleVector) / sqrtf(3.0f);
    dstLight.attenuationRadius *= scale;
    dstLight.pointRadius *= scale;
    dstLight.shadowOffset *= scale;

    if (srcLight.lightType == RT64_LIGHT_TYPE_POINT) {
        Vec3f localForward, localRight, localUp;
        gfx_rt64_point_light_basis(srcLight.pitch, srcLight.yaw, srcLight.roll, localForward, localRight, localUp);

        Vec3f worldForward, worldRight;
        linear_mtxf_mul_vec3f(transform, worldForward, localForward);
        linear_mtxf_mul_vec3f(transform, worldRight, localRight);
        vec3f_normalize(worldForward);
        vec3f_normalize(worldRight);
        gfx_rt64_point_light_angles(worldForward, worldRight, &dstLight.pitch, &dstLight.yaw, &dstLight.roll);

        if (srcLight.apertureEnabled) {
            Vec3f localApertureNormal, unusedApertureRight, unusedApertureUp;
            gfx_rt64_point_light_basis(srcLight.aperturePitch, srcLight.apertureYaw, 0.0f, localApertureNormal, unusedApertureRight, unusedApertureUp);

            Vec3f worldApertureNormal;
            linear_mtxf_mul_vec3f(transform, worldApertureNormal, localApertureNormal);
            vec3f_normalize(worldApertureNormal);
            f32 unusedApertureRoll;
            gfx_rt64_point_light_angles(worldApertureNormal, worldRight, &dstLight.aperturePitch, &dstLight.apertureYaw, &unusedApertureRoll);
        }

        dstLight.scaleX *= scale;
        dstLight.scaleY *= scale;
    }
}

static const GPUMesh sEmptyGPUMesh = {};

static void gfx_rt64_render_thread_upload_mesh(GPUMesh &dstMesh, const GameMesh &curMesh) {
    if (dstMesh.vertexBufferHash == curMesh.vertexBufferHash) {
        return;
    }

    if ((dstMesh.positionHash != 0) && (dstMesh.positionHash == curMesh.positionHash)) {
        RT64.lib.SetMeshVertexData(dstMesh.mesh, curMesh.vertexBuffer, curMesh.vertexCount, curMesh.vertexStride, RT64.indexTriangleList, curMesh.indexCount);
    } else {
        RT64.lib.SetMesh(dstMesh.mesh, curMesh.vertexBuffer, curMesh.vertexCount, curMesh.vertexStride, RT64.indexTriangleList, curMesh.indexCount);
    }

    dstMesh.vertexCount = curMesh.vertexCount;
    dstMesh.vertexStride = curMesh.vertexStride;
    dstMesh.indexCount = curMesh.indexCount;
    dstMesh.vertexBufferHash = curMesh.vertexBufferHash;
    dstMesh.positionHash = curMesh.positionHash;
}

template <typename Pool>
static void gfx_rt64_render_thread_evict_meshes(Pool &pool, int evictFrames) {
    auto poolIt = pool.begin();
    while (poolIt != pool.end()) {
        GPUMesh &pooledMesh = poolIt->second;
        if (pooledMesh.inUse) {
            pooledMesh.inUse = false;
            pooledMesh.unusedFrames = 0;
            poolIt++;
        } else if (++pooledMesh.unusedFrames >= evictFrames) {
            gfx_rt64_destroy_gpu_mesh(pooledMesh);
            poolIt = pool.erase(poolIt);
        } else {
            poolIt++;
        }
    }
}

static inline void gfx_rt64_render_thread_draw_display_list(u32 uid, GameFrame *curFrame) {
    auto &gpuDl = RT64.gpuDisplayLists[uid];
    const auto &curDisplayList = curFrame->displayLists.find(uid)->second;

    for (int i = 0; i < curDisplayList.drawCount; i++) {
        auto &dstInstance = gpuDl.instances[i];
        const auto &curMesh = curDisplayList.meshes[i];
        const auto &curInstance = curDisplayList.instances[i];

        RT64_MESH *usedMesh = nullptr;
        if (curMesh.raytrace) {
            auto staticIt = RT64.gpuStaticMeshes.find(curMesh.vertexBufferHash);
            const GPUMesh &staticMesh = (staticIt != RT64.gpuStaticMeshes.end()) ? staticIt->second : sEmptyGPUMesh;
            if (staticMesh.mesh != nullptr) {
                usedMesh = staticMesh.mesh;
                RT64.staticMeshesDrawn++;
            }
        }

        if (usedMesh == nullptr) {
            auto &dynamicMeshPool = curMesh.raytrace ? RT64.gpuDynamicRtMeshes : RT64.gpuDynamicRasterMeshes;
            u64 sizeKey = (u64)(curMesh.vertexCount);
            sizeKey = sizeKey * 0x9E3779B185EBCA87ull + curMesh.vertexStride;
            sizeKey = sizeKey * 0x9E3779B185EBCA87ull + curMesh.indexCount;

            const u64 ownerKey = ((u64)(uid) << 32) | (u32)(i);

            GPUMesh *exactMesh = nullptr;
            GPUMesh *sameOwnerMesh = nullptr;
            GPUMesh *anyFreeMesh = nullptr;
            auto poolRange = dynamicMeshPool.equal_range(sizeKey);
            for (auto poolIt = poolRange.first; poolIt != poolRange.second; ++poolIt) {
                GPUMesh &candidate = poolIt->second;
                if (candidate.inUse || (candidate.raytrace != curMesh.raytrace)) {
                    continue;
                }
                if (candidate.vertexBufferHash == curMesh.vertexBufferHash) {
                    exactMesh = &candidate;
                    break;
                }
                if ((sameOwnerMesh == nullptr) && (candidate.ownerKey == ownerKey)) {
                    sameOwnerMesh = &candidate;
                }
                if (anyFreeMesh == nullptr) {
                    anyFreeMesh = &candidate;
                }
            }

            GPUMesh *dynamicMesh = exactMesh ? exactMesh : (sameOwnerMesh ? sameOwnerMesh : anyFreeMesh);
            if (dynamicMesh == nullptr) {
                auto inserted = dynamicMeshPool.emplace(sizeKey, GPUMesh{});
                dynamicMesh = &inserted->second;
                dynamicMesh->mesh = RT64.lib.CreateMesh(RT64.device, curMesh.raytrace ? (RT64_MESH_RAYTRACE_ENABLED | RT64_MESH_RAYTRACE_UPDATABLE) : 0);
                dynamicMesh->vertexCount = curMesh.vertexCount;
                dynamicMesh->vertexStride = curMesh.vertexStride;
                dynamicMesh->indexCount = curMesh.indexCount;
                dynamicMesh->raytrace = curMesh.raytrace;
                RT64.meshesCreated++;
            }

            gfx_rt64_render_thread_upload_mesh(*dynamicMesh, curMesh);

            dynamicMesh->ownerKey = ownerKey;
            dynamicMesh->inUse = true;
            usedMesh = dynamicMesh->mesh;

            RT64.dynamicMeshesDrawn++;
        }

        assert(usedMesh != nullptr);

        // Create the instance if it doesn't exist yet.
        if (dstInstance.instance == nullptr) {
            dstInstance.instance = RT64.lib.CreateInstance(RT64.scene);
            memcpy(dstInstance.transform, curInstance.desc.transform, sizeof(Mat4));
        }

        // Update the instance.
        RT64_INSTANCE_DESC instDesc = curInstance.desc;
        instDesc.diffuseTexture = gfx_rt64_render_thread_find_texture(curInstance.textures.diffuse);
        instDesc.normalTexture = gfx_rt64_render_thread_find_texture(curInstance.textures.normal);
        instDesc.specularTexture = gfx_rt64_render_thread_find_texture(curInstance.textures.specular);
        instDesc.diffuse2Texture = gfx_rt64_render_thread_find_texture(curInstance.textures.diffuse2);
        instDesc.bumpTexture = gfx_rt64_render_thread_find_texture(curInstance.textures.bump);
        instDesc.mesh = usedMesh;

        instDesc.material.lockMask = 0.0f;

        // Assign the shader to the instance. Create if necessary.
        const auto &shader = curInstance.shader;
        instDesc.shader = gfx_rt64_render_thread_load_shader_variant(shader.program, shader.raytrace, shader.filter, shader.hAddr, shader.vAddr, shader.normalMap, shader.specularMap, shader.bumpMap);

        instDesc.shaderUniformBlocks = curInstance.uniformBlocks.empty() ? nullptr : curInstance.uniformBlocks.data();
        instDesc.shaderUniformBlockCount = (unsigned int)(curInstance.uniformBlocks.size());

        const f32 minDot = sqrtf(2.0f) / -2.0f;
        memcpy(instDesc.transform, curInstance.desc.transform, sizeof(Mat4));
        if (mtxf_axes_align(dstInstance.transform, instDesc.transform, minDot)) {
            mtxf_copy(instDesc.previousTransform, dstInstance.transform);
        } else {
            mtxf_copy(instDesc.previousTransform, instDesc.transform);
        }

        // Update the instance.
        RT64.lib.SetInstanceDescription(dstInstance.instance, &instDesc);
        mtxf_copy(dstInstance.transform, instDesc.transform);

        // Apply the display list instance light (if applicable).
        if (curInstance.light.groupBits > 0) {
            gfx_rt64_render_thread_add_light(curInstance.light, instDesc.transform);
        }

        gpuDl.drawCount++;
    }

    // Apply the display list light (if applicable).
    if (curDisplayList.light.groupBits > 0) {
        gfx_rt64_render_thread_add_light(curDisplayList.light, curDisplayList.transform);
    }
}

static void gfx_rt64_render_thread_draw_frame(GameFrame *curFrame, GameFrame *prevFrame, f32 deltaTimeMs) {
    LARGE_INTEGER elapsedMicro;

    RT64.staticMeshesDrawn = 0;
    RT64.dynamicMeshesDrawn = 0;
    RT64.meshesCreated = 0;
    RT64.meshesDestroyed = 0;

    // Copy the frame's static lights.
    memcpy(RT64.renderLights, curFrame->areaLights, sizeof(RT64_LIGHT) * curFrame->areaLightCount);
    RT64.renderLightCount = curFrame->areaLightCount;

    // Reset the draw counter for all active display lists.
    LARGE_INTEGER dlStart = gfx_rt64_profile_marker();
    for (auto &gpuDlPair : RT64.gpuDisplayLists) { gpuDlPair.second.drawCount = 0; }

    // Queue up all display lists first.
    RT64.lib.BeginMeshBatch(RT64.device);
    for (auto &dlPair : curFrame->displayLists) {
        gfx_rt64_render_thread_draw_display_list(dlPair.first, curFrame);
    }
    RT64.lib.EndMeshBatch(RT64.device);

    // Clean up any unused instances or meshes from the GPU display lists.
    auto gpuDlIt = RT64.gpuDisplayLists.begin();
    while (gpuDlIt != RT64.gpuDisplayLists.end()) {
        auto &dl = gpuDlIt->second;

        // Destroy all unused instances.
        while (dl.instances.size() > (size_t)(dl.drawCount)) {
            if (dl.instances.back().instance != nullptr) {
                RT64.lib.DestroyInstance(dl.instances.back().instance);
            }
            dl.instances.pop_back();
        }

        if (dl.drawCount > 0) {
            dl.idleFrames = 0;
        } else {
            dl.idleFrames++;
        }

        if (dl.idleFrames >= RT64_CACHED_MESH_EVICT_FRAMES) {
            gpuDlIt = RT64.gpuDisplayLists.erase(gpuDlIt);
        } else {
            gpuDlIt++;
        }
    }

    LARGE_INTEGER dlEnd = gfx_rt64_profile_marker();
    elapsedMicro = gfx_rt64_profile_delta(dlStart, dlEnd);
    double dlMs = elapsedMicro.QuadPart / 1000.0;

    const bool canReprojectView = curFrame->canReprojectView &&
        mtxf_axes_align(prevFrame->viewMatrix, curFrame->viewMatrix, 0.0f);

    RT64.lib.SetViewPerspective(RT64.view, curFrame->viewMatrix, curFrame->fovRadians, curFrame->nearDist, curFrame->farDist, canReprojectView);

    for (unsigned int i = 0; i < RT64.renderLightCount; i++) {
        const float minAttenuationExponent = 0.001f;
        if (!(RT64.renderLights[i].attenuationExponent > minAttenuationExponent)) {
            RT64.renderLights[i].attenuationExponent = minAttenuationExponent;
        }
    }

    // Update the scene.
    RT64.lib.SetSceneLights(RT64.scene, RT64.renderLights, RT64.renderLightCount);
    RT64.lib.SetSceneDescription(RT64.scene, curFrame->sceneDesc);

    RT64_TEXTURE *skyTexture = nullptr;
    if (curFrame->skyTextureKey != 0) {
        auto skyTexIt = RT64.gpuTextures.find(curFrame->skyTextureKey);
        if (skyTexIt != RT64.gpuTextures.end()) {
            skyTexture = skyTexIt->second.texture;
        }
    }
    RT64.lib.SetViewSkyPlane(RT64.view, skyTexture);

    // Additional information.
    if ((RT64.renderInspector != nullptr) && RT64.renderInspectorActive) {
        char dlMsMessage[128];
        snprintf(dlMsMessage, sizeof(dlMsMessage), "RENDER DL: %.3f ms\n", dlMs);
        RT64.lib.PrintMessageInspector(RT64.renderInspector, dlMsMessage);

        char infoMessage[128];
        snprintf(infoMessage, sizeof(infoMessage), "ST %u DYN %u CRE %u DTY %u\n", RT64.staticMeshesDrawn, RT64.dynamicMeshesDrawn, RT64.meshesCreated, RT64.meshesDestroyed);
        RT64.lib.PrintMessageInspector(RT64.renderInspector, infoMessage);
    }

    // Draw everything and update the window.
    RT64.lib.DrawDevice(RT64.device, gfx_rt64_use_vsync() ? 1 : 0, deltaTimeMs);

    for (auto *pool : { &RT64.gpuDynamicRasterMeshes, &RT64.gpuDynamicRtMeshes }) {
        gfx_rt64_render_thread_evict_meshes(*pool, RT64_CACHED_MESH_EVICT_FRAMES);
    }
}

static void gfx_rt64_render_thread_preprocess_frames(GameFrame *curFrame, GameFrame *prevFrame) {
    // Right click allows to pick a texture for editing from the viewport.
    RT64_INSTANCE *pickSearchInstance = nullptr;
    bool pickSearchTexture = false;
    bool pickSearchGeoLayout = false;
    if (RT64.renderInspectorActive) {
        const std::lock_guard<std::mutex> pickLock(RT64.pickTextureMutex);
        if (RT64.pickTexture || RT64.pickGeoLayout) {
            RT64_INSTANCE *instance = RT64.lib.GetViewRaytracedInstanceAt(RT64.view, RT64.pickCursorX, RT64.pickCursorY);
            if (instance != nullptr) {
                pickSearchInstance = instance;
                pickSearchTexture = RT64.pickTexture;
                pickSearchGeoLayout = RT64.pickGeoLayout;
            } else {
                if (RT64.pickTexture) { RT64.pickTextureHash = 0; }
            }

            RT64.pickTexture = false;
            RT64.pickGeoLayout = false;
        }
    }

    int remainingStaticMeshesForCache = RT64_CACHED_MESH_MAX_PER_FRAME;
    for (auto &dlPair : curFrame->displayLists) {
        auto &gpuDl = RT64.gpuDisplayLists[dlPair.first];
        const auto &curDisplayList = dlPair.second;

        // Make the vectors large enough to fit all the instances and meshes.
        if (gpuDl.instances.size() < (size_t)(curDisplayList.drawCount)) {
            gpuDl.instances.resize(curDisplayList.drawCount);
        }

        // Search for the matching instance inside the DLs and find the hash corresponding to the diffuse texture.
        if (pickSearchInstance != nullptr) {
            for (int i = 0; (i < gpuDl.drawCount) && (i < (int)(gpuDl.instances.size())); i++) {
                if (gpuDl.instances[i].instance == pickSearchInstance) {
                    const std::lock_guard<std::mutex> pickLock(RT64.pickTextureMutex);

                    if (pickSearchTexture) {
                        RT64.pickTextureHash = curDisplayList.instances[i].textureHash;
                    }

                    if (pickSearchGeoLayout && (curDisplayList.instances[i].geoLayout != nullptr)) {
                        RT64.pickedGeoLayout = curDisplayList.instances[i].geoLayout;
                    }

                    pickSearchInstance = nullptr;
                    break;
                }
            }
        }

        for (int i = 0; i < curDisplayList.drawCount; i++) {
            const auto &curMesh = curDisplayList.meshes[i];
            if (!curMesh.raytrace) {
                continue;
            }

            RT64.curTickMeshHashes.insert(curMesh.vertexBufferHash);

            if (RT64.prevTickMeshHashes.find(curMesh.vertexBufferHash) == RT64.prevTickMeshHashes.end()) {
                continue;
            }

            auto &staticMesh = RT64.gpuStaticMeshes[curMesh.vertexBufferHash];

            staticMesh.inUse = true;
            if (staticMesh.mesh != nullptr) {
                continue;
            }

            if (staticMesh.staticFrames < RT64_CACHED_MESH_REQUIRED_FRAMES) {
                staticMesh.staticFrames++;
                continue;
            }

            if (remainingStaticMeshesForCache <= 0) {
                continue;
            }

            staticMesh.mesh = RT64.lib.CreateMesh(RT64.device, RT64_MESH_RAYTRACE_ENABLED | RT64_MESH_RAYTRACE_FAST_TRACE);
            staticMesh.vertexBufferHash = curMesh.vertexBufferHash;
            RT64.lib.SetMesh(staticMesh.mesh, curMesh.vertexBuffer, curMesh.vertexCount, curMesh.vertexStride, RT64.indexTriangleList, curMesh.indexCount);

            remainingStaticMeshesForCache--;
        }
    }

    RT64.prevTickMeshHashes.swap(RT64.curTickMeshHashes);
    RT64.curTickMeshHashes.clear();

    if (pickSearchInstance != nullptr) {
        const std::lock_guard<std::mutex> pickLock(RT64.pickTextureMutex);
        if (pickSearchTexture) { RT64.pickTextureHash = 0; }
    }

    gfx_rt64_render_thread_evict_meshes(RT64.gpuStaticMeshes, RT64_CACHED_MESH_EVICT_FRAMES);
}

static void gfx_rt64_render_thread_handle_messages(void) {
    std::queue<InspectorMessage> messages;
    {
        const std::lock_guard<std::mutex> lock(RT64.inspectorMessageQueueMutex);
        messages.swap(RT64.inspectorMessageQueue);
    }

    if (RT64.renderInspector == nullptr) {
        return;
    }

    while (!messages.empty()) {
        const InspectorMessage &m = messages.front();
        RT64.lib.HandleMessageInspector(RT64.renderInspector, m.message, (WPARAM)(m.wParam), (LPARAM)(m.lParam));
        messages.pop();
    }
}

static void gfx_rt64_render_thread_upload_texture_queue(void) {
    std::queue<UploadTexture> textureUploadQueue;
    {
        const std::lock_guard<std::mutex> lock(RT64.textureUploadQueueMutex);
        textureUploadQueue.swap(RT64.textureUploadQueue);
    }

    while (!textureUploadQueue.empty()) {
        UploadTexture uploadTexture = textureUploadQueue.front();
        textureUploadQueue.pop();

        auto &gpuTexture = RT64.gpuTextures[uploadTexture.key];
        gpuTexture.hash = uploadTexture.hash;

        auto existingIt = RT64.hashToTexture.find(uploadTexture.contentHash);
        if (existingIt != RT64.hashToTexture.end()) {
            gpuTexture.texture = existingIt->second;
        } else {
            gpuTexture.texture = RT64.lib.CreateTexture(RT64.device, uploadTexture.desc);
            RT64.hashToTexture[uploadTexture.contentHash] = gpuTexture.texture;
        }

        free(uploadTexture.desc.bytes);
    }
}

void gfx_rt64_render_thread(void) {
    LARGE_INTEGER frameStart, frameEnd, preprocessStart, preprocessEnd, elapsedMicro;

    // Setup scene and view.
    RT64.scene = RT64.lib.CreateScene(RT64.device);
    RT64.view = RT64.lib.CreateView(RT64.scene);

    {
        const std::lock_guard<std::mutex> lock(RT64.renderViewDescMutex);
        RT64.renderViewDescChanged = true;
    }

    // Draw at least one empty frame to fill the window.
    RT64.lib.DrawDevice(RT64.device, gfx_rt64_use_vsync() ? 1 : 0, 0.0f);

    // Preload a blank texture.
    const int blankTextureSize = 64;
    int blankBytesCount = blankTextureSize * blankTextureSize * 4;
    unsigned char *blankBytes = (unsigned char *)(malloc(blankBytesCount));
    if (blankBytes == nullptr) {
        sys_fatal("RT64: failed to allocate the blank texture, ran out of memory.");
    }
    memset(blankBytes, 0xFF, blankBytesCount);

    RT64_TEXTURE_DESC texDesc = {};
    texDesc.bytes = blankBytes;
    texDesc.byteCount = blankBytesCount;
    texDesc.format = RT64_TEXTURE_FORMAT_RGBA8;
    texDesc.width = blankTextureSize;
    texDesc.height = blankTextureSize;
    texDesc.rowPitch = texDesc.width * 4;
    RT64.blankTexture = RT64.lib.CreateTexture(RT64.device, texDesc);
    free(blankBytes);

    // Upload any pending textures that the game has already queued up.
    gfx_rt64_render_thread_upload_texture_queue();

    // Unpause the game once the render thread has finished loading.
    RT64.pauseMode = false;

    int curFrameIndex = -1;
    int prevFrameIndex = -1;
    double frameDeltaTimeMs = 0.0;
    double interFrameDeltaTimeMs = 1000.0 / 60.0;
    LARGE_INTEGER lastFrameStart = {};
    while (RT64.renderThreadRunning) {
        // Create or destroy the inspector depending on the current state of the flag.
        if (RT64.renderInspectorActive && (RT64.renderInspector == nullptr)) {
            RT64.renderInspector = RT64.lib.CreateInspector(RT64.device);

            if (RT64.lib.SetInspectorMaterialDefaults != nullptr) {
                RT64.lib.SetInspectorMaterialDefaults(RT64.renderInspector, &RT64.defaultMaterial);
            }
        } else if (!RT64.renderInspectorActive && (RT64.renderInspector != nullptr)) {
            RT64.lib.DestroyInspector(RT64.renderInspector);
            RT64.renderInspector = nullptr;
        }

        gfx_rt64_render_thread_handle_messages();

        // Update the view description if modified.
        {
            const std::lock_guard<std::mutex> lock(RT64.renderViewDescMutex);
            if (RT64.renderViewDescChanged) {
                RT64.lib.SetViewDescription(RT64.view, RT64.renderViewDesc);
                RT64.renderViewDescChanged = false;
            } else if (RT64.renderInspectorActive && (RT64.lib.GetViewDescription != nullptr)) {
                RT64.lib.GetViewDescription(RT64.view, &RT64.inspectorViewDesc);
                RT64.inspectorViewDescValid = true;
            }
        }

        {
            std::unique_lock<std::mutex> lock(RT64.renderFrameIndexMutex);
            RT64.renderFrameCV.wait(lock, [] { return !RT64.renderThreadRunning || !RT64.pendingFrameIndices.empty(); });

            curFrameIndex = -1;
            if (!RT64.pendingFrameIndices.empty()) {
                curFrameIndex = RT64.pendingFrameIndices.front();
                RT64.pendingFrameIndices.pop_front();
                prevFrameIndex = (curFrameIndex == 0) ? (RT64_MAX_RENDER_FRAMES - 1) : (curFrameIndex - 1);
                RT64.gpuFrameIndex = curFrameIndex;
                RT64.barrierFrameIndex = prevFrameIndex;
            }
        }
        RT64.renderFrameCV.notify_all();

        gfx_rt64_render_thread_upload_texture_queue();

        if (curFrameIndex >= 0) {
            // Run any necessary preprocessing.
            preprocessStart = gfx_rt64_profile_marker();
            gfx_rt64_render_thread_preprocess_frames(&RT64.frames[curFrameIndex], &RT64.frames[prevFrameIndex]);
            preprocessEnd = gfx_rt64_profile_marker();
            elapsedMicro = gfx_rt64_profile_delta(preprocessStart, preprocessEnd);
            double preprocessTimeMs = elapsedMicro.QuadPart / 1000.0;

            // Print to the inspector the previous time it took to draw a frame.
            if ((RT64.renderInspector != nullptr) && RT64.renderInspectorActive) {
                const std::lock_guard<std::mutex> lock(RT64.renderInspectorMutex);
                char preprocessTimeMsg[64], renderDeltaTimeMsg[64];
                snprintf(preprocessTimeMsg, sizeof(preprocessTimeMsg), "RENDER PREPROCESS: %.3f ms\n", preprocessTimeMs);
                snprintf(renderDeltaTimeMsg, sizeof(renderDeltaTimeMsg), "RENDER FRAME: %.3f ms\n", frameDeltaTimeMs);
                RT64.lib.PrintClearInspector(RT64.renderInspector);
                RT64.lib.PrintMessageInspector(RT64.renderInspector, preprocessTimeMsg);
                RT64.lib.PrintMessageInspector(RT64.renderInspector, renderDeltaTimeMsg);
                for (const std::string &message : RT64.renderInspectorMessages) {
                    RT64.lib.PrintMessageInspector(RT64.renderInspector, message.c_str());
                }
                const std::lock_guard<std::mutex> lightingLock(RT64.levelAreaLightingMutex);
                const std::lock_guard<std::mutex> pickLock(RT64.pickTextureMutex);
                int levelIndex = gfx_rt64_get_level_index();
                int areaIndex = gfx_rt64_get_area_index();

                AreaLighting &areaLighting = gfx_rt64_get_or_add_area_lighting(levelIndex, areaIndex);

                // Inspect the current scene.
                RT64.lib.SetSceneInspector(RT64.renderInspector, &areaLighting.sceneDesc);

                // Inspect the current level's lights.
                RT64.lib.SetLightsInspector(RT64.renderInspector, areaLighting.lights, &areaLighting.lightCount, RT64_MAX_LEVEL_LIGHTS);

                // Inspect the current picked material.
                if (RT64.pickTextureHash > 0) {
                    const std::lock_guard<std::mutex> texModsLock(RT64.texModsMutex);
                    auto texNameIt = RT64.texNameMap.find(RT64.pickTextureHash);

                    char solidName[32];
                    snprintf(solidName, sizeof(solidName), "solid_%016llx", (unsigned long long)(RT64.pickTextureHash));
                    const std::string textureName = (texNameIt != RT64.texNameMap.end()) ? texNameIt->second : std::string(solidName);

                    if (texNameIt == RT64.texNameMap.end()) {
                        RT64.texNameMap[RT64.pickTextureHash] = textureName;
                        RT64.nameTexMap[textureName] = RT64.pickTextureHash;
                    }
                    RecordedMod *texMod = RT64.texMods[RT64.pickTextureHash];
                    if (texMod == nullptr) {
                        texMod = new RecordedMod();
                        RT64.texMods[RT64.pickTextureHash] = texMod;
                    }

                    if (texMod->materialMod == nullptr) {
                        texMod->materialMod = new RT64_MATERIAL();
                        texMod->materialMod->enabledAttributes = RT64_ATTRIBUTE_NONE;
                    }

                    RT64.lib.SetMaterialInspector(RT64.renderInspector, texMod->materialMod, textureName.c_str());
                    gfx_rt64_sync_inspector_map_names(RT64_INSPECTOR_PANEL_MATERIAL, texMod);
                }

                if (RT64.lib.SetGeoLayoutInspector != nullptr) {
                    RT64.lib.SetGeoLayoutInspector(RT64.renderInspector, RT64.pickedGeoLayoutMaterial,
                        &RT64.pickedGeoLayoutLight, &RT64.pickedGeoLayoutLightEnabled,
                        RT64.pickedGeoLayoutOrigins, RT64.pickedGeoLayoutOriginCount,
                        RT64.pickedGeoLayoutName.c_str());

                    const std::lock_guard<std::mutex> texModsLock(RT64.texModsMutex);
                    gfx_rt64_sync_inspector_map_names(RT64_INSPECTOR_PANEL_GEO_LAYOUT, RT64.pickedGeoLayoutMod);
                }
            }

            // Draw the frame and measure the time right before and right after.
            frameStart = gfx_rt64_profile_marker();

            if (lastFrameStart.QuadPart != 0) {
                interFrameDeltaTimeMs = gfx_rt64_profile_delta(lastFrameStart, frameStart).QuadPart / 1000.0;
            }
            lastFrameStart = frameStart;

            gfx_rt64_render_thread_apply_post_process_shader();

            gfx_rt64_render_thread_draw_frame(&RT64.frames[curFrameIndex], &RT64.frames[prevFrameIndex], (f32)(interFrameDeltaTimeMs));
            frameEnd = gfx_rt64_profile_marker();
            elapsedMicro = gfx_rt64_profile_delta(frameStart, frameEnd);
            frameDeltaTimeMs = elapsedMicro.QuadPart / 1000.0;

            // Clear the barrier.
            {
                const std::lock_guard<std::mutex> lock(RT64.renderFrameIndexMutex);
                RT64.gpuFrameIndex = -1;
                RT64.barrierFrameIndex = -1;
            }
            RT64.renderFrameCV.notify_all();
        }
    }
}

void gfx_rt64_sync_inspector_map_names(int panel, RecordedMod *mod) {
    if ((mod == nullptr) || (RT64.lib.SetInspectorMapNames == nullptr) || (RT64.lib.GetInspectorMapNames == nullptr)) {
        return;
    }

    const int mapNameSize = 256;
    char bumpMapName[mapNameSize] = {};
    char normalMapName[mapNameSize] = {};
    char specularMapName[mapNameSize] = {};
    if (RT64.lib.GetInspectorMapNames(RT64.renderInspector, panel, bumpMapName, normalMapName, specularMapName, mapNameSize)) {
        mod->bumpMapHash = (bumpMapName[0] != '\0') ? gfx_rt64_get_texture_name_hash(bumpMapName) : 0;
        mod->normalMapHash = (normalMapName[0] != '\0') ? gfx_rt64_get_texture_name_hash(normalMapName) : 0;
        mod->specularMapHash = (specularMapName[0] != '\0') ? gfx_rt64_get_texture_name_hash(specularMapName) : 0;

        if (bumpMapName[0] != '\0') { gfx_rt64_register_map_texture(bumpMapName, nullptr); }
        if (normalMapName[0] != '\0') { gfx_rt64_register_map_texture(normalMapName, nullptr); }
        if (specularMapName[0] != '\0') { gfx_rt64_register_map_texture(specularMapName, nullptr); }

        return;
    }

    const std::string bumpName = gfx_rt64_texture_mod_name(mod->bumpMapHash);
    const std::string normName = gfx_rt64_texture_mod_name(mod->normalMapHash);
    const std::string specName = gfx_rt64_texture_mod_name(mod->specularMapHash);
    RT64.lib.SetInspectorMapNames(RT64.renderInspector, panel, bumpName.c_str(), normName.c_str(), specName.c_str());
}

unsigned int gfx_rt64_clamp_percent(long long percent, unsigned int minPercent, unsigned int maxPercent) {
    if (percent < minPercent) { return minPercent; }
    if (percent > maxPercent) { return maxPercent; }
    return (unsigned int)(percent);
}

void gfx_rt64_adopt_inspector_view_desc(const RT64_VIEW_DESC &desc) {
    bool changed = false;
    auto adoptUInt = [&](unsigned int &configValue, unsigned int newValue) {
        if (configValue != newValue) { configValue = newValue; changed = true; }
    };
    auto adoptBool = [&](bool &configValue, bool newValue) {
        if (configValue != newValue) { configValue = newValue; changed = true; }
    };

    adoptUInt(configRT64ResScale, gfx_rt64_clamp_percent(lround(desc.resolutionScale * 100.0f), sMinResolutionScale, sMaxResolutionScale));
    adoptUInt(configRT64MaxLights, desc.maxLights);
    adoptUInt(configRT64MaxReflections, desc.maxReflections);
    adoptUInt(configRT64MotionBlurStrength, (unsigned int)(lround(desc.motionBlurStrength * 100.0f)));
    adoptUInt(configRT64UpscalerSharpness, (unsigned int)(lround(desc.upscalerSharpness * 100.0f)));
    adoptUInt(configRT64Upscaler, desc.upscaler);
    adoptUInt(configRT64UpscalerMode, desc.upscalerMode);
    adoptBool(configRT64SphereLights, desc.diSamples > 0);
    adoptBool(configRT64GI, desc.giSamples > 0);
    adoptBool(configRT64Denoiser, desc.denoiserEnabled);

    static bool sPendingSave = false;
    static LARGE_INTEGER sLastSaveTime = {};
    if (changed) { sPendingSave = true; }
    if (!sPendingSave) { return; }

    const LONGLONG minSaveIntervalMicros = 1000000;
    LARGE_INTEGER now = gfx_rt64_profile_marker();
    if ((sLastSaveTime.QuadPart != 0) && (gfx_rt64_profile_delta(sLastSaveTime, now).QuadPart < minSaveIntervalMicros)) {
        return;
    }

    sLastSaveTime = now;
    sPendingSave = false;
    configfile_save(configfile_name());
}

void gfx_rt64_publish_picked_geo_layout(void) {
    if (!RT64.renderInspectorActive) {
        return;
    }

    void *pickedGeoLayout = nullptr;
    {
        const std::lock_guard<std::mutex> pickLock(RT64.pickTextureMutex);
        pickedGeoLayout = RT64.pickedGeoLayout;
    }

    RT64_MATERIAL *material = nullptr;
    RecordedMod *pickedMod = nullptr;
    std::string geoName;
    if (pickedGeoLayout != nullptr) {
        auto geoNameIt = RT64.geoLayoutNameMap.find(pickedGeoLayout);
        if (geoNameIt != RT64.geoLayoutNameMap.end()) {
            geoName = geoNameIt->second;

            RecordedMod *&geoMod = RT64.geoLayoutMods[pickedGeoLayout];
            if (geoMod == nullptr) {
                geoMod = new RecordedMod();

                RT64.nameGeoLayoutMap[geoName] = pickedGeoLayout;
            }

            if (geoMod->materialMod == nullptr) {
                geoMod->materialMod = new RT64_MATERIAL();
                geoMod->materialMod->enabledAttributes = RT64_ATTRIBUTE_NONE;
            }

            material = geoMod->materialMod;
            pickedMod = geoMod;

            if (RT64.publishedGeoLayout != pickedGeoLayout) {
                RT64.publishedGeoLayout = pickedGeoLayout;
                RT64.pickedGeoLayoutLightEnabled = (geoMod->lightMod != nullptr);
                if (geoMod->lightMod != nullptr) {
                    RT64.pickedGeoLayoutLight = *geoMod->lightMod;
                } else {
                    memset(&RT64.pickedGeoLayoutLight, 0, sizeof(RT64_LIGHT));
                    vec3f_set(RT64.pickedGeoLayoutLight.diffuseColor, 255.0f, 255.0f, 255.0f);
                    vec3f_set(RT64.pickedGeoLayoutLight.specularColor, 255.0f, 255.0f, 255.0f);
                    RT64.pickedGeoLayoutLight.intensity = 1.0f;
                    RT64.pickedGeoLayoutLight.attenuationRadius = 1000.0f;
                    RT64.pickedGeoLayoutLight.pointRadius = 25.0f;
                    RT64.pickedGeoLayoutLight.attenuationExponent = 1.0f;
                    RT64.pickedGeoLayoutLight.groupBits = RT64_LIGHT_GROUP_DEFAULT;
                }
            }

            if (RT64.pickedGeoLayoutLightEnabled) {
                if (geoMod->lightMod == nullptr) {
                    geoMod->lightMod = new RT64_LIGHT();
                }

                *geoMod->lightMod = RT64.pickedGeoLayoutLight;
            } else {
                delete geoMod->lightMod;
                geoMod->lightMod = nullptr;
            }
        }
    }

    if (material == nullptr) {
        RT64.publishedGeoLayout = nullptr;
    }

    const std::lock_guard<std::mutex> pickLock(RT64.pickTextureMutex);
    RT64.pickedGeoLayoutMaterial = material;
    RT64.pickedGeoLayoutMod = pickedMod;
    RT64.pickedGeoLayoutName = geoName;
}

  //////////////
 // Textures //
//////////////

extern "C" const char *dynos_gfx_get_name(Gfx *gfx);
extern "C" const char *dynos_gfx_get_custom_name(Gfx *gfx);

extern "C" bool dynos_texture_get(const char *textureName, struct TextureInfo *outTextureInfo);
extern "C" bool dynos_texture_get_from_data(const Texture *tex, struct TextureInfo *outTextureInfo);
extern "C" u32 dynos_texture_get_generation(void);
extern "C" u8 *dynos_texture_convert_to_rgba32(const Texture *tex, u32 width, u32 height, u8 fmt, u8 siz);

u32 gfx_rt64_new_texture(const char *name) {
    // We reserve 0 for unassigned textures.
    u32 textureKey = 1 + (u32)(RT64.textures.size());
    auto &recordedTexture = RT64.textures[textureKey];
    recordedTexture.linearFilter = false;
    recordedTexture.cms = 0;
    recordedTexture.cmt = 0;
    recordedTexture.hash = 0;
    recordedTexture.pendingName = (name != nullptr) ? name : "";

    return textureKey;
}

static void gfx_rt64_name_content_hashed_texture(u64 hash) {
    if (RT64.texNameMap.find(hash) != RT64.texNameMap.end()) {
        return;
    }

    char nameBuf[32];
    snprintf(nameBuf, sizeof(nameBuf), "texture_%016llx", (unsigned long long)(hash));
    std::string name(nameBuf);
    RT64.texNameMap[hash] = name;
    RT64.nameTexMap[name] = hash;
}

static void gfx_rt64_register_texture_id(u64 hashID, u32 textureKey, const std::string &name) {
    const std::lock_guard<std::mutex> lock(RT64.texModsMutex);

    if (RT64.texNameMap.find(hashID) == RT64.texNameMap.end()) {
        RT64.texNameMap[hashID] = name;
    }
    RT64.nameTexMap[name] = hashID;

    if (textureKey != 0) {
        RT64.textureHashIdMap[hashID] = textureKey;
    }
}

u64 gfx_rt64_material_vanilla_name_hash(void) {
    if (RT64.materialDisplayList == RT64.materialNameHashDl) {
        return RT64.materialNameHashCached;
    }

    auto hashIt = RT64.materialNameHashes.find(RT64.materialDisplayList);
    if (hashIt != RT64.materialNameHashes.end()) {
        RT64.materialNameHashDl = RT64.materialDisplayList;
        RT64.materialNameHashCached = hashIt->second;
        return RT64.materialNameHashCached;
    }

    u64 nameHash = 0;
    const char *name = dynos_gfx_get_name((Gfx *)(RT64.materialDisplayList));
    if ((name != nullptr) && (name[0] != '\0')) {
        nameHash = gfx_rt64_texture_name_string_hash(name);

        gfx_rt64_register_texture_id(nameHash, 0, name);
    }

    RT64.materialNameHashes[RT64.materialDisplayList] = nameHash;
    RT64.materialNameHashDl = RT64.materialDisplayList;
    RT64.materialNameHashCached = nameHash;
    return nameHash;
}

u64 gfx_rt64_material_mod_name_hash(void) {
    auto hashIt = RT64.materialModNameHashes.find(RT64.materialDisplayList);
    if (hashIt != RT64.materialModNameHashes.end()) {
        return hashIt->second;
    }

    u64 nameHash = 0;
    const char *name = dynos_gfx_get_custom_name((Gfx *)(RT64.materialDisplayList));
    if ((name != nullptr) && (name[0] != '\0')) {
        nameHash = gfx_rt64_texture_name_string_hash(name);
        gfx_rt64_register_texture_id(nameHash, 0, name);
    }

    RT64.materialModNameHashes[RT64.materialDisplayList] = nameHash;
    return nameHash;
}

static void gfx_rt64_filter_texture_id(RecordedTexture &recorded, u32 textureKey, const std::string &name, u64 contentHash) {
    if (recorded.hash != 0) {
        return;
    }

    if (!name.empty()) {
        recorded.hash = gfx_rt64_texture_name_string_hash(name);
        gfx_rt64_register_texture_id(recorded.hash, textureKey, name);
    } else {
        recorded.hash = contentHash;
        RT64.textureHashIdMap[recorded.hash] = textureKey;
        gfx_rt64_name_content_hashed_texture(recorded.hash);
    }
}

static void gfx_rt64_queue_texture_upload(u32 textureKey, u64 hash, u64 contentHash, const RT64_TEXTURE_DESC &desc) {
    UploadTexture uploadTexture;
    uploadTexture.desc = desc;
    uploadTexture.hash = hash;
    uploadTexture.contentHash = contentHash;
    uploadTexture.key = textureKey;

    const std::lock_guard<std::mutex> lock(RT64.textureUploadQueueMutex);
    RT64.textureUploadQueue.push(uploadTexture);
}

static bool gfx_rt64_texture_desc_from_file(RT64_TEXTURE_DESC &texDesc, const char *path, const u8 *fileBuf, size_t fileBufSize) {
    // Use special case for loading DDS directly.
    if (strstr(path, ".dds") || strstr(path, ".DDS")) {
        texDesc.byteCount = (int)(fileBufSize);
        texDesc.bytes = malloc(texDesc.byteCount);
        if (texDesc.bytes == nullptr) {
            return false;
        }
        memcpy(texDesc.bytes, fileBuf, texDesc.byteCount);
        texDesc.width = texDesc.height = texDesc.rowPitch = -1;
        texDesc.format = RT64_TEXTURE_FORMAT_DDS;
        return true;
    }

    // Use stb image to load the file from memory instead if possible.
    int width, height;
    stbi_uc *data = stbi_load_from_memory(fileBuf, (int)(fileBufSize), &width, &height, nullptr, 4);
    if (data == nullptr) {
        return false;
    }

    texDesc.bytes = data;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.rowPitch = texDesc.width * 4;
    texDesc.byteCount = texDesc.height * texDesc.rowPitch;
    texDesc.format = RT64_TEXTURE_FORMAT_RGBA8;
    return true;
}

void gfx_rt64_upload_texture(u32 textureKey, const u8 *rgba32Buf, s32 width, s32 height) {
    XXHash64 hashStream(0);
    hashStream.add(rgba32Buf, (size_t)(width) * (size_t)(height) * 4);
    const u64 contentHash = hashStream.hash();

    RecordedTexture &recorded = RT64.textures[textureKey];
    gfx_rt64_filter_texture_id(recorded, textureKey, recorded.pendingName, contentHash);

    RT64_TEXTURE_DESC texDesc = {};
    texDesc.width = width;
    texDesc.height = height;
    texDesc.rowPitch = texDesc.width * 4;
    texDesc.format = RT64_TEXTURE_FORMAT_RGBA8;
    texDesc.byteCount = texDesc.height * texDesc.rowPitch;
    texDesc.bytes = malloc(texDesc.byteCount);
    if (texDesc.bytes == nullptr) {
        return;
    }
    memcpy(texDesc.bytes, rgba32Buf, texDesc.byteCount);

    gfx_rt64_queue_texture_upload(textureKey, recorded.hash, contentHash, texDesc);
}

static const char *sMapTextureExtensions[] = { "", ".png", ".dds", ".jpg", ".bmp" };

static std::string gfx_rt64_mod_texture_root(struct Mod *mod) {
    if ((mod == nullptr) || !mod->isDirectory || (mod->basePath[0] == '\0')) {
        return std::string();
    }

    std::string root = mod->basePath;
    if ((root.back() != '/') && (root.back() != '\\')) {
        root += "/";
    }

    return root + "textures/";
}

static bool gfx_rt64_try_map_texture_root(u64 nameHash, const char *name, const std::string &root) {
    if (root.empty()) {
        return false;
    }

    for (const char *extension : sMapTextureExtensions) {
        const std::string path = root + name + extension;
        if (fs_sys_file_exists(path.c_str())) {
            RT64.mapTexturePaths[nameHash] = path;
            RT64.mapTexturesLoaded.erase(nameHash);
            return true;
        }
    }

    return false;
}

bool gfx_rt64_register_map_texture(const char *name, const char *preferredRoot) {
    if ((name == nullptr) || (name[0] == '\0')) {
        return false;
    }

    const u64 nameHash = gfx_rt64_get_texture_name_hash(name);

    RT64.mapTextureNames[nameHash] = name;

    if ((preferredRoot != nullptr) && gfx_rt64_try_map_texture_root(nameHash, name, preferredRoot)) {
        return true;
    }

    for (int i = 0; i < gActiveMods.entryCount; i++) {
        if (gfx_rt64_try_map_texture_root(nameHash, name, gfx_rt64_mod_texture_root(gActiveMods.entries[i]))) {
            return true;
        }
    }

    return gfx_rt64_try_map_texture_root(nameHash, name, fs_get_write_path("textures/segment2/"));
}

static u32 gfx_rt64_load_map_texture(u64 nameHash, const std::string &path) {
    FILE *f = fopen(path.c_str(), "rb");
    if (f == nullptr) {
        fprintf(stderr, "RT64: unable to open the map texture '%s'.\n", path.c_str());
        return 0;
    }

    fseek(f, 0, SEEK_END);
    const long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fileSize <= 0) {
        fclose(f);
        fprintf(stderr, "RT64: the map texture '%s' is empty.\n", path.c_str());
        return 0;
    }

    std::vector<u8> fileBuf((size_t)(fileSize));
    const size_t readBytes = fread(fileBuf.data(), 1, fileBuf.size(), f);
    fclose(f);
    if (readBytes != fileBuf.size()) {
        fprintf(stderr, "RT64: unable to read the map texture '%s'.\n", path.c_str());
        return 0;
    }

    RT64_TEXTURE_DESC texDesc = {};
    if (!gfx_rt64_texture_desc_from_file(texDesc, path.c_str(), fileBuf.data(), fileBuf.size())) {
        fprintf(stderr, "RT64: stb_image was unable to decode the map texture '%s'.\n", path.c_str());
        return 0;
    }

    const u32 textureKey = 1 + (u32)(RT64.textures.size());
    RecordedTexture &recorded = RT64.textures[textureKey];
    recorded.linearFilter = true;
    recorded.cms = 0;
    recorded.cmt = 0;
    recorded.hash = nameHash;

    XXHash64 hashStream(0);
    hashStream.add(fileBuf.data(), fileBuf.size());
    gfx_rt64_queue_texture_upload(textureKey, nameHash, hashStream.hash(), texDesc);

    RT64.textureHashIdMap[nameHash] = textureKey;
    return textureKey;
}

static u32 gfx_rt64_load_dynos_map_texture(u64 nameHash, const std::string &name) {
    struct TextureInfo texInfo = {};
    if (!dynos_texture_get(name.c_str(), &texInfo)) {
        return 0;
    }

    if ((texInfo.texture == nullptr) || (texInfo.width == 0) || (texInfo.height == 0)) {
        return 0;
    }

    u8 *rgba32 = dynos_texture_convert_to_rgba32(texInfo.texture, texInfo.width, texInfo.height, texInfo.format, texInfo.size);
    if (rgba32 == nullptr) {
        fprintf(stderr, "RT64: unable to convert the map texture '%s' to RGBA8.\n", name.c_str());
        return 0;
    }

    RT64_TEXTURE_DESC texDesc = {};
    texDesc.width = (int)(texInfo.width);
    texDesc.height = (int)(texInfo.height);
    texDesc.rowPitch = texDesc.width * 4;
    texDesc.byteCount = texDesc.height * texDesc.rowPitch;
    texDesc.format = RT64_TEXTURE_FORMAT_RGBA8;
    texDesc.bytes = rgba32;

    const u32 textureKey = 1 + (u32)(RT64.textures.size());
    RecordedTexture &recorded = RT64.textures[textureKey];
    recorded.linearFilter = true;
    recorded.cms = 0;
    recorded.cmt = 0;
    recorded.hash = nameHash;

    XXHash64 hashStream(0);
    hashStream.add(rgba32, (size_t)(texDesc.byteCount));
    gfx_rt64_queue_texture_upload(textureKey, nameHash, hashStream.hash(), texDesc);

    RT64.textureHashIdMap[nameHash] = textureKey;
    return textureKey;
}

u32 gfx_rt64_map_texture_key(u64 nameHash) {
    auto hashIt = RT64.textureHashIdMap.find(nameHash);
    if (hashIt != RT64.textureHashIdMap.end()) {
        return (RT64.textures.find(hashIt->second) != RT64.textures.end()) ? hashIt->second : 0;
    }

    auto pathIt = RT64.mapTexturePaths.find(nameHash);
    auto nameIt = RT64.mapTextureNames.find(nameHash);
    if ((pathIt == RT64.mapTexturePaths.end()) && (nameIt == RT64.mapTextureNames.end())) {
        return 0;
    }

    if (!RT64.mapTexturesLoaded.insert(nameHash).second) {
        return 0;
    }

    if (pathIt != RT64.mapTexturePaths.end()) {
        return gfx_rt64_load_map_texture(nameHash, pathIt->second);
    }

    return gfx_rt64_load_dynos_map_texture(nameHash, nameIt->second);
}

// Panoramic skybox tiles :)

static const int sSkyboxTileCols = 8;
static const int sSkyboxTileRows = 8;
static const int sSkyboxTileStride = 10;
static const int sSkyboxTileListCount = sSkyboxTileStride * sSkyboxTileRows;

static const int sSkyboxTileUsedTexels = 31;
static const int sSkyboxTileTexels = 32;
static const double sSkyboxTileFilterInset = 0.5;

static const u32 sSkyboxPanoramaMaxSize = 4096;

static u8 *gfx_rt64_read_skybox_tile_rgba32(const Texture *tile, u32 *outWidth, u32 *outHeight) {
    struct TextureInfo texInfo = {};
    const Texture *texData = tile;
    u32 width = 32;
    u32 height = 32;
    u8 format = G_IM_FMT_RGBA;
    u8 size = G_IM_SIZ_16b;

    if (dynos_texture_get_from_data(tile, &texInfo) && (texInfo.texture != nullptr) && (texInfo.width != 0) && (texInfo.height != 0)) {
        texData = texInfo.texture;
        width = texInfo.width;
        height = texInfo.height;
        format = texInfo.format;
        size = texInfo.size;
    }

    u8 *rgba32 = dynos_texture_convert_to_rgba32(texData, width, height, format, size);
    if (rgba32 == nullptr) {
        return nullptr;
    }

    *outWidth = width;
    *outHeight = height;
    return rgba32;
}

static void gfx_rt64_blit_skybox_tile(u8 *dst, u32 dstRowPixels, u32 dstWidth, u32 dstHeight, const u8 *src, u32 srcWidth, u32 srcHeight, double srcOriginX, double srcOriginY, double srcSpanX, double srcSpanY) {
    const double scaleX = srcSpanX / dstWidth;
    const double scaleY = srcSpanY / dstHeight;

    for (u32 y = 0; y < dstHeight; y++) {
        const double sy = fmax((srcOriginY + ((y + 0.5) * scaleY)) - 0.5, 0.0);
        const u32 y0 = ((u32)(sy) < srcHeight) ? (u32)(sy) : (srcHeight - 1);
        const u32 y1 = ((y0 + 1) < srcHeight) ? (y0 + 1) : y0;
        const double fy = sy - y0;

        for (u32 x = 0; x < dstWidth; x++) {
            const double sx = fmax((srcOriginX + ((x + 0.5) * scaleX)) - 0.5, 0.0);
            const u32 x0 = ((u32)(sx) < srcWidth) ? (u32)(sx) : (srcWidth - 1);
            const u32 x1 = ((x0 + 1) < srcWidth) ? (x0 + 1) : x0;
            const double fx = sx - x0;

            const u8 *row0 = src + (size_t)(y0) * srcWidth * 4;
            const u8 *row1 = src + (size_t)(y1) * srcWidth * 4;
            const u8 *p00 = row0 + (size_t)(x0) * 4;
            const u8 *p10 = row0 + (size_t)(x1) * 4;
            const u8 *p01 = row1 + (size_t)(x0) * 4;
            const u8 *p11 = row1 + (size_t)(x1) * 4;
            u8 *out = dst + ((size_t)(y) * dstRowPixels + x) * 4;

            for (int c = 0; c < 4; c++) {
                const double top = p00[c] + (p10[c] - p00[c]) * fx;
                const double bottom = p01[c] + (p11[c] - p01[c]) * fx;
                out[c] = (u8)((top + (bottom - top) * fy) + 0.5);
            }
        }
    }
}

u32 gfx_rt64_stitch_skybox_texture(const Texture *const *tiles) {
    if (tiles == nullptr) {
        return 0;
    }

    const u32 texGeneration = dynos_texture_get_generation();
    XXHash64 keyHash(0);
    keyHash.add(&texGeneration, sizeof(texGeneration));
    keyHash.add(tiles, sSkyboxTileListCount * sizeof(const Texture *));
    const u64 cacheKey = keyHash.hash();

    auto cacheIt = RT64.stitchedSkyTextureKeys.find(cacheKey);
    if ((cacheIt != RT64.stitchedSkyTextureKeys.end()) && (RT64.textures.find(cacheIt->second) != RT64.textures.end())) {
        return cacheIt->second;
    }

    u32 tileWidth = 0;
    u32 tileHeight = 0;
    std::vector<u8 *> tileRgba32(sSkyboxTileCols * sSkyboxTileRows, nullptr);
    bool ok = true;

    for (int row = 0; ok && (row < sSkyboxTileRows); row++) {
        for (int col = 0; ok && (col < sSkyboxTileCols); col++) {
            u32 width, height;
            u8 *rgba32 = gfx_rt64_read_skybox_tile_rgba32(tiles[row * sSkyboxTileStride + col], &width, &height);
            if (rgba32 == nullptr) {
                fprintf(stderr, "RT64: unable to convert a skybox tile to RGBA8 for panorama stitching.\n");
                ok = false;
                break;
            }

            if (tileWidth == 0) {
                tileWidth = width;
                tileHeight = height;
            } else if ((width != tileWidth) || (height != tileHeight)) {
                fprintf(stderr, "RT64: skybox tiles don't agree on a size, can't stitch a panorama.\n");
                free(rgba32);
                ok = false;
                break;
            }

            tileRgba32[row * sSkyboxTileCols + col] = rgba32;
        }
    }

    if (!ok) {
        for (u8 *rgba32 : tileRgba32) { free(rgba32); }
        return 0;
    }

    const double srcOriginX = (tileWidth * sSkyboxTileFilterInset) / sSkyboxTileTexels;
    const double srcOriginY = (tileHeight * sSkyboxTileFilterInset) / sSkyboxTileTexels;
    const double srcSpanX = (tileWidth * (double)(sSkyboxTileUsedTexels)) / sSkyboxTileTexels;
    const double srcSpanY = (tileHeight * (double)(sSkyboxTileUsedTexels)) / sSkyboxTileTexels;
    const u32 maxCropWidth = sSkyboxPanoramaMaxSize / sSkyboxTileCols;
    const u32 maxCropHeight = sSkyboxPanoramaMaxSize / sSkyboxTileRows;

    u32 cropWidth = (u32)(srcSpanX + 0.5);
    u32 cropHeight = (u32)(srcSpanY + 0.5);
    if (cropWidth > maxCropWidth) { cropWidth = maxCropWidth; }
    if (cropHeight > maxCropHeight) { cropHeight = maxCropHeight; }

    const u32 panoramaWidth = cropWidth * sSkyboxTileCols;
    const u32 panoramaHeight = cropHeight * sSkyboxTileRows;

    u8 *panorama = (u8 *)malloc((size_t)(panoramaWidth) * panoramaHeight * 4);
    if (panorama == nullptr) {
        for (u8 *rgba32 : tileRgba32) { free(rgba32); }
        return 0;
    }

    for (int row = 0; row < sSkyboxTileRows; row++) {
        for (int col = 0; col < sSkyboxTileCols; col++) {
            const u8 *src = tileRgba32[row * sSkyboxTileCols + col];
            u8 *dst = panorama + ((size_t)(row) * cropHeight * panoramaWidth + (size_t)(col) * cropWidth) * 4;
            gfx_rt64_blit_skybox_tile(dst, panoramaWidth, cropWidth, cropHeight, src, tileWidth, tileHeight, srcOriginX, srcOriginY, srcSpanX, srcSpanY);
        }
    }

    for (u8 *rgba32 : tileRgba32) { free(rgba32); }

    RT64_TEXTURE_DESC texDesc = {};
    texDesc.width = (int)(panoramaWidth);
    texDesc.height = (int)(panoramaHeight);
    texDesc.rowPitch = texDesc.width * 4;
    texDesc.byteCount = texDesc.height * texDesc.rowPitch;
    texDesc.format = RT64_TEXTURE_FORMAT_RGBA8;
    texDesc.bytes = panorama;

    const u32 textureKey = 1 + (u32)(RT64.textures.size());
    RecordedTexture &recorded = RT64.textures[textureKey];
    recorded.linearFilter = true;
    recorded.cms = 0;
    recorded.cmt = 0;
    recorded.hash = cacheKey;

    XXHash64 contentHash(0);
    contentHash.add(panorama, (size_t)(texDesc.byteCount));
    gfx_rt64_queue_texture_upload(textureKey, cacheKey, contentHash.hash(), texDesc);

    RT64.stitchedSkyTextureKeys[cacheKey] = textureKey;
    return textureKey;
}

#endif // _WIN32
