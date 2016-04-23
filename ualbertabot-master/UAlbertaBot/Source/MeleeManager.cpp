#include "MeleeManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

MeleeManager::MeleeManager() 
{ 

}

void MeleeManager::executeMicro(const BWAPI::Unitset & targets) 
{
	assignTargetsOld(targets);
}

void MeleeManager::assignTargetsOld(const BWAPI::Unitset & targets)
{
    const BWAPI::Unitset & meleeUnits = getUnits();

	// figure out targets
	BWAPI::Unitset meleeUnitTargets;
	for (auto & target : targets) 
	{
		// conditions for targeting
		if (!(target->getType().isFlyer()) && 
			!(target->isLifted()) &&
			!(target->getType() == BWAPI::UnitTypes::Zerg_Larva) && 
			!(target->getType() == BWAPI::UnitTypes::Zerg_Egg) &&
			target->isVisible()) 
		{
			meleeUnitTargets.insert(target);
		}
	}
	// for each meleeUnit
	for (auto & meleeUnit : meleeUnits)
	{
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend) 
        {
            // run away if we meet the retreat critereon
            if (meleeUnitShouldRetreat(meleeUnit, targets))
            {
                BWAPI::Position fleeTo(BWAPI::Broodwar->self()->getStartLocation());

                Micro::SmartMove(meleeUnit, fleeTo);
            }
			// if there are targets
			else if (!meleeUnitTargets.empty())
			{
				// find the best target for this meleeUnit
				BWAPI::Unit target = getTarget(meleeUnit, meleeUnitTargets);

				// attack it
				Micro::SmartAttackUnit(meleeUnit, target);
			}
			// if there are no targets
			else
			{
				// if we're not near the order position
				if (meleeUnit->getDistance(order.getPosition()) > 100)
				{
					// move to it
					Micro::SmartMove(meleeUnit, order.getPosition());
				}
			}
		}

		if (Config::Debug::DrawUnitTargetInfo)
		{
			BWAPI::Broodwar->drawLineMap(meleeUnit->getPosition().x, meleeUnit->getPosition().y, 
			meleeUnit->getTargetPosition().x, meleeUnit->getTargetPosition().y, Config::Debug::ColorLineTarget);
		}
	}
}

std::pair<BWAPI::Unit, BWAPI::Unit> MeleeManager::findClosestUnitPair(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets)
{
    std::pair<BWAPI::Unit, BWAPI::Unit> closestPair(nullptr, nullptr);
    double closestDistance = std::numeric_limits<double>::max();

    for (auto & attacker : attackers)
    {
		//BWAPI::Broodwar->printf("our unit %d", attackers.size());
        BWAPI::Unit target = getTarget(attacker, targets);
        double dist = attacker->getDistance(attacker);

        if (!closestPair.first || (dist < closestDistance))
        {
            closestPair.first = attacker;
            closestPair.second = target;
            closestDistance = dist;
        }
    }

    return closestPair;
}

// get a target for the meleeUnit to attack
BWAPI::Unit MeleeManager::getTarget(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets)
{
	int count_enemyZealot = 0;
	int count_enemyDT = 0;
	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::infinity();
	BWAPI::Unit closestTarget = nullptr;

	// for each target possiblity
	for (auto & unit : targets)
	{
		if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot)
		{
			count_enemyZealot++;
		}
		if (unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar)
		{
			count_enemyDT++;
		}
		int priority = getAttackPriority(meleeUnit, unit);
		int distance = meleeUnit->getDistance(unit);

		// if it's a higher priority, or it's closer, set it
		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist))
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = unit;
		}
	}

	//BWAPI::Broodwar->printf("enemy zealot %d", count_enemyZealot);
	//BWAPI::Broodwar->printf("enemy DT %d", count_enemyDT);
	//BWAPI::Broodwar->printf("our unit %d", count_ourunits);

	return closestTarget;
}

	// get the attack priority of a type in relation to a zergling
