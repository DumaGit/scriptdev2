/* Copyright (C) 2006,2007 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* ScriptData
SDName: Boss_Janalai
SD%Complete: 100
SDComment: 
EndScriptData */

#include "../../sc_defines.h"

// Jan'alai
// --Spell
#define SPELL_FLAME_BREATH          43140
#define SPELL_FIRE_WALL             43113
#define SPELL_ENRAGE                44779
#define SPELL_TELETOCENTER          43098
#define SPELL_SUMMONALL             43097
// -- Fire Bob Spells
#define MOB_FIRE_BOMB               23920
#define SPELL_FIRE_BOMB_CHANNEL     42621    
#define SPELL_FIRE_BOMB_THROW       42628
#define SPELL_FIRE_BOMB_DUMMY       42629
#define SPELL_FIRE_BOMB_DAMAGE      42630 

// -- SAYs
#define SOUND_EVENT                 12031 //NOT USED need more information
#define SAY_EVENT                   "Spirits of da wind be your doom!" // NOT USED need more information
#define SOUND_FIRE_BOMBS            12032
#define SAY_FIRE_BOMBS              "I burn ya now!"
#define SOUND_SUMMON_HATCHER        12033
#define SAY_SUMMON_HATCHER          "Where ma hatcha? Get to work on dem eggs!"
#define SOUND_ALL_EGGS              12034
#define SAY_ALL_EGGS                "I show you strength... in numbers."
#define SOUND_BERSERK               12035
#define SAY_BERSERK                 "You done run outta time!"

#define SOUND_SLAY_1                12036
#define SAY_SLAY_1                  "It all be over now, mon!"
#define SOUND_SLAY_2                12037
#define SAY_SLAY_2                  "Tazaga-choo!"

#define SOUND_DEATH                 12038
#define SAY_DEATH                   "Zul'jin... got a surprise for you..."

#define SOUND_AGGRO_1               12039
#define SAY_AGGRO_1                 "Come, strangers. The spirit of the dragonhawk hot be hungry for worthy souls."
#define SOUND_AGGRO_2               12040
#define SAY_AGGRO_2                 "Come, friends. Your bodies gonna feed ma hatchlings, and your souls are going to feed me with power!"

// --Summons
#define MOB_AMANI_HATCHER           23818
#define MOB_HATCHLING               23598

// -- Hatcher Spells
#define SPELL_HATCH_EGG             43734

// -- Hatchling Spells
#define SPELL_FLAMEBUFFED           43299

const int area_dx = 44;
const int area_dy = 51;

float JanalainPos[1][3] = 
{
    {-33.93, 1149.27, 19}
};

float FireWallCoords[4][4] = 
{
    {-10.13, 1149.27, 19, 3.1415},
    {-33.93, 1123.90, 19, 0.5*3.1415},
    {-54.80, 1150.08, 19, 0},
    {-33.93, 1175.68, 19, 1.5*3.1415}
};

float hatcherway_l[5][3] =
{
    {-87.46,1170.09,6},
    {-74.41,1154.75,6},
    {-52.74,1153.32,19},
    {-33.37,1172.46,19},
    {-33.09,1203.87,19}
};

float hatcherway_r[5][3] =
{
    {-86.57,1132.85,6},
    {-73.94,1146.00,6},
    {-52.29,1146.51,19},
    {-33.57,1125.72,19},
    {-34.29,1095.22,19}
};


struct MANGOS_DLL_DECL boss_janalaiAI : public ScriptedAI
{
    boss_janalaiAI(Creature *c) : ScriptedAI(c)
    { 
        pInstance = (c->GetInstanceData()) ? ((ScriptedInstance*)c->GetInstanceData()) : NULL;
        EnterEvadeMode();
    }

    ScriptedInstance *pInstance;

    uint32 fire_breath_timer;
    uint32 bomb_timer;
    uint32 throw_timer;
    uint32 enrage_timer;
    uint32 finishedbomb_timer;
    uint32 bombcounter;
    uint32 hatchertime;
    uint32 eggs;
    uint32 wipetimer;
    uint32 reset_timer;
    bool noeggs;

    Unit* firebombs[40];
    Unit* throwcontroler;

    bool InCombat;
    bool bombing;


