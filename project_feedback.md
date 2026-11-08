# Review: VulkanEngine

**Reviewer's framing.** I read this as an admissions artifact, not as a product. I am not
grading you on whether this ships. I am grading you on three things: whether you understand
what the hardware is actually doing, whether you are rigorous about the parts you cannot see,
and whether the gaps in the project are gaps of *time* or gaps of *care*. Those are different,
and I can usually tell which is which.

I have deliberately not written fixes. Where I point at a file and a line, the work of
figuring out what is wrong there is the work I am evaluating. Most of the questions below have
one-sentence answers once you look at the right line.

---

## 1. Verdict up front

There is real competence here. Dynamic rendering instead of render passes, buffer-device-address
vertex pulling instead of vertex input state, a bindless descriptor array with
`UPDATE_AFTER_BIND`, a growable descriptor pool, a hand-rolled Karras LBVH with a borrowed
subgroup radix sort — this is not a tutorial clone that stopped at the triangle. You went past
where the tutorials end, which is the only thing that distinguishes an applicant in this area.
The `AABB::worldSpaceBounds` abs-basis transform (`src/primitive.hpp:55`), the
skybox `position.xyww` depth trick (`shaders/cubemap_vert.vert:29`), the `α = roughness²`
convention threaded correctly through `normalDistribution`/`kDirect`
(`shaders/shader.frag:113-142`), and the root-parent sentinel written at exactly the right byte
offset (`src/bvh.cpp:290-294`) are all things people get wrong. You got them right.

And then, consistently: **the ambitious parts are more carefully reasoned than the boring parts,
and the boring parts are where the project is actually broken.** The engine has only ever been
compiled unoptimized, with every compiler warning muted, against GPU work that has no way to
report whether it produced the right answer. None of that is a knowledge gap. All of it is a
rigour gap, and it is the gap that decides how this project reads to a committee.

Below, roughly in order of how much each item would weigh on an admissions decision.[^1]

---

## 2. You have never run this engine optimized

`CMakeLists.txt` line 29:

```cmake
set(CMAKE_BUILD_TYPE Debug)
```

This is a plain `set()` on a cache variable, after `project()`. It shadows the cache entry for
the rest of the directory, so `-DCMAKE_BUILD_TYPE=Release` on the command line is silently
discarded. I confirmed the actual compile line:

```
-g -std=gnu++20 -arch arm64
```

No `-O` flag of any kind. Every frame time you have ever observed, every judgement you have
made about what is fast or slow in this renderer, every reason you had to reach for a GPU BVH
instead of a CPU one — all of it was measured on an `-O0` binary with `assert` live and
`std::map`/`std::vector` iteration unoptimized.

For an application to a graphics program this is the single most damaging item in the review,
because performance reasoning is the thing I most need to believe you can do. I do not need a
profiler and a flamegraph from you. I need evidence that you know a measurement taken at `-O0`
is not a measurement. Relatedly: there are no GPU timestamp queries anywhere in the project, so
you also have no per-pass GPU timings. You have an fps counter computed at
`src/engine_app.cpp:195` whose value is never displayed or recorded.

---

## 3. Uninitialized state driving resource allocation

This is a class of bug, not one bug, and it is the kind a systems reviewer looks for first.

**`src/bvh.hpp:75`** — `uint32_t primitiveCount;`. Now read `src/bvh.cpp:145`:

```cpp
primitiveCount += obj.model->meshes.size();
```

Then look at every buffer size in `initializeAABB`, `initializeMortonCodeBuffers`, and
`initializeNodeBuffers` — all of them `primitiveCount` or `primitiveCount - 1`. Now read your
own comment at `src/engine_app.cpp:165-168`:

```cpp
// TODO: fix this please
//  FIXME: CATASTROPHIC BUG failing to create memory here, NEEDS FIX!
```

Trace what value `primitiveCount` holds at the instant the first `+=` executes. Then look at
`EngineDevice::createBuffer` (`src/engine_device.cpp:570`) and tell me what happens to the
`VkResult` from `vmaCreateBuffer`. Those two facts together are, I think, the entire story of
why your BVH is commented out of the main loop — and the reason it took you a while to find it
is the second fact, not the first. An allocator that cannot fail is an allocator that cannot
tell you anything.

The same class, elsewhere:

- `src/engine_mesh.hpp:85,90` — `vertexCount` and `indexCount` have no initializer.
  `totalIndexCount()` (`src/engine_mesh.cpp:122-127`) does `indexCount += surface.count`. That
  value is handed directly to `vkCmdDrawIndexed`. What is the draw count on a cold heap?
- `src/engine_mesh.hpp:80` — `localTransform` is an uninitialized `glm::mat4` read by
  `SimpleRenderSystem::render` on any frame where the node traversal did not reach that mesh.
- `src/engine_computer.cpp:69-74` — `getAvailableCommandBuffer()` returns an uninitialized
  `VkCommandBuffer`. Clang flags this one for you with `-Wuninitialized`; you have no warnings
  enabled, so nobody told you.
