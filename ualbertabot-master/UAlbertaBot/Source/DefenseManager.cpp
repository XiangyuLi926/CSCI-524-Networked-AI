#include "DefenseManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

DefenseManager::DefenseManager()
{
}

void DefenseManager::update(const BWAPI::Unitset & defenceUnits)
{
	_defenseUnits = defenceUnits;
	updateDefenseTeam();
}

void DefenseManager::updateDefenseTeam() 
{
	BWAPI::Unitset goodUnits;
	for (auto & unit : _defenseUnits)
	{
		if (unit->isCompleted() &&
			unit->getHitPoints() > 0 &&
			unit->exists() &&
			unit->getPosition().isValid() &&
			unit->getType() != BWAPI::UnitTypes::Unknown)
		{
			goodUnits.insert(unit);
		}
	}
	_defenseUnits = goodUnits;

	for (auto & unit : _defenseUnits)
	{
		Micro::SmartMove(unit, InformationManager::Instance().getNearestChokePoint());
	}
}

BWAPI::Unitset DefenseManager::getDefenseUnitSet() 
{
	return _defenseUnits;
}