    void EnterEvadeMode()
    {
        if(pInstance)
            pInstance->SetData("JanalaiEvent", 0);

        InCombat = false;

        fire_breath_timer = 8000;
        bomb_timer = 30000;
        enrage_timer = 300000;// 5 minutes
        finishedbomb_timer = 6000;
        throw_timer = 1000;
        bombcounter = 0;
        noeggs = false;
        hatchertime = 10000;
        wipetimer = 600000; // 10 mins       
        bombing =false;
        reset_timer = 5000;

        m_creature->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_INTERRUPT_CAST, true);   

        m_creature->RemoveAllAuras();
        m_creature->DeleteThreatList();
        m_creature->CombatStop();
        DoGoHome();            
    }

    void JustDied(Unit* Killer)
    {
        DoYell(SAY_DEATH, LANG_UNIVERSAL, NULL);
        DoPlaySoundToSet(m_creature,SOUND_DEATH);

        if(pInstance)
            pInstance->SetData("JanalaiEvent", 3);
    }

    void KilledUnit(Unit* victim)
    {
        switch(rand()%2)
        {        
        case 0:
            DoYell(SAY_SLAY_1, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature,SOUND_SLAY_1);
            break;

        case 1:
            DoYell(SAY_SLAY_2, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature,SOUND_SLAY_1);
            break;        
        }
    }

    void AttackStart(Unit *who)
    {
        if (!who)
            return;

        if (who->isTargetableForAttack() && who!= m_creature)
        {
            DoStartMeleeAttack(who);

            if (!InCombat)
            {
                if(pInstance)
                    pInstance->SetData("JanalaiEvent", 1);

                switch(rand()%2)
                {
                case 0:
                    DoYell(SAY_AGGRO_1, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(m_creature,SOUND_AGGRO_1);
                    break;

                case 1:
                    DoYell(SAY_AGGRO_2, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(m_creature,SOUND_AGGRO_2);
                    break; 
                }
                InCombat = true;
            }
        }    
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (!who || m_creature->getVictim())
            return;

        if (who->isTargetableForAttack() && who->isInAccessablePlaceFor(m_creature) && m_creature->IsHostileTo(who))
        {
            float attackRadius = m_creature->GetAttackDistance(who);
            if (m_creature->IsWithinDistInMap(who, attackRadius) && m_creature->GetDistanceZ(who) <= CREATURE_Z_ATTACK_RANGE && m_creature->IsWithinLOSInMap(who))
            {
                if(who->HasStealthAura())
                    who->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH); 
                DoStartMeleeAttack(who);

                if (!InCombat)
                {
                    if(pInstance)
                        pInstance->SetData("JanalaiEvent", 1);

                    switch(rand()%2)
                    {
                    case 0:
                        DoYell(SAY_AGGRO_1, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(m_creature,SOUND_AGGRO_1);
                        break;

                    case 1:
                        DoYell(SAY_AGGRO_2, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(m_creature,SOUND_AGGRO_2);
                        break; 
                    }
                    InCombat = true;
                }
            }
        }
    }

    void FireWall() // Create Firewall
    {
        Unit* wall = NULL;
        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[0][0],FireWallCoords[0][1],FireWallCoords[0][2],FireWallCoords[0][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);
        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[0][0],FireWallCoords[0][1]+5,FireWallCoords[0][2],FireWallCoords[0][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);
        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[0][0],FireWallCoords[0][1]-5,FireWallCoords[0][2],FireWallCoords[0][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);


        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[1][0]-2,FireWallCoords[1][1]-2,FireWallCoords[1][2],FireWallCoords[1][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);
        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[1][0]+2,FireWallCoords[1][1]+2,FireWallCoords[1][2],FireWallCoords[1][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);


        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[2][0],FireWallCoords[2][1],FireWallCoords[2][2],FireWallCoords[2][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);
        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[2][0],FireWallCoords[2][1]-5,FireWallCoords[2][2],FireWallCoords[2][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);
        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[2][0],FireWallCoords[2][1]+5,FireWallCoords[2][2],FireWallCoords[2][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);

        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[3][0]-2,FireWallCoords[3][1],FireWallCoords[3][2],FireWallCoords[3][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);
        wall = m_creature->SummonCreature(MOB_FIRE_BOMB,FireWallCoords[3][0]+2,FireWallCoords[3][1],FireWallCoords[3][2],FireWallCoords[3][3],TEMPSUMMON_TIMED_DESPAWN,11500);
        wall->CastSpell(wall,SPELL_FIRE_WALL,false);
    }

    void throwBombs()// create Bombs
    {
        float dx;
        float dy;
        for ( int i(0); i < 40; i++)
        {
            dx = (rand()%(area_dx))-(area_dx/2);
            dy = (rand()%(area_dy))-(area_dy/2);

            firebombs[i] = DoSpawnCreature(MOB_FIRE_BOMB,dx,dy,0,0,TEMPSUMMON_TIMED_DESPAWN,13000);
        };

        throwcontroler = DoSpawnCreature(MOB_FIRE_BOMB,0,0,1,0,TEMPSUMMON_TIMED_DESPAWN,10000);
        bombcounter = 0;
    }

    void throw5Bombs() //throwanimation
    {
        for ( int i(0); i < 4; i++)
        {
            throwcontroler->CastSpell(firebombs[bombcounter],SPELL_FIRE_BOMB_THROW,true);
            firebombs[bombcounter]->CastSpell(firebombs[bombcounter],SPELL_FIRE_BOMB_DUMMY,false);
            bombcounter ++;
        }
        throwcontroler->CastSpell(firebombs[bombcounter],SPELL_FIRE_BOMB_THROW,true);
        firebombs[bombcounter]->CastSpell(firebombs[bombcounter],SPELL_FIRE_BOMB_DUMMY,false);
        bombcounter ++;
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;        

        if(!bombing)// every Spell if NOT Bombing
        {

            //FIRE BREATH  several videos says every 8Secounds       
            if(fire_breath_timer < diff)
            {  
                Unit* target = NULL;
                target = SelectUnit(SELECT_TARGET_RANDOM,0);
                DoCast(target,SPELL_FLAME_BREATH);
                fire_breath_timer = 8000;            
            }else fire_breath_timer -=diff;

            if(bomb_timer < diff)
            {
                FireWall();
                bomb_timer = 20000+rand()%20000;
                m_creature->Relocate(JanalainPos[0][0],JanalainPos[0][1],JanalainPos[0][2],0);
                DoYell(SAY_FIRE_BOMBS, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature,SOUND_FIRE_BOMBS);
                throwBombs();
                bombing = true;
                //Teleport every Player into the middle
                Unit* Temp = NULL; 
                std::list<HostilReference*>::iterator i = m_creature->getThreatManager().getThreatList().begin();
                for (; i != m_creature->getThreatManager().getThreatList().end(); ++i)
                {

                    Temp = Unit::GetUnit((*m_creature),(*i)->getUnitGuid());
                    if (Temp)
                        DoCast(Temp,SPELL_SUMMONALL,true);
                }
                DoCast(m_creature,SPELL_TELETOCENTER,true);// only Effect Spell
                DoCast(m_creature,SPELL_FIRE_BOMB_CHANNEL,false);
                finishedbomb_timer = 11000;
            }else bomb_timer -=diff; 
        };

        if(bombing) // every Spell if Bombing
        {
            if(bombcounter < 40)
            {
                if(throw_timer < diff)
                {
                    throw5Bombs();
                    throw_timer = 1000;
                }else throw_timer -=diff;
            }

            if(finishedbomb_timer < diff)
            {
                bombing = false;
                finishedbomb_timer = 6000;
                m_creature->RemoveAura(SPELL_FIRE_BOMB_CHANNEL,0);
                m_creature->RemoveAura(SPELL_FIRE_BOMB_CHANNEL,1);
            }else finishedbomb_timer -=diff;
        };

        //Call Hatcher
        if(!noeggs && hatchertime < diff)
        {
            if(pInstance->GetData("J_Eggsleft")>0 || pInstance->GetData("J_Eggsright") > 0)
            { 
                Unit* hatcher = NULL;
                DoYell(SAY_SUMMON_HATCHER, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature,SOUND_SUMMON_HATCHER);
                hatcher = m_creature->SummonCreature(MOB_AMANI_HATCHER,hatcherway_l[0][0],hatcherway_l[0][1],hatcherway_l[0][2],0,TEMPSUMMON_CORPSE_TIMED_DESPAWN,10000);
                hatcher->Relocate(hatcherway_l[0][0],hatcherway_l[0][1],hatcherway_l[0][2]);
                hatcher = m_creature->SummonCreature(MOB_AMANI_HATCHER,hatcherway_r[0][0],hatcherway_r[0][1],hatcherway_r[0][2],0,TEMPSUMMON_CORPSE_TIMED_DESPAWN,10000);
                hatcher->Relocate(hatcherway_r[0][0],hatcherway_r[0][1],hatcherway_r[0][2]);
                hatchertime = 45000;
            }
            else
            {
                noeggs = true;
            };
        }else hatchertime -=diff;

        //Enrage after 5 minutes
        if(enrage_timer < diff && !bombing)
        {             
            DoYell(SAY_BERSERK, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature,SOUND_BERSERK);
            m_creature->InterruptSpell(CURRENT_GENERIC_SPELL);
            DoCast(m_creature,SPELL_ENRAGE);
            enrage_timer = 600000;             
        }else enrage_timer -=diff;

        //WIPE after 10 minutes
        if(wipetimer < diff)
        {     
            DoYell(SAY_BERSERK, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature,SOUND_BERSERK);  

            const CreatureInfo *cinfo = m_creature->GetCreatureInfo();
            m_creature->SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, (cinfo->mindmg * 10));
            m_creature->SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, (cinfo->maxdmg * 10));
            m_creature->UpdateDamagePhysical(BASE_ATTACK);      

            wipetimer = 30000;            
        }else wipetimer -=diff;

        //Hatch All
        if(!noeggs && (m_creature->GetHealth()*100) / m_creature->GetMaxHealth() < 35) 
        {
            DoYell(SAY_ALL_EGGS , LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature,SOUND_BERSERK); 
            eggs = pInstance->GetData("J_Eggsleft");
            int i;
            for(i=1;i<=eggs;i=i+1)
            {
                int r = (rand()%20 - 10);
                int s = (rand()%20 - 10);
                m_creature->SummonCreature(MOB_HATCHLING,JanalainPos[0][0]+s,JanalainPos[0][1]+r,JanalainPos[0][2],0,TEMPSUMMON_CORPSE_TIMED_DESPAWN,15000);

                pInstance->SetData("J_Hatchleft",1);
            };
            eggs = pInstance->GetData("J_Eggsright");

            for(i=1;i<=eggs;i=i+1)
            {
                int r = (rand()%20 - 10);
                int s = (rand()%20 - 10);
                m_creature->SummonCreature(MOB_HATCHLING,JanalainPos[0][0]+s,JanalainPos[0][1]+r,JanalainPos[0][2],0,TEMPSUMMON_CORPSE_TIMED_DESPAWN,15000);
                pInstance->SetData("J_Hatchright",1);
            };
            noeggs = true;
        };

        //check for reset ... exploit preventing ... pulled from his podest
        if(reset_timer < diff)
        {
            if(m_creature->GetPositionX() < -70 || m_creature->GetPositionX() > 0)
            {
                EnterEvadeMode();
                reset_timer = 5000; //every 5 Seca
            }
        }else reset_timer -=diff;

        DoMeleeAttackIfReady();    
    }    
};

