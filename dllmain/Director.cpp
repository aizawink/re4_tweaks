#include "dllmain.h"
#include "Game.h"
#include "Settings.h"
#include "Chaos.h"
#include "Director.h"
#include "DirectorCore.h"
#include <algorithm>
#include <cstdio>
#include <deque>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace aiza
{
namespace
{
struct Identity { cEm* pointer=nullptr; uint32_t guid=0; };
struct Template {EM_LIST data{};std::string label;};
struct Owned {uint32_t guid=0;unsigned wave=0,serial=0;float speed=1;};
struct Elite {uint32_t guid=0;float speed=1;};
struct Request {DirectorCommand command;uint64_t epoch,key;int count;};
struct Job {uint64_t key=0;unsigned wave=0;float hpFactor=1;Point point;};
struct Patch {unsigned index=0;char id=0,type=0,character=0;uint32_t flags=0;EM_LIST applied{};};
std::mutex viewMutex,requestMutex;
DirectorView view;
std::deque<Request> requests;
std::deque<Job> jobs;
std::map<uint64_t,Template> catalog;
std::unordered_map<cEm*,Owned> owned;
std::unordered_map<cEm*,Elite> elites;
std::unordered_set<uint32_t> replaced;
std::vector<Patch> patches;
GLOBAL_WK* currentGlobal=nullptr;
cEmMgr* currentManager=nullptr;
void* currentArray=nullptr;
void* currentRoom=nullptr;
cPlayer* currentPlayer=nullptr;
unsigned room=0,serial=0,wave=0;
uint64_t epoch=0,waveTemplate=0;
Point marker;
bool marked=false,verified=false,waves=false,waveSpawning=false,creating=false;
bool seedReady=false;
float waveWait=-1,timedClock=0;
int timedEvent=-1,lastSeed=0;
Random timedRandom;
ULONGLONG previousTick=0,nextSpawnTick=0;
std::string status="Enter gameplay to discover native templates.";

Point Position(cEm* actor) {return {actor->pos_94.x,actor->pos_94.y,actor->pos_94.z};}
bool SameScene()
{
    auto* g=GlobalPtr();auto* m=EmMgrPtr();
    return g==currentGlobal && m==currentManager && PlayerPtr()==currentPlayer &&
        (m ? m->m_Array_4 : nullptr)==currentArray && (g ? g->pRoom_40 : nullptr)==currentRoom &&
        (RoomData ? unsigned(RoomData->m_RoomNo_E) : 0)==room;
}
void SyncScene()
{
    if (SameScene()) return;
    // A new room owns the memory now. Never restore an old ESL snapshot into it.
    catalog.clear();owned.clear();elites.clear();replaced.clear();patches.clear();jobs.clear();
    marked=false;verified=false;waves=false;wave=0;serial=0;timedEvent=-1;timedClock=0;seedReady=false;
    ++epoch;currentGlobal=GlobalPtr();currentManager=EmMgrPtr();currentPlayer=PlayerPtr();
    currentArray=currentManager ? currentManager->m_Array_4 : nullptr;
    currentRoom=currentGlobal ? currentGlobal->pRoom_40 : nullptr;
    room=RoomData ? unsigned(RoomData->m_RoomNo_E) : 0;
    status="Room changed. Native templates will be rediscovered; mark a new point.";
}
bool Gameplay()
{
    auto* g=GlobalPtr();auto* p=PlayerPtr();
    if(!g || !p || !EmMgrPtr() || !RoomData || !room || !p->IsValid() || p->hp_324<=0) return false;
    if(g->Rno0_20!=uint8_t(GLOBAL_WK::Routine0::MainLoop)) return false;
    return !FlagIsSet(g->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_EVENT)) &&
        !FlagIsSet(g->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_MOVIE_ON)) &&
        !FlagIsSet(g->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_MOVIE2_ON)) &&
        !FlagIsSet(g->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_SUB_SCRN)) &&
        !FlagIsSet(g->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_DIEDEMO)) &&
        !FlagIsSet(g->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_PL_BOAT)) &&
        !FlagIsSet(g->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_PL_LADDER)) &&
        !FlagIsSet(g->flags_STOP_0_170,uint32_t(Flags_STOP::SPF_PL)) &&
        !FlagIsSet(g->flags_STOP_0_170,uint32_t(Flags_STOP::SPF_EM)) &&
        !FlagIsSet(g->Flags_SYSTEM_0_54,uint32_t(Flags_SYSTEM::SYS_ROOMJUMP));
}
void SetStatus(const std::string& message)
{
    if(status==message) return;
    status=message;spd::log()->info("[Aiza Director] {}",message);
}
bool Matches(const cEm& em,const EM_LIST& data)
{
    return em.id_100==uint8_t(data.id_1) && em.type_101==uint8_t(data.type_2) &&
        em.characterNum_3D8==uint8_t(data.Character_B) &&
        (em.flag_3D0&0xffffff00u)==(data.flag_4&0xffffff00u);
}
bool Witness(const Template& entry)
{
    if(!currentManager) return false;
    for(auto& actor:*currentManager)
        if(actor.IsValid() && actor.r_no_0_FC!=uint8_t(cEm::Routine0::Init) &&
            actor.pModelInfo_15C && actor.nParts_102>0 && Matches(actor,entry.data)) return true;
    return false;
}
void Register(cEm* actor,const EM_LIST* data)
{
    if(!actor || !data || !actor->IsValid() || owned.count(actor) || actor->sce_func_398 ||
        actor->hp_324<=0 || (!actor->pModelInfo_15C || !actor->nParts_102) || data->hp_8<=0 || data->room_18!=room ||
        !CatalogTarget(actor->id_100,re4t::cfg->bDirectorHeavyTemplates) ||
        (uint8_t(data->set_3)>0x0f && !ChainsawAppearance(actor->id_100,actor->type_101,actor->flag_3D0)) || !Matches(*actor,*data)) return;
    const uint64_t key=TemplateKey(actor->id_100,actor->type_101,actor->characterNum_3D8,actor->flag_3D0);
    if(catalog.count(key) || catalog.size()>=64) return;
    char label[192];
    std::snprintf(label,sizeof(label),"%s | id %02X type %02X weapon %02X",
        ChainsawAppearance(actor->id_100,actor->type_101,actor->flag_3D0) ? "Chainsaw template" : cEmMgr::EmIdToName(actor->id_100,true).c_str(),
        unsigned(actor->id_100),unsigned(actor->type_101),unsigned(actor->flag_3D0>>24));
    // Preserve the exact native descriptor. No fabricated weapon/model flags.
    catalog.emplace(key,Template{*data,label});
}
void ApplyElite(cEm* actor,unsigned token);
void Discover()
{
    if(!currentGlobal || !currentManager) return;
    for(auto& actor:*currentManager)
        if(actor.IsSpawnedByESL()) {
            Register(&actor,&currentGlobal->Em_list_5410[actor.emListIndex_3A0]);
            if(!actor.sce_func_398 && actor.r_no_0_FC==uint8_t(cEm::Routine0::Move)) ApplyElite(&actor,unsigned(actor.emListIndex_3A0));
        }
}
void ApplyElite(cEm* actor,unsigned token)
{
    if(!Gameplay() || !re4t::cfg->bChaosEnabled || !re4t::cfg->bDirectorElites || !actor ||
        actor->hp_324<=0 || !ReplacementHumanoid(actor->id_100)) return;
    auto found=elites.find(actor);
    if(found!=elites.end() && found->second.guid==actor->guid_F8) return;
    Random random(MixSeed(MixSeed(uint32_t(re4t::cfg->iDirectorSeed),room),token));
    const float chance=Bound(re4t::cfg->fDirectorEliteChance,0,100,20);
    if(random.Unit()*100>=chance) {elites[actor]={actor->guid_F8,1};return;}
    const float hp=Bound(re4t::cfg->fDirectorEliteHP,1,10,3);
    actor->hp_324=ScaleEnemyHP(actor->hp_324,hp);
    actor->hp_max_326=ScaleEnemyHP(actor->hp_max_326,hp);
    const float scale=Bound(re4t::cfg->fDirectorEliteScale,1,4,1.8f);
    actor->scale_AC.x*=scale;actor->scale_AC.y*=scale;actor->scale_AC.z*=scale;
    ((cEm10*)actor)->Scale_498=actor->scale_AC;
    elites[actor]={actor->guid_F8,Bound(re4t::cfg->fDirectorEliteSpeed,1,5,2)};
}
unsigned OwnedAlive(unsigned group=0)
{
    unsigned count=0;if(!currentManager) return 0;
    for(auto& actor:*currentManager) {
        auto it=owned.find(&actor);
        if(it!=owned.end() && it->second.guid==actor.guid_F8 && actor.IsValid() && actor.hp_324>0 &&
            (!group || it->second.wave==group)) ++count;
    }return count;
}
void ClearOwned()
{
    if(!currentManager) return;
    std::vector<Identity> remove;
    for(auto& actor:*currentManager) {
        auto it=owned.find(&actor);
        if(it!=owned.end() && it->second.guid==actor.guid_F8 && actor.IsValid())
            remove.push_back({&actor,actor.guid_F8});
    }
    for(auto identity:remove) {
        auto* actor=identity.pointer;
        if(!DirectorLiveActor(actor) || actor->guid_F8!=identity.guid) continue;
        if(actor->pEmCatch_3C0 || (currentPlayer && currentPlayer->m_pEm_558==actor)) continue;
        ChaosForgetActor(actor);elites.erase(actor);owned.erase(actor);
        currentManager->destroy(actor);
    }
    jobs.clear();waves=false;waveSpawning=false;
    SetStatus(owned.empty() ? "Additional enemies removed. Native enemies were not selected." : "Some additional actors are attached to another actor. Release the grab/action, then clear again.");
}
void StopWaves(const char* reason)
{
    waves=false;waveSpawning=false;waveWait=-1;
    for(auto it=jobs.begin();it!=jobs.end();) if(it->wave) it=jobs.erase(it);else ++it;
    SetStatus(reason);
}
bool Queue(uint64_t key,unsigned count,unsigned group,float hpFactor)
{
    auto it=catalog.find(key);
    if(!marked) {SetStatus("Spawn failed: mark a spawn point while standing on the floor first.");return false;}
    if(!verified) {SetStatus("Spawn failed: waiting for native ESL coordinate verification. Reload this room once.");return false;}
    if(it==catalog.end() || !Witness(it->second)) {SetStatus("Spawn failed: selected native template is not available in this room.");return false;}
    if(!DirectorNativeSpawnAvailable()) {SetStatus("Spawn failed: native spawn hook is not available.");return false;}
    if(owned.size()+jobs.size()+count>24) {SetStatus("Spawn failed: 24 additional-object limit. Clear additional enemies first.");return false;}
    for(unsigned i=0;i<count;++i) jobs.push_back({key,group,hpFactor,marker});
    SetStatus("Spawn queued: "+std::to_string(count)+". Native results will be checked.");return true;
}
bool Spawn(const Job& job)
{
    auto it=catalog.find(job.key);
    if(it==catalog.end() || !Witness(it->second)) {SetStatus("Spawn stopped: template/model witness is no longer present. Rediscover it in this room.");return false;}
    EM_LIST data=it->second.data;
    int16_t x,y,z;
    if(!EncodeESLPosition(job.point,x,y,z)) {SetStatus("Spawn failed: marked position is outside ESL coordinate range.");return false;}
    data.s_pos_C={x,y,z};data.hp_8=int16_t(std::clamp(re4t::cfg->iDirectorSpawnHP,100,32767));
    data.be_flag_0=EM_BE_FLAG_ALIVE;
    data.percentageMotionSpeed_1C=data.percentageScale_1E=0;
    std::unordered_set<uint32_t> existing;
    for(auto& actor:*currentManager) if(actor.IsValid()) existing.insert(actor.guid_F8);
    creating=true;cEm* result=DirectorNativeSpawn(&data);creating=false;
    if(!DirectorLiveActor(result) || existing.count(result->guid_F8))
    {SetStatus("Spawn failed: native creator returned no new compatible object (pool/resource failure).");return false;}
    owned[result]={result->guid_F8,job.wave,++serial,Bound(re4t::cfg->fDirectorSpawnSpeed,.2f,5,1)};ChaosForgetActor(result);
    if(result->id_100!=uint8_t(data.id_1))
    {SetStatus("Spawn failed: native creator returned a different ID; object tracked for Clear, no extra scaling applied.");return false;}
    if(result->hp_324<=0) {SetStatus("Spawn failed: native initialization produced a dead object; stop and clear additional enemies.");return false;}
    const int16_t hp=ScaleEnemyHP(data.hp_8,job.hpFactor);
    result->hp_324=result->hp_max_326=hp;
    const float scale=Bound(re4t::cfg->fDirectorSpawnScale,.5f,3,1);
    result->scale_AC.x*=scale;result->scale_AC.y*=scale;result->scale_AC.z*=scale;
    if(CatalogHumanoid(result->id_100)) ((cEm10*)result)->Scale_498=result->scale_AC;
    ApplyElite(result,0x80000000u|serial);
    SetStatus("Spawn succeeded: "+it->second.label+" | GUID "+std::to_string(result->guid_F8));
    return true;
}
void Publish()
{
    DirectorView next;next.epoch=epoch;next.marked=marked;next.coordinatesVerified=verified;
    next.replaced=unsigned(replaced.size());
    next.owned=unsigned(owned.size());next.alive=OwnedAlive();next.wave=wave;next.waves=waves;
    next.queued=unsigned(jobs.size());next.status=status;
    static const char* eventNames[]={"Rocket Party","Fast Enemies","Slow Enemies","Tornado Party","BRRRT Madness"};
    if(timedEvent>=0 && re4t::cfg->bChaosEnabled && re4t::cfg->bDirectorTimed)
        next.timedEvent=eventNames[timedEvent];
    for(auto& entry:catalog) next.templates.push_back({entry.first,entry.second.label,Witness(entry.second)});
    // Camera-cone candidate, not a raycast: obstacles can obscure this actor.
    if(re4t::cfg->bDirectorTargetHUD && currentGlobal && currentPlayer && currentManager) {
        const auto camera=currentGlobal->Camera_74.CamPoint_A4.Campos_0;
        const auto target=currentGlobal->Camera_74.CamPoint_A4.Target_C;
        float x=target.x-camera.x,y=target.y-camera.y,z=target.z-camera.z;
        const float length=std::sqrt(x*x+y*y+z*z);float best=.985f;
        for(auto& actor:*currentManager) {
            if(!actor.IsValid() || actor.hp_324<=0 || !CatalogTarget(actor.id_100,true)) continue;
            float dx=actor.pos_94.x-camera.x,dy=actor.pos_94.y+140-camera.y,dz=actor.pos_94.z-camera.z;
            const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
            if(length<1 || distance<1) continue;
            const float dot=(dx*x+dy*y+dz*z)/(length*distance);
            if(dot>best) {
                best=dot;char label[220];
                std::snprintf(label,sizeof(label),"Aim candidate: %s | ID %02X type %02X | HP %d/%d | GUID %u%s",
                    cEmMgr::EmIdToName(actor.id_100,true).c_str(),unsigned(actor.id_100),unsigned(actor.type_101),
                    int(actor.hp_324),int(actor.hp_max_326),actor.guid_F8,DirectorOwns(&actor)?" [added]":"");next.target=label;
            }
        }
    }
    std::lock_guard<std::mutex> lock(viewMutex);view=std::move(next);
}
}

