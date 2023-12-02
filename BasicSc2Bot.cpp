#include "BasicSc2Bot.h"
#include <iostream>

#define DEBUG_MODE true
#define VERBOSE false

using namespace sc2;
using namespace std;

/*
===========================================================================
                            SC2 API FUNCTIONS
===========================================================================
*/

/* This function is called when the game starts. */
void BasicSc2Bot::OnGameStart()
{
    if (VERBOSE)
    {
        std::cout << "It's Gamin time" << std::endl;
    }
}
/* This function is called on each game step. */
void BasicSc2Bot::OnStep()
{
    TryBuildExtractor();
    TryBuildSpawningPool();
    TryNaturallyExpand();
    TryFillGasExtractor();
    TryCreateZergQueen();
    TryResearchMetabolicBoost();
}

/* This function is called when a new unit is created. */
void BasicSc2Bot::OnUnitCreated(const Unit *unit)
{
    if (VERBOSE)
    {
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_OVERLORD)
        {
            std::cout << "Overlord created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_DRONE)
        {
            std::cout << "Drone created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_EXTRACTOR)
        {
            std::cout << "Extractor created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL)
        {
            std::cout << "Spawingpool created with tag: " << unit->tag << std::endl;
        }
        if (unit->unit_type == sc2::UNIT_TYPEID::ZERG_HATCHERY)
        {
            std::cout << "Hatcher created with tag: " << unit->tag << std::endl;
        }
    }
}

/* This function is called when a unit becomes idle. */
void BasicSc2Bot::OnUnitIdle(const Unit *unit)
{
    switch (unit->unit_type.ToType())
    {
    case UNIT_TYPEID::ZERG_LARVA:
    {
        // Get the # of supply used and # of drones and overlords on field + in production
        size_t usedSupply = Observation()->GetFoodUsed();
        size_t sum_overlords = CountUnitType(UNIT_TYPEID::ZERG_OVERLORD) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_OVERLORD);
        size_t sum_drones = CountUnitType(UNIT_TYPEID::ZERG_DRONE) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_DRONE);

        // What to do when idle?
        if (sum_overlords < 2 && sum_drones == 13) // when we make our 13th drone we need to make an Overlord
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
        }
        else if (usedSupply >= 19 && sum_overlords < 3) // when we reach supply 19 we need to make an Overlord
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
        }
        else if (usedSupply < 17) // cap # of drones at 17 during prep phase (before queens)
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_DRONE);
        }
        break;
    }
    case UNIT_TYPEID::ZERG_DRONE:
    {
        // Find the nearest mineral patch and command the drone to gather from it.
        const Unit *mineral_target = FindNearestMineralPatch(unit->pos);
        if (!mineral_target)
        {
            break;
        }
        Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
        break;
    }
    case UNIT_TYPEID::ZERG_OVERLORD:
    {
        // Move the Overlord to an enemy base's natural expansion.
        const GameInfo &game_info = Observation()->GetGameInfo();
        Actions()->UnitCommand(unit, ABILITY_ID::MOVE_MOVEPATROL, FindExpansionLocation(15000.0f, 20000.0f));
        break;
    }
    default:
    {
        break;
    }
    }
}

/*
===========================================================================
                            ON-STEP FUNCTIONS
===========================================================================
*/

/* Attempts to build an Extractor when conditions are met. */
void BasicSc2Bot::TryBuildExtractor()
{
    // Get drones and extractors and # of supply used
    const ObservationInterface *observation = Observation();
    const Units &drones = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_DRONE));
    const Units &extractors = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR));
    int usedSupply = Observation()->GetFoodUsed();

    // We only need 1 extractor at supply 17, we do not want to build more than we need
    if (!extractors.empty() || usedSupply != 17)
    {
        return;
    }

    // Issue the build command to build an extractor on the geyser
    if (DEBUG_MODE)
    {
        std::cout << "We have reached a checkpoint. We have " << drones.size() << " drones & " << extractors.size() << " extractors we will try to build a extractor" << std::endl;
    }
    TryBuildStructure(ABILITY_ID::BUILD_EXTRACTOR);
}