int MeleeManager::getAttackPriority(BWAPI::Unit attacker, BWAPI::Unit unit) 
{
	BWAPI::UnitType type = unit->getType();

	//if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss) {
	//	int totalheath = unit->getHitPoints() + unit->getShields();
	//}

	int distance = attacker->getDistance(unit);
	
	int range = attacker->getType().groundWeapon().maxRange();
	//BWAPI::Broodwar->printf("distance %d", range);
	//BWAPI::Position base(BWAPI::Broodwar->self()->getStartLocation());

	//BWAPI::Broodwar->printf("total %d", unit->getHitPoints() + unit->getShields());
	//BWAPI::Broodwar->printf("hit %d", unit->getHitPoints());
	//BWAPI::Broodwar->printf("shield %d", unit->getShields());
	
	//attack range of zealot is 15
	//attack range of dark templar is 15
	//Health Point of zealot is 100, Shield 60

	if (attacker->getType() == BWAPI::UnitTypes::Protoss_Zealot)
	{
		if (unit->getType() == BWAPI::UnitTypes::Protoss_Zealot)						//zealot vs zealot
		{
			if (distance > range)														//out zealot is not in the attack region
			{
				if (distance <= 150)
				{
					if (unit->getHitPoints() + unit->getShields() < 1)
					{
						return 360;														//highest priority
					}
					return 160 / (unit->getHitPoints() + unit->getShields()) + 3000 / distance;
				}
				else if (distance > 150)												//distance is too long, do not attack
				{
					return 0;
				}
			}
			else if (distance <= range)													//our zealot is being attacked
			{
				return 160 / (unit->getHitPoints() + unit->getShields()) + 360;
			}
		}
		if (unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar)					//zealot vs dark templar
		{
			//BWAPI::Broodwar->printf("distance %d", unit->getType().groundWeapon().maxRange());	//attack range of dark templar is 15
			if (distance > range)														//out zealot is not in the attack region
			{
				if (distance <= 150)
				{
					if (unit->getHitPoints() + unit->getShields() < 1)
					{
						return 360;														//highest priority
					}
					return 160 / (unit->getHitPoints() + unit->getShields()) + 3000 / distance;
				}
				else if (distance > 150)												//distance is too long, do not attack
				{
					return 0;
				}
			}
			else if (distance <= range)													//our zealot is being attacked
			{
				return 160 / (unit->getHitPoints() + unit->getShields()) + 360;
			}
		}
	}

    if (attacker->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar 
        && unit->getType() == BWAPI::UnitTypes::Terran_Missile_Turret
        && (BWAPI::Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Protoss_Dark_Templar) == 0))
    {
        return 13;
    }

	if (attacker->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar && unit->getType().isWorker())
	{
		return 12;
	}

	// highest priority is something that can attack us or aid in combat
    if (type ==  BWAPI::UnitTypes::Terran_Bunker)
    {
        return 11;
    }
    else if (type == BWAPI::UnitTypes::Terran_Medic || 
		(type.groundWeapon() != BWAPI::WeaponTypes::None && !type.isWorker()) || 
		type ==  BWAPI::UnitTypes::Terran_Bunker ||
		type == BWAPI::UnitTypes::Protoss_High_Templar ||
		type == BWAPI::UnitTypes::Protoss_Reaver ||
		(type.isWorker() && unitNearChokepoint(unit))) 
	{
		return 10;
	} 
	// next priority is worker
	else if (type.isWorker()) 
	{
		return 9;
	}
    // next is special buildings
	else if (type == BWAPI::UnitTypes::Zerg_Spawning_Pool)
	{
		return 5;
	}
	// next is special buildings
	else if (type == BWAPI::UnitTypes::Protoss_Pylon)
	{
		return 5;
	}
	// next is buildings that cost gas
	else if (type.gasPrice() > 0)
	{
		return 4;
	}
	else if (type.mineralPrice() > 0)
	{
		return 3;
	}
	// then everything else
	else
	{
		return 1;
	}
}

BWAPI::Unit MeleeManager::closestMeleeUnit(BWAPI::Unit target, const BWAPI::Unitset & meleeUnitsToAssign)
{
	double minDistance = 0;
	BWAPI::Unit closest = nullptr;

	for (auto & meleeUnit : meleeUnitsToAssign)
	{
		double distance = meleeUnit->getDistance(target);
		if (!closest || distance < minDistance)
		{
			minDistance = distance;
			closest = meleeUnit;
		}
	}
	
	return closest;
}

