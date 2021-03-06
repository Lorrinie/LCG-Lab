#include "RenderFramework.hpp"

#include "Helper.hpp"

void RenderFramework::update(void * sender, float deltaTime)
{
	mOccupancyGeometry.clear();

	//new occupancy geometry
	mOccupancyHistogramTree->setEyePosition(mPosition);
	mOccupancyHistogramTree->getOccupancyGeometry(mOccupancyGeometry);
	
	//new ray segment list
	renderRaySegmentList();
}

void RenderFramework::render(void * sender, float deltaTime)
{
	float rgba[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	mRasterizerState->setCullMode(CullMode::Back);
	mRasterizerState->setFillMode(FillMode::Solid);
	mDepthStencilState->setDepthEnable(false);

	mGraphics->clearState();
	mGraphics->clearRenderTarget(mSwapChain->getRenderTarget(), rgba);

	mGraphics->setRenderTarget(mSwapChain->getRenderTarget());
	mGraphics->setViewPort(0, 0, (float)mWidth, (float)mHeight);

	mGraphics->setVertexBuffer(mCubeVertexBuffer);
	mGraphics->setIndexBuffer(mCubeIndexBuffer);
	mGraphics->setInputLayout(mInputLayout);

	mGraphics->setRasterizerState(mRasterizerState);
	mGraphics->setDepthStencilState(mDepthStencilState);

	mGraphics->setVertexShader(mRayCastRenderingVertexShader);
	mGraphics->setPixelShader(mRayCastRenderingPixelShader);

	mGraphics->setConstantBuffer(mMatrixBuffer, 0);

	mGraphics->setResourceUsage(mVolumeTextureUsage, 0);
	mGraphics->setResourceUsage(mRaySegmentListCountSRVUsage, 1);
	mGraphics->setResourceUsage(mRaySegmentListDepthSRVUsage, 2);
	mGraphics->setResourceUsage(mRaySegmentListBoxTypeSRVUsage, 3);
	mGraphics->setResourceUsage(mRaySegmentListEventTypeSRVUsage, 4);

	mGraphics->setPrimitiveType(PrimitiveType::TriangleList);

	mGraphics->drawIndexed(36, 0, 0);

	mSwapChain->present(false);
}

void RenderFramework::renderInstance(int requireInstanceCount)
{
	//instance 
	if ((int)mInstanceData.size() < requireInstanceCount) return;

	for (size_t i = mInstanceData.size(); i < MAX_INSTANCE_PER_DRAW; i++)
		mInstanceData.push_back(InstanceData());

	mInstanceBuffer->update(&mInstanceData[0]);

	mGraphics->setConstantBuffer(mInstanceBuffer, 1);

	mGraphics->drawIndexedInstanced(36, requireInstanceCount, 0, 0);

	mInstanceData.clear();
}

void RenderFramework::renderRaySegmentList()
{
	float rgba[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	unsigned int clearValue[4] = { 0,0,0,0 };

	mGraphics->clearUnorderedAccessUsageUint(mRaySegmentListCountUAVUsage, clearValue);

	mRasterizerState->setFillMode(FillMode::Solid);
	mRasterizerState->setCullMode(CullMode::None);
	mDepthStencilState->setDepthEnable(false);

	mGraphics->clearState();

	mGraphics->setRenderTarget(mOccupancyGeometryRenderTarget);
	mGraphics->clearRenderTarget(mOccupancyGeometryRenderTarget, rgba);

	mGraphics->setViewPort(0, 0, (float)mWidth, (float)mHeight);

	mGraphics->setVertexBuffer(mCubeVertexBuffer);
	mGraphics->setIndexBuffer(mCubeIndexBuffer);
	mGraphics->setInputLayout(mInputLayout);

	mGraphics->setRasterizerState(mRasterizerState);
	mGraphics->setDepthStencilState(mDepthStencilState);

	mGraphics->setVertexShader(mOccupancyGeometryVertexShader);
	mGraphics->setPixelShader(mOccupancyGeometryPixelShader);

	mGraphics->setConstantBuffer(mMatrixBuffer, 0);

	std::vector<UnorderedAccessUsage*> unorderedAccessUsages(4);

	//commit the RWTexture
	unorderedAccessUsages[0] = mRaySegmentListCountUAVUsage;
	unorderedAccessUsages[1] = mRaySegmentListDepthUAVUsage;
	unorderedAccessUsages[2] = mRaySegmentListBoxTypeUAVUsage;
	unorderedAccessUsages[3] = mRaySegmentListEventTypeUAVUsage;

	mGraphics->setUnorderedAccessUsage(unorderedAccessUsages);

	mGraphics->setPrimitiveType(PrimitiveType::TriangleList);

	//to do: use the draw instance
	for (auto it = mOccupancyGeometry.begin(); it != mOccupancyGeometry.end(); it++) {
		OccupancyHistogramNodeCompareComponent component = *it;
		InstanceData data;

		//transform 
		data.Transform = glm::translate(glm::mat4(1), (component.Node->AxiallyAlignedBoundingBox.Max + component.Node->AxiallyAlignedBoundingBox.Min) * 0.5f);
		data.Transform = glm::scale(glm::mat4(data.Transform), component.Node->AxiallyAlignedBoundingBox.Max - component.Node->AxiallyAlignedBoundingBox.Min);

		data.Setting[0] = (int)component.IsFrontFace;
		data.Setting[1] = (int)component.Node->Type;
		data.Setting[2] = (int)(component.Node->Parent != nullptr ? component.Node->Parent->Type : OccupancyType::Empty);

		mInstanceData.push_back(data);

		renderInstance(MAX_INSTANCE_PER_DRAW);
	}

	if (mInstanceData.size() != (size_t)0) renderInstance((int)mInstanceData.size());
}

auto RenderFramework::sampleVolumeData(AxiallyAlignedBoundingBox sampleBox, int width, int height, int depth, std::vector<unsigned char>& volumeData) -> float
{
	int startX, startY, startZ;
	int endX, endY, endZ;

	//find the area where we want to sample
	startX = (int)(sampleBox.Min.x * width);
	startY = (int)(sampleBox.Min.y * height);
	startZ = (int)(sampleBox.Min.z * depth);

	endX = (int)(sampleBox.Max.x * width);
	endY = (int)(sampleBox.Max.y * height);
	endZ = (int)(sampleBox.Max.z * depth);

	float sum = 0;
	int count = 0;

	for (int x = startX; x < endX; x++) {
		for (int y = startY; y < endY; y++) {
			for (int z = startZ; z < endZ; z++) {
				int index = z * width * height + y * width + x;

				sum = sum + volumeData[index] / 255.0f;
				count = count + 1;
			}
		}
	}

	return sum / count;
}

void RenderFramework::buildState()
{
	mRasterizerState = mFactory->createRasterizerState();
	mDepthStencilState = mFactory->createDepthStencilState();
}

void RenderFramework::buildCamera()
{
	mPosition = glm::vec3(0.0f, -5.0f, 0.0f);
	mLookAt = glm::vec3(0.0f, 0.0f, 0.0f);
	mUp = glm::vec3(0.0f, 0.0f, -1.0f);
}

void RenderFramework::buildBuffer()
{
	mCubeVertexBuffer = mFactory->createVertexBuffer(sizeof(Mesh::Vertex) * 8, sizeof(Mesh::Vertex), ResourceInfo::VertexBuffer());
	mCubeIndexBuffer = mFactory->createIndexBuffer(sizeof(unsigned int) * 36, ResourceInfo::IndexBuffer());
	mMatrixBuffer = mFactory->createConstantBuffer(sizeof(mMatrix), ResourceInfo::ConstantBuffer());
	mInstanceBuffer = mFactory->createConstantBuffer(sizeof(InstanceData) * MAX_INSTANCE_PER_DRAW, ResourceInfo::ConstantBuffer());

	mMatrix[0] = glm::scale(glm::mat4(1), glm::vec3(3, 3, 3));
	mMatrix[1] = glm::lookAt(mPosition, mLookAt, mUp);
	mMatrix[2] = glm::perspectiveFov(glm::pi<float>() * 0.55f, (float)mWidth, (float)mHeight, 1.0f, 100.0f);
	mMatrix[3][0] = glm::vec4(mPosition, 0.0f);

	mCubeVertexBuffer->update(Mesh::Cube(glm::vec3(1.0f)).vertices().data());
	mCubeIndexBuffer->update(Mesh::Cube(glm::vec3(1.0f)).indices().data());
	mMatrixBuffer->update(mMatrix);
}

void RenderFramework::buildShader()
{
	mOccupancyGeometryVertexShader = mFactory->createVertexShader(Helper::ReadFile("OccupancyGeometryVertexShader.cso"), true);
	mOccupancyGeometryPixelShader = mFactory->createPixelShader(Helper::ReadFile("OccupancyGeometryPixelShader.cso"), true);
	
	mRayCastRenderingVertexShader = mFactory->createVertexShader(Helper::ReadFile("RayCastVertexShader.cso"), true);
	mRayCastRenderingPixelShader = mFactory->createPixelShader(Helper::ReadFile("RayCastPixelShader.cso"), true);
}

void RenderFramework::buildTexture()
{
	mOccupancyGeometryRenderTexture = mFactory->createTexture2D(mWidth, mHeight, PixelFormat::R8G8B8A8Unknown, BindUsage::RenderTargetUsage);

	const auto resourceInfo = ResourceInfo(BindUsage(
		static_cast<unsigned int>(BindUsage::ShaderResourceUsage) |
		static_cast<unsigned int>(BindUsage::UnorderedAccessUsage)));

	mRaySegmentListCountTexture = mFactory->createTexture2D(mWidth, mHeight, PixelFormat::R32Uint, resourceInfo);
	mRaySegmentListDepthTexture = mFactory->createTexture3D(mWidth, mHeight, MAX_RAYSEGMENT, PixelFormat::R32Float, resourceInfo);
	mRaySegmentListBoxTypeTexture = mFactory->createTexture3D(mWidth, mHeight, MAX_RAYSEGMENT, PixelFormat::R8Uint, resourceInfo);
	mRaySegmentListEventTypeTexture = mFactory->createTexture3D(mWidth, mHeight, MAX_RAYSEGMENT, PixelFormat::R8Uint, resourceInfo);

	mOccupancyGeometryRenderTarget = mFactory->createRenderTarget(mOccupancyGeometryRenderTexture, PixelFormat::R8G8B8A8Unknown);

	mRaySegmentListCountUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListCountTexture, PixelFormat::R32Uint);
	mRaySegmentListDepthUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListDepthTexture, PixelFormat::R32Float);
	mRaySegmentListBoxTypeUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListBoxTypeTexture, PixelFormat::R8Uint);
	mRaySegmentListEventTypeUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListEventTypeTexture, PixelFormat::R8Uint);

	mRaySegmentListCountSRVUsage = mFactory->createResourceUsage(mRaySegmentListCountTexture, PixelFormat::R32Uint);
	mRaySegmentListDepthSRVUsage = mFactory->createResourceUsage(mRaySegmentListDepthTexture, PixelFormat::R32Float);
	mRaySegmentListBoxTypeSRVUsage = mFactory->createResourceUsage(mRaySegmentListBoxTypeTexture, PixelFormat::R8Uint);
	mRaySegmentListEventTypeSRVUsage = mFactory->createResourceUsage(mRaySegmentListEventTypeTexture, PixelFormat::R8Uint);
}

