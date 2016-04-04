#include "DefenseManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

DefenseManager::DefenseManager()
	: _initialized(false)
{
}

void DefenseManager::update(const BWAPI::Unitset & defenceUnits)
{
	_defenceUnits = defenceUnits;
	for (auto & unit : _defenceUnits)
	{
		Micro::SmartMove(unit, InformationManager::Instance().getNearestChokePoint());
	}
}

void DefenseManager::updateDefenseTeam() {

}
