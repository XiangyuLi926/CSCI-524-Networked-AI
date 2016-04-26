#include "Common.h"
#include "GameCommander.h"
#include "UnitUtil.h"
#define numOfSecond 10
using namespace UAlbertaBot;

GameCommander::GameCommander() 
    : _initialScoutSet(false)
{
	center = getCenter();
}

void GameCommander::update()
{
	_timerManager.startTimer(TimerManager::All);
	
	// populate the unit vectors we will pass into various managers
	handleUnitAssignments();
	/*if (BWAPI::Broodwar->getFrameCount() % (48 * numOfSecond) < (24 * numOfSecond))
	moveToScatter(400);
	else
	moveToCenter();*/

	//goSquare();

	//lead();

	// utility managers
	_timerManager.startTimer(TimerManager::InformationManager);
	InformationManager::Instance().update();
	_timerManager.stopTimer(TimerManager::InformationManager);

	_timerManager.startTimer(TimerManager::MapGrid);
	MapGrid::Instance().update();
	_timerManager.stopTimer(TimerManager::MapGrid);

	_timerManager.startTimer(TimerManager::MapTools);
	//MapTools::Instance().update();
	_timerManager.stopTimer(TimerManager::MapTools);

	_timerManager.startTimer(TimerManager::Search);
	BOSSManager::Instance().update(35 - _timerManager.getTotalElapsed());
	_timerManager.stopTimer(TimerManager::Search);

	// economy and base managers
	_timerManager.startTimer(TimerManager::Worker);
	WorkerManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Worker);

	_timerManager.startTimer(TimerManager::Production);
	ProductionManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Production);

	_timerManager.startTimer(TimerManager::Building);
	BuildingManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Building);

	// combat and scouting managers
	_timerManager.startTimer(TimerManager::Combat);
	//_defenseManager.update(_defenseUnits);
	_combatCommander.update(_combatUnits);
	_timerManager.stopTimer(TimerManager::Combat);

	_timerManager.startTimer(TimerManager::Scout);
    ScoutManager::Instance().update();
	_timerManager.stopTimer(TimerManager::Scout);
		
	_timerManager.stopTimer(TimerManager::All);

	drawDebugInterface();
}

void GameCommander::drawDebugInterface()
{
	InformationManager::Instance().drawExtendedInterface();
	InformationManager::Instance().drawUnitInformation(425,30);
	InformationManager::Instance().drawMapInformation();
	BuildingManager::Instance().drawBuildingInformation(200,50);
	BuildingPlacer::Instance().drawReservedTiles();
	ProductionManager::Instance().drawProductionInformation(30, 50);
	BOSSManager::Instance().drawSearchInformation(490, 100);
    BOSSManager::Instance().drawStateInformation(250, 0);
    
	_combatCommander.drawSquadInformation(200, 30);
    _timerManager.displayTimers(490, 225);
    drawGameInformation(4, 1);

	// draw position of mouse cursor
	if (Config::Debug::DrawMouseCursorInfo)
	{
		int mouseX = BWAPI::Broodwar->getMousePosition().x + BWAPI::Broodwar->getScreenPosition().x;
		int mouseY = BWAPI::Broodwar->getMousePosition().y + BWAPI::Broodwar->getScreenPosition().y;
		BWAPI::Broodwar->drawTextMap(mouseX + 20, mouseY, " %d %d", mouseX, mouseY);
	}
}

void GameCommander::drawGameInformation(int x, int y)
{
    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Players:");
	BWAPI::Broodwar->drawTextScreen(x+50, y, "%c%s \x04vs. %c%s", BWAPI::Broodwar->self()->getTextColor(), BWAPI::Broodwar->self()->getName().c_str(), 
                                                                  BWAPI::Broodwar->enemy()->getTextColor(), BWAPI::Broodwar->enemy()->getName().c_str());
	y += 12;
		
    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Strategy:");
	BWAPI::Broodwar->drawTextScreen(x+50, y, "\x03%s %s", Config::Strategy::StrategyName.c_str(), Config::Strategy::FoundEnemySpecificStrategy ? "(enemy specific)" : "");
	BWAPI::Broodwar->setTextSize();
	y += 12;

    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Map:");
	BWAPI::Broodwar->drawTextScreen(x+50, y, "\x03%s", BWAPI::Broodwar->mapFileName().c_str());
	BWAPI::Broodwar->setTextSize();
	y += 12;

    BWAPI::Broodwar->drawTextScreen(x, y, "\x04Time:");
    BWAPI::Broodwar->drawTextScreen(x+50, y, "\x04%d %4dm %3ds", BWAPI::Broodwar->getFrameCount(), (int)(BWAPI::Broodwar->getFrameCount()/(23.8*60)), (int)((int)(BWAPI::Broodwar->getFrameCount()/23.8)%60));
}

