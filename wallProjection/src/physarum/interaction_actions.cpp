#include "Physarum.h"

void Physarum::actionChangeSigmaCount(int dir)
{
    sigmaCount = (sigmaCount + sigmaCountModulo + dir) % sigmaCountModulo;
    penMoveLatestTime = getTime();
    latestSigmaChangeTime = getTime();
}

void Physarum::actionChangeParams(int dir)
{
    pointsDataManager.changeParamIndex(dir);

    transitionTriggerTime = getTime();
}

void Physarum::actionSwapParams()
{
    pointsDataManager.swapUsedPoints();

    transitionTriggerTime = getTime();
}

void Physarum::actionRandomParams()
{
    pointsDataManager.useRandomIndices();

    transitionTriggerTime = getTime();
}

void Physarum::actionChangeColorMode()
{
    colorModeType = (colorModeType + 1) % NUMBER_OF_COLOR_MODES;
}

void Physarum::actionTriggerWave(int x, int y)
{
    waveXarray[currentWaveIndex] = x;
    waveYarray[currentWaveIndex] = y;
    waveTriggerTimes[currentWaveIndex] = getTime();
    waveSavedSigmas[currentWaveIndex] = currentActionAreaSizeSigma;

    currentWaveIndex = (currentWaveIndex + 1) % MAX_NUMBER_OF_WAVES;

    waveActionAreaSizeSigma = currentActionAreaSizeSigma;
}

void Physarum::actionChangeDisplayType()
{
    displayType = (displayType + 1) % 2;
}

void Physarum::actionChangeSelectionIndex(int dir)
{
    pointsDataManager.changeSelectionIndex(dir);
}

void Physarum::actionSpawnParticles(int spawnType)
{
    particlesSpawn = spawnType;
    if (spawnType == 2)
    {
        setRandomSpawn();
    }
}