CreatureAI* GetAI_boss_janalaiAI(Creature *_Creature)
{
    return new boss_janalaiAI (_Creature);
}

struct MANGOS_DLL_DECL mob_jandalai_firebombAI : public ScriptedAI
{
    mob_jandalai_firebombAI(Creature *c) : ScriptedAI(c)
    { 
        EnterEvadeMode();
    }


    uint32 bomb_timer;

    void EnterEvadeMode()
    {
        m_creature->RemoveAllAuras();
        m_creature->DeleteThreatList();
        m_creature->CombatStop();
        DoGoHome();
        bomb_timer = 12000;
    }

    void UpdateAI(const uint32 diff)
    {
        if(bomb_timer < diff) //Boom
        {
            m_creature->CastSpell(m_creature,SPELL_FIRE_BOMB_DAMAGE,false);
            bomb_timer = 1800000;
        }else bomb_timer -=diff;
    }
};

CreatureAI* GetAI_mob_jandalai_firebombAI(Creature *_Creature)
{
    return new mob_jandalai_firebombAI (_Creature);
}

struct MANGOS_DLL_DECL mob_amanishi_hatcherAI : public ScriptedAI
{
    mob_amanishi_hatcherAI(Creature *c) : ScriptedAI(c)
    { 
        pInstance = (c->GetInstanceData()) ? ((ScriptedInstance*)c->GetInstanceData()) : NULL;
        EnterEvadeMode();
    }

