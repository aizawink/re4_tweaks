#pragma once
#include "dllmain/ChaosCore.h"
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cassert>
#include <iostream>
#include <functional>
using DWORD=unsigned long;
using ULONGLONG=unsigned long long;
using uint8_t=std::uint8_t;
using uint32_t=std::uint32_t;
using uint64_t=std::uint64_t;
using ITEM_ID=unsigned;
constexpr int VK_F6=117, VK_F7=118;
ULONGLONG clockTick=1000;
short keyStates[256]={};
DWORD foreground=1;
ULONGLONG GetTickCount64() { return clockTick; }
short GetAsyncKeyState(int key) { return keyStates[key]; }
void* GetForegroundWindow() { return nullptr; }
void GetWindowThreadProcessId(void*,DWORD* pid) { *pid=foreground; }
DWORD GetCurrentProcessId() { return 1; }
struct Vec { float x=0,y=0,z=0; Vec()=default; Vec(float a,float b,float c):x(a),y(b),z(c) {} };
struct cDmgInfo { uint8_t m_Wep_6=7, m_Flag_4=0, m_Timer_5=0; };
class cEm {
public:
    enum class Routine0:uint8_t { Init,Move,Damage,Die };
    bool valid=true;
    uint8_t r_no_0_FC=1,id_100=0x15,emListIndex_3A0=0;
    int hp_324=1200;
    struct { float Seq_speed_C0=1; } Motion_1D8;
    Vec pos_94, pos_old_110;
    cDmgInfo m_DmgInfo_328;
    bool IsValid() { return valid; }
    bool IsSpawnedByESL() { return valid && emListIndex_3A0!=255; }
    void matUpdate() {}
};
struct cObjWep { unsigned itemId_350=35; } gun;
struct cPlWep { cObjWep* m_pWep_34=&gun; uint8_t m_ShotTimer_23=15; } plWep;
class cPlayer:public cEm { public:cPlWep* Wep_7D8=&plWep; } player;
struct cEmMgr {
    std::vector<cEm*> items;
    void* m_Array_4=reinterpret_cast<void*>(1);
    struct Iterator {
        std::vector<cEm*>::iterator it;
        cEm& operator*() { return **it; }
        Iterator& operator++() { ++it;return *this; }
        bool operator!=(const Iterator& other) { return it!=other.it; }
    };
    Iterator begin() { return {items.begin()}; }
    Iterator end() { return {items.end()}; }
} manager;
enum class Flags_STATUS {STA_EVENT,STA_MOVIE_ON,STA_MOVIE2_ON,STA_SUB_SCRN,STA_DIEDEMO,STA_PL_BOAT,STA_PL_LADDER};
enum class Flags_STOP { SPF_PL,SPF_EM };
enum class Flags_SYSTEM { SYS_ROOMJUMP };
enum class Flags_DEBUG { DBG_INF_BULLET };
enum class KEY_BTN:uint64_t {KEY_FORWARD=1,KEY_BACK=2,KEY_LEFT=4,KEY_RIGHT=8,KEY_KAMAE=16,KEY_RUN=64,KEY_FIRE=128,KEY_KAMAE_KNIFE=2048};
struct GLOBAL_WK {
    void* pRoom_40=reinterpret_cast<void*>(2);
    uint32_t flags_STATUS_0_501C[1]={}, flags_STOP_0_170[1]={}, Flags_SYSTEM_0_54[1]={}, flags_DEBUG_0_60[1]={};
} globals;
bool globalsAvailable=true;
GLOBAL_WK* GlobalPtr() { return globalsAvailable ? &globals : nullptr; }
cPlayer* PlayerPtr() { return &player; }
cEmMgr* EmMgrPtr() { return &manager; }
bool FlagIsSet(uint32_t* flags, uint32_t bit) { return (*flags & (1u<<bit))!=0; }
void FlagSet(uint32_t* flags,uint32_t bit,bool on) { if(on) *flags|=1u<<bit;else *flags&=~(1u<<bit); }
uint64_t heldKeys=0,triggerKeys=0;
uint64_t Key_btn_on() { return heldKeys; }
uint32_t* ptrKey_btn_trg=reinterpret_cast<uint32_t*>(&triggerKeys);
namespace bio4 {
    bool fireHeld=false;
    bool Fire() { return fireHeld; }
    bool (*joyFireOn)()=&Fire;
    uint8_t Number(ITEM_ID) { return 7; }
    uint8_t (*WeaponId2WeaponNo)(ITEM_ID)=&Number;
}
namespace spd { struct Logger { template<class... T> void info(const char*,T...) {} }; Logger logger; Logger* log() {return &logger;} }
struct ImVec2 { float x,y; ImVec2(float a,float b):x(a),y(b){} };
#define IM_COL32(...) 0u
namespace ImGui {
    struct Draw { void AddRectFilled(ImVec2,ImVec2,unsigned,float){} void AddText(ImVec2,unsigned,const char*){} } draw;
    Draw* GetForegroundDrawList() {return &draw;}
    ImVec2 CalcTextSize(const char*) {return {400,40};}
}
