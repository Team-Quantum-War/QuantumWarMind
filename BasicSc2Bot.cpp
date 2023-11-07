#include "BasicSc2Bot.h"
#include <iostream>

#define DEBUG_MODE true
#define VERBOSE false

using namespace sc2;
using namespace std;

void BasicSc2Bot::OnGameStart()
{
    if (VERBOSE)
    {
        std::cout << "It's Gamin time" << std::endl;
    }
}

void BasicSc2Bot::OnStep()
{
    TryBuildExtractor();
    TryBuildSpawningPool();
    TryCreateZergQueen();
    TryFillGasExtractor();
    TryResearchMetabolicBoost();
}

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

bool BasicSc2Bot::TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_to_build_type)
{
    const ObservationInterface *observation = Observation();

    // If a unit already is building a supply structure of this type, do nothing.
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
        }
    }
    if (ability_type_for_structure == ABILITY_ID::BUILD_EXTRACTOR)
    {
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, FindNearestExtractor(ability_type_for_structure));
    }
    else
    {
        Point2D build_location = FindNearestBuildLocation(UNIT_TYPEID::ZERG_HATCHERY);
        Actions()->UnitCommand(unit_to_build, ability_type_for_structure, build_location);
    }
    return true;
}

void BasicSc2Bot::OnUnitIdle(const Unit *unit)
{
    switch (unit->unit_type.ToType())
    {
    case UNIT_TYPEID::ZERG_LARVA:
    {
        // When we make our 13th drone we need to make a Overlord
        int usedSupply = Observation()->GetFoodUsed();
        size_t sum_overlords = CountUnitType(UNIT_TYPEID::ZERG_OVERLORD) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_OVERLORD);
        size_t sum_drones = CountUnitType(UNIT_TYPEID::ZERG_DRONE) + CountEggUnitsInProduction(ABILITY_ID::TRAIN_DRONE);
        if (sum_overlords < 2 && sum_drones == 13)
        {
            if (DEBUG_MODE)
            {
                std::cout << "We have reached a checkpoint. We have " << sum_overlords << " overlords & " << sum_drones << " drones we will try to train another overlord" << std::endl;
            }
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
        }
        else if (usedSupply == 19)
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_OVERLORD);
        }
        else
        {
            Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_DRONE);
        }
        break;
    }
    case UNIT_TYPEID::ZERG_DRONE:
    {
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
        const GameInfo &game_info = Observation()->GetGameInfo();
        // I have to comment this out so that the 3rd overlord works!
        // I don't know why when I move the overlords, it just keeps spawning more and more overlords, not just a third one.
        // Actions()->UnitCommand(unit, ABILITY_ID::MOVE_MOVEPATROL, game_info.enemy_start_locations.front());
        break;
    }
    default:
    {
        break;
    }
    }
}

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

void BasicSc2Bot::TryBuildExtractor()
{
    const ObservationInterface *observation = Observation();
    const Units &drones = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_DRONE));
    const Units &extractors = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR));
    // we only need 1 extractor, we do not want to build more than we need
    if (drones.size() < 16 || !extractors.empty())
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

bool BasicSc2Bot::TryBuildSpawningPool()
{
    const ObservationInterface *observation = Observation();
    std::vector<sc2::Tag> units_with_commands = Actions()->Commands();
    if (observation->GetMinerals() < 200 || CountUnitType(UNIT_TYPEID::ZERG_EXTRACTOR) < 1)
    {
        return false;
    }
    if (CountUnitType(UNIT_TYPEID::ZERG_SPAWNINGPOOL) > 0)
    {
        return true;
    }

    if (DEBUG_MODE)
    {
        std::cout << "We have reached a checkpoint. We will try to build a spawning pool" << std::endl;
    }
    return TryBuildStructure(ABILITY_ID::BUILD_SPAWNINGPOOL);
}

// void BasicSc2Bot::TryExpandNewHatchery()
// {
// }

/*
Function that creates a zerg queen when conditions are met
condition: For each spawning pool we will have one zerg queen
*/
void BasicSc2Bot::TryCreateZergQueen()
{
    const Units &hatcheries = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));
    if (CountUnitType(UNIT_TYPEID::ZERG_SPAWNINGPOOL) > CountUnitType(UNIT_TYPEID::ZERG_QUEEN) && GetQueensInQueue(hatcheries.front()) < CountUnitType(UNIT_TYPEID::ZERG_SPAWNINGPOOL))
    {// must make sure that we are not ordering more in queue by the time one queen is created
        Actions()->UnitCommand(hatcheries.front(), sc2::ABILITY_ID::TRAIN_QUEEN);
        //cout << "QUEEN CREATE NOW LOL" << endl;
    }
}

/*
This function sends drones to extractors to extract gas.
*/
void BasicSc2Bot::TryFillGasExtractor()
{
    //std::cout << "HERE BRO LOL" << std::endl;
    std::vector<const sc2::Unit *> mineralGatheringDrones = GetMineralGatheringDrones();
    const Units &extractors = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_EXTRACTOR)); // get all extractor
    // iterate over all the extractor and see if the extractor is valid, fully built and not being harvested already
    for (const auto &extractor : extractors)
    {
        if (!extractor || extractor->build_progress < 1.0f || IsExtractorBeingHarvested(extractor))
        {
            return;
        }

        int max_drones = 0; // for now we are giving 13 drones to collect from one extractor
        for (const auto &drone : mineralGatheringDrones)
        {
            if (max_drones > 13)
                break;
            Actions()->UnitCommand(drone, sc2::ABILITY_ID::HARVEST_GATHER, extractor);
            max_drones++;
        }
    }
}

