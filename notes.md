struct Geosurface {
  uint32_t startIndex;
  uint32_t count;
};
notes: the start of where each surface (primitive of a mesh) start and how many indices there are 

struct MeshAsset {
  string name;

  vector<Geosurface> surfaces;
  GPUMeshBuffers meshBuffers;
};

notes: self explanatory - name, surface (to get offsets for primitive), and the associated buffers 


struct AllocatedBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo info;
};

 | 
\/

struct GPUMeshBuffers {
  AllocatedBuffer indexBuffer;
  AllocatedBuffer vertexBuffer;
  VkDeviceAddress vertexBufferAddress;
};

**notes**: in mesh MeshAsset

struct GLTFMEtallic_Roughness {

struct MaterialConstants {
	glm::vec4 colorFactors;
	glm::vec4 metal_rough_factors;
	//padding, we need it anyway for uniform buffers
	glm::vec4 extra[14];
};


struct MaterialResources {
	AllocatedImage colorImage;
	VkSampler colorSampler;
	AllocatedImage metalRoughImage;
	VkSampler metalRoughSampler;
	VkBuffer dataBuffer;
	uint32_t dataBufferOffset;
};

Description Writer
};

\/

struct AllocatedImage {
  VkImage _image;
	VmaAllocation _allocation;
	VkImageView _defaultView;
	int mipLevels;
};

class IRenderable {

  virtual void Draw(mat4 topMatrix, DrawContext& ctx) = 0;
}

class IRenderable {

    virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) = 0;
};

struct/class Node {}

struct RenderObject {
	uint32_t indexCount;
	uint32_t firstIndex;
	VkBuffer indexBuffer;

	MaterialInstance* material;

	glm::mat4 transform;
	VkDeviceAddress vertexBufferAddress;
};

struct GLTFMaterial {
  MaterialInstance data;
};

struct MaterialInstance {
    MaterialPipeline* pipeline;
    VkDescriptorSet materialSet;
    MaterialPass passType;
};