/* Attempts to build a Spawning Pool when conditions are met. */
void BasicSc2Bot::TryBuildSpawningPool()
{
    const ObservationInterface *observation = Observation();
    int usedSupply = Observation()->GetFoodUsed();

    // We only build pool when we have enough minerals after extractor's completion at supply 17
    if (observation->GetMinerals() < 200 || CountUnitType(UNIT_TYPEID::ZERG_EXTRACTOR) < 1 || usedSupply != 17)
    {
        return;
    }

    // We only build pool ONCE
    if (CountUnitType(UNIT_TYPEID::ZERG_SPAWNINGPOOL) > 0)
    {
        return;
    }

    // Issue the build command to build an spawning pool
    if (DEBUG_MODE)
    {
        std::cout << "We have reached a checkpoint. We will try to build a spawning pool" << std::endl;
    }
    TryBuildStructure(ABILITY_ID::BUILD_SPAWNINGPOOL);
}

/* Attempts to naturally expand when conditions are met. */
void BasicSc2Bot::TryNaturallyExpand()
{
    const sc2::ObservationInterface *observation = Observation();
    int usedSupply = Observation()->GetFoodUsed();

    // Check if there's enough resources to expand & other conditions met
    const int mineralsRequired = 300;
    if (observation->GetMinerals() >= mineralsRequired && usedSupply >= 17 && CountUnitType(UNIT_TYPEID::ZERG_HATCHERY) < 2)
    {
        // Issue a build command for the Hatchery at a valid expansion location.
        if (DEBUG_MODE)
        {
            cout << "We have reached a checkpoint. We will try to build another hatchery at a natural expansion." << endl;
        }
        TryBuildStructure(ABILITY_ID::BUILD_HATCHERY);
    }

    return;
}

/* Attempts to fill gas extractors with drones as soon as the gas extractor is built. */
void BasicSc2Bot::TryFillGasExtractor()
{
    // Get mineral-gathering drones and extractors
    std::vector<const sc2::Unit *> mineralGatheringDrones = GetCurrentHarvestingDrones();
    const Units &extractors = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR));

    // Check conditions and attempt to fill gas
    for (const auto &extractor : extractors)
    {
        if (!extractor || extractor->build_progress < 1.0f || extractor->assigned_harvesters > 0 || isLingSpeedResearched)
        {
            return;
        }
        if (DEBUG_MODE)
        {
            std::cout << "Attempting to fill gas extractor: " << extractor->tag << std::endl;
        }
        int max_drones = extractor->ideal_harvesters; // we only ever need 3 drones to fill an extractor to max capacity
        while (max_drones > 0)
        {
            const auto &drone = mineralGatheringDrones.back();
            sc2::ActionInterface *actions = Actions();
            actions->UnitCommand(drone, sc2::ABILITY_ID::HARVEST_GATHER, extractor); // just send our first 3 drones in the list to go get gas
            --max_drones;
            mineralGatheringDrones.pop_back();
        }
    }
}

/* Attempts to create a Zerg Queen as soon as the Pool is completed. */
void BasicSc2Bot::TryCreateZergQueen()
{
    // Make queens only in the first hatchery
    const Units &hatcheries = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));
    const Unit *mainHatchery = GetMainBaseHatcheryLocation(); // get main hatchery

    // Check for queen creation process going on in queue
    bool makingQueen = false;
    if (GetQueensInQueue(mainHatchery) > 0)
    {
        makingQueen = true;
    }

    // Make two queens
    if (CountUnitType(sc2::UNIT_TYPEID::ZERG_SPAWNINGPOOL) > 0 &&
        CountUnitType(sc2::UNIT_TYPEID::ZERG_HATCHERY) > CountUnitType(sc2::UNIT_TYPEID::ZERG_QUEEN) &&
        !makingQueen)
    {
        Actions()->UnitCommand(mainHatchery, sc2::ABILITY_ID::TRAIN_QUEEN);
    }
}

/*
This function researches Zergling speed once enough resources are gathered and then pull out gas-harvesting drones to collect minerals again by making them idle.
*/
void BasicSc2Bot::TryResearchMetabolicBoost()
{
    const ObservationInterface *observation = Observation();

    // Check if we have 100 gas and a spawning pool to research
    if (observation->GetVespene() >= 100)
    {
        const Units &spawningPools = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_SPAWNINGPOOL));

        // Check if you have a Spawning Pool and ling speed has not been researched yet
        if (!spawningPools.empty())
        {
            const Unit *spawningPool = spawningPools.front(); // Use the first available Spawning Pool

            Actions()->UnitCommand(spawningPool, ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);
            if (DEBUG_MODE)
            {
                std::cout << "We are researching ling speed right now!" << std::endl;
            }

            // Pull workers off gas extractors
            std::vector<const sc2::Unit *> gasGatheringDrones = GetCurrentHarvestingDrones();
            for (const auto &drone : gasGatheringDrones)
            {
                Actions()->UnitCommand(drone, ABILITY_ID::STOP);
            }

            isLingSpeedResearched = true;
        }
    }

    return;
}