- `src/engine_texture.cpp:17-20` — the `EngineTexture(device, ID)` constructor stores neither
  the ID nor any handle. If `Init()` is never called on such a texture, what does `~EngineTexture`
  pass to `vkDestroySampler`?
- `src/engine_pipeline.hpp:20-28` — every `VkPipeline*StateCreateInfo` member of
  `PipelineConfigInfo` is declared without `{}`. `defaultPipelineConfigInfo` sets `sType` and
  the fields it cares about. Which fields does it *not* set, and what are you therefore handing
  to `vkCreateGraphicsPipelines`? Check `pNext` and `flags` on each one.

**Your build enables no warnings at all.** Not `-Wall`, not `-Wextra`. I turned them on and got
`-Wreorder-ctor` in four files, `-Wuninitialized` in `engine_computer.cpp`, `-Wmissing-braces`
seven times in `engine_renderer.cpp`, and `-Wunused-function` from `engine_ui.hpp`. The compiler
has been trying to tell you about several of the problems in this review for nine months and
you have it muted. Turn them on before you submit anything to anyone.

While you're there, `src/engine_renderer.cpp:191` and `:201`:

```cpp
.clearValue = {0.0, 0.0, 0.0},   // color
.clearValue = {1.0, 0.0},        // depth
```

`VkClearValue` is a union. Work out which member each of those initializes, and why the depth
one produces the depth value you wanted anyway. Then decide whether you are comfortable relying
on that.

---

## 4. Synchronization: the weakest area, and the one that matters most

If you want to do GPU work at a research level, synchronization is the skill. This is where the
project is furthest from adequate.

**The picking pass has no synchronization whatsoever.** Read `OffScreenRenderer::submitCommandBuffer`
(`src/engine_renderer.cpp:374-398`): no semaphores, `nullptr` fence, and every line of the
fence logic commented out. Now read `readPixelValue` (`:511-517`), which opens by waiting on
`imageRenderedFences[currentIndex]`. What signals that fence? Trace it. Then explain to me what
guarantees the GPU has finished writing the ID image before the host reads the staging buffer.
Your commit `ef3bad6` is titled "fence issue debugged" — this is the thing that was not debugged.

Then the host read itself (`:519-557`), which I want you to cost out in bytes and stalls:

- One `EngineBuffer` allocated and freed per click.
- `copyImageToBuffer` of the **entire** framebuffer — at 1280×980 with a 2× pixel density
  that's ~20 MB transferred — to read **one** `uint32_t`.
- Four separate `beginSingleTimeCommands`/`endSingleTimeCommands` round trips, each ending in
  `vkQueueWaitIdle` (`src/engine_device.cpp:607`). Four full device stalls per click.
- The staging buffer is created with `VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT`
  and is then read by the host. What memory properties does VMA pick for a sequential-write
  allocation, and what is the cost of a host *read* from that kind of memory on Apple Silicon?
- `invalidate()` is never called before the read. When is that required?

**Cross-queue with no dependency.** `EngineComputer::endComputation` submits to
`geDevice.computeQueue()` (`src/engine_computer.cpp:111`). Everything else submits to
`graphicsQueue()`. The BVH output buffers carry `VK_BUFFER_USAGE_VERTEX_BUFFER_BIT`
(`src/bvh.cpp:228,330`), so they are intended to be consumed by graphics. There is no semaphore
between the two queues and no queue-family ownership transfer, and the buffers are
`VK_SHARING_MODE_EXCLUSIVE` (`src/engine_device.cpp:564`). Why does this appear to work on your
machine right now? Read `findQueueFamilies` (`src/engine_device.cpp:449-461`) very carefully —
look at which flag bit each of the two branches tests — and then tell me what `graphicsFamily`
and `computeFamily` actually contain. Then ask yourself what happens on a discrete GPU with
separate families.

**`src/engine_computer.cpp:88`** — `vkWaitForFences(..., UINT16_MAX)`. What unit is that
parameter? What duration did you just ask for? And what does `vkWaitForFences` return when it
gives up, and where do you check it?

**`src/engine_swapchain.cpp:112,122`** — both `imageAvailableSemaphores` and
`renderFinishedSemaphores` are indexed by `currentFrame`. One of those two is correct to index
by frame and one is not. Which, and why? (Hint: which one does `vkQueuePresentKHR` wait on, and
what object does the presentation engine associate it with?) This is the single most common
WSI bug in existence and it is worth being able to explain rather than just patch.

**`src/engine_app.cpp:197`** —

```cpp
int lastFrame = 0;
```

That declaration is *inside* the `while (!shouldQuit)` loop. Line 318 then does
`frameInfo.frameIndex = lastFrame;` and line 324 does `lastFrame = frameIndex;`. What value does
`lastFrame` have at line 318 on every single frame? Now follow it into
`OutlineSystem::render` (`src/systems/outline_system.cpp:79`), which uses it to select which
offscreen image to sample — while the offscreen pass you just submitted wrote a *different*
one. Two bugs stacked: the wrong image, and no dependency between the write and the read.