// assigns units to various managers
void GameCommander::handleUnitAssignments()
{
	_validUnits.clear();
    _combatUnits.clear();

	// filter our units for those which are valid and usable
	setValidUnits();

	// set each type of unit
	setScoutUnits();
	//setDefenseUnits();
	setCombatUnits();
}

bool GameCommander::isAssigned(BWAPI::Unit unit) const
{
	return /*_defenseUnits.contains(unit) ||*/_combatUnits.contains(unit) || _scoutUnits.contains(unit);
}

// validates units as usable for distribution to various managers
void GameCommander::setValidUnits()
{
	// make sure the unit is completed and alive and usable
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (UnitUtil::IsValidUnit(unit))
		{	
			_validUnits.insert(unit);
		}
	}
}

void GameCommander::setScoutUnits()
{
    // if we haven't set a scout unit, do it
    if (_scoutUnits.empty() && !_initialScoutSet)
    {
        BWAPI::Unit supplyProvider = getFirstSupplyProvider();

		// if it exists
		if (supplyProvider)
		{
			// grab the closest worker to the supply provider to send to scout
			BWAPI::Unit workerScout = getClosestWorkerToTarget(supplyProvider->getPosition());

			// if we find a worker (which we should) add it to the scout units
			if (workerScout)
			{
                ScoutManager::Instance().setWorkerScout(workerScout);
				assignUnit(workerScout, _scoutUnits);
                _initialScoutSet = true;
			}
		}
    }
}

void GameCommander::setDefenseUnits()
{
	_defenseUnits = _defenseManager.getDefenseUnitSet();
	for (auto & unit : _validUnits)
	{
		if (_defenseUnits.size() >= 4)
			break;
		if (!isAssigned(unit) && UnitUtil::IsCombatUnit(unit))
		{
			assignUnit(unit, _defenseUnits);
		}
	}
}
// sets combat units to be passed to CombatCommander
void GameCommander::setCombatUnits()
{
	for (auto & unit : _validUnits)
	{
		if (!isAssigned(unit) && UnitUtil::IsCombatUnit(unit) || unit->getType().isWorker())		
		{	
			assignUnit(unit, _combatUnits);
		}
	}
}

BWAPI::Unit GameCommander::getFirstSupplyProvider()
{
	BWAPI::Unit supplyProvider = nullptr;

	if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg)
	{
		for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		{
			if (unit->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool)
			{
				supplyProvider = unit;
			}
		}
	}
	else
	{
		
		for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		{
			if (unit->getType() == BWAPI::Broodwar->self()->getRace().getSupplyProvider())
			{
				supplyProvider = unit;
			}
		}
	}

	return supplyProvider;
}

void GameCommander::onUnitShow(BWAPI::Unit unit)			
{ 
	InformationManager::Instance().onUnitShow(unit); 
	WorkerManager::Instance().onUnitShow(unit);
}

void GameCommander::onUnitHide(BWAPI::Unit unit)			
{ 
	InformationManager::Instance().onUnitHide(unit); 
}

void GameCommander::onUnitCreate(BWAPI::Unit unit)		
{ 
	InformationManager::Instance().onUnitCreate(unit); 
}

void GameCommander::onUnitComplete(BWAPI::Unit unit)
{
	InformationManager::Instance().onUnitComplete(unit);
}

void GameCommander::onUnitRenegade(BWAPI::Unit unit)		
{ 
	InformationManager::Instance().onUnitRenegade(unit); 
}

void GameCommander::onUnitDestroy(BWAPI::Unit unit)		
{ 	
	ProductionManager::Instance().onUnitDestroy(unit);
	WorkerManager::Instance().onUnitDestroy(unit);
	InformationManager::Instance().onUnitDestroy(unit); 
}

void GameCommander::onUnitMorph(BWAPI::Unit unit)		
{ 
	InformationManager::Instance().onUnitMorph(unit);
	WorkerManager::Instance().onUnitMorph(unit);
}