/*
===========================================================================
                            HELPER FUNCTIONS
===========================================================================
*/

/* Attempts to build a structure with the specified ability type and unit type. */
bool BasicSc2Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_to_build_type)
{
    const ObservationInterface *observation = Observation();

    // If a unit already is building a supply structure of this type or is harvesting minerals, do nothing.
    // Also get an drone to build the structure.
    const Unit *unit_to_build = nullptr;
    Units units = observation->GetUnits(Unit::Alliance::Self);
    for (const auto &unit : units)
    {
        for (const auto &order : unit->orders)
        {
            if (order.ability_id == ability_type_for_structure)
            {
                return false;
            }
        }
        if (unit->unit_type == unit_to_build_type)
        {
            unit_to_build = unit;
            break;
        }
    }

    // Build structure based on the PROVIDED structure type
    if (ability_type_for_structure == ABILITY_ID::BUILD_EXTRACTOR)
    {
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, FindNearestGeyser(ability_type_for_structure));
    }
    else if (ability_type_for_structure == ABILITY_ID::BUILD_HATCHERY)
    {
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, FindExpansionLocation(45.0f, 1000.0f));
    }
    else
    {
        Point2D build_location = FindNearestBuildLocationTo(UNIT_TYPEID::ZERG_HATCHERY);
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, build_location);
    }
    return true;
}

/* Finds the nearest mineral patch to the specified location. */
const Unit *BasicSc2Bot::FindNearestMineralPatch(const Point2D &start)
{
    Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
    float distance = std::numeric_limits<float>::max();
    const Unit *target = nullptr;
    for (const auto &u : units)
    {
        if (u->unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD && u->pos.x != start.x && u->pos.y != start.y)
        {
            float d = DistanceSquared2D(u->pos, start);
            if (d < distance)
            {
                distance = d;
                target = u;
            }
        }
    }
    return target;
}

/* Counts the number of units of the specified type on map. */
size_t BasicSc2Bot::CountUnitType(UNIT_TYPEID unit_type)
{
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

/* Counts number of a certain unit currently in production, takes in the name of the ability that makes the unit, ex ABILITY::TRAIN_OVERLORD */
size_t BasicSc2Bot::CountEggUnitsInProduction(ABILITY_ID unit_ability)
{
    Units units = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EGG));
    size_t unit_count = 0;
    for (const auto &unit : units)
    {
        if (unit->orders.front().ability_id == unit_ability)
        {
            ++unit_count;
        }
    }
    return unit_count;
}

/* Finds a suitable build location near the specified unit type. */
Point2D BasicSc2Bot::FindNearestBuildLocationTo(sc2::UNIT_TYPEID type_)
{
    const ObservationInterface *observation = Observation();
    const Units &units = observation->GetUnits(Unit::Alliance::Self, IsUnit(type_));

    // Check if you have at least one unit
    if (units.empty())
    {
        if (DEBUG_MODE)
        {
            std::cout << "We have no units" << std::endl;
        }
        return Point2D(0.0f, 0.0f); // Return an invalid location
    }

    const Unit *unit = units.front(); // Use the first units

    // Find a suitable location nearby the units
    Point2D buildLocation;
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    buildLocation = Point2D(unit->pos.x + rx * 5.0f, unit->pos.y + ry * 5.0f);

    if (DEBUG_MODE)
    {
        std::cout << "We will build at (" << buildLocation.x << "," << buildLocation.y << ")" << std::endl;
    }
    return buildLocation;
}

/* Finds the nearest geyser for building an Extractor. */
const Unit *BasicSc2Bot::FindNearestGeyser(ABILITY_ID unit_ability)
{
    const ObservationInterface *observation = Observation();
    const Units &units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));

    const Unit *hatchery = units.front(); // Use the first Hatchery

    if (unit_ability == ABILITY_ID::BUILD_EXTRACTOR)
    {
        const Units &geysers = Observation()->GetUnits(Unit::Alliance::Neutral, IsUnit(UNIT_TYPEID::NEUTRAL_VESPENEGEYSER));
        const Unit *closest_geyser = geysers.front();
        for (const auto &geyser : geysers)
        {
            if (Distance3D(hatchery->pos, closest_geyser->pos) > Distance3D(hatchery->pos, geyser->pos))
            {
                closest_geyser = geyser;
            }
        }
        return closest_geyser;
    }

    return nullptr;
}