void RenderFramework::buildVolumeData()
{
	std::ifstream file("Teddybear.raw", std::ios::ate | std::ios::binary);

	size_t fileSize = (size_t)file.tellg();

	mVolumeData.resize(fileSize);

	file.seekg(0, std::ios::beg);
	file.read((char*)&mVolumeData[0], fileSize);
	file.close();

	mOccupancyHistogramTree = new OccupancyHistogramTree();

	mOccupancyHistogramTree->setMaxDepth(MAX_DEPTH);
	mOccupancyHistogramTree->setSize(AxiallyAlignedBoundingBox(-CUBE_SIZE * 0.5f, CUBE_SIZE * 0.5f));

	int width = 128;
	int height = 128;
	int depth = 62;

	int size = (int)std::pow(2, MAX_DEPTH - 1);

	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			for (int z = 0; z < size; z++) {
				glm::vec3 position;

				position.x = ((float)x + 0.5f) / size;
				position.y = ((float)y + 0.5f) / size;
				position.z = ((float)z + 0.5f) / size;

				position = position * CUBE_SIZE - CUBE_SIZE * 0.5f;

				AxiallyAlignedBoundingBox sampleBox;

				sampleBox.Min = glm::vec3((float)x / size, (float)y / size, (float)z / size);
				sampleBox.Max = glm::vec3((float)(x + 1) / size, (float)(y + 1) / size, (float)(z + 1) / size);

				float sampleValue = sampleVolumeData(sampleBox, width, height, depth, mVolumeData);

				if (sampleValue <= EMPTY_LIMIT)
					mOccupancyHistogramTree->insertNoEmpty(position, OccupancyType::Empty);
				else
					mOccupancyHistogramTree->insertNoEmpty(position, OccupancyType::NoEmpty);
			}
		}
	}

	mOccupancyHistogramTree->buildVirtualTree();
	mOccupancyHistogramTree->setEyePosition(mPosition);
	mOccupancyHistogramTree->getOccupancyGeometry(mOccupancyGeometry);

	//build volume texture
	mVolumeTexture = mFactory->createTexture3D(width, height, depth, PixelFormat::R8Unknown, ResourceInfo::ShaderResource());
	mVolumeTextureUsage = mFactory->createResourceUsage(mVolumeTexture, PixelFormat::R8Unknown);

	mVolumeTexture->update(&mVolumeData[0], 0, 0, 0, width, height, depth);
}

