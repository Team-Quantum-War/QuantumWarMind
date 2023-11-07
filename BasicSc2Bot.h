#ifndef BASIC_SC2_BOT_H_
#define BASIC_SC2_BOT_H_

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_args.h"
#include "sc2lib/sc2_lib.h"
#include "sc2utils/sc2_manage_process.h"
#include "sc2utils/sc2_arg_parser.h"
#include "sc2api/sc2_unit_filters.h"

using namespace sc2;

class BasicSc2Bot : public Agent
{
public:
    virtual void OnGameStart() final;
    virtual void OnStep() final;
    virtual void OnUnitCreated(const Unit *unit) final;
    virtual void OnUnitIdle(const Unit *unit) final;

private:
    bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::ZERG_DRONE);
    void TryBuildExtractor();
    bool TryBuildSpawningPool();
    void TryBuildHatchery();
    void TryCreateZergQueen();
    void TryFillGasExtractor();
    void TryResearchMetabolicBoost();
    const Unit *FindNearestMineralPatch(const Point2D &start);

    size_t CountUnitType(UNIT_TYPEID unit_type);
    size_t CountEggUnitsInProduction(ABILITY_ID unit_ability);
    Point2D FindNearestBuildLocation(sc2::UNIT_TYPEID type_);
    const Unit *FindNearestExtractor(ABILITY_ID unit_ability);
    int GetQueensInQueue(const sc2::Unit *hatchery);
    std::vector<const sc2::Unit *> GetMineralGatheringDrones();
    std::vector<const Unit*> GetGasGatheringDrones();
    bool IsExtractorBeingHarvested(const sc2::Unit *extractor);
};

#endif