**Barriers in `constructTree`.** You wrote a careful `BufferAccess` abstraction
(`src/engine_buffer.hpp:14-28`) with a good doc comment about why recording two dispatches into
one command buffer does not order them. Then in `src/bvh.cpp:45-135`, which transitions are
present and which are missing? Specifically: `resetBuffers` issues `vkCmdFillBuffer` (transfer
stage) into the same command buffer immediately before the first compute dispatch reads those
buffers. Where is that barrier? And `primitiveAABBs` is written by a transfer on another queue
and read by `mortonGeneration` — where is *that* one? You built the right tool and then did not
apply it systematically. Enable synchronization validation
(`VK_LAYER_SETTING` / `VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT`) and see
what it says; you currently enable only the default validation layer with
warning+error severity (`src/engine_device.cpp:317`), no sync validation, no best-practices
layer, no `VK_EXT_debug_utils` object naming.

**`OffScreenRenderer::beginFrame` (`src/engine_renderer.cpp:400-406`)** calls `init()` again on
resize. Walk through `init()` and count what is destroyed before being replaced. Look
particularly at `createSyncObject()` and `createCommandBuffer()`, and at whether the device is
idle when this happens. Then: `OutlineSystem` captured raw `VkImage`/`VkImageView` handles from
those images at construction (`src/systems/outline_system.cpp:44-49`) and baked them into
descriptor sets. What is the state of those descriptors after one resize?

And `src/engine_renderer.cpp:442-447` checks the result of `vkQueueSubmit` against
`VK_ERROR_OUT_OF_DATE_KHR` and `VK_SUBOPTIMAL_KHR`. Which function returns those?

---

## 5. The LBVH

This is the part of the project you should be leading with in an application, and it is also
the part where I most want to see rigor, because a wrong BVH renders *almost* correctly and a
slow one renders perfectly.

**There is no consumer and no verification.** `BVHAccel::doesIntersect()` and
`inOrderTraversal()` are declared (`src/bvh.hpp:29-31`) and never defined. Nothing traverses
the tree. There is no readback, no CPU reference implementation to diff against, no invariant
check (every node reachable once, leaf count == n, parent/child consistency, child AABBs
contained in parent). Your README says the AABB debug view "is what makes it possible to tell
whether the tree above them is right" — but the debug view draws the *input* AABBs from
`EngineMesh::getBBox()` (`src/systems/bbox_system.cpp:52`), not `nodeAABBs`. You are looking at
the leaves and inferring the tree. You cannot see the tree at all.

This is the gap between a project and research, and it is the thing I would press you on in an
interview. How do you know the tree is correct? What would a wrong tree look like on screen?
Build the thing that answers that before you build anything else on top.

**`src/bvh.cpp:55-58`** —

```cpp
pushBound.sceneDiff = glm::vec4{40.f};
pushBound.sceneMin  = glm::vec4{-20.f};
```

Meanwhile `sceneMinBound` and `sceneDiffBound` are members (`src/bvh.hpp:76-77`), initialized
to sentinels, and never computed or used. So the Morton grid is a hardcoded 40-unit cube. Two
questions. First, in `shaders/morton_code.comp:37-39`:

```glsl
uint x = expandBits(min(uint(p.x * 1024.0), 1023u));
```

what does `uint()` do to a negative float in GLSL, and does the `min` protect you? Second — and
this is the more interesting one — what does a loose scene bound cost you in Morton code
quality, and therefore in tree quality? Quantify it: if your geometry occupies 5 units of a
40-unit grid, how many of your 10 bits per axis are doing work?

**Push constants, `src/bvh.cpp:84-89`, `:99-104`, `:124-129`.** Every one of those three
`vkCmdPushConstants` calls passes `mortonGeneration->getPipelineLayout()` while a *different*
pipeline is bound. Each pipeline has its own layout (`ComputePipeline::createPipelineLayout`).
What does the spec require about the layout used to record push constants versus the bound
pipeline's layout? Separately, all three pass `sizeof(PushConstantBVHWithBounds)` — 36 bytes —
with `&push`, where `push` is a `PushConstantBVH`. How large is that object? What is being read?

**The sort's ping-pong parity.** `shaders/single_radixsort.comp:52-53` and `:149-155`. Work out,
for `ITERATIONS = 4`, which buffer holds the sorted keys and which holds the sorted payload when
the loop exits. Then compare against what `treeDS` binds at binding 0
(`src/bvh.cpp:424-428`) and what `mergeAABBDS` binds at binding 1 (`:430-436`). Then look at
which buffer your barrier at `src/bvh.cpp:92-96` protects. Three things have to agree about
parity and I do not believe they do.

**`shaders/aabb_propagate.comp:55`** — this is the best question in the project:

```glsl
while (parentNode != -1 && atomicAdd(internalNodes[parentNode].visited, 1) > 0) {
```

