#include "dllmain.h"
#include "Game.h"
#include "Settings.h"
#include "Chaos.h"
#include "ChaosCore.h"
#include "Director.h"
#include "DirectorCore.h"
#include "Utils.h"
#include "imgui.h"
#include <atomic>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>

namespace aiza
{
namespace
{
struct Rate
{
    float original=1, applied=1;
    bool changed=false;
    void Apply(cEm* actor, float factor)
    {
        original=actor->Motion_1D8.Seq_speed_C0;
        if (!std::isfinite(original) || original<=0) return;
        applied=Bound(original*factor, 0.05f, 40, original);
        actor->Motion_1D8.Seq_speed_C0=applied;
        changed=true;
    }
    void Restore(cEm* actor)
    {
        // Keep intentional native rate changes, e.g. weapon/animation switches.
        if (changed && actor->Motion_1D8.Seq_speed_C0==applied)
            actor->Motion_1D8.Seq_speed_C0=original;
        changed=false;
    }
};
struct Enemy
{
    unsigned id=0, index=255;
    uint32_t guid=0;
    int hpBefore=0;
    float cooldown=0;
    Point before;
    Rate rate;
    Flight flight;
};
struct Scene
{
    GLOBAL_WK* globals=nullptr;
    cPlayer* player=nullptr;
    cEmMgr* manager=nullptr;
    void* array=nullptr;
    void* room=nullptr;
    bool operator==(const Scene& other) const
    {
        return globals==other.globals && player==other.player && manager==other.manager &&
            array==other.array && room==other.room;
    }
};
Scene scene;
std::unordered_map<cEm*, Enemy> enemies;
Rate playerRate;
std::atomic<bool> pulseRequest{false}, bowlingRequest{false};
std::atomic<unsigned> flyingCount{0};
bool wantsFire=false, pulseThisFrame=false, activeFrame=false, ammoInjected=false;
int weaponNo=-1;
float deltaSeconds=0;
ULONGLONG previousTick=0, lastFireTick=0;
bool f6Down=false, f7Down=false, f9Down=false;
bool bowlingThisFrame=false;

Point Read(cEm* actor) { return {actor->pos_94.x, actor->pos_94.y, actor->pos_94.z}; }
void Place(cEm* actor, Point position)
{
    if (!Finite(position)) return;
    actor->pos_94=Vec(position.x, position.y, position.z);
    actor->pos_old_110=actor->pos_94;
    actor->matUpdate();
}
Scene CurrentScene()
{
    Scene result;
    result.globals=GlobalPtr(); result.player=PlayerPtr(); result.manager=EmMgrPtr();
    if (result.manager) result.array=result.manager->m_Array_4;
    if (result.globals) result.room=result.globals->pRoom_40;
    return result;
}
bool Supported(cEm& em)
{
    return (em.IsSpawnedByESL() || DirectorOwns(&em)) && IsChaosTarget(em.id_100, re4t::cfg->bChaosIncludeHeavy) &&
        em.r_no_0_FC!=uint8_t(cEm::Routine0::Init) && Finite(Read(&em));
}
bool Blocked(const Scene& s)
{
    if (!s.globals || !s.player || !s.manager || !s.array || !s.player->IsValid() || s.player->hp_324<=0)
        return true;
    auto* g=s.globals;
    return FlagIsSet(g->flags_STATUS_0_501C, uint32_t(Flags_STATUS::STA_EVENT)) ||
        FlagIsSet(g->flags_STATUS_0_501C, uint32_t(Flags_STATUS::STA_MOVIE_ON)) ||
        FlagIsSet(g->flags_STATUS_0_501C, uint32_t(Flags_STATUS::STA_MOVIE2_ON)) ||
        FlagIsSet(g->flags_STATUS_0_501C, uint32_t(Flags_STATUS::STA_SUB_SCRN)) ||
        FlagIsSet(g->flags_STATUS_0_501C, uint32_t(Flags_STATUS::STA_DIEDEMO)) ||
        FlagIsSet(g->flags_STATUS_0_501C, uint32_t(Flags_STATUS::STA_PL_BOAT)) ||
        FlagIsSet(g->flags_STATUS_0_501C, uint32_t(Flags_STATUS::STA_PL_LADDER)) ||
        FlagIsSet(g->flags_STOP_0_170, uint32_t(Flags_STOP::SPF_PL)) ||
        FlagIsSet(g->flags_STOP_0_170, uint32_t(Flags_STOP::SPF_EM)) ||
        FlagIsSet(g->Flags_SYSTEM_0_54, uint32_t(Flags_SYSTEM::SYS_ROOMJUMP));
}
void CancelCurrentFlights(const Scene& current)
{
    // Enumerate the *current* manager before touching any previously held pointer.
    if (current.manager && current==scene)
        for (auto& em : *current.manager)
        {
            auto it=enemies.find(&em);
            if (it!=enemies.end() && em.IsValid() && it->second.id==em.id_100 &&
                it->second.index==em.emListIndex_3A0 && it->second.guid==em.guid_F8 && it->second.flight.active)
                Place(&em, it->second.flight.Cancel());
        }
    enemies.clear(); flyingCount=0;
}
void Hotkeys()
{
    const bool f6=(GetAsyncKeyState(VK_F6)&0x8000)!=0;
    const bool f7=(GetAsyncKeyState(VK_F7)&0x8000)!=0;
    const bool f9=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
    DWORD pid=0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    if (pid==GetCurrentProcessId())
    {
        if (f6 && !f6Down)
        {
            re4t::cfg->bChaosEnabled=!re4t::cfg->bChaosEnabled;
            re4t::cfg->HasUnsavedChanges=true;
            spd::log()->info("[Aiza Chaos] F6 master {}", re4t::cfg->bChaosEnabled ? "ON" : "OFF");
        }
        if (f7 && !f7Down) pulseRequest=true;
        if (f9 && !f9Down) bowlingRequest=true;
    }
    f6Down=f6; f7Down=f7; f9Down=f9;
}
unsigned ActiveFlights()
{
    unsigned count=0;
    for (const auto& entry : enemies) count+=entry.second.flight.active ? 1u : 0u;
    return count;
}
void Launch(cEm* actor, Enemy& state, Point player, const char* reason, int mode=-1)
{
    if (state.flight.active || state.cooldown>0 || ActiveFlights()>=16) return;
    state.flight.Start(Read(actor), player, mode<0 ? DirectorFlightMode(re4t::cfg->iChaosLaunchMode) : mode,
        re4t::cfg->fChaosLaunchHeight, re4t::cfg->fChaosFlightDuration, re4t::cfg->fChaosLaunchSpread);
    state.cooldown=Bound(re4t::cfg->fChaosLaunchCooldown, 0.2f, 5, 0.7f);
    spd::log()->info("[Aiza Chaos] Launch {} ESL {} id {:02X} mode {}",
        reason, state.index, state.id, re4t::cfg->iChaosLaunchMode);
}
}

void ChaosOnSpawn(cEm* enemy)
{
    // Existing native spawn hooks tell us when a manager slot is reused.
    // Never restore old motion/flight state to a newly spawned object.
    if (enemy && enemy->r_no_0_FC==uint8_t(cEm::Routine0::Init))
        enemies.erase(enemy);
}
void ChaosForgetActor(cEm* enemy) { enemies.erase(enemy); }
bool ChaosActorFlying(cEm* enemy) {auto it=enemies.find(enemy);return it!=enemies.end() && it->second.guid==enemy->guid_F8 && it->second.flight.active;}
void ChaosRequestPulse() { pulseRequest=true; }
void ChaosRequestBowling() { bowlingRequest=true; }
bool ChaosWantsFireTrigger() { return wantsFire; }

void ChaosBeforeUpdate()
{
    wantsFire=false; activeFrame=false; weaponNo=-1; pulseThisFrame=false;
    if (!re4t::cfg) return;
    Hotkeys();
    DirectorBeforeUpdate();
    const auto now=GetTickCount64();
    deltaSeconds=previousTick ? Bound(float(now-previousTick)/1000, 0, 0.1f, 0) : 0;
    previousTick=now;
    const Scene current=CurrentScene();
    if (!(current==scene))
    {
        enemies.clear(); playerRate={}; ammoInjected=false; lastFireTick=0;
        flyingCount=0; scene=current;
    }
    if (!re4t::cfg->bChaosEnabled || Blocked(current))
    {
        CancelCurrentFlights(current);
        pulseRequest=false; bowlingRequest=false;
        return;
    }
    activeFrame=true;
    pulseThisFrame=pulseRequest.exchange(false);
    bowlingThisFrame=bowlingRequest.exchange(false) && re4t::cfg->bChaosBowling;
    auto* player=current.player;
    const uint64_t keys=Key_btn_on();
    const bool aiming=(keys&uint64_t(KEY_BTN::KEY_KAMAE))!=0;
    const bool firing=aiming && bio4::joyFireOn && bio4::joyFireOn();
    unsigned item=0;
    if (player->Wep_7D8 && player->Wep_7D8->m_pWep_34)
        item=unsigned(player->Wep_7D8->m_pWep_34->itemId_350);
    if (firing && IsChaosGun(item) && bio4::WeaponId2WeaponNo)
    {
        lastFireTick=now;
        weaponNo=int(bio4::WeaponId2WeaponNo(ITEM_ID(item)));
    }
    else if (now-lastFireTick<350 && IsChaosGun(item) && bio4::WeaponId2WeaponNo)
        weaponNo=int(bio4::WeaponId2WeaponNo(ITEM_ID(item)));

    const uint64_t movement=uint64_t(KEY_BTN::KEY_FORWARD)|uint64_t(KEY_BTN::KEY_BACK)|
        uint64_t(KEY_BTN::KEY_LEFT)|uint64_t(KEY_BTN::KEY_RIGHT);
    const bool running=(keys&movement) && (keys&uint64_t(KEY_BTN::KEY_RUN)) &&
        !aiming && !(keys&uint64_t(KEY_BTN::KEY_KAMAE_KNIFE));
    float speed=1;
    if (re4t::cfg->bChaosFastRun && running)
        speed=Bound(re4t::cfg->fChaosRunSpeed, 1, 15, 5);
    if ((re4t::cfg->bChaosFastFire || DirectorFireFactor()>1) && firing && IsChaosGun(item))
    {
        speed=Bound((re4t::cfg->bChaosFastFire ? re4t::cfg->fChaosFireSpeed : 1)*DirectorFireFactor(), 1, 30, 12);
        player->Wep_7D8->m_ShotTimer_23=0;
        wantsFire=true;
    }
    playerRate={};
    if (speed!=1) playerRate.Apply(player, speed);
    ammoInjected=re4t::cfg->bChaosInfiniteAmmo &&
        !FlagIsSet(current.globals->flags_DEBUG_0_60, uint32_t(Flags_DEBUG::DBG_INF_BULLET));
    if (ammoInjected) FlagSet(current.globals->flags_DEBUG_0_60, uint32_t(Flags_DEBUG::DBG_INF_BULLET), true);

    for (auto& em : *current.manager)
    {
        if (!em.IsValid()) continue;
        const float directorSpeed=DirectorEliteSpeed(&em);
        if (!Supported(em) && !DirectorOwns(&em) && directorSpeed==1) continue;
        auto& state=enemies[&em];
        if (state.id!=em.id_100 || state.index!=em.emListIndex_3A0 || state.guid!=em.guid_F8) state={};
        state.id=em.id_100; state.index=em.emListIndex_3A0; state.guid=em.guid_F8;
        state.hpBefore=em.hp_324; state.before=Read(&em);
        state.cooldown=std::max(0.0f, state.cooldown-deltaSeconds);
        state.rate={};
        const float enemyFactor=(re4t::cfg->bChaosEnemySpeed ? re4t::cfg->fChaosEnemySpeed : 1)*DirectorEnemyFactor()*directorSpeed;
        if (em.hp_324>0 && enemyFactor!=1)
            state.rate.Apply(&em, Bound(enemyFactor, 0.2f, 10, 2));
    }
}

void ChaosAfterUpdate()
{
    if (!activeFrame) return;
    activeFrame=false;
    const Scene current=CurrentScene();
    if (!(current==scene))
    {
        enemies.clear(); playerRate={}; flyingCount=0;
        // Global work may survive a room change, so restore only our own flag.
        if (ammoInjected && current.globals==scene.globals && current.globals)
            FlagSet(current.globals->flags_DEBUG_0_60, uint32_t(Flags_DEBUG::DBG_INF_BULLET), false);
        ammoInjected=false;
        return;
    }
    playerRate.Restore(current.player);
    if (ammoInjected) FlagSet(current.globals->flags_DEBUG_0_60, uint32_t(Flags_DEBUG::DBG_INF_BULLET), false);
    ammoInjected=false;
    std::unordered_set<cEm*> present;
    const bool blocked=Blocked(current) || !re4t::cfg->bChaosEnabled;
    const Point player=Read(current.player);
    const float radius=Bound(re4t::cfg->fChaosPulseRadius, 200, 6000, 2500);
    bool meleeHit=false;
    for (auto& em : *current.manager)
    {
        auto it=enemies.find(&em);
        if (it==enemies.end() || !em.IsValid()) continue;
        auto& state=it->second;
        if (state.id!=em.id_100 || state.index!=em.emListIndex_3A0 || state.guid!=em.guid_F8) continue;
        present.insert(&em);
        state.rate.Restore(&em);
        if (blocked || !Supported(em))
        {
            if (state.flight.active) Place(&em, state.flight.Cancel());
            continue;
        }
        // Do not overwrite a native teleport or event transition within this frame.
        if (Distance2(Read(&em), state.before)>8000.0f*8000.0f)
        {
            state.flight.active=false;
            continue;
        }
        const auto& damage=em.m_DmgInfo_328;
        const bool shotHit=(re4t::cfg->bChaosLaunchShots || DirectorForceLaunch()) && state.hpBefore>0 &&
            em.hp_324<state.hpBefore && weaponNo>=0 && int(damage.m_Wep_6)==weaponNo &&
            (damage.m_Flag_4!=0 || damage.m_Timer_5!=0);
        if(re4t::cfg->bChaosAutoBowling && state.hpBefore>0 && em.hp_324<state.hpBefore &&
            weaponNo<0 && FlagIsSet(current.globals->flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_PL_ACTION)) &&
            FrontCone(player,Read(&em),current.player->ang_A0.y,900)) meleeHit=true;
        if (shotHit) Launch(&em, state, player, "shot");
        if (pulseThisFrame && em.hp_324>0 && Distance2(Read(&em), player)<=radius*radius)
            Launch(&em, state, player, "F7 pulse");
        if (state.flight.active) Place(&em, state.flight.Step(deltaSeconds));
    }
    if(!blocked && re4t::cfg->bChaosBowling && (bowlingThisFrame || meleeHit))
        for(auto& em:*current.manager) {
            auto it=enemies.find(&em);
            if(it!=enemies.end() && present.count(&em) && em.IsValid() && em.hp_324>0 && Supported(em) &&
                FrontCone(player,Read(&em),current.player->ang_A0.y,Bound(re4t::cfg->fChaosBowlingRadius,200,2500,1200)))
                Launch(&em,it->second,player,meleeHit ? "melee bowling" : "F9 bowling",1);
        }
    for (auto it=enemies.begin(); it!=enemies.end(); )
        if (!present.count(it->first)) it=enemies.erase(it); else ++it;
    flyingCount=ActiveFlights();
    if (pulseThisFrame)
        spd::log()->info("[Aiza Chaos] F7 pulse processed; {} flights active", flyingCount.load());
}

