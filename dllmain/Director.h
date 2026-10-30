#pragma once
#include <cstdint>
#include <string>
#include <vector>
class cEm;
struct EM_LIST;
namespace aiza
{
struct DirectorTemplateView { uint64_t key=0; std::string label; bool available=false; };
struct DirectorView
{
    uint64_t epoch=0;
    bool marked=false, coordinatesVerified=false, waves=false;
    unsigned owned=0, alive=0, wave=0, queued=0, replaced=0;
    std::string status, target, timedEvent;
    std::vector<DirectorTemplateView> templates;
};
enum class DirectorCommand {MarkPoint,Spawn,ClearOwned,StartWaves,StopWaves};
DirectorView DirectorSnapshot();
void DirectorRequest(DirectorCommand command,uint64_t epoch,uint64_t key=0,int count=1);
void DirectorBeforeUpdate();
void DirectorAfterUpdate();
void DirectorObserveInit(cEm* actor,const EM_LIST* data);
void DirectorObserveReady(cEm* actor,const EM_LIST* data);
void DirectorPreparePending();
void DirectorRestorePending();
bool DirectorOwns(cEm* actor);
float DirectorEliteSpeed(cEm* actor);
float DirectorRunFactor();
float DirectorFireFactor();
float DirectorEnemyFactor();
int DirectorFlightMode(int configured);
bool DirectorForceLaunch();
std::string DirectorOverlay();
// Implemented in ModExpansion.cpp using the already-resolved upstream pointer.
cEm* DirectorNativeSpawn(EM_LIST* data);
bool DirectorNativeSpawnAvailable();
bool DirectorLiveActor(cEm* actor);
}