The "second child wins" counter is the right idea and you implemented it correctly. But the
atomic orders *arrival*; the thing you then read is `nodeAABBs[left]` or `nodeAABBs[right]`,
written non-atomically by a thread in a different workgroup. What guarantees that write is
*visible* to you? Look at how `NodeAABBs` is declared at `:38`, think about where a GPU caches
SSBO writes and at what level that cache is coherent, and then look up what qualifiers and
barrier intrinsics GLSL gives you for exactly this. On a small scene with few primitives you
will likely never observe the failure. That is what makes it worth understanding rather than
stumbling into.

**`shaders/radix_tree_build.comp`.**
- Line 39: `if (bitXOR == 0u) return 32;`. What does Karras's δ do with duplicate keys, and why?
  What shape of tree do you get for a scene with co-located primitives — say, several meshes
  whose centroids land in the same 1024³ cell?
- Line 49: `int d = sign(delta(i,i+1) - delta(i,i-1));`. `sign` has three possible results.
  Reference implementations have two. Trace `d == 0` all the way to line 103 and describe the
  node you just built.
- `src/bvh.cpp:105`: `dispatch(..., primitiveCount - 1)` on a `uint32_t`. What is the dispatch
  size when the scene has one primitive, or none?
- Sizing: `internalNodes` is allocated with `primitiveCount` elements, `nodeAABBs` with
  `primitiveCount - 1`. How many internal nodes does a radix tree over *n* leaves have? One of
  those two is the deliberate number.

**`shaders/copy_aabb.comp`** — written, compiled, pipeline created (`createAABBPipeline`,
`src/bvh.cpp:440-447`), and never dispatched. Three problems, and I mention it because the
*intent* is right (move AABB generation onto the GPU) and the execution is unfinished:
- Line 46: `if (x > n) return;` — one character.
- Line 44: `int x = int(gl_GlobalInvocationID.x / 3);` — three invocations compute and write the
  same `aabbOut[x]`. Why three?
- Lines 48-50: `vertices[3*x]`, `vertices[3*x+1]`, `vertices[3*x+2]`. Your geometry is
  *indexed* — `EngineMesh` builds an index buffer and the vertices are deduplicated. What
  triangles is this shader actually bounding? The index buffer is not even bound to this
  pipeline.
- The push constant block declares `{uint, VertexBuffer}`; the host creates the layout with
  `sizeof(AABBSortPipelinePush)`, which is two `mat4`s. Those are not the same struct.

**Scale.** `radixSortPipeline->dispatch(commandBuffer, &sortMortonCodesDS[index], 1)`
(`src/bvh.cpp:90`) with `groupCount(1, 256)` → one workgroup. That is what "single" radix sort
means, so it is not a mistake — but state the consequence out loud: on an M-series GPU your
sort runs on one core out of ten to forty. Fine for a handful of mesh-level AABBs. The moment
this becomes a triangle-level BVH (which is the only version anyone cares about) it is the
bottleneck. Also: `shaders/single_radixsort.comp:19` declares `SUBGROUP_SIZE` as a
specialization constant defaulting to 32 with the comment "NVIDIA only!", and
`ComputePipeline::createComputePipeline` never sets `pSpecializationInfo`. You never query
`VkPhysicalDeviceSubgroupProperties::subgroupSize`. You are running correct code on this GPU by
coincidence, and the shader's own comment warned you.

**Per-frame CPU path.** `BVHAccel::updateAABBDataIndex` → `traverseAndUpdateAABB` →
`copyAABBBuffer`, every frame. That walks every mesh, calls `obj.transform.mat4()` per mesh
(six `sin`/`cos` recomputed per mesh — see `src/engine_game_object.cpp:6`), transforms the AABB
on the CPU, allocates a fresh staging buffer, and copies via `geDevice.copyBuffer` which ends
in `vkQueueWaitIdle`. A full device stall, every frame, to upload 48 bytes per primitive, at the
front of a GPU algorithm. `copy_aabb.comp` exists precisely to delete this path. Finish it.

Finally: `src/bvh.cpp:16` includes
`"sdl/vendored/SDL/src/joystick/hidapi/steam/controller_structs.h"`. A Steam Controller HID
header, in your BVH. Your editor put it there and you did not notice. Read your diffs.

---

## 6. Graphics correctness

**Normal mapping does not exist.** `shaders/shader.frag:116-118`:

```glsl
if (bool(matProperties.numFeatures & GLSL_HAS_NORMAL_MAP)) {
  vec4 normalValue = texture(bindlessTextures[...], fragUV);
}
```

The sample is assigned to a local and the local dies at the closing brace. The material system
loads normal maps (`src/engine_model.cpp:374-378`), the feature bit is set, the texture is
uploaded — and the shader throws the value away. Your README lists normal mapping under
Features. Separately: no `TANGENT` attribute is read in `loadVertices`, no tangents are
generated, and `EngineMesh::Vertex` has no tangent field. So even wiring up the sample leaves
you without a basis to rotate it into. That is the actual work item, and it is a good one.

**The normal matrix is composed wrongly.** `shaders/shader.vert:49`:

```glsl
fragNormalWorld = normalize(mat3(push.normalMatrix * push.localTransform) * v.normal);
```

