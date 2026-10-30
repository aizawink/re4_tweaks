"""Run production Director code with a controlled object pool/native creator.
The harness verifies scheduling and ownership, not the native game's ABI/assets.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'dllmain/Director.cpp').read_text()
source = 'namespace aiza' + source.split('namespace aiza', 1)[1]
settings = (root / 'dllmain/Settings.h').read_text()
fields = settings.split('// Aiza Chaos runtime; everything defaults to OFF.', 1)[1].split('// TRAINER_HOTKEYS', 1)[0]
config = 'struct Config {' + fields + ' bool HasUnsavedChanges=false; } config;\nnamespace re4t { Config* cfg=&config; }\n'
native = r'''
std::array<cEm10,80> pool;
unsigned nativeCalls=0,nextGuid=1000,forgotten=0;
bool nativeFail=false;
namespace aiza {
void ChaosForgetActor(cEm*) {++forgotten;}
bool DirectorNativeSpawnAvailable() {return true;}
cEm* DirectorNativeSpawn(EM_LIST* data) {
    ++nativeCalls;if(nativeFail) return nullptr;
    for(auto& actor:pool) if(!actor.valid) {
        actor=cEm10();actor.guid_F8=nextGuid++;
        actor.id_100=uint8_t(data->id_1);actor.type_101=uint8_t(data->type_2);
        actor.flag_3D0=data->flag_4;actor.characterNum_3D8=uint8_t(data->Character_B);
        actor.hp_324=actor.hp_max_326=data->hp_8;actor.emListIndex_3A0=255;
        actor.pos_94=Vec(float(data->s_pos_C.x)*10,float(data->s_pos_C.y)*10,float(data->s_pos_C.z)*10);
        return &actor;
    }return nullptr;
}
}
'''
cases = r'''
void Tick(unsigned ms=250) {
    clockTick+=ms;aiza::DirectorBeforeUpdate();aiza::DirectorAfterUpdate();
}
void Command(aiza::DirectorCommand command,uint64_t key=0,int count=1) {
    aiza::DirectorRequest(command,aiza::DirectorSnapshot().epoch,key,count);Tick();
}
void NativeWitness(unsigned slot,unsigned index,uint8_t id,uint8_t type,uint32_t flags) {
    auto& actor=pool[slot];actor=cEm10();actor.guid_F8=nextGuid++;
    actor.emListIndex_3A0=uint8_t(index);actor.id_100=id;actor.type_101=type;actor.flag_3D0=flags;
    actor.hp_324=actor.hp_max_326=5000;actor.r_no_0_FC=0;
    auto& data=globals.Em_list_5410[index];data=EM_LIST();data.be_flag_0=EM_BE_FLAG_ALIVE|EM_BE_FLAG_SET;
    data.id_1=char(id);data.type_2=char(type);data.flag_4=flags;data.hp_8=5000;
    data.set_3=type==4 ? 0x22 : 0x0e;
    aiza::DirectorObserveInit(&actor,&data);
    actor.r_no_0_FC=1;aiza::DirectorObserveReady(&actor,&data);
}
unsigned OwnedAlive() {
    unsigned count=0;for(auto& actor:pool) if(actor.valid && actor.hp_324>0 && aiza::DirectorOwns(&actor)) ++count;
    return count;
}
int main() {
    for(auto& actor:pool) {actor.valid=false;manager.items.push_back(&actor);}
    manager.m_Array_4=pool.data();manager.m_nArray_8=unsigned(pool.size());
    config.bChaosEnabled=true;
    NativeWitness(0,60,0x15,4,0x10000020u);
    NativeWitness(1,61,0x15,0,0x05000020u);Tick();
    auto state=aiza::DirectorSnapshot();assert(state.coordinatesVerified && state.templates.size()==2 && !state.marked);
    const auto saw=aiza::TemplateKey(0x15,4,2,0x10000020u);
    config.uDirectorSelectedTemplate=saw;
    // No marked point means no native call; a bad pointer cannot be dereferenced.
    Command(aiza::DirectorCommand::Spawn,saw,3);assert(nativeCalls==0);
    assert(!aiza::DirectorLiveActor(reinterpret_cast<cEm*>(1)));
    assert(!aiza::DirectorLiveActor(reinterpret_cast<cEm*>(reinterpret_cast<uintptr_t>(pool.data())+1)));
    player.pos_94=Vec(100,20,300);Command(aiza::DirectorCommand::MarkPoint);
    config.iDirectorSpawnHP=9000;config.fDirectorSpawnScale=2;config.fDirectorSpawnSpeed=3;
    Command(aiza::DirectorCommand::Spawn,saw,3);Tick();Tick();
    state=aiza::DirectorSnapshot();assert(state.owned==3 && state.alive==3 && state.queued==0);
    for(auto& actor:pool) if(aiza::DirectorOwns(&actor)) {
        assert(actor.hp_324==9000 && actor.scale_AC.x==2 && actor.Scale_498.x==2);
        assert(actor.pos_94.x==100 && actor.pos_94.y==20 && actor.pos_94.z==300);
        assert(aiza::DirectorEliteSpeed(&actor)==3 && actor.emListIndex_3A0==255);
    }
    assert(pool[0].hp_324==5000 && pool[1].scale_AC.x==1);
    // Slot reuse does not inherit ownership. Attached actors are not destroyed.
    pool[2].guid_F8=nextGuid++;pool[3].pEmCatch_3C0=&player;Tick();
    assert(!aiza::DirectorOwns(&pool[2]));
    const auto destroyed=manager.destroyed;Command(aiza::DirectorCommand::ClearOwned);
    assert(manager.destroyed==destroyed+1 && pool[0].valid && pool[1].valid && pool[2].valid && pool[3].valid);
    pool[3].pEmCatch_3C0=nullptr;Command(aiza::DirectorCommand::ClearOwned);
    assert(aiza::DirectorSnapshot().owned==0);
    // Native failure cancels remaining jobs; no repeated automatic attempts.
    nativeFail=true;auto calls=nativeCalls;Command(aiza::DirectorCommand::Spawn,saw,5);
    assert(nativeCalls==calls+1 && aiza::DirectorSnapshot().queued==0);
    Tick();assert(nativeCalls==calls+1);nativeFail=false;
    // Pending replacement preserves original event routine, sight byte and position.
    auto& pending=globals.Em_list_5410[90];pending=EM_LIST();pending.be_flag_0=EM_BE_FLAG_ALIVE;
    pending.id_1=0x15;pending.type_2=0;pending.set_3=0x0e;pending.flag_4=0x05000037;
    pending.s_pos_C={12,34,56};config.bDirectorReplace=true;config.iDirectorReplaceMode=2;
    aiza::DirectorPreparePending();assert(pending.type_2==4 && pending.flag_4==0x10000037);
    assert(pending.set_3==0x0e && pending.s_pos_C.x==12 && pending.be_flag_0==EM_BE_FLAG_ALIVE);
    auto* replacement=aiza::DirectorNativeSpawn(&pending);assert(replacement);
    replacement->emListIndex_3A0=90;pending.be_flag_0|=EM_BE_FLAG_SET;
    aiza::DirectorObserveReady(replacement,&pending);aiza::DirectorObserveReady(replacement,&pending);
    aiza::DirectorRestorePending();aiza::DirectorAfterUpdate();
    assert(pending.type_2==0 && pending.flag_4==0x05000037 && (pending.be_flag_0&EM_BE_FLAG_SET));
    assert(aiza::DirectorSnapshot().replaced==1 && !aiza::DirectorOwns(replacement));
    config.bDirectorReplace=false;
    // A cached descriptor alone cannot prove its model is still present.
    for(auto& actor:pool) actor.pModelInfo_15C=nullptr;
    calls=nativeCalls;Command(aiza::DirectorCommand::Spawn,saw);assert(nativeCalls==calls);
    for(auto& actor:pool) actor.pModelInfo_15C=reinterpret_cast<void*>(1);
    // Only added actors count toward wave completion; a live native actor remains.
    config.iDirectorWaveStart=3;config.iDirectorWaveGrowth=2;config.iDirectorWaveLimit=2;
    config.fDirectorWaveDelay=0;config.fDirectorSpawnScale=1;config.fDirectorSpawnSpeed=1;
    config.fDirectorWaveHPGrowth=.5f;Command(aiza::DirectorCommand::StartWaves,saw);Tick();Tick();
    assert(OwnedAlive()==3 && aiza::DirectorSnapshot().wave==1);
    for(auto& actor:pool) if(aiza::DirectorOwns(&actor)) actor.hp_324=0;
    Tick();Tick();for(int i=0;i<5;++i) Tick();
    assert(OwnedAlive()==5 && aiza::DirectorSnapshot().wave==2 && pool[0].hp_324==5000);
    for(auto& actor:pool) if(aiza::DirectorOwns(&actor) && actor.hp_324>0) assert(actor.hp_324==13500);
    for(auto& actor:pool) if(aiza::DirectorOwns(&actor)) actor.hp_324=0;
    Tick();assert(!aiza::DirectorSnapshot().waves);Command(aiza::DirectorCommand::ClearOwned);
    // Failed creation stops a wave, and master OFF cancels all queued jobs.
    nativeFail=true;Command(aiza::DirectorCommand::StartWaves,saw);
    assert(!aiza::DirectorSnapshot().waves && aiza::DirectorSnapshot().queued==0);nativeFail=false;
    Command(aiza::DirectorCommand::Spawn,saw,5);config.bChaosEnabled=false;calls=nativeCalls;
    Tick();assert(nativeCalls==calls && aiza::DirectorSnapshot().queued==0);
    Command(aiza::DirectorCommand::ClearOwned);config.bChaosEnabled=true;
    // Elite scaling is once per GUID, and speed obeys its toggle.
    config.bDirectorElites=true;config.fDirectorEliteChance=100;config.fDirectorEliteHP=3;
    config.fDirectorEliteScale=2;Tick();assert(pool[0].hp_324==15000 && pool[0].scale_AC.x==2);
    Tick();assert(pool[0].hp_324==15000 && pool[0].scale_AC.x==2);
    config.bDirectorElites=false;assert(aiza::DirectorEliteSpeed(&pool[0])==1);
    // Temporary events leave saved sliders untouched and pause with gameplay.
    config.bDirectorTimed=true;config.fChaosFireSpeed=7;Tick();
    assert(!aiza::DirectorSnapshot().timedEvent.empty() && config.fChaosFireSpeed==7);
    auto event=aiza::DirectorSnapshot().timedEvent;
    FlagSet(globals.flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_PL_LADDER),true);
    calls=nativeCalls;Command(aiza::DirectorCommand::Spawn,saw);assert(nativeCalls==calls);
    Tick(10000);assert(aiza::DirectorSnapshot().timedEvent==event);
    FlagSet(globals.flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_PL_LADDER),false);
    // A queued old-room request is rejected, and old ownership is discarded.
    const auto oldEpoch=aiza::DirectorSnapshot().epoch;
    globals.pRoom_40=reinterpret_cast<void*>(3);roomData.m_RoomNo_E=0x102;
    Tick();assert(aiza::DirectorSnapshot().epoch!=oldEpoch && !aiza::DirectorSnapshot().marked);
    assert(aiza::DirectorSnapshot().owned==0 && aiza::DirectorSnapshot().templates.empty());
    calls=nativeCalls;aiza::DirectorRequest(aiza::DirectorCommand::Spawn,oldEpoch,saw,3);Tick();assert(nativeCalls==calls);
    std::cout << "Production Director spawn, waves, replacement, ownership, failure and room changes: PASS\n";
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'director_runtime.cpp'
    binary = Path(tmp) / 'director_runtime'
    cpp.write_text('#include "tests/chaos_runtime_stubs.h"\n#include "dllmain/Director.h"\n#include "dllmain/DirectorCore.h"\n#include <array>\n#include <deque>\n#include <map>\n#include <mutex>\n' + config + native + source + cases)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-I', str(root), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