    ScriptedInstance *pInstance;

    uint32 waypoint;
    bool waytype;
    bool start;
    bool hatch;
    bool wait;
    uint32 hatchlings;
    uint32 waittimer;
    uint32 eggs;
    uint32 delete_timer;

    void EnterEvadeMode()
    {
        waypoint = 0;
        waytype = 0;
        hatch = false;
        start = false;
        wait = false;
        waittimer = 4000;
        hatchlings = 0;
        eggs = 0;
        delete_timer = 10000;

        m_creature->RemoveAllAuras();
        m_creature->DeleteThreatList();
        m_creature->CombatStop();
        DoGoHome();
    }

    void UpdateAI(const uint32 diff)
    { 
        if(pInstance->GetData("JanalaiEvent") == 1)
        {
            if(!start && !hatch)
            {
                waytype = ( m_creature->GetPositionY() > 1150);
                waypoint = 1;
                start = true;
            }

            if(start && !hatch)
            {
                if(wait)
                {
                    if(waittimer < diff)
                    {
                        wait = false;
                        waittimer = 4000;
                        waypoint++;
                        if (waypoint == 5)
                        {
                            hatch = true;
                            waittimer = 0;
                            hatchlings = 1;
                        };
                    }
                    else 
                    {
                        waittimer -=diff;
                    };
                }
                else
                {
                    m_creature->GetMotionMaster()->Clear();
                    if(waytype)
                    {
                        m_creature->SendMoveToPacket(hatcherway_l[waypoint][0],hatcherway_l[waypoint][1],hatcherway_l[waypoint][2], false, waittimer);
                        m_creature->Relocate(hatcherway_l[waypoint][0],hatcherway_l[waypoint][1],hatcherway_l[waypoint][2]);
                    }
                    else
                    {
                        m_creature->SendMoveToPacket(hatcherway_r[waypoint][0],hatcherway_r[waypoint][1],hatcherway_r[waypoint][2], false, waittimer);
                        m_creature->Relocate(hatcherway_r[waypoint][0],hatcherway_r[waypoint][1],hatcherway_r[waypoint][2]);
                    }
                    wait= true;
                }
            }

            if(start && hatch)
            {   

                if(waittimer < diff)
                {
                    waittimer = 4000;
                    Unit* hatchling;

                    if(waytype)
                    {
                        eggs = pInstance->GetData("J_Eggsleft"); 
                        if(eggs > 0)
                            DoCast(m_creature,SPELL_HATCH_EGG);               
                        int i;
                        for(i=1;i<=hatchlings;i=i+1)
                        {
                            eggs = pInstance->GetData("J_Eggsleft");                        
                            if(eggs <= 0)
                            {
                                if (waytype) waytype = false; else waytype = true;
                                waittimer = 15000;
                                hatch = false;
                                waypoint = 4;
                                wait = true;
                                i = hatchlings;
                            }
                            else
                            {   
                                hatchling = DoSpawnCreature(MOB_HATCHLING,rand()%4-2,rand()%4-2,0,0,TEMPSUMMON_CORPSE_TIMED_DESPAWN,15000);
                                pInstance->SetData("J_Hatchleft",1);
                            };
                        };
                        DoCast(m_creature,SPELL_HATCH_EGG);
                        if(hatchlings < 5)                    
                            hatchlings++;
                    }
                    else
                    {
                        eggs = pInstance->GetData("J_Eggsright"); 
                        if(eggs > 0)
                            DoCast(m_creature,SPELL_HATCH_EGG);               
                        int i;
                        for(i=1;i<=hatchlings;i=i+1)
                        {
                            eggs = pInstance->GetData("J_Eggsright");                        
                            if(eggs <= 0)
                            {
                                if (waytype) waytype = false; else waytype = true;
                                waittimer = 15000;
                                hatch = false;
                                waypoint = 4;
                                wait = true;
                                i = hatchlings;
                            }
                            else
                            {
                                hatchling = DoSpawnCreature(MOB_HATCHLING,rand()%4-2,rand()%4-2,0,0,TEMPSUMMON_CORPSE_TIMED_DESPAWN,15000);
                                pInstance->SetData("J_Hatchright",1);
                            };
                        };
                        DoCast(m_creature,SPELL_HATCH_EGG);                 
                        hatchlings++;
                    };
                }else waittimer -=diff;
            }
        }
        else
        {            
            if(delete_timer < diff)
            {
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_NORMAL, NULL, false);
                delete_timer = 10000;
            }else delete_timer -=diff;         
        }
    }
};