`normalMatrix` is built from the object transform alone (`TransformComponent::normalMatrix`,
inverse scale on the diagonal). `localTransform` is the glTF node transform. Write out the
correct normal matrix for the composed transform `M_obj · M_local` and compare it to
`N_obj · M_local`. Under what conditions on `M_local` are they equal? glTF node transforms carry
rotation and non-uniform scale routinely, so this is not hypothetical — your normals are wrong
on any node with a non-trivial local transform, which is most of them.

**Colour space.** `src/engine_texture.cpp:220` hardcodes `VK_FORMAT_R8G8B8A8_SRGB` for every
texture, and `src/engine_model.cpp:266,291,327` passes SRGB for every glTF image. Which glTF
texture types are defined to be in linear space rather than sRGB? What does the sampler do to
a metallic-roughness texture in an sRGB-format image, and what does that do to your `roughness`
and `metallic` at `shaders/shader.frag:109-111`? Note also that `Init` takes an `imageFormat`
parameter used only for the *view* while the image is always created SRGB — think about format
compatibility rules there.

**No mipmaps.** `src/engine_texture.cpp:218` — `mipLevels = 1`; `:364` — `maxLod = 0.0f`. You
request anisotropic filtering at max degree (`:355-356`) over a single mip level, and
`EngineModel::getFilter`/`getSampler` (`src/engine_model.cpp:403-434`) decode the glTF sampler's
mipmap mode and are then **never called**. Two things to say about this. The visual one:
minification aliasing. The one I care about more, given what you say you want to study: what
happens to the texture cache hit rate when a 2048² texture is minified to a few pixels and
adjacent fragments sample texels hundreds of bytes apart? Put a number on the bandwidth you are
burning. That is the reasoning I want to see from a candidate in this area.

**The cubemap is a mirror, not IBL.** `shaders/shader.frag:152-156`:

```glsl
vec3 ambientSpecular = fragSkybox * f0 * (1.0 - roughness);
```

One mip-0 sample along the mirror direction, scaled ad hoc. `kIBL` is defined at `:92-95` and
never used. No prefiltered roughness mip chain, no irradiance map for the diffuse term, no
split-sum BRDF LUT. Also check energy conservation: `kD` is applied inside the direct loop at
`:147` but the `ambient` term at `:158` multiplies albedo unconditionally — what should a
metal's diffuse response be? And there is no tone mapping or exposure anywhere, with `1/d²`
attenuation feeding an 8-bit sRGB target. Calling this "cubemap reflections" in the README is
fair; calling the lighting PBR is a stretch while the ambient term is this.

**`fragColor`** is written by the vertex shader (`shader.vert:51`), declared as an input in the
fragment shader (`shader.frag:11`), and never read. Vertex colours are loaded from glTF,
interpolated across every triangle, and discarded.

**The outline pass cannot ever draw.** `shaders/outline.frag:21`: `discard;` — unconditional, at
the end of `main`. Every fragment is killed, so every `outColor` write above it is dead. Your
last commit message says "bugs still present"; this is one line. The logic above it is also
inverted — `if (uv.x != push.selectedID) outColor = red;` fires for any neighbour that is not
the selected object, which is most of the screen. State the silhouette predicate you actually
want in terms of the centre texel and its neighbours. Also: `texelFetch` on a `usampler2D`
returns `uvec4` and you assign it to `vec4` then compare against a `uint` — carry the integer
type through and the comparison stops being a float equality test. And `outline.vert:3`
declares the sampler in the vertex stage while the descriptor binding was created with
`VK_SHADER_STAGE_FRAGMENT_BIT` only (`src/engine_app.cpp:143-148`).

**Two passes disagree about the geometry they draw.** `SimpleRenderSystem::render` issues one
`vkCmdDrawIndexed(p.count, 1, p.startIndex, ...)` per surface
(`src/systems/simple_render_system.cpp:115-120`). `OffscreenSystem::render` calls
`mesh->draw()`, which is `vkCmdDrawIndexed(indexCount, surfaces_.size(), 0, 0, 0)`
(`src/engine_mesh.cpp:186`) — total index count, instanced by surface count, offset zero. What
geometry does each pass rasterize? They must match for picking to mean anything, and the second
form draws every surface's indices once per surface. (`startIndex` is computed in the loader and
ignored here.)

**Material indices.** `src/systems/simple_render_system.cpp:80,122-127`:

```cpp
std::map<int,bool> visited_materials;
...
materialOffset = visited_materials.size();
```

`populateMatTexDescriptorSets` (`src/engine_app.cpp:421-463`) assigns descriptor sets by
incrementing `materialCount` across *all* materials of each model in `geObjects` order. The
render loop reconstructs the offset as the number of *distinct materials actually drawn so far*.
Construct a case where a model owns more materials than its surfaces reference, and say what
material the next model gets bound. Also: a fresh `std::map` heap-allocating per frame, used as
a set, never cleared between objects.

