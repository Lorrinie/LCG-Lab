#include <WindowsFramework.hpp>
#include <Mesh.hpp>

#include "FileManager.hpp"

#include <fstream>

class MyRenderFramework : public WindowsFramework {
private:
	VertexBuffer* mVertexBuffer;
	IndexBuffer* mIndexBuffer;
	ConstantBuffer* mConstantBuffer;

	InputLayout* mInputLayout;

	VertexShader* mVertexShader;
	PixelShader* mPixelShader;

	Texture3D* mVolumeTexture;

	ResourceUsage* mVolumeUsage;

	RasterizerState* mRasterizerState;
	DepthStencilState* mDepthStencilState;

	std::vector<unsigned char> mVolumeData;

	glm::mat4 mCameraTranslation;
	glm::mat4 mMatrix[4];
	glm::vec3 mCameraPosition;
protected:
	
	virtual void update(void* sender, float deltaTime)override {
		//mCameraTranslation = glm::rotate(glm::mat4(1), 0.5f, glm::vec3(0, 0, -1));// *mCameraTranslation;
		mCameraTranslation = glm::rotate(glm::mat4(1), glm::pi<float>() * 0.5f * 0.01f, glm::vec3(0, 0, -1)) * mCameraTranslation;

		mCameraPosition = mCameraTranslation * glm::vec4(0, -2, 0, 1);

		mMatrix[1] = glm::lookAt(mCameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
		mMatrix[3][0] = glm::vec4(mCameraPosition, 0.0f);

		mConstantBuffer->update(&mMatrix[0]);
	}

	virtual void render(void* sender, float deltaTime)override {
		float rgba[4] = { 0.f, 0.f, 0.f, 1.0f };
		
		mRasterizerState->setCullMode(CullMode::Back);
		mRasterizerState->setFillMode(FillMode::Solid);

		mDepthStencilState->setDepthEnable(true);
		mDepthStencilState->setComparisonMode(ComparisonMode::Less);

		mGraphics->clearState();

		mGraphics->setRenderTarget(mSwapChain->getRenderTarget());
		mGraphics->setViewPort(0, 0, (float)mWidth, (float)mHeight);
		
		mGraphics->setRasterizerState(mRasterizerState);
		mGraphics->setDepthStencilState(mDepthStencilState);

		mGraphics->clearRenderTarget(mSwapChain->getRenderTarget(), rgba);

		mGraphics->setInputLayout(mInputLayout);
		mGraphics->setVertexBuffer(mVertexBuffer);
		mGraphics->setIndexBuffer(mIndexBuffer);

		mGraphics->setVertexShader(mVertexShader);
		mGraphics->setPixelShader(mPixelShader);
		
		mGraphics->setConstantBuffer(mConstantBuffer, 0);
		mGraphics->setResourceUsage(mVolumeUsage, 2);

		mGraphics->setPrimitiveType(PrimitiveType::TriangleList);
		mGraphics->drawIndexed(36, 0, 0);

		mSwapChain->present(true);
	}
public:
	MyRenderFramework(const std::string &name, int width, int height) :
		WindowsFramework(name, width, height) {

		buildVolumeData();

		buildBuffer();
		buildShader();
		buildState();
		buildInputLayout();
	}

	void buildBuffer() {
		mVertexBuffer = mFactory->createVertexBuffer(8 * sizeof(Mesh::Vertex), sizeof(Mesh::Vertex), ResourceInfo::VertexBuffer());
		mIndexBuffer = mFactory->createIndexBuffer(36 * sizeof(unsigned int), ResourceInfo::IndexBuffer());
		mConstantBuffer = mFactory->createConstantBuffer(sizeof(mMatrix), ResourceInfo::ConstantBuffer());

		mCameraPosition = glm::vec3(0, -5, 0);
		mCameraTranslation = glm::translate(glm::mat4(1), glm::vec3(0, -5, 0));

		mMatrix[0] = glm::mat4(1);		
		mMatrix[1] = glm::lookAt(mCameraPosition, glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
		mMatrix[2] = glm::perspectiveFov(glm::pi<float>() * 0.55f, (float)mWidth, (float)mHeight, 1.0f, 100.0f);
		mMatrix[3][0] = glm::vec4(mCameraPosition, 0.0f);

		mVertexBuffer->update(Mesh::Cube(glm::vec3(3.0f)).vertices().data());
		mIndexBuffer->update(Mesh::Cube(glm::vec3(3.0f)).indices().data());
		mConstantBuffer->update(&mMatrix[0]);
	}

	void buildShader() {
		auto vertexShaderCode = FileManager::ReadFile("VertexShader.cso");
		auto pixelShaderCode = FileManager::ReadFile("PixelShader.cso");
		
		mVertexShader = mFactory->createVertexShader(vertexShaderCode, true);
		mPixelShader = mFactory->createPixelShader(pixelShaderCode, true);
	}

	void buildState() {
		mRasterizerState = mFactory->createRasterizerState();
		mDepthStencilState = mFactory->createDepthStencilState();

		mDepthStencilState->setDepthEnable(true);
		mDepthStencilState->setComparisonMode(ComparisonMode::Less);

		mRasterizerState->setFillMode(FillMode::Solid);
		mRasterizerState->setCullMode(CullMode::Back);
	}

	void buildInputLayout() {

		mInputLayout = mFactory->createInputLayout();

		mInputLayout->addElement(InputLayoutElement("POSITION", sizeof(glm::vec3)));
		mInputLayout->addElement(InputLayoutElement("TEXCOORD", sizeof(glm::vec3)));
	}

	void buildVolumeData() {
		//std::ifstream file("volume", std::ios::ate | std::ios::binary);
		std::ifstream file("Teddybear.raw", std::ios::ate | std::ios::binary);

		size_t fileSize = (size_t)file.tellg();

		mVolumeData.resize(fileSize);

		file.seekg(0, std::ios::beg);
		file.read((char*)&mVolumeData[0], fileSize);
		file.close();

		mVolumeTexture = mFactory->createTexture3D(100, 100, 100, PixelFormat::R8Unknown, ResourceInfo::ShaderResource());
		mVolumeTexture->update(&mVolumeData[0], 0, 0, 0, mVolumeTexture->getWidth(), mVolumeTexture->getHeight(), mVolumeTexture->getDepth());

		mVolumeUsage = mFactory->createResourceUsage(mVolumeTexture, PixelFormat::R8Unknown);
	}

	~MyRenderFramework() {
		mFactory->destroyVertexBuffer(mVertexBuffer);
		mFactory->destroyIndexBuffer(mIndexBuffer);
		mFactory->destroyConstantBuffer(mConstantBuffer);

		mFactory->destroyInputLayout(mInputLayout);
		mFactory->destroyVertexShader(mVertexShader);
		mFactory->destroyPixelShader(mPixelShader);

		mFactory->destroyRasterizerState(mRasterizerState);
		mFactory->destroyDepthStencilState(mDepthStencilState);

		mFactory->destroyTexture3D(mVolumeTexture);

		mFactory->destroyResourceUsage(mVolumeUsage);
	}
};

int main() {
	
	MyRenderFramework framework("VolumeRendering", 1920, 1080);

	framework.showWindow();
	framework.runLoop();
}