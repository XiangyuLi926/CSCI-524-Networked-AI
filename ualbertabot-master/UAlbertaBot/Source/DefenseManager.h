#pragma once

#include "Common.h"
#include "Squad.h"
#include "InformationManager.h"
//#include "SquadData.h"
//#include "StrategyManager.h"
namespace UAlbertaBot
{
	class DefenseManager
	{
		BWAPI::Unitset  _defenceUnits;
		bool            _initialized;
		//to update the defence team member 
		void            updateDefenseTeam();
		//define number and types
		///int             getNumType(BWAPI::Unitset & units, BWAPI::UnitType type);
		//find the suitable unit to the defend position
		///BWAPI::Unit     findClosestDefender(const Squad & defenseSquad, BWAPI::Position pos, bool flyingDefender);
		// identify each nearest chokepoints for each base
		///BWAPI::Position getDefendLocation();

		//void            initializeTeam();
		//  void            verifyTeamUniqueMembership();
		//void            emptySquad(Squad & squad, BWAPI::Unitset & unitsToAssign); 
		//int             getNumGroundDefendersInTeam(Squad & squad);

		//void            updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded);
		//int             defendWithWorkers();

		//int             numZerglingsInOurBase();
		//bool            beingBuildingRushed();
	public:
		DefenseManager();
		//~DefenceManager();
		void update(const BWAPI::Unitset & defenseUnits);
		// void drawSquadInformation(int x, int y);
	};
}