**Minor but worth seeing:** `defaultPipelineConfigInfo` hardcodes
`rasterizationSamples = VK_SAMPLE_COUNT_4_BIT` (`src/engine_pipeline.cpp:165`) and
`PipeLineSettings::sampleCount` defaults the same way (`src/systems/system.hpp:24`), while
`EngineDevice::setMaxSampleCount` can legitimately fall back to 1× — and does not find the
*maximum* anything, it tests one bit (`src/engine_device.cpp:164-176`).
`BboxRenderer` draws 36 vertices in `VK_POLYGON_MODE_LINE` per box
(`src/systems/bbox_system.cpp:64`) — which edges does a wireframed triangle list give you that a
line list would not? `glm::mat4` projection uses conventional [0,1] depth with `COMPARE_OP_LESS`
over a 0.1–1000 range; given everything else you've read about depth precision, why not
reverse-Z? And `src/systems/outline_system.cpp:94` issues one fullscreen triangle **per selected
object**, each re-reading the whole ID buffer.

---

## 7. Memory, bandwidth, and the hardware story

You say you want to study memory management and hardware acceleration. Here is where this
project does and does not support that claim.

**Every upload is a full device stall.** `EngineDevice::endSingleTimeCommands`
(`src/engine_device.cpp:597-610`) submits and calls `vkQueueWaitIdle`. Count the calls on a
cold start: each texture costs three (two transitions plus the copy), each mesh costs two
(`src/engine_mesh.cpp:175-181` issues two separate `copyBuffer` calls from one staging buffer),
plus the cubemap, plus every BVH AABB upload. For a scene with a few dozen textures that is a
hundred-plus full pipeline drains, strictly serialized, on the graphics queue. You have a
`computeQueue`; you have no transfer queue; nothing is batched into a single command buffer with
one fence. This is the most legible performance story in the whole project and it is untouched.

**`VMA` is configured for Vulkan 1.0.** `createAllocator` (`src/engine_device.cpp:276-285`) sets
`physicalDevice`, `device`, `instance`, and the buffer-device-address flag. It does not set
`vulkanApiVersion`, and the return value is discarded. What does VMA default to, and which
allocation strategies (dedicated allocations, `*MemoryRequirements2`, 1.1+ memory budget
queries) does it then decline to use? You enabled `VK_API_VERSION_1_4` on the instance and then
told your allocator it was 2016.

**MSAA attachments.** `src/engine_swapchain.cpp:268` requests
`VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT` with `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`. What
memory property makes `TRANSIENT` actually mean something, and what does it buy you on a
tile-based GPU like Apple's? You are allocating three full-resolution 4×-MSAA colour targets
*and* three 4×-MSAA depth targets (`:300-351`) — compute that footprint at 2560×1960 and ask how
many of each you need, given that `storeOp` is already `DONT_CARE` and the resolve happens
in-pass (`src/engine_renderer.cpp:186-190`, which is correctly done).

**One `VkSampler` per texture.** `EngineTexture::createSampler` is called from every
constructor, so a hundred textures produce a hundred identical samplers.
`maxSamplerAllocationCount` is a real device limit (commonly 4000) and samplers are descriptor-
adjacent state, not per-image state. You already built a bindless image array — what is the
natural companion structure for samplers?

**Geometry copied needlessly.** `EngineMesh::createBuffer` takes
`std::vector<uint32_t> indices, std::vector<Vertex> vertices` **by value**
(`src/engine_mesh.cpp:129-131`), called from a constructor that already took them by const
reference. That is a full copy of every mesh's geometry per mesh, at `-O0`, during load.

**Push constant budget.** `SimplePushConstantData` is three `mat4`s plus a device address — 200
bytes (`src/systems/simple_render_system.cpp:17-22`). What is the Vulkan *guaranteed minimum*
`maxPushConstantsSize`? You never read `properties.limits.maxPushConstantsSize` anywhere. Also
note two of those three matrices are per-object, re-pushed per mesh.

**Device feature validation.** `isDeviceSuitable` (`src/engine_device.cpp:292-310`) checks queue
families, extension support, swapchain adequacy, and `samplerAnisotropy`. `createLogicalDevice`
then requests `dynamicRendering`, `bufferDeviceAddress`, and six descriptor-indexing features
(`:224-246`). `vkGetPhysicalDeviceFeatures2` is never called. What happens on a device that
does not support `descriptorBindingSampledImageUpdateAfterBind`?

---

## 8. Design and discipline

I will be brief here because these are symptoms, not causes — but a reviewer reads them as
signal about how you work.

- **Every render system privately inherits its base.** `class SimpleRenderSystem : RenderSystem`
  (`src/systems/simple_render_system.hpp:12`), and the same in all six. `class` default
  inheritance is private. So `RenderSystem::render` is pure virtual, overridden six times, and
  callable through the base from nowhere. The destructor is also non-virtual
  (`src/systems/system.cpp:25`). Decide whether you want a polymorphic base; right now you have
  the syntax of one and none of the function.
- **`struct Shader` holds two `const std::string&`** (`src/systems/system.hpp:16-19`) and is
  instantiated from string literals at namespace scope in a header (`src/shaderList.hpp`). Work
  out why that is currently legal, then decide whether you want a type whose correctness depends
  on lifetime-extension rules that most readers cannot recite.