/* Gets all queens in production in a hatchery. */
int BasicSc2Bot::GetQueensInQueue(const sc2::Unit *hatchery)
{
    int queensInQueue = 0;
    if (hatchery)
    {
        const auto &orders = hatchery->orders; // get all the orders in hatchery
        // iterate over the orders and see if there is any queen to be trained
        for (const auto &order : orders)
        {
            if (order.ability_id == sc2::ABILITY_ID::TRAIN_QUEEN)
            {
                queensInQueue++;
            }
        }
    }
    return queensInQueue;
}

/* Gets all mineral-gathering drones*/
std::vector<const sc2::Unit *> BasicSc2Bot::GetCurrentHarvestingDrones()
{
    std::vector<const sc2::Unit *> mineralGatheringDrones;
    std::vector<const sc2::Unit *> units = Observation()->GetUnits(sc2::Unit::Alliance::Self);

    for (const auto &unit : units)
    {
        if (unit->orders.size() > 0) // check if unit is not idle, i.e. has an order
        {
            if (unit->orders[0].ability_id == sc2::ABILITY_ID::HARVEST_GATHER)
            {
                mineralGatheringDrones.push_back(unit);
            }
        }
    }
    return mineralGatheringDrones;
}

/* This function filters out mineral patches that are too close to the main base to ensure a reasonable distance between bases. */
sc2::Point2D BasicSc2Bot::FindExpansionLocation(float minDistanceSquared, float maxDistanceSquared)
{
    const sc2::ObservationInterface *observation = Observation();
    const sc2::GameInfo &gameInfo = observation->GetGameInfo();

    // Get main base location.
    const sc2::Point2D mainBaseLocation = observation->GetStartLocation();

    // Get all mineral patches on the map.
    const sc2::Units mineralPatches = observation->GetUnits(sc2::Unit::Alliance::Neutral, sc2::IsUnit(sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD));

    // Filter out mineral patches that are too close to or too far from the main base.
    std::vector<const sc2::Unit *> validMineralPatches;
    for (const auto &mineralPatch : mineralPatches)
    {
        float distance = DistanceSquared2D(mineralPatch->pos, mainBaseLocation);
        if (distance > minDistanceSquared && distance < maxDistanceSquared)
        {
            validMineralPatches.push_back(mineralPatch);
        }
    }

    cout << "SIZE OF MINERAL PATCHES: " << validMineralPatches.size() << endl;

    // Randomly select a valid mineral patch as near the expansion location.
    if (!validMineralPatches.empty())
    {
        const sc2::Unit *selectedMineralPatch = GetRandomElement(validMineralPatches);

        // Calculate the buildable location by adding an offset to the target location.
        float rx = GetRandomScalar();
        float ry = GetRandomScalar();
        sc2::Point2D build_location = Point2D(selectedMineralPatch->pos.x + rx * 10.0f, selectedMineralPatch->pos.y + ry * 10.0f);

        return build_location;
    }

    // If no suitable mineral patch is found, return the main base location.
    return mainBaseLocation;
}

/* This function calculates the squared distance between two points. */
float BasicSc2Bot::DistanceSquared2D(const sc2::Point2D &p1, const sc2::Point2D &p2)
{
    return pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2);
}

/* This function returns a random element from a vector, used to randomly select a mineral patch from the valid options. */
const sc2::Unit *BasicSc2Bot::GetRandomElement(const std::vector<const sc2::Unit *> &elements)
{
    if (elements.empty())
    {
        return nullptr;
    }
    const int randomIndex = rand() % elements.size();
    return elements[randomIndex];
}

/* Gets the location of the Hatchery at the main base. */
const Unit *BasicSc2Bot::GetMainBaseHatcheryLocation()
{
    const sc2::ObservationInterface *observation = Observation();
    const sc2::Point2D mainBaseLocation = observation->GetStartLocation();

    const Units &hatcheries = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));
    for (const auto &hatchery : hatcheries)
    {
        if (DistanceSquared2D(hatchery->pos, mainBaseLocation) < 1.0f)
        {
            // Return the Hatchery at the main base.
            return hatchery;
        }
    }

    // Return a null
    return nullptr;
}
