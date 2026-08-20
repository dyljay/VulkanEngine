# graphics shaders
/opt/homebrew/bin/glslang -V shaders/shader.vert -o shaders/vert.spv
/opt/homebrew/bin/glslang -V shaders/shader.frag -o shaders/frag.spv
/opt/homebrew/bin/glslang -V shaders/point_light.vert -o shaders/point_light.vert.spv
/opt/homebrew/bin/glslang -V shaders/point_light.frag -o shaders/point_light.frag.spv
/opt/homebrew/bin/glslang -V shaders/cubemap_vert.vert -o shaders/cubemap_vert.vert.spv
/opt/homebrew/bin/glslang -V shaders/cubemap_frag.frag -o shaders/cubemap_frag.frag.spv
/opt/homebrew/bin/glslang -V shaders/id_obj.vert -o shaders/id_obj.vert.spv
/opt/homebrew/bin/glslang -V shaders/id_obj.frag -o shaders/id_obj.frag.spv
/opt/homebrew/bin/glslang -V shaders/bbox.vert -o shaders/bbox.vert.spv
/opt/homebrew/bin/glslang -V shaders/bbox.frag -o shaders/bbox.frag.spv

# compute shaders
/opt/homebrew/bin/glslang -V shaders/morton_code.comp -o shaders/morton_code.comp.spv
/opt/homebrew/bin/glslang -V shaders/single_radixsort.comp -o shaders/single_radixsort.comp.spv
/opt/homebrew/bin/glslang -V shaders/radix_tree_build.comp -o shaders/radix_tree_build.comp.spv
/opt/homebrew/bin/glslang -V shaders/aabb_propagate.comp -o shaders/aabb_propagate.comp.spv
