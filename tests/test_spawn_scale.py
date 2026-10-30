"""Exercise the production scale hook with a small native-object test harness.
This verifies control flow, not the real game's memory layout or animations.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "dllmain/ModExpansion.cpp").read_text()
function = source.split("void SetEmListParamExtensions(", 1)[1].split("\ncEm* (__cdecl* EmSetEvent)", 1)[0]
function = "void SetEmListParamExtensions(" + function
harness = r'''
#include "dllmain/ChaosMath.h"
#include <cassert>
#include <iostream>
using std::uint8_t;
constexpr unsigned EM_BE_FLAG_MODEXP_SPEEDSCALE = 0x40;
struct Vec { float x=1, y=1, z=1; };
struct EM_LIST {
    unsigned be_flag_0=0, room_18=0x101;
    uint8_t id_1=0x15;
    unsigned percentageMotionSpeed_1C=0, percentageScale_1E=0;
};
struct cEm {
    enum class Routine0 : uint8_t { Init, Move, Damage, Die };
    bool valid=true, dieOnInit=false;
    int moves=0, hp_324=1200;
    uint8_t r_no_0_FC=0, emListIndex_3A0=0, id_100=0x15;
    Vec scale_AC;
    struct { float Seq_speed_C0=1; } Motion_1D8;
    bool IsValid() { return valid; }
    bool IsSpawnedByESL() { return valid && emListIndex_3A0!=255; }
    void move() { ++moves; r_no_0_FC=dieOnInit?3:1; if(dieOnInit) hp_324=0; }
};
struct cEm10 : cEm { Vec Scale_498; };
struct GLOBAL_WK { EM_LIST Em_list_5410[256]; } globals;
GLOBAL_WK* GlobalPtr() { return &globals; }
struct Config { bool bChaosEnabled=true; bool bTrainerVillageSizeChaos=true; float fTrainerVillageSizeMin=1.5f, fTrainerVillageSizeMax=1.5f; } config;
namespace re4t { Config* cfg=&config; }
namespace aiza { void ChaosOnSpawn(cEm*) {} }
bool IsGanado(int id) { return id==0x15; }
float GetRandomFloat(float low, float high) { assert(low<=high); return low; }
namespace spd { struct Logger { template<class... T> void info(const char*,T...) {} }; Logger logger; Logger* log() { return &logger; } }
'''
cases = r'''
int main() {
    auto& entry=globals.Em_list_5410[0];
    cEm10 enemy;
    SetEmListParamExtensions(nullptr, &entry);
    SetEmListParamExtensions(&enemy, nullptr);
    EM_LIST stackEntry;
    SetEmListParamExtensions(&enemy, &stackEntry);
    assert(enemy.moves==0 && enemy.scale_AC.x==1);
    SetEmListParamExtensions(&enemy, &entry);
    assert(enemy.moves==1 && enemy.scale_AC.x==1.5f && enemy.Scale_498.x==1.5f);
    SetEmListParamExtensions(&enemy, &entry);
    assert(enemy.moves==1 && enemy.scale_AC.x==1.5f);
    config.bChaosEnabled=false;
    cEm10 masterOff;
    SetEmListParamExtensions(&masterOff, &entry);
    assert(masterOff.moves==0);
    config.bChaosEnabled=true;
    config.bTrainerVillageSizeChaos=false;
    cEm10 disabled;
    SetEmListParamExtensions(&disabled, &entry);
    assert(disabled.moves==0 && disabled.scale_AC.x==1);
    config.bTrainerVillageSizeChaos=true;
    entry.room_18=0x100;
    cEm10 otherRoom;
    SetEmListParamExtensions(&otherRoom, &entry);
    assert(otherRoom.moves==0);
    entry.room_18=0x101;
    cEm10 merchant; merchant.id_100=0x18;
    SetEmListParamExtensions(&merchant, &entry);
    assert(merchant.moves==0);
    cEm10 event; event.emListIndex_3A0=255;
    SetEmListParamExtensions(&event, &entry);
    assert(event.moves==0);
    cEm10 mismatched; mismatched.emListIndex_3A0=1;
    SetEmListParamExtensions(&mismatched, &entry);
    assert(mismatched.moves==0);
    cEm10 dead; dead.hp_324=0;
    SetEmListParamExtensions(&dead, &entry);
    assert(dead.moves==0);
    cEm10 diesDuringInit; diesDuringInit.dieOnInit=true;
    SetEmListParamExtensions(&diesDuringInit, &entry);
    assert(diesDuringInit.moves==1 && diesDuringInit.scale_AC.x==1);
    entry.be_flag_0=EM_BE_FLAG_MODEXP_SPEEDSCALE;
    entry.percentageScale_1E=125;
    entry.percentageMotionSpeed_1C=200;
    cEm10 authored;
    SetEmListParamExtensions(&authored, &entry);
    assert(authored.scale_AC.x==1.25f && authored.Motion_1D8.Seq_speed_C0==2);
    entry.percentageScale_1E=entry.percentageMotionSpeed_1C=0;
    cEm10 authoredEmpty;
    SetEmListParamExtensions(&authoredEmpty, &entry);
    assert(authoredEmpty.moves==0);
    std::cout << "Production spawn-scale control flow: PASS\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/"spawn_test.cpp"
    binary=Path(tmp)/"spawn_test"
    cpp.write_text(harness+function+cases)
    subprocess.run(["g++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-fsanitize=undefined", "-I", str(root), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
