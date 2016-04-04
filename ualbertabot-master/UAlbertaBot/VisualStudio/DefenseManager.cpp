#include "DefenseManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

DefenseManager::DefenseManager()
	: _initialized(false)
{
}
void DefenseManager::update(const BWAPI::Unitset & defenceUnits)
{
	updateDefenseTeam();
	_defenceUnits = defenceUnits;
}
void DefenseManager::updateDefenseTeam()
{
	// clean up the _units vector just in case one of them died
	BWAPI::Unitset goodUnits;
	for (auto & unit : _defenceUnits)
	{
		if (unit->isCompleted() &&
			unit->getHitPoints() > 0 &&
			unit->exists() &&
			unit->getPosition().isValid() &&
			unit->getType() != BWAPI::UnitTypes::Unknown)
		{
			goodUnits.insert(unit);
		}
		
		if (!unit->getType().isWorker() && goodUnits.contains(unit))
		{
			Micro::SmartMove(unit, InformationManager::Instance().getNearestChokePoint());
		}
	}
	_defenceUnits = goodUnits;
}