- **Four struct layouts are hand-duplicated between C++ and GLSL** with nothing enforcing
  agreement: `GlobalUbo`, `MaterialProperties`, `Vertex`, `PointLight`, plus `MAX_TEXTURES`
  (`src/engine_texture.hpp:20` vs `shaders/shader.frag:9`) and `MAX_LIGHTS` (a `#define` vs a
  literal `[10]` in five shaders). To your credit the std140/std430 padding in
  `MaterialProperties` is actually correct and commented — you clearly thought about it once.
  But there is no `static_assert`, no generated header, no single source of truth. The next
  field you add to a UBO will silently desynchronize, and the symptom will be garbage lighting,
  not a compile error.
- **Two copies of `vulkan_core.h` in the same translation units.** Nine files include
  `"sdl/vendored/SDL/src/video/khronos/vulkan/vulkan_core.h"` — SDL's internal vendored Vulkan
  headers — alongside the real SDK's `<vulkan/vulkan_core.h>`. Which one wins is an include-guard
  race. Delete all of them.
- **The same headers are included under two different paths** (`"engine_device.hpp"` and
  `"src/engine_device.hpp"`) in different files. `core.hpp` and `src/render_systems.hpp` have
  **no include guard at all**.
- **`CMakeLists.txt:23`** globs `src/render_sysetms/*.cpp` — misspelled, and the directory
  does not exist. `GLOB_RECURSE` on `src/*.cpp` already covers everything, so this line has
  never done anything.
- **No pipeline cache.** `vkCreateGraphicsPipelines` is called with `VK_NULL_HANDLE`
  (`src/engine_pipeline.cpp:100`) for seven pipelines, every launch.
- **`GraphicsPipeline::readFile`** returns `std::vector<char>` which is then
  `reinterpret_cast<const uint32_t*>` (`src/engine_pipeline.cpp:124`). What alignment does
  `std::vector<char>` guarantee? Is `codeSize` checked to be a multiple of 4?
- **`EngineDescriptorPool::freeDescriptors`** calls `vkFreeDescriptorSets`
  (`src/engine_descriptor.cpp:156`) on pools created without
  `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT` (`:296-297` sets only
  `UPDATE_AFTER_BIND`). Currently unreachable; still wrong.
- **`EngineDescriptorPoolGrowable::allocateDescriptorSet`** (`:326-334`) loops until allocation
  succeeds, with your own `// FIXME: handle return vals properly`. Name a failure mode other
  than pool exhaustion and say what that loop does.
- **Dead code with no owner.** `EngineScene` is an empty class with three undeclared-but-declared
  members — it would fail to link if used. `EngineCamera` has eight unused private members
  (`Yaw`, `Pitch`, `MovementSpeed`, `Zoom`, …) and an undefined `updateCameraVectors()`.
  `EngineController::KeyStates` stores keycodes while the implementation hardcodes scancodes, and
  carries a `std::vector<VkImage>` and an undefined `VkResult render()`.
  `GraphicsPipeline::enableAlphaBlending` is never called. `EngineUI::setStyle` copies the style
  and discards it; `renderLightUI` is never called and calls `ImGui::End()` inside the
  `if (ImGui::Begin(...))` block, which is the one thing ImGui tells you never to do. You said
  not to clean up commented-out work in progress and I have not — but these are not
  work-in-progress, they are abandoned, and they make the real surface area hard to find.
- **Three different formatting styles** in the tree (2-space Google-ish in most files, 4-space
  with different brace placement in `engine_descriptor`/`engine_camera`/`engine_window`,
  LLVM-style in `engine_game_object`/`pbrMaterials`) and no `.clang-format` committed.
- **No tests, no CI, no asserts on device limits.** 127 commits over nine months and nothing that
  can tell you the engine still works except launching it and looking. For a project whose
  hardest component (the BVH) produces output you cannot see, that is the gap that matters.
- `src/engine_app.cpp:289` reads `ImGui::GetIO().MouseDownOwned[0]` — an internal field. The
  public query for "is the UI eating my click" is right next to it.
- `src/engine_app.cpp:392` does `geObjects.at(objectID)` on an ID read from a GPU pixel. `.at()`
  throws. Given section 4, how confident are you in that value?
- `build.sh` has shader compilation commented out, so `.spv` files can silently lag their
  sources; `CMakeLists.txt` *does* have a working shader target. Pick one.
- `models/` is in `.gitignore`, two of the three models the app loads are untracked, and
  `notes.md` (referenced in your own `CLAUDE.md`) does not exist. A clean clone throws on
  startup.

---

## 9. README honesty

Your README is well written, and I appreciate the explicit note about which code is yours and
which was generated — that is the right instinct and more candour than most applicants show.
Keep doing that.

But three of the bullets under Features are not true of the code in this repository: normal
mapping (sampled and discarded), object picking (marked "redoing", and currently unsynchronized),
and the implication that the AABB debug view validates the BVH (it draws the inputs). Whatever
you send to an admissions committee, make the claims match the code exactly. I checked, and I
check because I expect to find drift; finding three is worse than finding a shorter list.