bool DirectorLiveActor(cEm* actor)
{
    auto* manager=EmMgrPtr();if(!actor || !manager || !manager->m_Array_4 || !manager->m_blockSize_C) return false;
    const auto begin=reinterpret_cast<std::uintptr_t>(manager->m_Array_4),address=reinterpret_cast<std::uintptr_t>(actor);
    const uint64_t size=uint64_t(manager->m_nArray_8)*manager->m_blockSize_C;
    return address>=begin && uint64_t(address-begin)<size && (address-begin)%manager->m_blockSize_C==0 && actor->IsValid();
}
bool DirectorOwns(cEm* actor)
{
    auto it=owned.find(actor);return it!=owned.end() && actor && it->second.guid==actor->guid_F8;
}
float DirectorEliteSpeed(cEm* actor)
{
    auto it=elites.find(actor);
    float result=re4t::cfg->bDirectorElites && it!=elites.end() && it->second.guid==actor->guid_F8 ? it->second.speed : 1;
    auto own=owned.find(actor);
    if(own!=owned.end() && own->second.guid==actor->guid_F8) result*=own->second.speed;
    return result;
}
DirectorView DirectorSnapshot() {std::lock_guard<std::mutex> lock(viewMutex);return view;}
void DirectorRequest(DirectorCommand command,uint64_t sceneEpoch,uint64_t key,int count)
{
    std::lock_guard<std::mutex> lock(requestMutex);if(requests.size()<16) requests.push_back({command,sceneEpoch,key,count});
}
void DirectorObserveInit(cEm* actor,const EM_LIST* data)
{
    SyncScene();if(!actor || !data || actor->r_no_0_FC!=uint8_t(cEm::Routine0::Init) || data->room_18!=room) return;
    const Point expected={float(data->s_pos_C.x)*10,float(data->s_pos_C.y)*10,float(data->s_pos_C.z)*10};
    if(Finite(expected) && Distance2(Position(actor),expected)<1) verified=true;
}
void DirectorObserveReady(cEm* actor,const EM_LIST* data)
{
    SyncScene();if(creating || !DirectorLiveActor(actor) || !data) return;
    for(const auto& patch:patches)
        if(data==&currentGlobal->Em_list_5410[patch.index] && Matches(*actor,patch.applied))
            replaced.insert(actor->guid_F8);
    Register(actor,data);
    if(actor->IsSpawnedByESL()) ApplyElite(actor,unsigned(actor->emListIndex_3A0));
    DirectorPreparePending();
}
void DirectorPreparePending()
{
    SyncScene();if(!re4t::cfg->bChaosEnabled || !re4t::cfg->bDirectorReplace || !Gameplay() || catalog.empty()) return;
    std::vector<const Template*> choices;
    for(auto& entry:catalog)
        if(ReplacementHumanoid(uint8_t(entry.second.data.id_1)) && Witness(entry.second) &&
            (re4t::cfg->iDirectorReplaceMode!=2 || ChainsawAppearance(uint8_t(entry.second.data.id_1),uint8_t(entry.second.data.type_2),entry.second.data.flag_4)))
            choices.push_back(&entry.second);
    if(choices.empty()) {SetStatus("Replacement waiting: no compatible native template (chainsaw mode requires a chainsaw witness).");return;}
    if(re4t::cfg->iDirectorReplaceMode==0) {
        auto it=catalog.find(uint64_t(re4t::cfg->uDirectorSelectedTemplate));
        if(it==catalog.end() || !Witness(it->second) || !ReplacementHumanoid(uint8_t(it->second.data.id_1))) {
            SetStatus("Replacement waiting: select an available humanoid template in the catalog.");return;
        }
        choices={&it->second};
    }
    for(unsigned index=0;index<255;++index) {
        auto& data=currentGlobal->Em_list_5410[index];
        if(data.room_18!=room || !(data.be_flag_0&EM_BE_FLAG_ALIVE) ||
            (data.be_flag_0&(EM_BE_FLAG_SET|EM_BE_FLAG_DIE|EM_BE_FLAG_MODEXP_SPEEDSCALE)) ||
            data.hp_8<=0 || uint8_t(data.set_3)>0x0f || !ReplacementHumanoid(uint8_t(data.id_1))) continue;
        if(std::any_of(patches.begin(),patches.end(),[&](const Patch& p){return p.index==index;})) continue;
        Random random(MixSeed(MixSeed(uint32_t(re4t::cfg->iDirectorSeed),room),index));
        const auto& replacement=choices[random.Pick(unsigned(choices.size()))]->data;
        if(data.id_1==replacement.id_1 && data.type_2==replacement.type_2 &&
            data.Character_B==replacement.Character_B &&
            (data.flag_4&0xffffff00u)==(replacement.flag_4&0xffffff00u)) continue;
        Patch patch{index,data.id_1,data.type_2,data.Character_B,data.flag_4,{}};
        data.id_1=replacement.id_1;data.type_2=replacement.type_2;data.Character_B=replacement.Character_B;
        // Equipment bytes come from a native witness; retain sight/behavior byte,
        // original routine, ESL index, position, trigger/lifecycle and room data.
        data.flag_4=(data.flag_4&255u)|(replacement.flag_4&0xffffff00u);
        patch.applied=data;patches.push_back(patch);
    }
}
void DirectorRestorePending()
{
    if(!SameScene()) {patches.clear();return;}
    for(const auto& patch:patches) {
        auto& data=currentGlobal->Em_list_5410[patch.index];
        if(data.room_18==room && data.id_1==patch.applied.id_1 && data.type_2==patch.applied.type_2 &&
            data.Character_B==patch.applied.Character_B && data.flag_4==patch.applied.flag_4) {
            data.id_1=patch.id;data.type_2=patch.type;data.Character_B=patch.character;data.flag_4=patch.flags;
        }
    }patches.clear();
}
void DirectorBeforeUpdate()
{
    SyncScene();const auto now=GetTickCount64();
    const float dt=previousTick ? Bound(float(now-previousTick)/1000,0,.1f,0) : 0;previousTick=now;
    if(!Gameplay()) {DirectorRestorePending();Publish();return;}
    Discover();
    std::deque<Request> pending;{std::lock_guard<std::mutex> lock(requestMutex);pending.swap(requests);}
    for(const auto& request:pending) {
        if(request.epoch!=epoch) {SetStatus("Request cancelled: the room changed. Select a current template again.");continue;}
        if(request.command==DirectorCommand::ClearOwned) {ClearOwned();continue;}
        if(request.command==DirectorCommand::StopWaves) {StopWaves("Waves stopped. Existing additional enemies remain until cleared/killed.");continue;}
        if(!re4t::cfg->bChaosEnabled) {SetStatus("Request ignored: turn Chaos master ON first.");continue;}
        if(request.command==DirectorCommand::MarkPoint) {
            marker=Position(currentPlayer);marked=Finite(marker);SetStatus("Spawn point marked at Leon's position. Move away before spawning.");
        }
        if(request.command==DirectorCommand::Spawn) Queue(request.key,unsigned(std::clamp(request.count,1,5)),0,1);
        if(request.command==DirectorCommand::StartWaves) {
            if(waves) {SetStatus("Waves already running. Stop them before starting again.");continue;}
            if(!jobs.empty()) {SetStatus("Finish the current spawn queue before starting waves.");continue;}
            wave=1;waveTemplate=request.key;waveSpawning=true;waveWait=-1;
            waves=Queue(waveTemplate,WaveCount(re4t::cfg->iDirectorWaveStart,re4t::cfg->iDirectorWaveGrowth,1),1,1);
        }
    }
    if(!re4t::cfg->bChaosEnabled) {
        if(waves) StopWaves("Waves stopped: Chaos master OFF.");
        jobs.clear();timedEvent=-1;timedClock=0;
        DirectorRestorePending();Publish();return;
    }
    const int seed=re4t::cfg->iDirectorSeed;
    if(!seedReady || lastSeed!=seed) {seedReady=true;lastSeed=seed;timedRandom=Random(MixSeed(uint32_t(seed),room));timedClock=0;timedEvent=-1;}
    if(re4t::cfg->bDirectorTimed) {
        timedClock-=dt;
        if(timedClock<=0) {timedEvent=int(timedRandom.Pick(5));timedClock=Bound(re4t::cfg->fDirectorTimedInterval,5,120,20);}
    } else {timedEvent=-1;timedClock=0;}
    if(!jobs.empty() && now>=nextSpawnTick) {
        Job job=jobs.front();jobs.pop_front();nextSpawnTick=now+250;
        if(!Spawn(job)) {jobs.clear();if(waves) StopWaves((status+" Waves stopped; no automatic retry.").c_str());}
    }
    if(waves && jobs.empty()) {
        if(waveSpawning) {waveSpawning=false;waveWait=-1;}
        if(OwnedAlive(wave)==0) {
            if(wave>=unsigned(std::clamp(re4t::cfg->iDirectorWaveLimit,1,50))) StopWaves("All configured waves completed.");
            else if(waveWait<0) waveWait=Bound(re4t::cfg->fDirectorWaveDelay,0,20,2);
            else if((waveWait-=dt)<=0) {
                ++wave;waveWait=-1;waveSpawning=true;
                if(!Queue(waveTemplate,WaveCount(re4t::cfg->iDirectorWaveStart,re4t::cfg->iDirectorWaveGrowth,int(wave)),wave,
                    WaveHPFactor(int(wave),re4t::cfg->fDirectorWaveHPGrowth))) StopWaves((status+" Waves stopped.").c_str());
            }
        }
    }
    DirectorPreparePending();Publish();
}
void DirectorAfterUpdate()
{
    DirectorRestorePending();SyncScene();
    std::unordered_set<cEm*> present;
    if(currentManager) for(auto& actor:*currentManager) if(actor.IsValid()) {
        present.insert(&actor);
        auto own=owned.find(&actor);if(own!=owned.end() && own->second.guid!=actor.guid_F8) owned.erase(own);
        auto elite=elites.find(&actor);if(elite!=elites.end() && elite->second.guid!=actor.guid_F8) elites.erase(elite);
    }
    for(auto it=owned.begin();it!=owned.end();) if(!present.count(it->first)) it=owned.erase(it);else ++it;
    for(auto it=elites.begin();it!=elites.end();) if(!present.count(it->first)) it=elites.erase(it);else ++it;
    Publish();
}
float DirectorRunFactor() {return 1;}
float DirectorFireFactor() {return re4t::cfg->bDirectorTimed && timedEvent==4 ? 20.0f : 1.0f;}
float DirectorEnemyFactor() {return re4t::cfg->bDirectorTimed ? timedEvent==1 ? 3.0f : timedEvent==2 ? .35f : 1.0f : 1.0f;}
int DirectorFlightMode(int configured) {return re4t::cfg->bDirectorTimed && (timedEvent==0 || timedEvent==3) ? timedEvent==3 ? 2 : 0 : configured;}
bool DirectorForceLaunch() {return re4t::cfg->bDirectorTimed && (timedEvent==0 || timedEvent==3);}
std::string DirectorOverlay()
{
    auto snapshot=DirectorSnapshot();std::string result;
    if(!snapshot.timedEvent.empty()) result="Event: "+snapshot.timedEvent;
    if(snapshot.waves) result+=(result.empty()?"":" | ")+std::string("Wave ")+std::to_string(snapshot.wave)+" | Added alive "+std::to_string(snapshot.alive);
    if(!snapshot.target.empty()) result+=(result.empty()?"":"\n")+snapshot.target;
    return result;
}
}