void ChaosDrawOverlay()
{
    if (!re4t::cfg || !re4t::cfg->bChaosShowHUD) return;
    char text[256];
    std::snprintf(text, sizeof(text), "AIZA CHAOS %s | F6 ON/OFF | F7 Shockwave | F9 Bowling\nRun %.1fx | Fire %.1fx | Enemy %.1fx | Flying %u/16",
        re4t::cfg->bChaosEnabled ? "ON" : "OFF",
        re4t::cfg->bChaosEnabled && re4t::cfg->bChaosFastRun ? re4t::cfg->fChaosRunSpeed : 1,
        re4t::cfg->bChaosEnabled && re4t::cfg->bChaosFastFire ? re4t::cfg->fChaosFireSpeed : 1,
        re4t::cfg->bChaosEnabled && re4t::cfg->bChaosEnemySpeed ? re4t::cfg->fChaosEnemySpeed : 1,
        flyingCount.load());
    auto* draw=ImGui::GetForegroundDrawList();
    const std::string extra=DirectorOverlay();
    const std::string full=std::string(text)+(extra.empty() ? "" : "\n"+extra);
    const ImVec2 size=ImGui::CalcTextSize(full.c_str());
    draw->AddRectFilled(ImVec2(14,62), ImVec2(26+size.x,76+size.y), IM_COL32(0,0,0,175), 5);
    draw->AddText(ImVec2(20,68), IM_COL32(255,220,80,255), full.c_str());
}
}