void RenderFramework::buildInputLayout()
{
	mInputLayout = mFactory->createInputLayout();

	mInputLayout->addElement(InputLayoutElement("POSITION", sizeof(glm::vec3)));
	mInputLayout->addElement(InputLayoutElement("TEXCOORD", sizeof(glm::vec3)));
}

void RenderFramework::destoryState()
{
	mFactory->destroyRasterizerState(mRasterizerState);
	mFactory->destroyDepthStencilState(mDepthStencilState);
}

void RenderFramework::destoryCamera()
{
}

void RenderFramework::destoryBuffer()
{
	mFactory->destroyVertexBuffer(mCubeVertexBuffer);
	mFactory->destroyIndexBuffer(mCubeIndexBuffer);
	mFactory->destroyConstantBuffer(mMatrixBuffer);
	mFactory->destroyConstantBuffer(mInstanceBuffer);
}

void RenderFramework::destoryShader()
{
	mFactory->destroyVertexShader(mOccupancyGeometryVertexShader);
	mFactory->destroyPixelShader(mOccupancyGeometryPixelShader);

	mFactory->destroyVertexShader(mRayCastRenderingVertexShader);
	mFactory->destroyPixelShader(mRayCastRenderingPixelShader);
}

void RenderFramework::destoryTexture()
{
	mFactory->destroyTexture2D(mOccupancyGeometryRenderTexture);

	mFactory->destroyTexture2D(mRaySegmentListCountTexture);
	mFactory->destroyTexture3D(mRaySegmentListDepthTexture);
	mFactory->destroyTexture3D(mRaySegmentListBoxTypeTexture);
	mFactory->destroyTexture3D(mRaySegmentListEventTypeTexture);

	mFactory->destroyRenderTarget(mOccupancyGeometryRenderTarget);

	mFactory->destroyUnorderedAccessUsage(mRaySegmentListCountUAVUsage);
	mFactory->destroyUnorderedAccessUsage(mRaySegmentListDepthUAVUsage);
	mFactory->destroyUnorderedAccessUsage(mRaySegmentListBoxTypeUAVUsage);
	mFactory->destroyUnorderedAccessUsage(mRaySegmentListEventTypeUAVUsage);

	mFactory->destroyResourceUsage(mRaySegmentListCountSRVUsage);
	mFactory->destroyResourceUsage(mRaySegmentListDepthSRVUsage);
	mFactory->destroyResourceUsage(mRaySegmentListBoxTypeSRVUsage);
	mFactory->destroyResourceUsage(mRaySegmentListEventTypeSRVUsage);
}

void RenderFramework::destoryVolumeData()
{
	Utility::Delete(mOccupancyHistogramTree);
	
	mFactory->destroyTexture3D(mVolumeTexture);
	mFactory->destroyResourceUsage(mVolumeTextureUsage);
}

void RenderFramework::destoryInputLayout()
{
	mFactory->destroyInputLayout(mInputLayout);
}

RenderFramework::RenderFramework(const std::string & name, int width, int height)
	: WindowsFramework(name, width, height)
{

}

RenderFramework::~RenderFramework() 
{
	destoryState();
	destoryCamera();
	destoryBuffer();
	destoryShader();
	destoryTexture();
	destoryVolumeData();
	destoryInputLayout();
}

void RenderFramework::build()
{
	buildState();
	buildCamera();
	buildBuffer();
	buildShader();
	buildTexture();
	buildVolumeData();
	buildInputLayout();

	renderRaySegmentList();
}