// helper function
size_t BasicSc2Bot::CountUnitType(UNIT_TYPEID unit_type)
{
    return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
}

// helper function that counts number of a certain unit currently in production
// takes in the name of the ability that makes the unit, ex ABILITY::TRAIN_OVERLORD
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

Point2D BasicSc2Bot::FindNearestBuildLocation(sc2::UNIT_TYPEID type_)
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
    // You can use the position of the units to calculate the location
    Point2D buildLocation;
    float rx = GetRandomScalar();
    float ry = GetRandomScalar();
    buildLocation = Point2D(unit->pos.x + rx * 5.0f, unit->pos.y + ry * 5.0f); // Example: Build 3 units to the right of the Hatchery

    // You should add more logic to check if the build location is valid, such as ensuring it's on creep or open ground.
    if (DEBUG_MODE)
    {
        std::cout << "We will build at (" << buildLocation.x << "," << buildLocation.y << ")" << std::endl;
    }
    return buildLocation;
}

/*
This function is used find the nearest extractor from the first hatchery
*/
const Unit *BasicSc2Bot::FindNearestExtractor(ABILITY_ID unit_ability)
{
    const ObservationInterface *observation = Observation();
    const Units &units = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_HATCHERY));

    const Unit *hatchery = units.front(); // Use the first Hatchery

    if (unit_ability == ABILITY_ID::BUILD_EXTRACTOR)
    { // if we are trying to build an extractor we want the closest one
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
}

/*
Returns the number of queens that are queued to be hatched in hatchery
*/
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

/*
This function is used to find all the drones that are currently gathering minerals.
Returns a vector object with all the drones ordered to gather minerals.
*/
std::vector<const sc2::Unit *> BasicSc2Bot::GetMineralGatheringDrones()
{
    std::vector<const sc2::Unit *> mineralGatheringDrones;
    //iterate over all the units
    for (const auto &unit : Observation()->GetUnits(sc2::Unit::Alliance::Self))
    {
        if (unit->orders.size() > 0) //check if unit is not idle, i.e. has an order
        {
            if (unit->orders[0].ability_id == sc2::ABILITY_ID::HARVEST_GATHER)
            {
                mineralGatheringDrones.push_back(unit);
            }
        }
    }

    return mineralGatheringDrones;
}

// std::vector<const Unit*> BasicSc2Bot::GetGasGatheringDrones() {
//     const ObservationInterface* observation = Observation();
//     std::vector<const Unit*> gasGatheringDrones; // vector to store all gas gathering drones

//     const Units& workers = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_DRONE));
//     for (const auto& worker : workers) {
//         if (worker->orders.size() > 0) {
//             for (const auto& order : worker->orders) {
//                 // Check if the worker's order is to gather gas from an Extractor
//                 if (order.ability_id == ABILITY_ID::HARVEST_GATHER && order.target_unit_tag != 0) {
//                     const Unit* target = observation->GetUnit(order.target_unit_tag);
//                     if (target && target->unit_type == UNIT_TYPEID::NEUTRAL_VESPENEGEYSER) {
//                         gasGatheringDrones.push_back(worker);
//                         break;  // No need to check further orders for this worker
//                     }
//                 }
//             }
//         }
//     }
    
//     return gasGatheringDrones;
// }

/*
This function checks if a extractor is already being harvesed by zerg drones or not
*/
bool BasicSc2Bot::IsExtractorBeingHarvested(const sc2::Unit *extractor)
{
    //invalid extractor
    if (!extractor)
    {
        return false;
    }
    // check if any unit belonging to player is already being harvested or is ordered to be harvested
    for (const auto &unit : Observation()->GetUnits(sc2::Unit::Alliance::Self))
    {
        if (unit->orders.size() > 0)
        {
            for (const auto &order : unit->orders)
            {
                if (order.ability_id == sc2::ABILITY_ID::HARVEST_GATHER && order.target_unit_tag == extractor->tag)
                {
                    return true;
                }
            }
        }
    }
    return false;
}


/*
Function that reserches Zergling speed once enough resources are gathered and then pull out drones to reserch minerals again
*/
void BasicSc2Bot::TryResearchMetabolicBoost() {
    const ObservationInterface *observation = Observation();

    // Check if we have 100 gas and a spawning pool to research
    if (observation->GetVespene() >= 100) {
        const Units& spawningPools = observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::ZERG_SPAWNINGPOOL));
        
        // Check if you have a Spawning Pool
        if (!spawningPools.empty()) {
            const Unit* spawningPool = spawningPools.front(); // Use the first available Spawning Pool
            Actions()->UnitCommand(spawningPool, ABILITY_ID::RESEARCH_ZERGLINGMETABOLICBOOST);

            // // Pull workers off gas extractors
            // std::vector<const sc2::Unit*> gasGatheringDrones = GetGasGatheringDrones();
            // for (const auto& drone : gasGatheringDrones) {
            //     // Check if the drone's order is to gather gas
            //     if (drone->orders.size() > 0 && drone->orders[0].ability_id == ABILITY_ID::HARVEST_GATHER) {
            //         Actions()->UnitCommand(drone, ABILITY_ID::HARVEST_RETURN);
            //     }
            // }
        }
    }
}        