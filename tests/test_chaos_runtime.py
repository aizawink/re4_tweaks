"""Compile the production runtime against controlled native-object stubs.
This tests lifecycle/restoration; only the game can prove native weapon behavior.
"""
from pathlib import Path
import subprocess
import os
import tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'dllmain/Chaos.cpp').read_text()
source='namespace aiza'+source.split('namespace aiza',1)[1]
settings=(root/'dllmain/Settings.h').read_text()
fields=settings.split('// Aiza Chaos runtime; everything defaults to OFF.',1)[1].split('// TRAINER_HOTKEYS',1)[0]
config='struct Config {'+fields+' bool HasUnsavedChanges=false; } config;\nnamespace re4t { Config* cfg=&config; }\n'
scheduler=(root/'dllmain/Game.cpp').read_text().split('    aiza::ChaosBeforeUpdate();',1)[1].split('    aiza::ChaosAfterUpdate();',1)[0]
scheduler='void RunScheduler() {\n    aiza::ChaosBeforeUpdate();'+scheduler+'    aiza::ChaosAfterUpdate();\n}\n'
cases=r'''
int main()
{
    cEm enemy;
    manager.items={&enemy};
    config.bChaosEnabled=true;
    config.bChaosFastRun=true; config.fChaosRunSpeed=8;
    config.bChaosEnemySpeed=true; config.fChaosEnemySpeed=3;
    config.bChaosInfiniteAmmo=true;
    heldKeys=uint64_t(KEY_BTN::KEY_RUN)|uint64_t(KEY_BTN::KEY_FORWARD);
    clockTick+=16; aiza::ChaosBeforeUpdate();
    assert(player.Motion_1D8.Seq_speed_C0==8 && enemy.Motion_1D8.Seq_speed_C0==3);
    assert(FlagIsSet(globals.flags_DEBUG_0_60,0));
    aiza::ChaosAfterUpdate();
    assert(player.Motion_1D8.Seq_speed_C0==1 && enemy.Motion_1D8.Seq_speed_C0==1);
    assert(!FlagIsSet(globals.flags_DEBUG_0_60,0));
    // An existing native infinite-ammo preference must survive Chaos.
    FlagSet(globals.flags_DEBUG_0_60,0,true);
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate();
    assert(FlagIsSet(globals.flags_DEBUG_0_60,0));
    FlagSet(globals.flags_DEBUG_0_60,0,false);
    // Preserve native animation-rate changes made during the scheduler.
    clockTick+=16; aiza::ChaosBeforeUpdate();
    enemy.Motion_1D8.Seq_speed_C0=.5f;
    aiza::ChaosAfterUpdate(); assert(enemy.Motion_1D8.Seq_speed_C0==.5f);
    enemy.Motion_1D8.Seq_speed_C0=1;
    // Fire acceleration and trigger injection require a gun and aim+fire.
    heldKeys=uint64_t(KEY_BTN::KEY_KAMAE); bio4::fireHeld=true;
    config.bChaosFastFire=true; config.fChaosFireSpeed=12;
    clockTick+=16; aiza::ChaosBeforeUpdate();
    assert(aiza::ChaosWantsFireTrigger() && plWep.m_ShotTimer_23==0);
    assert(player.Motion_1D8.Seq_speed_C0==12);
    aiza::ChaosAfterUpdate(); assert(player.Motion_1D8.Seq_speed_C0==1);
    gun.itemId_350=1;
    clockTick+=16; aiza::ChaosBeforeUpdate();
    assert(!aiza::ChaosWantsFireTrigger()); aiza::ChaosAfterUpdate(); gun.itemId_350=35;
    // Persistent damage flags alone do not repeatedly launch enemies.
    config.bChaosLaunchShots=true; enemy.m_DmgInfo_328.m_Flag_4=1;
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate();
    assert(enemy.pos_94.y==0);
    clockTick+=16; aiza::ChaosBeforeUpdate(); enemy.hp_324-=100;
    aiza::ChaosAfterUpdate(); assert(enemy.pos_94.y>0);
    // Master OFF restores known launch origin immediately.
    config.bChaosEnabled=false;
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate();
    assert(enemy.pos_94.x==0 && enemy.pos_94.y==0 && enemy.pos_94.z==0);
    assert(player.Motion_1D8.Seq_speed_C0==1);
    // F7 pulse, pause cancellation, and no timer continuing through a menu.
    config.bChaosEnabled=true; config.bChaosLaunchShots=false;
    aiza::ChaosRequestPulse();
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate(); assert(enemy.pos_94.y>0);
    FlagSet(globals.flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_SUB_SCRN),true);
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate(); assert(enemy.pos_94.y==0);
    FlagSet(globals.flags_STATUS_0_501C,uint32_t(Flags_STATUS::STA_SUB_SCRN),false);
    // Slot reused mid-update: do not restore the old rate or position.
    clockTick+=16; aiza::ChaosBeforeUpdate();
    enemy.r_no_0_FC=0; aiza::ChaosOnSpawn(&enemy);
    enemy.Motion_1D8.Seq_speed_C0=.25f; enemy.pos_94.y=123;
    enemy.r_no_0_FC=1; aiza::ChaosAfterUpdate();
    assert(enemy.Motion_1D8.Seq_speed_C0==.25f && enemy.pos_94.y==123);
    enemy.Motion_1D8.Seq_speed_C0=1; enemy.pos_94.y=0;
    // Scene changes invalidate pending actor access, but restore our global bit.
    clockTick+=16; aiza::ChaosBeforeUpdate();
    globals.pRoom_40=reinterpret_cast<void*>(3);
    enemy.valid=false; aiza::ChaosAfterUpdate();
    assert(!FlagIsSet(globals.flags_DEBUG_0_60,0));
    enemy.valid=true; enemy.Motion_1D8.Seq_speed_C0=1; player.Motion_1D8.Seq_speed_C0=1;
    // Do not write a stale pointer after despawn. Test under ASan below.
    auto* temporary=new cEm; manager.items={temporary};
    clockTick+=16; aiza::ChaosBeforeUpdate(); manager.items.clear(); delete temporary;
    aiza::ChaosAfterUpdate();
    // The simultaneous launch limit is enforced for a large pulse.
    std::vector<cEm> crowd(24);
    for(unsigned i=0;i<crowd.size();++i) { crowd[i].emListIndex_3A0=uint8_t(i); manager.items.push_back(&crowd[i]); }
    aiza::ChaosRequestPulse();
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate();
    unsigned airborne=0;for(auto& em:crowd) if(em.pos_94.y>0) ++airborne;
    assert(airborne==16);
    config.bChaosEnabled=false;
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate();
    for(auto& em:crowd) assert(em.pos_94.y==0);
    // F9 starts a forward-only blast, then the normal trajectory advances it.
    cEm front,behind;front.pos_94.z=500;behind.pos_94.z=-500;behind.emListIndex_3A0=1;
    manager.items={&front,&behind};config.bChaosEnabled=true;config.bChaosBowling=true;
    heldKeys=0;bio4::fireHeld=false;keyStates[VK_F9]=short(0x8000);
    clockTick+=16;aiza::ChaosBeforeUpdate();aiza::ChaosAfterUpdate();
    keyStates[VK_F9]=0;
    clockTick+=16;aiza::ChaosBeforeUpdate();aiza::ChaosAfterUpdate();
    assert(front.pos_94.y>0 && behind.pos_94.y==0);
    config.bChaosEnabled=false;
    clockTick+=16;aiza::ChaosBeforeUpdate();aiza::ChaosAfterUpdate();
    assert(front.pos_94.y==0 && front.pos_94.z==500);
    heldKeys=uint64_t(KEY_BTN::KEY_KAMAE);bio4::fireHeld=true;
    // F6 is edge-triggered and only accepted in the game foreground.
    keyStates[VK_F6]=short(0x8000);
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate(); assert(config.bChaosEnabled);
    clockTick+=16; aiza::ChaosBeforeUpdate(); aiza::ChaosAfterUpdate(); assert(config.bChaosEnabled);
    keyStates[VK_F6]=0;clockTick+=16;aiza::ChaosBeforeUpdate();aiza::ChaosAfterUpdate();
    foreground=2;keyStates[VK_F6]=short(0x8000);
    clockTick+=16;aiza::ChaosBeforeUpdate();aiza::ChaosAfterUpdate();assert(config.bChaosEnabled);
    keyStates[VK_F6]=0;foreground=1;
    // The actual scheduler injection restores only synthetic fire, not other bits.
    triggerKeys=0; nativeUpdate=[](void*,void*) {
        assert(triggerKeys&uint64_t(KEY_BTN::KEY_FIRE));triggerKeys|=1ull<<20;
    };
    clockTick+=16; RunScheduler();
    assert(triggerKeys==(1ull<<20));
    triggerKeys=uint64_t(KEY_BTN::KEY_FIRE);
    clockTick+=16; RunScheduler();
    assert(triggerKeys&uint64_t(KEY_BTN::KEY_FIRE));
    std::cout << "Production Chaos lifecycle, shots, pulse, input, pause, despawn and restoration: PASS\n";
}
'''
forward='namespace aiza {void ChaosBeforeUpdate();void ChaosAfterUpdate();bool ChaosWantsFireTrigger();void DirectorBeforeUpdate() {} bool DirectorOwns(cEm*) {return false;}float DirectorFireFactor(){return 1;}float DirectorEnemyFactor(){return 1;}float DirectorEliteSpeed(cEm*){return 1;}int DirectorFlightMode(int x){return x;}bool DirectorForceLaunch(){return false;}std::string DirectorOverlay(){return "";}}\n'
native='std::function<void(void*,void*)> nativeUpdate=[](void*,void*){};\nvoid cSceSys__scheduler(void* a,void* b) {nativeUpdate(a,b);}\nvoid* thisptr=nullptr;void* unused=nullptr;\n'
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'chaos_runtime.cpp';binary=Path(tmp)/'chaos_runtime'
    cpp.write_text('#include "tests/chaos_runtime_stubs.h"\n#include "dllmain/DirectorCore.h"\n'+config+forward+source+native+scheduler+cases)
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I',str(root),str(cpp),'-o',str(binary)],check=True)
    # LeakSanitizer cannot run under ptrace in the hosted executor.
    # Address/undefined-behavior checks remain enabled.
    subprocess.run([str(binary)],check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
