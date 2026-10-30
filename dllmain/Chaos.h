#pragma once
class cEm;
namespace aiza
{
void ChaosBeforeUpdate();
void ChaosAfterUpdate();
void ChaosOnSpawn(cEm* enemy);
void ChaosForgetActor(cEm* enemy);
void ChaosRequestBowling();
bool ChaosActorFlying(cEm* enemy);
bool ChaosWantsFireTrigger();
void ChaosRequestPulse();
void ChaosDrawOverlay();
}
