#include "SparseLeapManager.hpp"
#include "SharedMacro.hpp"

SparseLeapManager::SparseLeapManager(Factory* factory, int width, int height) :
	mFactory(factory), mWidth(width), mHeight(height), mCube(), mOccupancyGeometryVertexShader(nullptr),
	mOccupancyGeometryPixelShader(nullptr),
	mOccupancyGeometryRenderTexture(nullptr),
	mRaySegmentListCountTexture(nullptr),
	mRaySegmentListDepthTexture(nullptr),
	mRaySegmentListBoxTypeTexture(nullptr),
	mRaySegmentListEventTypeTexture(nullptr),
	mOccupancyGeometryRenderTarget(nullptr),
	mRaySegmentListCountUAVUsage(nullptr),
	mRaySegmentListDepthUAVUsage(nullptr),
	mRaySegmentListBoxTypeUAVUsage(nullptr),
	mRaySegmentListEventTypeUAVUsage(nullptr),
	mRaySegmentListCountSRVUsage(nullptr),
	mRaySegmentListDepthSRVUsage(nullptr),
	mRaySegmentListBoxTypeSRVUsage(nullptr),
	mRaySegmentListEventTypeSRVUsage(nullptr),
	mOccupancyHistogramTree(nullptr) {
}

void SparseLeapManager::initialize(const glm::vec3 &cube)
{
	mCube = cube;

	//create shader
	mOccupancyGeometryVertexShader = mFactory->createVertexShader(Helper::readFile("OccupancyGeometryVertexShader.cso"), true);
	mOccupancyGeometryPixelShader = mFactory->createPixelShader(Helper::readFile("OccupancyGeometryPixelShader.cso"), true);

	//create texture
	mOccupancyGeometryRenderTexture = mFactory->createTexture2D(mWidth, mHeight, PixelFormat::R8G8B8A8Unknown, ResourceInfo(BindUsage::RenderTargetUsage));

	const auto resourceInfo = ResourceInfo(BindUsage(
		static_cast<unsigned int>(BindUsage::ShaderResourceUsage) | 
		static_cast<unsigned int>(BindUsage::UnorderedAccessUsage)));

	mRaySegmentListCountTexture = mFactory->createTexture2D(mWidth, mHeight, PixelFormat::R32Uint, resourceInfo);
	mRaySegmentListDepthTexture = mFactory->createTexture3D(mWidth, mHeight, MAX_RAYSEGMENT_COUNT, PixelFormat::R32Float, resourceInfo);
	mRaySegmentListBoxTypeTexture = mFactory->createTexture3D(mWidth, mHeight, MAX_RAYSEGMENT_COUNT, PixelFormat::R8Uint, resourceInfo);
	mRaySegmentListEventTypeTexture = mFactory->createTexture3D(mWidth, mHeight, MAX_RAYSEGMENT_COUNT, PixelFormat::R8Uint, resourceInfo);

	mOccupancyGeometryRenderTarget = mFactory->createRenderTarget(mOccupancyGeometryRenderTexture, PixelFormat::R8G8B8A8Unknown);

	mRaySegmentListCountUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListCountTexture, PixelFormat::R32Uint);
	mRaySegmentListDepthUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListDepthTexture, PixelFormat::R32Float);
	mRaySegmentListBoxTypeUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListBoxTypeTexture, PixelFormat::R8Uint);
	mRaySegmentListEventTypeUAVUsage = mFactory->createUnorderedAccessUsage(mRaySegmentListEventTypeTexture, PixelFormat::R8Uint);

	mRaySegmentListCountSRVUsage = mFactory->createResourceUsage(mRaySegmentListCountTexture, PixelFormat::R32Uint);
	mRaySegmentListDepthSRVUsage = mFactory->createResourceUsage(mRaySegmentListDepthTexture, PixelFormat::R32Float);
	mRaySegmentListBoxTypeSRVUsage = mFactory->createResourceUsage(mRaySegmentListBoxTypeTexture, PixelFormat::R8Uint);
	mRaySegmentListEventTypeSRVUsage = mFactory->createResourceUsage(mRaySegmentListEventTypeTexture, PixelFormat::R8Uint);

	//create OccupancyHistogramTree
	mOccupancyHistogramTree = new OccupancyHistogramTree();

	mOccupancyHistogramTree->setMaxDepth(MAX_DEPTH);
	mOccupancyHistogramTree->setSize(AxiallyAlignedBoundingBox(-cube * 0.5f, cube * 0.5f));
}

void SparseLeapManager::finalize()
{
	mFactory->destroyVertexShader(mOccupancyGeometryVertexShader);
	mFactory->destroyPixelShader(mOccupancyGeometryPixelShader);

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

	Utility::Delete(mOccupancyHistogramTree);
}

void SparseLeapManager::update(const glm::vec3& cameraPosition)
{
	mOccupancyGeometry.clear();

	//new occupancy geometry
	mOccupancyHistogramTree->setEyePosition(cameraPosition);
	mOccupancyHistogramTree->getOccupancyGeometry(mOccupancyGeometry);
}

auto SparseLeapManager::cube() const -> glm::vec3
{
	return mCube;
}

auto SparseLeapManager::tree() const -> OccupancyHistogramTree*
{
	return mOccupancyHistogramTree;
}

SparseLeapManager::~SparseLeapManager()
{
	
}