CreatureAI* GetAI_mob_amanishi_hatcherAI(Creature *_Creature)
{
    return new mob_amanishi_hatcherAI (_Creature);
}

struct MANGOS_DLL_DECL mob_hatchlingAI : public ScriptedAI
{
    mob_hatchlingAI(Creature *c) : ScriptedAI(c)
    { 
        pInstance = (c->GetInstanceData()) ? ((ScriptedInstance*)c->GetInstanceData()) : NULL;
        EnterEvadeMode();
    }

    ScriptedInstance *pInstance;

    uint32 buffer_timer;
    uint32 delete_timer;
    bool start;

    void EnterEvadeMode()
    {
        m_creature->RemoveAllAuras();
        m_creature->DeleteThreatList();
        m_creature->CombatStop();
        DoGoHome();

        buffer_timer = 7000;
        delete_timer = 10000;
        start = false;
    }

    void AttackStart(Unit *who)
    {
        if (!who)
            return;

        if (who->isTargetableForAttack() && who!= m_creature)
        {
            DoStartMeleeAttack(who);
        }    
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (!who || m_creature->getVictim())
            return;

        if (who->isTargetableForAttack() && who->isInAccessablePlaceFor(m_creature) && m_creature->IsHostileTo(who))
        {
            float attackRadius = m_creature->GetAttackDistance(who);
            if (m_creature->IsWithinDistInMap(who, attackRadius) && m_creature->GetDistanceZ(who) <= CREATURE_Z_ATTACK_RANGE && m_creature->IsWithinLOSInMap(who))
            {
                if(who->HasStealthAura())
                    who->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH); 
                DoStartMeleeAttack(who);
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {    
        if(!start)
        {
            if(m_creature->GetPositionY() > 1150)
            {
                m_creature->SendMoveToPacket(hatcherway_l[3][0]+rand()%4-2,hatcherway_l[3][1]+rand()%4-2,hatcherway_l[3][2],false,4000);
                m_creature->Relocate(hatcherway_l[3][0]+rand()%4-2,hatcherway_l[3][1]+rand()%4-2,hatcherway_l[3][2]);
            }else
            {
                m_creature->SendMoveToPacket(hatcherway_r[3][0]+rand()%4-2,hatcherway_r[3][1]+rand()%4-2,hatcherway_r[3][2],false,4000);
                m_creature->Relocate(hatcherway_r[3][0]+rand()%4-2,hatcherway_r[3][1]+rand()%4-2,hatcherway_r[3][2]);
            };
            start = true;
        }

        if(delete_timer < diff && !(pInstance->GetData("JanalaiEvent") == 1))
        {
            if(!(m_creature->getVictim()))
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_NORMAL, NULL, false);
            delete_timer = 10000;
        }else delete_timer -=diff;   

        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;        

        if (buffer_timer < diff)
        {
            Unit* target = NULL;
            target = SelectUnit(SELECT_TARGET_RANDOM,0);
            DoCast(target,SPELL_FLAMEBUFFED);
            buffer_timer = 7000;
        }else buffer_timer -=diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_hatchlingAI(Creature *_Creature)
{
    return new mob_hatchlingAI (_Creature);
}

void AddSC_boss_janalai()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_janalai";
    newscript->GetAI = GetAI_boss_janalaiAI;
    m_scripts[nrscripts++] = newscript;  

    newscript = new Script;
    newscript->Name="mob_jandalai_firebomb";
    newscript->GetAI = GetAI_mob_jandalai_firebombAI;
    m_scripts[nrscripts++] = newscript;  

    newscript = new Script;
    newscript->Name="mob_amanishi_hatcher";
    newscript->GetAI = GetAI_mob_amanishi_hatcherAI;
    m_scripts[nrscripts++] = newscript; 

    newscript = new Script;
    newscript->Name="mob_hatchling";
    newscript->GetAI = GetAI_mob_hatchlingAI;
    m_scripts[nrscripts++] = newscript; 
}