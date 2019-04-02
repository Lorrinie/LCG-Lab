#pragma once

#include <WindowsFramework.hpp>
#include <OrbitCamera.hpp>
#include <Mesh.hpp>

#include "Helper.hpp"
#include "SharedMacro.hpp"
#include "VirtualMemoryManager.hpp"

class VMRenderFramework : public WindowsFramework {
private:
	InputLayout* mInputLayout;
	IndexBuffer* mIndexBuffer;
	VertexBuffer* mVertexBuffer;
	ConstantBuffer* mMatrixBuffer;

	PixelShader* mPixelShader;
	VertexShader* mVertexShader;

	RasterizerState* mRasterizerState;

	Input* mInput;
	bool mMouseButtonState[3];

	Mesh mCubeMesh;

	Camera* mCamera;

	Camera mFixCamera[3];
	OrbitCamera mViewCamera;
	MatrixStructure mMatrixStructure;

	VirtualMemoryManager* mVirtualMemoryManager;

	virtual void render(void* sender, float mDeltaTime)override;
	virtual void update(void* sender, float mDeltaTime)override;
	
	virtual void mouseMove(void* sender, MouseMoveEvent* eventArg)override;
	virtual void mouseUp(void* sender, MouseClickEvent* eventArg)override;
	virtual void mouseDown(void* sender, MouseClickEvent* eventArg)override;

	void initializeInputStage();
	void initializeShaderStage();
	void initializeRasterizerStage();

	void destoryInputStage();
	void destoryShaderStage();
	void destoryRasterizerStage();
public:
	VMRenderFramework(const std::string &name, int width, int height);

	~VMRenderFramework();

	void initialize();
};