bool MeleeManager::meleeUnitShouldRetreat(BWAPI::Unit meleeUnit, const BWAPI::Unitset & targets)
{
    // terran don't regen so it doesn't make any sense to retreat
    if (meleeUnit->getType().getRace() == BWAPI::Races::Terran)
    {
        return false;
    }

    // we don't want to retreat the melee unit if its shields or hit points are above the threshold set in the config file
    // set those values to zero if you never want the unit to retreat from combat individually
    if (meleeUnit->getShields() > Config::Micro::RetreatMeleeUnitShields || meleeUnit->getHitPoints() > Config::Micro::RetreatMeleeUnitHP)
    {
        return false;
    }

    // if there is a ranged enemy unit within attack range of this melee unit then we shouldn't bother retreating since it could fire and kill it anyway
    for (auto & unit : targets)
    {
        int groundWeaponRange = unit->getType().groundWeapon().maxRange();
        if (groundWeaponRange >= 64 && unit->getDistance(meleeUnit) < groundWeaponRange)
        {
            return false;
        }
    }

    return true;
}


// still has bug in it somewhere, use Old version
void MeleeManager::assignTargetsNew(const BWAPI::Unitset & targets)
{
    const BWAPI::Unitset & meleeUnits = getUnits();

	// figure out targets
	BWAPI::Unitset meleeUnitTargets;
	for (auto & target : targets) 
	{
		// conditions for targeting
		if (!(target->getType().isFlyer()) && 
			!(target->isLifted()) &&
			!(target->getType() == BWAPI::UnitTypes::Zerg_Larva) && 
			!(target->getType() == BWAPI::UnitTypes::Zerg_Egg) &&
			target->isVisible()) 
		{
			meleeUnitTargets.insert(target);
		}
	}

    BWAPI::Unitset meleeUnitsToAssign(meleeUnits);
    std::map<BWAPI::Unit, int> attackersAssigned;

    for (auto & unit : meleeUnitTargets)
    {
        attackersAssigned[unit] = 0;
    }

    int smallThreshold = BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg ? 3 : 1;
    int bigThreshold = BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg ? 12 : 3;

    // keep assigning targets while we have attackers and targets remaining
    while (!meleeUnitsToAssign.empty() && !meleeUnitTargets.empty())
    {
        auto attackerAssignment = findClosestUnitPair(meleeUnitsToAssign, meleeUnitTargets);
        BWAPI::Unit & attacker = attackerAssignment.first;
        BWAPI::Unit & target = attackerAssignment.second;

        UAB_ASSERT_WARNING(attacker, "We should have chosen an attacker!");

        if (!attacker)
        {
            break;
        }

        if (!target)
        {
            Micro::SmartMove(attacker, order.getPosition());
            continue;
        }

        Micro::SmartAttackUnit(attacker, target);

        // update the number of units assigned to attack the target we found
        int & assigned = attackersAssigned[attackerAssignment.second];
        assigned++;

        // if it's a small / fast unit and there's more than 2 things attacking it already, don't assign more
        if ((target->getType().isWorker() || target->getType() == BWAPI::UnitTypes::Zerg_Zergling) && (assigned >= smallThreshold))
        {
            meleeUnitTargets.erase(target);
        }
        // if it's a building and there's more than 10 things assigned to it already, don't assign more
        else if (assigned > bigThreshold)
        {
            meleeUnitTargets.erase(target);
        }

        meleeUnitsToAssign.erase(attacker);
    }

    // if there's no targets left, attack move to the order destination
    if (meleeUnitTargets.empty())
    {
        for (auto & unit : meleeUnitsToAssign)    
        {
			if (unit->getDistance(order.getPosition()) > 100)
			{
				// move to it
				Micro::SmartMove(unit, order.getPosition());
                BWAPI::Broodwar->drawLineMap(unit->getPosition(), order.getPosition(), BWAPI::Colors::Yellow);
			}
        }
    }
}
