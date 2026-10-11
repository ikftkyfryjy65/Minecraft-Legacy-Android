#include "stdafx.h"

#include "Packet.h"
#include "MaterialColor.h"
#include "Material.h"
#include "Tile.h"
#include "HatchetItem.h"
#include "pickaxeitem.h"
#include "ShovelItem.h"
#include "BlockReplacements.h"
#include "Biome.h"
#include "Item.h"
#include "FurnaceRecipes.h"
#include "Recipes.h"
#include "Stats.h"
#include "Achievements.h"
#include "Skeleton.h"
#include "PigZombie.h"
#include "TileEntity.h"
#include "EntityIO.h"
#include "SharedConstants.h"
#include "MobCategory.h"
#include "LevelChunk.h"
#include "MineShaftPieces.h"
#include "StrongholdFeature.h"
#include "VillageFeature.h"
#include "LevelType.h"
#include "EnderMan.h"
#include "PotionBrewing.h"
#include "Enchantment.h"
#include "VillagePieces.h"
#include "RandomScatteredLargeFeature.h"

#include "Minecraft.World.h"
#include "../Minecraft.Client/ServerLevel.h"
#include "SparseLightStorage.h"
#include "sparsedatastorage.h"
#include "McRegionChunkStorage.h"
#include "Villager.h"
#include "LevelSettings.h"

#ifdef _DURANGO
#include "DurangoStats.h"
#else
#include "CommonStats.h"
#endif

#include <cstdio>
#include <exception>

// Preserve initializer order while identifying and isolating a throwing initializer.
#define MCE_WORLD_CTOR(expr) do { \
    std::fprintf(stderr, "[world-ctor] begin: %s\n", #expr); \
    try { \
        expr; \
        std::fprintf(stderr, "[world-ctor] ok: %s\n", #expr); \
    } catch (const std::exception& e) { \
        std::fprintf(stderr, "[world-ctor] EXCEPTION: %s: %s\n", #expr, e.what()); \
    } catch (...) { \
        std::fprintf(stderr, "[world-ctor] UNKNOWN EXCEPTION: %s\n", #expr); \
    } \
} while (0)

void MinecraftWorld_RunStaticCtors()
{
	// The ordering of these static ctors can be important. If they are within statement blocks then
	// DO NOT CHANGE the ordering - 4J Stu

	MCE_WORLD_CTOR(Packet::staticCtor());

	{
		MCE_WORLD_CTOR(MaterialColor::staticCtor());
		MCE_WORLD_CTOR(Material::staticCtor());
		// Tile::staticCtor(); // MCE-SKIP-DEBUG
		MCE_WORLD_CTOR(HatchetItem::staticCtor());
		MCE_WORLD_CTOR(PickaxeItem::staticCtor());
		MCE_WORLD_CTOR(ShovelItem::staticCtor());
		MCE_WORLD_CTOR(BlockReplacements::staticCtor());
		MCE_WORLD_CTOR(Biome::staticCtor());	
		MCE_WORLD_CTOR(Item::staticCtor());
		MCE_WORLD_CTOR(FurnaceRecipes::staticCtor());
		MCE_WORLD_CTOR(Recipes::staticCtor());	
#ifdef _DURANGO
		MCE_WORLD_CTOR(GenericStats::setInstance(new DurangoStats()));
#else
		MCE_WORLD_CTOR(GenericStats::setInstance(new CommonStats()));
		MCE_WORLD_CTOR(Stats::staticCtor());
#endif
		//Achievements::staticCtor(); // 4J Stu - This is now called from within the Stats::staticCtor()
		MCE_WORLD_CTOR(Skeleton::staticCtor());
		MCE_WORLD_CTOR(PigZombie::staticCtor());
		MCE_WORLD_CTOR(TileEntity::staticCtor());
		MCE_WORLD_CTOR(EntityIO::staticCtor());
		MCE_WORLD_CTOR(MobCategory::staticCtor());

		MCE_WORLD_CTOR(Item::staticInit());
		MCE_WORLD_CTOR(LevelChunk::staticCtor());

		MCE_WORLD_CTOR(LevelType::staticCtor());

		MCE_WORLD_CTOR(MineShaftPieces::staticCtor());
		MCE_WORLD_CTOR(StrongholdFeature::staticCtor());
		MCE_WORLD_CTOR(VillagePieces::Smithy::staticCtor());
		MCE_WORLD_CTOR(VillageFeature::staticCtor());
		MCE_WORLD_CTOR(RandomScatteredLargeFeature::staticCtor());
	}
	MCE_WORLD_CTOR(EnderMan::staticCtor());
	MCE_WORLD_CTOR(PotionBrewing::staticCtor());
	MCE_WORLD_CTOR(Enchantment::staticCtor());

	MCE_WORLD_CTOR(SharedConstants::staticCtor());

	MCE_WORLD_CTOR(ServerLevel::staticCtor());
	MCE_WORLD_CTOR(SparseLightStorage::staticCtor());
	MCE_WORLD_CTOR(CompressedTileStorage::staticCtor());
	MCE_WORLD_CTOR(SparseDataStorage::staticCtor());
	MCE_WORLD_CTOR(McRegionChunkStorage::staticCtor());
	MCE_WORLD_CTOR(Villager::staticCtor());
	MCE_WORLD_CTOR(GameType::staticCtor());
}

#undef MCE_WORLD_CTOR