---

## 10. What I would want to see

If you asked me what to do between now and an application deadline, in this order:

1. **Turn on `-Wall -Wextra`** and fix everything it says. Then make
   `-DCMAKE_BUILD_TYPE=Release` actually work and re-measure everything you think you know
   about this engine's performance.
2. **Default-initialize every member of every struct in the project**, then go back and read
   section 3. I think one of those is the "catastrophic bug."
3. **Build the thing that tells you whether the BVH is correct.** A CPU reference build over the
   same AABBs, a readback, and a diff. Invariant checks. Then draw `nodeAABBs`, not the leaves.
   Until that exists you are writing GPU code blind, and the next bug will cost you a week.
4. **Fix picking properly** — one fence, one semaphore, a one-pixel copy region — and write down
   why each is needed. Synchronization you can explain is worth more to me than a feature list.
5. **Enable sync validation and best-practices layers** and work through the output. I expect it
   to be loud.
6. **Pick one of normal mapping or real IBL and finish it end to end**, including the tangent
   basis or the prefiltered mip chain. One complete thing beats five partial ones, and partial
   ones are what the feature list currently is.
7. Delete the dead code. It is hiding the good code.

## 11. Overall assessment

### What earns you a serious read

You chose your own curriculum, and you chose well. Buffer-device-address vertex pulling,
bindless descriptors with `UPDATE_AFTER_BIND`, dynamic rendering, and a from-paper Karras LBVH
is not a trajectory anyone assigns. Most applications I see in this area are a Shirley
raytracer, a Unity capstone, or coursework with the scaffolding still attached. A nine-month,
127-commit, from-scratch Vulkan renderer that reaches for a GPU acceleration structure is
top-decile *ambition*, and ambition is the part I cannot teach. The execution rigour is median.
That asymmetry is the whole review.

### The one real deficit

Every item in this document collapses into a single thing: **you have no feedback loop.**
Warnings muted, validation at default severity with no synchronization or best-practices layers,
no optimized build, no GPU timestamp queries, no tests, no readback, no CPU reference for the
tree. The bugs are not the problem. The bugs are *downstream*. Someone with exactly your
knowledge and a working instrument would have found most of them unaided — which is precisely
why their presence tells me less about what you know than it appears to.

In graphics this matters more than in most subfields, because wrong GPU code renders *almost*
correctly. A malformed BVH still draws a picture. A race you cannot observe on MoltenVK is a
correct program on your laptop. Which brings up the theme I would press hardest in an interview:
you are being carried by your platform in at least four places — one universal queue, stronger
serialization than the spec promises — and you have been reading that generosity as the spec's
guarantees. Learning to tell those apart is most of the discipline.

### On the BVH specifically

The right thing to have picked, the wrong thing to have at 80%. An acceleration structure with
no traversal, no consumer, and no verification is the least informative possible state: all of
the cost, none of the payoff, and no evidence it works. One complete, verified, measured
component is worth more to a committee than five partial ones — and the feature list currently
reads as five partial ones.

### The most promising line in the repository is in `TODO.md`

> *Diff Geometry — within UI, add options to find key quantities*

That is a research instinct, and it is buried in a gitignored scratch file while the README leads
with a feature inventory. Curvature, geodesics, discrete differential operators on these meshes
— that is a direction, not a demo, and it is the thing I would want to talk to you about. Lead
with the intellectual thread. *"I built a renderer so I would have a substrate for geometry
processing, and here is the BVH I need for it"* is a far stronger framing than a bullet list,
and it appears to also be the truth. Right now the project undersells its own motivation.

### Bottom line

Admit-adjacent, not waved through. This project demonstrates you will work hard and unsupervised
on genuinely difficult material, which is the main predictor of surviving a research MS. What it
does not yet demonstrate is that you can tell when you are wrong.

None of that is a ceiling on your ability. Every item in this review is a day or two of work for
someone who already knows this much Vulkan, and several are one line. What it is, is a ceiling
on what I can currently *infer*, because the project does not yet distinguish "knows how to
validate GPU work" from "got lucky on this GPU." Build the instrument — a week, mostly flags and
one reference implementation — and that second question answers itself.

I would rather read this project again in two months with validation clean, a Release build, and
a verified tree than read a longer feature list. The raw material is here. Go be rigorous about
it.

---

[^1]: On the state I received it in: the working tree did not compile —
`src/engine_app.hpp:6` could not resolve `engine_ui.hpp`, because `engine_ui.{hpp,cpp}` had been
moved to `src/ui/` without adding that directory to `target_include_directories`. You have since
told me this was a file reorganisation in progress, which I accept; a dirty tree mid-refactor is
not a discipline signal and I have struck it from the assessment. Worth being precise about what
that absolves, though. The unoptimised-only build (§2), the muted warnings (§3), the
uninitialised members (§3), the unsynchronised picking path (§4), and the unverifiable BVH (§5)
are all committed and months old. One item off the list; the rest stand.
