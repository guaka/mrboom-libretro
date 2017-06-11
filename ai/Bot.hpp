#pragma once
#include "MrboomHelper.hpp"
#include "GridFunctions.hpp"


class Bot {
public:
Bot(int playerIndex);
void initBot();
int  bestBonusCell();
int  bestCellToDropABomb();
int  bestSafeCell();
bool isInMiddleOfCell();
bool isSomewhatInTheMiddleOfCell();
bool isThereABombUnderMe();
void stopWalking();
void startPushingBombDropButton();
void startPushingRemoteButton();
void startPushingJumpButton();
void stopPushingBombDropButton();
void stopPushingRemoteButton();
void stopPushingJumpButton();
bool walkToCell(int cell);
bool amISafe();
int getCurrentCell();
void printGrid();
void printCellInfo(int cell);
int howManyBombsLeft();
uint8_t calculateBestCellToDropABomb();
uint8_t calculateBestCellToPickUpBonus();
int _playerIndex;
#ifdef DEBUG
enum Button _direction1FrameAgo;
enum Button _direction2FramesAgo;
int _shiveringCounter;
#endif
public:
travelCostGrid travelGrid;
uint32_t bestExplosionsGrid[grid_size_x][grid_size_y];  // score based on the nb of bricks one of my bomb there would break or of the proximity from a monster
uint8_t calculatedBestCellToDropABomb;
uint8_t calculatedBestCellToPickUpBonus;
uint32_t flameGrid[grid_size_x][grid_size_y];     // 0: no flame, 1..FLAME_DURATION+1: time with a flame, FLAME_DURATION+2: time before end of flame
bool dangerGrid[grid_size_x][grid_size_y]; // used to track all dangers, including the ones we don't know the timing: true means a flame is coming (possibily under a remote controled bomb...), or that a monster is there
bool pushingDropBombButton;
};