BWAPI::Unit GameCommander::getClosestUnitToTarget(BWAPI::UnitType type, BWAPI::Position target)
{
	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 100000;

	for (auto & unit : _validUnits)
	{
		if (unit->getType() == type)
		{
			double dist = unit->getDistance(target);
			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}

BWAPI::Unit GameCommander::getClosestWorkerToTarget(BWAPI::Position target)
{
	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 100000;

	for (auto & unit : _validUnits)
	{
		if (!isAssigned(unit) && unit->getType().isWorker() && WorkerManager::Instance().isFree(unit))
		{
			double dist = unit->getDistance(target);
			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}

void GameCommander::assignUnit(BWAPI::Unit unit, BWAPI::Unitset & set)
{
    if (_scoutUnits.contains(unit)) { _scoutUnits.erase(unit); }
	//else if (_defenseUnits.contains(unit)) { _defenseUnits.erase(unit); }
    else if (_combatUnits.contains(unit)) { _combatUnits.erase(unit); }

    set.insert(unit);
}

BWAPI::Position GameCommander::getCenter() {
	BWAPI::Unitset _unitSet;
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (!unit->getType().isBuilding())
			_unitSet.insert(unit);
	}
	return _unitSet.getPosition();
}
void GameCommander::moveToCenter() {
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (!unit->getType().isBuilding() && unit->getDistance(center) > 63)
			Micro::SmartMove(unit, center);
	}
}
void GameCommander::moveToScatter(int range) {
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (!unit->getType().isBuilding())
		{
			BWAPI::Position destination;
			BWAPI::Position unitPosition = unit->getPosition();
			int distance = unitPosition.getDistance(center);

			if (distance == 0)
			{
				destination.x = center.x + range;
				destination.y = center.y;
				Micro::SmartMove(unit, destination);
			}
			else
			{
				destination.x = (int)(range * ((float)(unitPosition.x - center.x) / (float)distance) + (float)center.x);
				destination.y = (int)(range * ((float)(unitPosition.y - center.y) / (float)distance) + (float)center.y);
				Micro::SmartMove(unit, destination);
			}


		}
	}
}

void GameCommander::goSquare()
{
	BWAPI::Position p1;
	BWAPI::Position p2;
	BWAPI::Position p3;
	BWAPI::Position p4;

	p1 = center;

	p2.x = p1.x + 400;
	p2.y = p1.y;

	p3.x = p2.x;
	p3.y = p2.y + 400;

	p4.x = p3.x - 400;
	p4.y = p3.y;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (24 * numOfSecond))
			Micro::SmartMove(unit, p2);
		else if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (48 * numOfSecond))
			Micro::SmartMove(unit, p3);
		else if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (72 * numOfSecond))
			Micro::SmartMove(unit, p4);
		else
			Micro::SmartMove(unit, p1);
	}
}

void GameCommander::lead()
{
	BWAPI::Position p1;
	BWAPI::Position p2;
	BWAPI::Position p3;
	BWAPI::Position p4;

	BWAPI::Position z1;
	BWAPI::Position z2;
	BWAPI::Position z3;
	BWAPI::Position z4;
	//BWAPI::Position leadPosition;

	p1 = center;
	p2.x = p1.x + 400;
	p2.y = p1.y + 100;
	p3.x = p2.x + 200;
	p3.y = p2.y + 400;
	p4.x = p3.x - 400;
	p4.y = p3.y + 300;

	z2.x = (int)0.99 * (p2.x - p1.x) + p1.x;
	z2.y = (int)0.99 * (p2.y - p1.y) + p1.y;
	z3.x = (int)0.99 * (p3.x - p2.x) + p2.x;
	z3.y = (int)0.99 * (p3.y - p2.y) + p2.y;
	z4.x = (int)0.99 * (p4.x - p3.x) + p3.x;
	z4.y = (int)0.99 * (p4.y - p3.y) + p3.y;
	z1.x = (int)0.99 * (p1.x - p4.x) + p4.x;
	z1.y = (int)0.99 * (p1.y - p4.y) + p4.y;

	//z2.x = p2.x - 300;
	//z2.y = p2.y - 300;
	//z3.x = p3.x + 300;
	//z3.y = p3.y - 300;
	//z4.x = p4.x + 300;
	//z4.y = p4.y + 300;
	//z1.x = p1.x - 300;
	//z1.y = p1.y + 300;


	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{

		if (unit->getType() == BWAPI::UnitTypes::Protoss_Dragoon)
		{
			if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (24 * numOfSecond))
				Micro::SmartMove(unit, p2);
			else if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (48 * numOfSecond))
				Micro::SmartMove(unit, p3);
			else if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (72 * numOfSecond))
				Micro::SmartMove(unit, p4);
			else
				Micro::SmartMove(unit, p1);
		}
		else
		{
			if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (24 * numOfSecond))
				Micro::SmartMove(unit, z2);
			else if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (48 * numOfSecond))
				Micro::SmartMove(unit, z3);
			else if (BWAPI::Broodwar->getFrameCount() % (96 * numOfSecond) < (72 * numOfSecond))
				Micro::SmartMove(unit, z4);
			else
				Micro::SmartMove(unit, z1);
		}
	}
}

