#pragma once

#include <cassert>
#include <vector>

#include <glm/glm.hpp>

#include "AddressMap.hpp"

class BlockCache : public AddressMap<byte> {
private:
	static Size mBlockCacheSize;
public:
	BlockCache(const Size &size, byte* data);

	BlockCache(const Size &size);

	BlockCache() : BlockCache(mBlockCacheSize) {}

	static void setBlockCacheSize(const Size &size);

	static auto getBlockCacheSize() -> Size;
};

class BlockTable : public AddressMap<BlockCache*> {
private:
	std::vector<BlockCache> mMemoryPool;

	static void deleteBlockCache(BlockCache* &blockCache);
protected:
	std::vector<VirtualLink*> mMapRelation;
public:
	BlockTable(const Size &size);

	~BlockTable();

	virtual void mallocAddress(VirtualLink* virtualLink);

	virtual void clearUpAddress(const VirtualAddress &address);

	virtual void mapAddress(const glm::vec3 &position, const Size &size, BlockCache* blockCache, VirtualLink* virtualLink);

	virtual auto queryAddress(const glm::vec3 &position, const Size& size, VirtualLink* virtualLink) -> BlockCache*;
};
