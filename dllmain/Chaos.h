#pragma once
class cEm;
namespace aiza
{
void ChaosBeforeUpdate();
void ChaosAfterUpdate();
void ChaosOnSpawn(cEm* enemy);
bool ChaosWantsFireTrigger();
void ChaosRequestPulse();
void ChaosDrawOverlay();
}
