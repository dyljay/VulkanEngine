# Current Tasks:

## Functional fixes:
~~1. Update to include the /path/to/shaders/ folder to the project so i don't have to use absolute path~~
~~2. set up automatic compilation for shaders and place into /shader/ folder~~
~~- this will benefit me when I am working with lighting and don't have to recompile every file manually~~
3. update to make sure the number of semaphores = images in the frame buffer. make sure each one can have one or use the khronos proposed fix and just assign each image a semaphore instead of each frame
- https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Frames_in_flight


## Upcoming tasks:
1. **!REFACTOR THE CODE!** it's already a pain to work with
- will take a while
2. (related to 1) Re-read all the code and build a mental model of what is going on *will take a while*
- https://vulkan-tutorial.com/images/vulkan_pipeline_block_diagram.png
3. Apply image to skybox
- reference: https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdBlitImage.html
- use this command to copy an image to the color buffer before rendering to it with other drawing commands
4. Implement ImGUI
5. Lighting
6. generate mipmaps before runtime and store them for use during runtime

## Optional tasks:
1. On the staging buffer section (https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer) there is a challenge portion to give a shot to
- make sure mipmaps are submitted to graphics queue

# General Learning Questions:
1. Why do we stage memory allocation then bind the item to that memory instead of just attaching that object in memory in 1 step?

# Future, long-term plans:

## Collision detection:
