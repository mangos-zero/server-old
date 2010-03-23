/*
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2010 MaNGOSZero <http://github.com/mangoszero/mangoszero/>
 *
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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Opcodes.h"
#include "Log.h"
#include "UpdateMask.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "DynamicObject.h"
#include "Group.h"
#include "UpdateData.h"
#include "ObjectAccessor.h"
#include "Policies/SingletonImp.h"
#include "Totem.h"
#include "Creature.h"
#include "ScriptCalls.h"
#include "Formulas.h"
#include "BattleGround.h"
#include "CreatureAI.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

#define NULL_AURA_SLOT 0xFF

pAuraHandler AuraHandler[TOTAL_AURAS]=
{
    &Aura::HandleNULL,                                      //  0 SPELL_AURA_NONE
    &Aura::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
    &Aura::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
    &Aura::HandlePeriodicDamage,                            //  3 SPELL_AURA_PERIODIC_DAMAGE
    &Aura::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
    &Aura::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
    &Aura::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
    &Aura::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
    &Aura::HandlePeriodicHeal,                              //  8 SPELL_AURA_PERIODIC_HEAL
    &Aura::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
    &Aura::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
    &Aura::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
    &Aura::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
    &Aura::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &Aura::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &Aura::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD    implemented in Unit::DoAttackDamage
    &Aura::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
    &Aura::HandleNoImmediateEffect,                         // 17 SPELL_AURA_MOD_STEALTH_DETECT
    &Aura::HandleInvisibility,                              // 18 SPELL_AURA_MOD_INVISIBILITY
    &Aura::HandleInvisibilityDetect,                        // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &Aura::HandleAuraModTotalHealthPercentRegen,            // 20 SPELL_AURA_OBS_MOD_HEALTH
    &Aura::HandleAuraModTotalManaPercentRegen,              // 21 SPELL_AURA_OBS_MOD_MANA
    &Aura::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
    &Aura::HandlePeriodicTriggerSpell,                      // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL
    &Aura::HandlePeriodicEnergize,                          // 24 SPELL_AURA_PERIODIC_ENERGIZE
    &Aura::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
    &Aura::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
    &Aura::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
    &Aura::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
    &Aura::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
    &Aura::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
    &Aura::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &Aura::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &Aura::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &Aura::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &Aura::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &Aura::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
    &Aura::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
    &Aura::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
    &Aura::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &Aura::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &Aura::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
    &Aura::HandleAuraProcTriggerSpell,                      // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
    &Aura::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
    &Aura::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
    &Aura::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
    &Aura::HandleUnused,                                    // 46 SPELL_AURA_MOD_PARRY_SKILL    obsolete?
    &Aura::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &Aura::HandleUnused,                                    // 48 SPELL_AURA_MOD_DODGE_SKILL    obsolete?
    &Aura::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &Aura::HandleUnused,                                    // 50 SPELL_AURA_MOD_BLOCK_SKILL    obsolete?
    &Aura::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &Aura::HandleAuraModCritPercent,                        // 52 SPELL_AURA_MOD_CRIT_PERCENT
    &Aura::HandlePeriodicLeech,                             // 53 SPELL_AURA_PERIODIC_LEECH
    &Aura::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
    &Aura::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &Aura::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
    &Aura::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &Aura::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &Aura::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &Aura::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &Aura::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &Aura::HandleNULL,                                      // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL
    &Aura::HandleUnused,                                    // 63 SPELL_AURA_PERIODIC_MANA_FUNNEL obsolete?
    &Aura::HandlePeriodicManaLeech,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH
    &Aura::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED
    &Aura::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &Aura::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &Aura::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &Aura::HandleSchoolAbsorb,                              // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalcAbsorbResist
    &Aura::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS      Useless, used by only one spell that has only visual effect
    &Aura::HandleModSpellCritChanceShool,                   // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &Aura::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &Aura::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &Aura::HandleNoImmediateEffect,                         // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
    &Aura::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE
    &Aura::HandleFarSight,                                  // 76 SPELL_AURA_FAR_SIGHT
    &Aura::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &Aura::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
    &Aura::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &Aura::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
    &Aura::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT
    &Aura::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &Aura::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &Aura::HandleModRegen,                                  // 84 SPELL_AURA_MOD_REGEN
    &Aura::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN
    &Aura::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &Aura::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &Aura::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT
    &Aura::HandlePeriodicDamagePCT,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &Aura::HandleUnused,                                    // 90 SPELL_AURA_MOD_RESIST_CHANCE  Useless
    &Aura::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &Aura::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &Aura::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &Aura::HandleNoImmediateEffect,                         // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::RegenerateAll
    &Aura::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &Aura::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Spell::SelectMagnetTarget
    &Aura::HandleManaShield,                                // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalcAbsorbResist
    &Aura::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &Aura::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &Aura::HandleUnused,                                    //100 SPELL_AURA_AURAS_VISIBLE obsolete? all player can see all auras now
    &Aura::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &Aura::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &Aura::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &Aura::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &Aura::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &Aura::HandleAddModifier,                               //107 SPELL_AURA_ADD_FLAT_MODIFIER
    &Aura::HandleAddModifier,                               //108 SPELL_AURA_ADD_PCT_MODIFIER
    &Aura::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &Aura::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT
    &Aura::HandleNULL,                                      //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER
    &Aura::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS
    &Aura::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusForVictim
    &Aura::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT
    &Aura::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonus
    &Aura::HandleUnused,                                    //119 SPELL_AURA_SHARE_PET_TRACKING useless
    &Aura::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &Aura::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &Aura::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &Aura::HandleModTargetResistance,                       //123 SPELL_AURA_MOD_TARGET_RESISTANCE
    &Aura::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &Aura::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &Aura::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &Aura::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &Aura::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &Aura::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &Aura::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &Aura::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &Aura::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonus
    &Aura::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &Aura::HandleHaste,                                     //138 SPELL_AURA_MOD_HASTE
    &Aura::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
    &Aura::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
    &Aura::HandleRangedAmmoHaste,                           //141 SPELL_AURA_MOD_RANGED_AMMO_HASTE
    &Aura::HandleAuraModBaseResistancePCT,                  //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &Aura::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &Aura::HandleAuraSafeFall,                              //144 SPELL_AURA_SAFE_FALL                         implemented in WorldSession::HandleMovementOpcodes
    &Aura::HandleUnused,                                    //145 SPELL_AURA_CHARISMA obsolete?
    &Aura::HandleUnused,                                    //146 SPELL_AURA_PERSUADED obsolete?
    &Aura::HandleModMechanicImmunityMask,                   //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK     implemented in Unit::IsImmunedToSpell and Unit::IsImmunedToSpellEffect (check part)
    &Aura::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &Aura::HandleNoImmediateEffect,                         //149 SPELL_AURA_RESIST_PUSHBACK
    &Aura::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &Aura::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
    &Aura::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE implemented in Creature::GetAttackDistance
    &Aura::HandleNoImmediateEffect,                         //153 SPELL_AURA_SPLIT_DAMAGE_FLAT
    &Aura::HandleNoImmediateEffect,                         //154 SPELL_AURA_MOD_STEALTH_LEVEL
    &Aura::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING
    &Aura::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN
    &Aura::HandleNULL,                                      //157 SPELL_AURA_PET_DAMAGE_MULTI
    &Aura::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &Aura::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT      only for Honorless Target spell
    &Aura::HandleNoImmediateEffect,                         //160 SPELL_AURA_MOD_AOE_AVOIDANCE                 implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT
    &Aura::HandleAuraPowerBurn,                             //162 SPELL_AURA_POWER_BURN_MANA
    &Aura::HandleNoImmediateEffect,                         //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS_MELEE
    &Aura::HandleUnused,                                    //164 useless, only one test spell
    &Aura::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &Aura::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &Aura::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &Aura::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonus, Unit::MeleeDamageBonus
    &Aura::HandleNoImmediateEffect,                         //169 SPELL_AURA_MOD_CRIT_PERCENT_VERSUS           implemented in Unit::DealDamageBySchool, Unit::DoAttackDamage, Unit::SpellCriticalBonus
    &Aura::HandleNULL,                                      //170 SPELL_AURA_DETECT_AMORE       only for Detect Amore spell
    &Aura::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &Aura::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &Aura::HandleUnused,                                    //173 SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &Aura::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonus (by default intellect, dependent from SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT)
    &Aura::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonus
    &Aura::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &Aura::HandleNULL,                                      //177 SPELL_AURA_AOE_CHARM
    &Aura::HandleNoImmediateEffect,                         //178 SPELL_AURA_MOD_DEBUFF_RESISTANCE          implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &Aura::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonus
    &Aura::HandleUnused,                                    //181 SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS unused
    &Aura::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &Aura::HandleNoImmediateEffect,                         //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746, implemented in ThreatCalcHelper::calcThreat
    &Aura::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
    &Aura::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
    &Aura::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
    &Aura::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
    &Aura::HandleUnused,                                    //189 SPELL_AURA_MOD_RATING (not used in 1.12.1)
    &Aura::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
    &Aura::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
};

Aura::Aura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, Unit *target, Unit *caster, Item* castItem) :
m_procCharges(0), m_spellmod(NULL), m_effIndex(eff), m_caster_guid(0), m_target(target),
m_timeCla(1000), m_castItemGuid(castItem?castItem->GetGUID():0), m_auraSlot(MAX_AURAS),
m_positive(false), m_permanent(false), m_isPeriodic(false), m_isTrigger(false), m_isAreaAura(false),
m_isPersistent(false), m_removeMode(AURA_REMOVE_BY_DEFAULT), m_isRemovedOnShapeLost(true), m_deleted(false),
m_periodicTimer(0), m_periodicTick(0), m_PeriodicEventId(0), m_AuraDRGroup(DIMINISHING_NONE), m_in_use(0)
{
    assert(target);

    assert(spellproto && spellproto == sSpellStore.LookupEntry( spellproto->Id ) && "`info` must be pointer to sSpellStore element");

    m_spellProto = spellproto;

    m_currentBasePoints = currentBasePoints ? *currentBasePoints : m_spellProto->EffectBasePoints[eff];

    m_isPassive = IsPassiveSpell(GetId());
    m_positive = IsPositiveEffect(GetId(), m_effIndex);

    m_isSingleTargetAura = IsSingleTargetSpell(m_spellProto);

    m_applyTime = time(NULL);

    int32 damage;
    if(!caster)
    {
        m_caster_guid = target->GetGUID();
        damage = m_currentBasePoints+1;                     // stored value-1
        m_maxduration = target->CalculateSpellDuration(m_spellProto, m_effIndex, target);
    }
    else
    {
        m_caster_guid = caster->GetGUID();

        damage        = caster->CalculateSpellDamage(m_spellProto,m_effIndex,m_currentBasePoints,target);
        m_maxduration = caster->CalculateSpellDuration(m_spellProto, m_effIndex, target);
    }

    if(m_maxduration == -1 || m_isPassive && m_spellProto->DurationIndex == 0)
        m_permanent = true;

    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;

    if(!m_permanent && modOwner)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_DURATION, m_maxduration);

    m_duration = m_maxduration;

    if(modOwner)
        modOwner->ApplySpellMod(GetId(), SPELLMOD_ACTIVATION_TIME, m_periodicTimer);

    sLog.outDebug("Aura: construct Spellid : %u, Aura : %u Duration : %d Target : %d Damage : %d", m_spellProto->Id, m_spellProto->EffectApplyAuraName[eff], m_maxduration, m_spellProto->EffectImplicitTargetA[eff],damage);

    SetModifier(AuraType(m_spellProto->EffectApplyAuraName[eff]), damage, m_spellProto->EffectAmplitude[eff], m_spellProto->EffectMiscValue[eff]);

    m_isDeathPersist = IsDeathPersistentSpell(m_spellProto);

    if(m_spellProto->procCharges)
    {
        m_procCharges = m_spellProto->procCharges;

        if(modOwner)
            modOwner->ApplySpellMod(GetId(), SPELLMOD_CHARGES, m_procCharges);
    }
    else
        m_procCharges = -1;

    m_isRemovedOnShapeLost = (m_caster_guid==m_target->GetGUID() && m_spellProto->Stances &&
                            !(m_spellProto->AttributesEx2 & 0x80000) && !(m_spellProto->Attributes & 0x10000));
}

Aura::~Aura()
{
}

AreaAura::AreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, target, caster, castItem)
{
    m_isAreaAura = true;

    // caster==NULL in constructor args if target==caster in fact
    Unit* caster_ptr = caster ? caster : target;

    m_radius = GetSpellRadius(sSpellRadiusStore.LookupEntry(GetSpellProto()->EffectRadiusIndex[m_effIndex]));
    if(Player* modOwner = caster_ptr->GetSpellModOwner())
        modOwner->ApplySpellMod(GetId(), SPELLMOD_RADIUS, m_radius);

    switch(spellproto->Effect[eff])
    {
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
            m_areaAuraType = AREA_AURA_PARTY;
            if (target->GetTypeId() == TYPEID_UNIT && ((Creature*)target)->isTotem())
                m_modifier.m_auraname = SPELL_AURA_NONE;
            break;
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
            m_areaAuraType = AREA_AURA_PET;
            break;
        default:
            sLog.outError("Wrong spell effect in AreaAura constructor");
            ASSERT(false);
            break;
    }
}

AreaAura::~AreaAura()
{
}

PersistentAreaAura::PersistentAreaAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, target, caster, castItem)
{
    m_isPersistent = true;
}

PersistentAreaAura::~PersistentAreaAura()
{
}

SingleEnemyTargetAura::SingleEnemyTargetAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, Unit *target,
Unit *caster, Item* castItem) : Aura(spellproto, eff, currentBasePoints, target, caster, castItem)
{
    if (caster)
        m_casters_target_guid = caster->GetTypeId()==TYPEID_PLAYER ? ((Player*)caster)->GetSelection() : caster->GetTargetGUID();
    else
        m_casters_target_guid = 0;
}

SingleEnemyTargetAura::~SingleEnemyTargetAura()
{
}

Unit* SingleEnemyTargetAura::GetTriggerTarget() const
{
    return ObjectAccessor::GetUnit(*m_target, m_casters_target_guid);
}

Aura* CreateAura(SpellEntry const* spellproto, SpellEffectIndex eff, int32 *currentBasePoints, Unit *target, Unit *caster, Item* castItem)
{
    if (IsAreaAuraEffect(spellproto->Effect[eff]))
        return new AreaAura(spellproto, eff, currentBasePoints, target, caster, castItem);

    uint32 triggeredSpellId = spellproto->EffectTriggerSpell[eff];

    return new Aura(spellproto, eff, currentBasePoints, target, caster, castItem);
}

Unit* Aura::GetCaster() const
{
    if(m_caster_guid == m_target->GetGUID())
        return m_target;

    //return ObjectAccessor::GetUnit(*m_target,m_caster_guid);
    //must return caster even if it's in another grid/map
    Unit *unit = ObjectAccessor::GetUnitInWorld(*m_target,m_caster_guid);
    return unit && unit->IsInWorld() ? unit : NULL;
}

void Aura::SetModifier(AuraType t, int32 a, uint32 pt, int32 miscValue)
{
    m_modifier.m_auraname = t;
    m_modifier.m_amount = a;
    m_modifier.m_miscvalue = miscValue;
    m_modifier.periodictime = pt;
}

void Aura::Update(uint32 diff)
{
    if (m_duration > 0)
    {
        m_duration -= diff;
        if (m_duration < 0)
            m_duration = 0;
        m_timeCla -= diff;

        // GetEffIndex()==0 prevent double/triple apply manaPerSecond/manaPerSecondPerLevel to same spell with many auras
        // all spells with manaPerSecond/manaPerSecondPerLevel have aura in effect 0
        if (GetEffIndex() == EFFECT_INDEX_0 && m_timeCla <= 0)
        {
            if(Unit* caster = GetCaster())
            {
                Powers powertype = Powers(m_spellProto->powerType);
                int32 manaPerSecond = m_spellProto->manaPerSecond + m_spellProto->manaPerSecondPerLevel * caster->getLevel();
                m_timeCla = 1*IN_MILISECONDS;
                if (manaPerSecond)
                {
                    if(powertype==POWER_HEALTH)
                        caster->ModifyHealth(-manaPerSecond);
                    else
                        caster->ModifyPower(powertype,-manaPerSecond);
                }
            }
        }
    }

    // Channeled aura required check distance from caster
    if(IsChanneledSpell(m_spellProto) && m_caster_guid != m_target->GetGUID())
    {
        Unit* caster = GetCaster();
        if(!caster)
        {
            m_target->RemoveAura(GetId(), GetEffIndex());
            return;
        }

        // need check distance for channeled target only
        if (caster->GetChannelObjectGUID() == m_target->GetGUID())
        {
            // Get spell range
            float max_range = GetSpellMaxRange(sSpellRangeStore.LookupEntry(m_spellProto->rangeIndex));

            if(Player* modOwner = caster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_RANGE, max_range, NULL);

            if(!caster->IsWithinDistInMap(m_target, max_range))
            {
                m_target->RemoveAura(GetId(), GetEffIndex());
                return;
            }
        }
    }

    if(m_isPeriodic && (m_duration >= 0 || m_isPassive || m_permanent))
    {
        m_periodicTimer -= diff;
        if(m_periodicTimer <= 0)                            // tick also at m_periodicTimer==0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            if( m_modifier.m_auraname == SPELL_AURA_MOD_REGEN ||
                m_modifier.m_auraname == SPELL_AURA_MOD_POWER_REGEN ||
                                                            // Cannibalize, eating items and other spells
                m_modifier.m_auraname == SPELL_AURA_OBS_MOD_HEALTH ||
                                                            // Eating items and other spells
                m_modifier.m_auraname == SPELL_AURA_OBS_MOD_MANA )
            {
                ApplyModifier(true);
                return;
            }
            // update before applying (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_modifier.periodictime;

            ++m_periodicTick;                               // for some infinity auras in some cases can overflow and reset
            if(m_isTrigger)
                TriggerSpell();
            else
                PeriodicTick();
        }
    }
}

void AreaAura::Update(uint32 diff)
{
    // update for the caster of the aura
    if(m_caster_guid == m_target->GetGUID())
    {
        Unit* caster = m_target;

        if( !caster->hasUnitState(UNIT_STAT_ISOLATED) )
        {
            Unit* owner = caster->GetCharmerOrOwner();
            if (!owner)
                owner = caster;
            std::list<Unit *> targets;

            switch(m_areaAuraType)
            {
                case AREA_AURA_PARTY:
                {
                    Group *pGroup = NULL;

                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        pGroup = ((Player*)owner)->GetGroup();

                    if( pGroup)
                    {
                        uint8 subgroup = ((Player*)owner)->GetSubGroup();
                        for(GroupReference *itr = pGroup->GetFirstMember(); itr != NULL; itr = itr->next())
                        {
                            Player* Target = itr->getSource();
                            if(Target && Target->isAlive() && Target->GetSubGroup()==subgroup && caster->IsFriendlyTo(Target))
                            {
                                if(caster->IsWithinDistInMap(Target, m_radius))
                                    targets.push_back(Target);
                                Pet *pet = Target->GetPet();
                                if(pet && pet->isAlive() && caster->IsWithinDistInMap(pet, m_radius))
                                    targets.push_back(pet);
                            }
                        }
                    }
                    else
                    {
                        // add owner
                        if( owner != caster && caster->IsWithinDistInMap(owner, m_radius) )
                            targets.push_back(owner);
                        // add caster's pet
                        Unit* pet = caster->GetPet();
                        if( pet && caster->IsWithinDistInMap(pet, m_radius))
                            targets.push_back(pet);
                    }
                    break;
                }
                case AREA_AURA_PET:
                {
                    if(owner != caster)
                        targets.push_back(owner);
                    break;
                }
            }

            for(std::list<Unit *>::iterator tIter = targets.begin(); tIter != targets.end(); tIter++)
            {
                if((*tIter)->HasAura(GetId(), m_effIndex))
                    continue;

                if(SpellEntry const *actualSpellInfo = sSpellMgr.SelectAuraRankForPlayerLevel(GetSpellProto(), (*tIter)->getLevel()))
                {
                    int32 actualBasePoints = m_currentBasePoints;
                    // recalculate basepoints for lower rank (all AreaAura spell not use custom basepoints?)
                    if(actualSpellInfo != GetSpellProto())
                        actualBasePoints = actualSpellInfo->EffectBasePoints[m_effIndex];
                    AreaAura *aur = new AreaAura(actualSpellInfo, m_effIndex, &actualBasePoints, (*tIter), caster, NULL);
                    aur->SetAuraDuration(GetAuraDuration());
                    (*tIter)->AddAura(aur);
                }
            }
        }
        Aura::Update(diff);
    }
    else                                                    // aura at non-caster
    {
        Unit* caster = GetCaster();

        Aura::Update(diff);

        // remove aura if out-of-range from caster (after teleport for example)
        // or caster is isolated or caster no longer has the aura
        // or caster is (no longer) friendly
        bool needFriendly = true;
        if( !caster || caster->hasUnitState(UNIT_STAT_ISOLATED) ||
            !caster->IsWithinDistInMap(m_target, m_radius)      ||
            !caster->HasAura(GetId(), GetEffIndex())            ||
            caster->IsFriendlyTo(m_target) != needFriendly
           )
        {
            m_target->RemoveAura(GetId(), GetEffIndex());
        }
        else if( m_areaAuraType == AREA_AURA_PARTY)         // check if in same sub group
        {
            // not check group if target == owner or target == pet
            if (caster->GetCharmerOrOwnerGUID() != m_target->GetGUID() && caster->GetGUID() != m_target->GetCharmerOrOwnerGUID())
            {
                Player* check = caster->GetCharmerOrOwnerPlayerOrPlayerItself();

                Group *pGroup = check ? check->GetGroup() : NULL;
                if( pGroup )
                {
                    Player* checkTarget = m_target->GetCharmerOrOwnerPlayerOrPlayerItself();
                    if(!checkTarget || !pGroup->SameSubGroup(check, checkTarget))
                        m_target->RemoveAura(GetId(), GetEffIndex());
                }
                else
                    m_target->RemoveAura(GetId(), GetEffIndex());
            }
        }
        else if( m_areaAuraType == AREA_AURA_PET )
        {
            if( m_target->GetGUID() != caster->GetCharmerOrOwnerGUID() )
                m_target->RemoveAura(GetId(), GetEffIndex());
        }
    }
}

void PersistentAreaAura::Update(uint32 diff)
{
    bool remove = false;

    // remove the aura if its caster or the dynamic object causing it was removed
    // or if the target moves too far from the dynamic object
    if(Unit *caster = GetCaster())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId(), GetEffIndex());
        if (dynObj)
        {
            if (!m_target->IsWithinDistInMap(dynObj, dynObj->GetRadius()))
                remove = true;
        }
        else
            remove = true;
    }
    else
        remove = true;

    Aura::Update(diff);

    if(remove)
        m_target->RemoveAura(GetId(), GetEffIndex());
}

void Aura::ApplyModifier(bool apply, bool Real)
{
    AuraType aura = m_modifier.m_auraname;

    SetInUse(true);
    if(aura < TOTAL_AURAS)
        (*this.*AuraHandler [aura])(apply, Real);
    SetInUse(false);
}

void Aura::UpdateAuraDuration()
{
    if(m_auraSlot >= MAX_AURAS || m_isPassive)
        return;

    if( m_target->GetTypeId() == TYPEID_PLAYER)
    {
        WorldPacket data(SMSG_UPDATE_AURA_DURATION, 5);
        data << (uint8)m_auraSlot << (uint32)m_duration;
        ((Player*)m_target)->SendDirectMessage(&data);

        data.Initialize(SMSG_SET_EXTRA_AURA_INFO, (8+1+4+4+4));
        data << m_target->GetPackGUID();
        data << uint8(m_auraSlot);
        data << uint32(GetId());
        data << uint32(GetAuraMaxDuration());
        data << uint32(GetAuraDuration());
        ((Player*)m_target)->SendDirectMessage(&data);
    }

    // not send in case player loading (will not work anyway until player not added to map), sent in visibility change code
    if(m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading())
        return;

    Unit* caster = GetCaster();

    if(caster && caster->GetTypeId() == TYPEID_PLAYER && caster != m_target)
        SendAuraDurationForCaster((Player*)caster);
}

void Aura::SendAuraDurationForCaster(Player* caster)
{
    WorldPacket data(SMSG_SET_EXTRA_AURA_INFO_NEED_UPDATE, (8+1+4+4+4));
    data << m_target->GetPackGUID();
    data << uint8(m_auraSlot);
    data << uint32(GetId());
    data << uint32(GetAuraMaxDuration());                   // full
    data << uint32(GetAuraDuration());                      // remain
    caster->GetSession()->SendPacket(&data);
}

bool Aura::IsNeedVisibleSlot(Unit const* caster) const
{
    bool totemAura = caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem();

    // special area auras cases
    switch(m_spellProto->Effect[GetEffIndex()])
    {
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
            // passive auras (except totem auras) do not get placed in caster slot
            return (m_target != caster || totemAura || !m_isPassive) && m_modifier.m_auraname != SPELL_AURA_NONE;
        default:
            break;
    }

    // passive auras (except totem auras) do not get placed in the slots
    return !m_isPassive || totemAura;
}

void Aura::_AddAura()
{
    if (!GetId())
        return;
    if(!m_target)
        return;

    // we can found aura in NULL_AURA_SLOT and then need store state instead check slot != NULL_AURA_SLOT
    bool samespell = false;
    bool secondaura = false;
    uint8 slot = NULL_AURA_SLOT;

    for(uint8 i = 0; i < 3; i++)
    {
        Unit::spellEffectPair spair = Unit::spellEffectPair(GetId(), SpellEffectIndex(i));
        for(Unit::AuraMap::const_iterator itr = m_target->GetAuras().lower_bound(spair); itr != m_target->GetAuras().upper_bound(spair); ++itr)
        {
            // allow use single slot only by auras from same caster
            if(itr->second->GetCasterGUID()==GetCasterGUID())
            {
                // Check for coexisting Weapon-proced Auras
                samespell = !isWeaponBuffCoexistableWith(itr->second);
                if (m_effIndex > itr->second->GetEffIndex())
                     secondaura = true;
                slot = itr->second->GetAuraSlot();
                break;
            }
        }

        if(samespell)
            break;
    }

    // not call total regen auras at adding
    switch (m_modifier.m_auraname)
    {
        case SPELL_AURA_OBS_MOD_HEALTH:
        case SPELL_AURA_OBS_MOD_MANA:
            m_periodicTimer = m_modifier.periodictime;
            break;
        case SPELL_AURA_MOD_REGEN:
        case SPELL_AURA_MOD_POWER_REGEN:
            m_periodicTimer = 5000;
            break;
    }

    // register aura
    if (getDiminishGroup() != DIMINISHING_NONE )
        m_target->ApplyDiminishingAura(getDiminishGroup(),true);

    Unit* caster = GetCaster();

    // set infinity cooldown state for spells
    if(caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (m_spellProto->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE)
        {
            Item* castItem = m_castItemGuid ? ((Player*)caster)->GetItemByGuid(m_castItemGuid) : NULL;
            ((Player*)caster)->AddSpellAndCategoryCooldowns(m_spellProto,castItem ? castItem->GetEntry() : 0, NULL,true);
        }
    }

    if (IsNeedVisibleSlot(caster))
    {
        if(!samespell)                                      // new slot need
        {
            if (IsPositive())                               // empty positive slot
            {
                for (uint8 i = 0; i < MAX_POSITIVE_AURAS; i++)
                {
                    if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
                    {
                        slot = i;
                        break;
                    }
                }
            }
            else                                            // empty negative slot
            {
                for (uint8 i = MAX_POSITIVE_AURAS; i < MAX_AURAS; i++)
                {
                    if (m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + i)) == 0)
                    {
                        slot = i;
                        break;
                    }
                }
            }

            SetAuraSlot( slot );

            // Not update fields for not first spell's aura, all data already in fields
            if(!secondaura)
            {
                if(slot < MAX_AURAS)                        // slot found
                {
                    SetAura(slot, false);
                    SetAuraFlag(slot, true);
                    SetAuraLevel(slot,caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));
                    UpdateAuraCharges();

                    // update for out of range group members
                    m_target->UpdateAuraForGroup(slot);
                }

                UpdateAuraDuration();
            }
        }
        else                                                // use found slot
        {
            SetAuraSlot( slot );
            // Not recalculate stack count for second aura of the same spell
            if (!secondaura)
                UpdateSlotCounterAndDuration(true);
        }

        // Update Seals information
        if( IsSealSpell(GetSpellProto()) )
            m_target->ModifyAuraState(AURA_STATE_JUDGEMENT, true);
    }
}

bool Aura::_RemoveAura()
{
    // Remove all triggered by aura spells vs unlimited duration
    // except same aura replace case
    if(m_removeMode!=AURA_REMOVE_BY_STACK)
        CleanupTriggeredSpells();

    Unit* caster = GetCaster();

    if(caster && IsPersistent())
    {
        DynamicObject *dynObj = caster->GetDynObject(GetId(), GetEffIndex());
        if (dynObj)
            dynObj->RemoveAffected(m_target);
    }

    // unregister aura
    if (getDiminishGroup() != DIMINISHING_NONE )
        m_target->ApplyDiminishingAura(getDiminishGroup(),false);

    //passive auras do not get put in slots
    // Note: but totem can be not accessible for aura target in time remove (to far for find in grid)
    //if(m_isPassive && !(caster && caster->GetTypeId() == TYPEID_UNIT && ((Creature*)caster)->isTotem()))
    //    return false;

    uint8 slot = GetAuraSlot();

    if(slot >= MAX_AURAS)                                   // slot not set
        return false;

    if(m_target->GetUInt32Value((uint16)(UNIT_FIELD_AURA + slot)) == 0)
        return false;

    bool samespell = false;
    bool sameaura = false;

    // find other aura in same slot (current already removed from list)
    for(int i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        Unit::spellEffectPair spair = Unit::spellEffectPair(GetId(), SpellEffectIndex(i));
        for(Unit::AuraMap::const_iterator itr = m_target->GetAuras().lower_bound(spair); itr != m_target->GetAuras().upper_bound(spair); ++itr)
        {
            if(itr->second->GetAuraSlot() == slot)
            {
                samespell = true;

                if(GetEffIndex()==i)
                    sameaura = true;

                break;
            }
        }
        if(samespell)
            break;
    }

    // only remove icon when the last aura of the spell is removed (current aura already removed from list)
    if (samespell)
    {
        if(sameaura)                                       // decrease count for spell, only for same aura effect, or this spell auras in remove process.
            UpdateSlotCounterAndDuration(false);

        return false;
    }

    SetAura(slot, true);
    SetAuraFlag(slot, false);
    SetAuraLevel(slot,caster ? caster->getLevel() : sWorld.getConfig(CONFIG_UINT32_MAX_PLAYER_LEVEL));

    SetAuraApplication(slot, 0);

    if (m_removeMode != AURA_REMOVE_BY_DELETE)
    {
        // update for out of range group members
        m_target->UpdateAuraForGroup(slot);

        //*****************************************************
        // Update target aura state flag (at last aura remove)
        //*****************************************************
        uint32 removeState = 0;
        uint64 removeFamilyFlag = m_spellProto->SpellFamilyFlags;
        switch(m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_PALADIN:
                if (IsSealSpell(m_spellProto))
                    removeState = AURA_STATE_JUDGEMENT;     // Update Seals information
                break;
        }

        // Remove state (but need check other auras for it)
        if (removeState)
        {
            bool found = false;
            Unit::AuraMap& Auras = m_target->GetAuras();
            for(Unit::AuraMap::iterator i = Auras.begin(); i != Auras.end(); ++i)
            {
                SpellEntry const *auraSpellInfo = (*i).second->GetSpellProto();
                if(auraSpellInfo->SpellFamilyName  == m_spellProto->SpellFamilyName &&
                    auraSpellInfo->SpellFamilyFlags & removeFamilyFlag)
                {
                    found = true;
                    break;
                }
            }

            // this has been last aura
            if(!found)
                m_target->ModifyAuraState(AuraState(removeState), false);
        }

        // reset cooldown state for spells
        if(caster && caster->GetTypeId() == TYPEID_PLAYER)
        {
            if ( GetSpellProto()->Attributes & SPELL_ATTR_DISABLED_WHILE_ACTIVE )
                // note: item based cooldowns and cooldown spell mods with charges ignored (unknown existed cases)
                ((Player*)caster)->SendCooldownEvent(GetSpellProto());
        }
    }

    return true;
}

void Aura::SetAuraFlag(uint32 slot, bool add)
{
    uint32 index    = slot >> 3;
    uint32 byte     = (slot & 7) << 2;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAFLAGS + index);
    if (add)
        val |= ((uint32)AFLAG_MASK << byte);
    else
        val &= ~((uint32)AFLAG_MASK << byte);

    m_target->SetUInt32Value(UNIT_FIELD_AURAFLAGS + index, val);
}

void Aura::SetAuraLevel(uint32 slot,uint32 level)
{
    uint32 index    = slot / 4;
    uint32 byte     = (slot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURALEVELS + index);
    val &= ~(0xFF << byte);
    val |= (level << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURALEVELS + index, val);
}

void Aura::SetAuraApplication(uint32 slot, int8 count)
{
    uint32 index    = slot / 4;
    uint32 byte     = (slot % 4) * 8;
    uint32 val      = m_target->GetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index);
    val &= ~(0xFF << byte);
    val |= ((uint8(count)) << byte);
    m_target->SetUInt32Value(UNIT_FIELD_AURAAPPLICATIONS + index, val);
}

void Aura::UpdateSlotCounterAndDuration(bool add)
{
    uint8 slot = GetAuraSlot();
    if(slot >= MAX_AURAS)
        return;

    // calculate amount of similar auras by same effect index (similar different spells)
    int8 count = 0;

    // calculate auras and update durations in case aura adding
    Unit::AuraList const& aura_list = m_target->GetAurasByType(GetModifier()->m_auraname);
    for(Unit::AuraList::const_iterator i = aura_list.begin();i != aura_list.end(); ++i)
    {
        if( (*i)->GetId()==GetId() && (*i)->GetEffIndex()==m_effIndex &&
            (*i)->GetCasterGUID()==GetCasterGUID() )
        {
            ++count;

            if(add)
                (*i)->SetAuraDuration(GetAuraDuration());
        }
    }

    // at aura add aura not added yet, at aura remove aura already removed
    // in field stored (count-1)
    if(!add)
        --count;

    SetAuraApplication(slot, count);

    UpdateAuraDuration();
}

bool Aura::isAffectedOnSpell(SpellEntry const *spell) const
{
    if (m_spellmod)
        return m_spellmod->isAffectedOnSpell(spell);

    // Check family name
    if (spell->SpellFamilyName != m_spellProto->SpellFamilyName)
        return false;

    uint64 mask = sSpellMgr.GetSpellAffectMask(GetId(),GetEffIndex());
    return (mask & spell->SpellFamilyFlags);
}

void Aura::ReapplyAffectedPassiveAuras( Unit* target )
{
    std::set<uint32> affectedPassives;

    for(Unit::AuraMap::const_iterator itr = target->GetAuras().begin(); itr != target->GetAuras().end(); ++itr)
    {
        // permanent passive
        if (itr->second->IsPassive() && itr->second->IsPermanent() &&
            // non deleted and not same aura (any with same spell id)
            !itr->second->IsDeleted() && itr->second->GetId() != GetId() &&
            // only applied by self
            itr->second->GetCasterGUID() == target->GetGUID() &&
            // and affected by spellmod
            isAffectedOnSpell(itr->second->GetSpellProto()))
        {
            affectedPassives.insert(itr->second->GetId());
        }
    }

    for(std::set<uint32>::const_iterator set_itr = affectedPassives.begin(); set_itr != affectedPassives.end(); ++set_itr)
    {
        target->RemoveAurasDueToSpell(*set_itr);
        target->CastSpell(m_target, *set_itr, true);
    }
}

struct ReapplyAffectedPassiveAurasHelper
{
    explicit ReapplyAffectedPassiveAurasHelper(Aura* _aura) : aura(_aura) {}
    void operator()(Unit* unit) const { aura->ReapplyAffectedPassiveAuras(unit); }
    Aura* aura;
};

void Aura::ReapplyAffectedPassiveAuras()
{
    // not reapply spell mods with charges (use original value because processed and at remove)
    if (m_spellProto->procCharges)
        return;

    // not reapply some spell mods ops (mostly speedup case)
    switch (m_modifier.m_miscvalue)
    {
        case SPELLMOD_DURATION:
        case SPELLMOD_CHARGES:
        case SPELLMOD_NOT_LOSE_CASTING_TIME:
        case SPELLMOD_CASTING_TIME:
        case SPELLMOD_COOLDOWN:
        case SPELLMOD_COST:
        case SPELLMOD_ACTIVATION_TIME:
        case SPELLMOD_CASTING_TIME_OLD:
            return;
    }

    // reapply talents to own passive persistent auras
    ReapplyAffectedPassiveAuras(m_target);

    // re-apply talents/passives/area auras applied to pet/totems (it affected by player spellmods)
    m_target->CallForAllControlledUnits(ReapplyAffectedPassiveAurasHelper(this),true,false,false);
}

bool Aura::isWeaponBuffCoexistableWith(Aura *ref)
{
    // Exclude Debuffs
    if (!IsPositive())
        return false;

    // Exclude Non-generic Buffs
    if (GetSpellProto()->SpellFamilyName != SPELLFAMILY_GENERIC)
        return false;

    // Exclude Stackable Buffs [ie: Blood Reserve]
    if (GetSpellProto()->StackAmount)
        return false;

    // only self applied player buffs
    if (m_target->GetTypeId() != TYPEID_PLAYER || m_target->GetGUID() != GetCasterGUID())
        return false;

    Item* castItem = ((Player*)m_target)->GetItemByGuid(GetCastItemGUID());
    if (!castItem)
        return false;

    // Limit to Weapon-Slots
    if (!castItem->IsEquipped() ||
        (castItem->GetSlot() != EQUIPMENT_SLOT_MAINHAND && castItem->GetSlot() != EQUIPMENT_SLOT_OFFHAND))
        return false;

    // form different weapons
    return ref->GetCastItemGUID() != GetCastItemGUID();
}

/*********************************************************/
/***               BASIC AURA FUNCTION                 ***/
/*********************************************************/
void Aura::HandleAddModifier(bool apply, bool Real)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER || !Real)
        return;

    SpellEntry const *spellInfo = GetSpellProto();
    if(!spellInfo)
        return;

    if(m_modifier.m_miscvalue >= MAX_SPELLMOD)
        return;

    if (apply)
    {
        // Add custom charges for some mod aura
        switch (m_spellProto->Id)
        {
            case 17941:    // Shadow Trance
            case 22008:    // Netherwind Focus
            case 34936:    // Backlash
                m_procCharges = 1;
                break;
        }

        m_spellmod = new SpellModifier(
            SpellModOp(m_modifier.m_miscvalue),
            SpellModType(m_modifier.m_auraname),            // SpellModType value == spell aura types
            m_modifier.m_amount,
            this,
            m_procCharges > 0 ? m_procCharges : 0);
    }

    // unapply spell mod (including deleting m_spellmod)
    ((Player*)m_target)->AddSpellMod(m_spellmod, apply);

    ReapplyAffectedPassiveAuras();
}

void Aura::TriggerSpell()
{
    const uint64& casterGUID = GetCasterGUID();
    Unit* target = GetTriggerTarget();

    if(!casterGUID || !target)
        return;

    // generic casting code with custom spells and target/caster customs
    uint32 trigger_spell_id = GetSpellProto()->EffectTriggerSpell[m_effIndex];

    SpellEntry const *triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
    SpellEntry const *auraSpellInfo = GetSpellProto();
    uint32 auraId = auraSpellInfo->Id;

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == NULL)
    {
        switch(auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch(auraId)
                {
                    // Firestone Passive (1-5 ranks)
                    case 758:
                    case 17945:
                    case 17947:
                    case 17949:
                    case 27252:
                    {
                        if (target->GetTypeId() != TYPEID_PLAYER)
                            return;
                        Item* item = ((Player*)target)->GetWeaponForAttack(BASE_ATTACK);
                        if (!item)
                            return;
                        uint32 enchant_id = 0;
                        switch (GetId())
                        {
                             case   758: enchant_id = 1803; break;   // Rank 1
                             case 17945: enchant_id = 1823; break;   // Rank 2
                             case 17947: enchant_id = 1824; break;   // Rank 3
                             case 17949: enchant_id = 1825; break;   // Rank 4
                             case 27252: enchant_id = 2645; break;   // Rank 5
                             default:
                                 return;
                        }
                        // remove old enchanting before applying new
                        ((Player*)target)->ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,false);
                        item->SetEnchantment(TEMP_ENCHANTMENT_SLOT, enchant_id, m_modifier.periodictime+1000, 0);
                        // add new enchanting
                        ((Player*)target)->ApplyEnchantment(item,TEMP_ENCHANTMENT_SLOT,true);
                        return;
                    }
//                    // Periodic Mana Burn
//                    case 812: break;
//                    // Polymorphic Ray
//                    case 6965: break;
//                    // Fire Nova (1-7 ranks)
//                    case 8350:
//                    case 8508:
//                    case 8509:
//                    case 11312:
//                    case 11313:
//                    case 25540:
//                    case 25544:
//                        break;
                    // Thaumaturgy Channel
                    case 9712: trigger_spell_id = 21029; break;
//                    // Egan's Blaster
//                    case 17368: break;
//                    // Haunted
//                    case 18347: break;
//                    // Ranshalla Waiting
//                    case 18953: break;
//                    // Inferno
//                    case 19695: break;
//                    // Frostwolf Muzzle DND
//                    case 21794: break;
//                    // Alterac Ram Collar DND
//                    case 21866: break;
//                    // Celebras Waiting
//                    case 21916: break;
                    // Brood Affliction: Bronze
                    case 23170:
                    {
                        m_target->CastSpell(m_target, 23171, true, NULL, this);
                        return;
                    }
//                    // Mark of Frost
//                    case 23184: break;
                    // Restoration
                    case 23493:
                    {
                        int32 heal = target->GetMaxHealth() / 10;
                        target->DealHeal(target, heal, auraSpellInfo);

                        if (int32 mana = target->GetMaxPower(POWER_MANA))
                        {
                            mana /= 10;
                            target->ModifyPower(POWER_MANA, mana);
                            target->SendEnergizeSpellLog(target, 23493, mana, POWER_MANA);
                        }
                        break;
                    }
//                    // Stoneclaw Totem Passive TEST
//                    case 23792: break;
//                    // Axe Flurry
//                    case 24018: break;
//                    // Mark of Arlokk
//                    case 24210: break;
//                    // Restoration
//                    case 24379: break;
//                    // Happy Pet
//                    case 24716: break;
//                    // Dream Fog
//                    case 24780: break;
//                    // Cannon Prep
//                    case 24832: break;
                    // Shadow Bolt Whirl
                    case 24834:
                    {
                        uint32 spellForTick[8] = { 24820, 24821, 24822, 24823, 24835, 24836, 24837, 24838 };
                        uint32 tick = GetAuraTicks();
                        if(tick < 8)
                        {
                            trigger_spell_id = spellForTick[tick];

                            // casted in left/right (but triggered spell have wide forward cone)
                            float forward = m_target->GetOrientation();
                            float angle = m_target->GetOrientation() + ( tick % 2 == 0 ? M_PI_F / 2 : - M_PI_F / 2);
                            m_target->SetOrientation(angle);
                            target->CastSpell(target, trigger_spell_id, true, NULL, this, casterGUID);
                            m_target->SetOrientation(forward);
                        }
                        return;
                    }
//                    // Stink Trap
//                    case 24918: break;
//                    // Mark of Nature
//                    case 25041: break;
//                    // Agro Drones
//                    case 25152: break;
                    // Consume
                    case 25371:
                    {
                        int32 bpDamage = target->GetMaxHealth()*10/100;
                        target->CastCustomSpell(target, 25373, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
                        return;
                    }
//                    // Pain Spike
//                    case 25572: break;
//                    // Rotate 360
//                    case 26009: break;
//                    // Rotate -360
//                    case 26136: break;
//                    // Consume
//                    case 26196: break;
//                    // Berserk
//                    case 26615: break;
//                    // Defile
//                    case 27177: break;
//                    // Teleport: IF/UC
//                    case 27601: break;
//                    // Five Fat Finger Exploding Heart Technique
//                    case 27673: break;
//                    // Nitrous Boost
//                    case 27746: break;
//                    // Steam Tank Passive
//                    case 27747: break;
                    // Frost Blast
                    case 27808:
                    {
                        int32 bpDamage = target->GetMaxHealth()*26/100;
                        target->CastCustomSpell(target, 29879, &bpDamage, NULL, NULL, true, NULL, this, casterGUID);
                        return;
                    }
//                    // Detonate Mana
//                    case 27819: break;
//                    // Controller Timer
//                    case 28095: break;
//                    // Stalagg Chain
//                    case 28096: break;
//                    // Stalagg Tesla Passive
//                    case 28097: break;
//                    // Feugen Tesla Passive
//                    case 28109: break;
//                    // Feugen Chain
//                    case 28111: break;
//                    // Mark of Didier
//                    case 28114: break;
//                    // Communique Timer, camp
//                    case 28346: break;
//                    // Icebolt
//                    case 28522: break;
//                    // Silithyst
//                    case 29519: break;
                    // Inoculate Nestlewood Owlkin
                    case 29528:
                        if(target->GetTypeId() != TYPEID_UNIT)// prevent error reports in case ignored player target
                            return;
                        break;
//                    // Overload
//                    case 29768: break;
//                    // Return Fire
//                    case 29788: break;
//                    // Return Fire
//                    case 29793: break;
//                    // Return Fire
//                    case 29794: break;
//                    // Guardian of Icecrown Passive
//                    case 29897: break;
                    // Feed Captured Animal
                    case 29917: trigger_spell_id = 29916; break;
//                    // Flame Wreath
//                    case 29946: break;
//                    // Flame Wreath
//                    case 29947: break;
//                    // Mind Exhaustion Passive
//                    case 30025: break;
//                    // Nether Beam - Serenity
//                    case 30401: break;
                    // Extract Gas
                    case 30427:
                    {
                        Unit* caster = GetCaster();
                        if (!caster)
                            return;
                        // move loot to player inventory and despawn target
                        if(caster->GetTypeId() ==TYPEID_PLAYER &&
                           target->GetTypeId() == TYPEID_UNIT &&
                           ((Creature*)target)->GetCreatureInfo()->type == CREATURE_TYPE_GAS_CLOUD)
                        {
                            Player* player = (Player*)caster;
                            Creature* creature = (Creature*)target;
                            // missing lootid has been reported on startup - just return
                            if (!creature->GetCreatureInfo()->SkinLootId)
                                return;

                            player->AutoStoreLoot(creature->GetCreatureInfo()->SkinLootId,LootTemplates_Skinning,true);

                            creature->ForcedDespawn();
                        }
                        return;
                        break;
                    }
                    // Quake
                    case 30576: trigger_spell_id = 30571; break;
//                    // Burning Maul
//                    case 30598: break;
//                    // Regeneration
//                    case 30799:
//                    case 30800:
//                    case 30801:
//                        break;
//                    // Despawn Self - Smoke cloud
//                    case 31269: break;
//                    // Time Rift Periodic
//                    case 31320: break;
//                    // Corrupt Medivh
//                    case 31326: break;
                    // Doom
                    case 31347:
                    {
                        m_target->CastSpell(m_target,31350,true);
                        m_target->DealDamage(m_target, m_target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                        return;
                    }
                    // Spellcloth
                    case 31373:
                    {
                        // Summon Elemental after create item
                        target->SummonCreature(17870, 0, 0, 0, target->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                        return;
                    }
//                    // Bloodmyst Tesla
//                    case 31611: break;
                    // Doomfire
                    case 31944:
                    {
                        int32 damage = m_modifier.m_amount * ((GetAuraDuration() + m_modifier.periodictime) / GetAuraMaxDuration());
                        target->CastCustomSpell(target, 31969, &damage, NULL, NULL, true, NULL, this, casterGUID);
                        return;
                    }
//                    // Teleport Test
//                    case 32236: break;
//                    // Earthquake
//                    case 32686: break;
                    default:
                        break;
                }
                break;
            }
            case SPELLFAMILY_MAGE:
            {
                switch(auraId)
                {
                    // Invisibility
                    case 66:
                    {
                        if(!m_duration)
                            m_target->CastSpell(m_target, 32612, true, NULL, this);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }
//            case SPELLFAMILY_WARRIOR:
//            {
//                switch(auraId)
//                {
//                    // Wild Magic
//                    case 23410: break;
//                    // Corrupted Totems
//                    case 23425: break;
//                    default:
//                        break;
//                }
//                break;
//            }
//            case SPELLFAMILY_PRIEST:
//            {
//                switch(auraId)
//                {
//                    // Blue Beam
//                    case 32930: break;
//                    default:
//                        break;
//                }
 //               break;
 //           }
            case SPELLFAMILY_DRUID:
            {
                switch(auraId)
                {
                    // Cat Form
                    // trigger_spell_id not set and unknown effect triggered in this case, ignoring for while
                    case 768:
                        return;
                    // Frenzied Regeneration
                    case 22842:
                    case 22895:
                    case 22896:
                    case 26999:
                    {
                        int32 LifePerRage = GetModifier()->m_amount;

                        int32 lRage = m_target->GetPower(POWER_RAGE);
                        if(lRage > 100)                                     // rage stored as rage*10
                            lRage = 100;
                        m_target->ModifyPower(POWER_RAGE, -lRage);
                        int32 FRTriggerBasePoints = int32(lRage*LifePerRage/10);
                        m_target->CastCustomSpell(m_target, 22845, &FRTriggerBasePoints, NULL, NULL, true, NULL, this);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }

//            case SPELLFAMILY_HUNTER:
//            {
//                switch(auraId)
//                {
//                    //Frost Trap Aura
//                    case 13810:
//                        return;
//                    // Tame spells
//                    case 19597:         // Tame Ice Claw Bear
//                    case 19676:         // Tame Snow Leopard
//                    case 19677:         // Tame Large Crag Boar
//                    case 19678:         // Tame Adult Plainstrider
//                    case 19679:         // Tame Prairie Stalker
//                    case 19680:         // Tame Swoop
//                    case 19681:         // Tame Dire Mottled Boar
//                    case 19682:         // Tame Surf Crawler
//                    case 19683:         // Tame Armored Scorpid
//                    case 19684:         // Tame Webwood Lurker
//                    case 19685:         // Tame Nightsaber Stalker
//                    case 19686:         // Tame Strigid Screecher
//                    case 30100:         // Tame Crazed Dragonhawk
//                    case 30103:         // Tame Elder Springpaw
//                    case 30104:         // Tame Mistbat
//                    case 30647:         // Tame Barbed Crawler
//                    case 30648:         // Tame Greater Timberstrider
//                    case 30652:         // Tame Nightstalker
//                        return;
//                    default:
//                        break;
//                }
//                break;
//            }
            case SPELLFAMILY_SHAMAN:
            {
                switch(auraId)
                {
                    // Lightning Shield (The Earthshatterer set trigger after cast Lighting Shield)
                    case 28820:
                    {
                        // Need remove self if Lightning Shield not active
                        Unit::AuraMap const& auras = target->GetAuras();
                        for(Unit::AuraMap::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                        {
                            SpellEntry const* spell = itr->second->GetSpellProto();
                            if( spell->SpellFamilyName == SPELLFAMILY_SHAMAN &&
                                (spell->SpellFamilyFlags & UI64LIT(0x0000000000000400)))
                                return;
                        }
                        target->RemoveAurasDueToSpell(28820);
                        return;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
        // Reget trigger spell proto
        triggeredSpellInfo = sSpellStore.LookupEntry(trigger_spell_id);
        if(triggeredSpellInfo == NULL)
        {
            sLog.outError("Aura::TriggerSpell: Spell %u have 0 in EffectTriggered[%d], not handled custom case?",GetId(),GetEffIndex());
            return;
        }
    }
    else
    {
        // Spell exist but require custom code
        switch(auraId)
        {
            // Curse of Idiocy
            case 1010:
            {
                // TODO: spell casted by result in correct way mostly
                // BUT:
                // 1) target show casting at each triggered cast: target don't must show casting animation for any triggered spell
                //      but must show affect apply like item casting
                // 2) maybe aura must be replace by new with accumulative stat mods instead stacking

                // prevent cast by triggered auras
                if(casterGUID == target->GetGUID())
                    return;

                // stop triggering after each affected stats lost > 90
                int32 intelectLoss = 0;
                int32 spiritLoss = 0;

                Unit::AuraList const& mModStat = target->GetAurasByType(SPELL_AURA_MOD_STAT);
                for(Unit::AuraList::const_iterator i = mModStat.begin(); i != mModStat.end(); ++i)
                {
                    if ((*i)->GetId() == 1010)
                    {
                        switch((*i)->GetModifier()->m_miscvalue)
                        {
                            case STAT_INTELLECT: intelectLoss += (*i)->GetModifier()->m_amount; break;
                            case STAT_SPIRIT:    spiritLoss   += (*i)->GetModifier()->m_amount; break;
                            default: break;
                        }
                    }
                }

                if(intelectLoss <= -90 && spiritLoss <= -90)
                    return;

                break;
            }
            // Mana Tide
            case 16191:
            {
                target->CastCustomSpell(target, trigger_spell_id, &m_modifier.m_amount, NULL, NULL, true, NULL, this);
                return;
            }
        }
    }
    // All ok cast by default case
    target->CastSpell(target, triggeredSpellInfo, true, NULL, this, casterGUID);
}

/*********************************************************/
/***                  AURA EFFECTS                     ***/
/*********************************************************/

void Aura::HandleAuraDummy(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if (!Real)
        return;

    // AT APPLY
    if (apply)
    {
        switch(m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch(GetId())
                {
                    case 7057:                                      // Haunting Spirits
                        // expected to tick with 30 sec period (tick part see in Aura::PeriodicTick)
                        m_isPeriodic = true;
                        m_modifier.periodictime = 30*IN_MILISECONDS;
                        m_periodicTimer = m_modifier.periodictime;
                        return;
                    case 13139:                                     // net-o-matic
                        // root to self part of (root_target->charge->root_self sequence
                        if (Unit* caster = GetCaster())
                            caster->CastSpell(caster, 13138, true, NULL, this);
                        return;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                // Earth Shield
                if (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x40000000000))
                {
                    // prevent double apply bonuses
                    if (m_target->GetTypeId() != TYPEID_PLAYER || !((Player*)m_target)->GetSession()->PlayerLoading())
                        if (Unit* caster = GetCaster())
                            m_modifier.m_amount = caster->SpellHealingBonus(GetSpellProto(), m_modifier.m_amount, SPELL_DIRECT_DAMAGE, m_target);
                    return;
                }
                break;
            }
        }
    }
    // AT REMOVE
    else
    {
        if (m_target->GetTypeId() == TYPEID_PLAYER &&
            (GetSpellProto()->Effect[EFFECT_INDEX_0] == 72 || GetSpellProto()->Effect[EFFECT_INDEX_0] == 6 &&
            (GetSpellProto()->EffectApplyAuraName[EFFECT_INDEX_0] == 1 || GetSpellProto()->EffectApplyAuraName[EFFECT_INDEX_0] == 128)))
        {
            // spells with SpellEffect=72 and aura=4: 6196, 6197, 21171, 21425
            ((Player*)m_target)->SetFarSightGUID(0);
            WorldPacket data(SMSG_CLEAR_FAR_SIGHT_IMMEDIATE, 0);
            ((Player*)m_target)->GetSession()->SendPacket(&data);
            return;
        }

        if (IsQuestTameSpell(GetId()) && m_target->isAlive())
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->isAlive())
                return;

            uint32 finalSpelId = 0;
            switch(GetId())
            {
                case 19548: finalSpelId = 19597; break;
                case 19674: finalSpelId = 19677; break;
                case 19687: finalSpelId = 19676; break;
                case 19688: finalSpelId = 19678; break;
                case 19689: finalSpelId = 19679; break;
                case 19692: finalSpelId = 19680; break;
                case 19693: finalSpelId = 19684; break;
                case 19694: finalSpelId = 19681; break;
                case 19696: finalSpelId = 19682; break;
                case 19697: finalSpelId = 19683; break;
                case 19699: finalSpelId = 19685; break;
                case 19700: finalSpelId = 19686; break;
                case 30646: finalSpelId = 30647; break;
                case 30653: finalSpelId = 30648; break;
                case 30654: finalSpelId = 30652; break;
                case 30099: finalSpelId = 30100; break;
                case 30102: finalSpelId = 30103; break;
                case 30105: finalSpelId = 30104; break;
            }

            if (finalSpelId)
                caster->CastSpell(m_target, finalSpelId, true, NULL, this);

            return;
        }

        switch(GetId())
        {
            case 12479:                                     // Hex of Jammal'an
                m_target->CastSpell(m_target, 12480, true, NULL, this);
                return;
            case 28169:                                     // Mutating Injection
            {
                // Mutagen Explosion
                m_target->CastSpell(m_target, 28206, true, NULL, this);
                // Poison Cloud
                m_target->CastSpell(m_target, 28240, true, NULL, this);
                return;
            }
        }
    }

    // AT APPLY & REMOVE

    switch(m_spellProto->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            switch(GetId())
            {
                case 24658:                                 // Unstable Power
                {
                    uint32 spellId = 24659;
                    if (apply)
                    {
                        SpellEntry const *spell = sSpellStore.LookupEntry(spellId);
                        Unit* caster = GetCaster();
                        if (!spell || !caster)
                            return;

                        for (uint32 i = 0; i < spell->StackAmount; ++i)
                            caster->CastSpell(m_target, spellId, true, NULL, NULL, GetCasterGUID());
                        return;
                    }
                    m_target->RemoveAurasDueToSpell(spellId);
                    return;
                }
                case 24661:                                 // Restless Strength
                {
                    uint32 spellId = 24662;
                    if (apply)
                    {
                        SpellEntry const* spell = sSpellStore.LookupEntry(spellId);
                        Unit* caster = GetCaster();
                        if (!spell || !caster)
                            return;

                        for (uint32 i=0; i < spell->StackAmount; ++i)
                            caster->CastSpell(m_target, spell->Id, true, NULL, NULL, GetCasterGUID());

                        return;
                    }
                    m_target->RemoveAurasDueToSpell(spellId);
                    return;
                }
                case 29266:                                 // Permanent Feign Death
                {
                    // Unclear what the difference really is between them.
                    // Some has effect1 that makes the difference, however not all.
                    // Some appear to be used depending on creature location, in water, at solid ground, in air/suspended, etc
                    // For now, just handle all the same way
                    if (m_target->GetTypeId() == TYPEID_UNIT)
                        m_target->SetFeignDeath(apply);

                    return;
                }
                case 27978:
                    if (apply)
                        m_target->m_AuraFlags |= UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    else
                        m_target->m_AuraFlags |= ~UNIT_AURAFLAG_ALIVE_INVISIBLE;
                    return;
            }
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            // Predatory Strikes
            if (m_target->GetTypeId()==TYPEID_PLAYER && GetSpellProto()->SpellIconID == 1563)
            {
                ((Player*)m_target)->UpdateAttackPowerAndDamage();
                return;
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            // Improved Weapon Totems
            if( GetSpellProto()->SpellIconID == 57 && m_target->GetTypeId()==TYPEID_PLAYER )
            {
                if(apply)
                {
                    switch (m_effIndex)
                    {
                        case 0:
                            // Windfury Totem
                            m_spellmod = new SpellModifier(SPELLMOD_EFFECT1,SPELLMOD_PCT,m_modifier.m_amount,GetId(),UI64LIT(0x00200000000));
                            break;
                        case 1:
                            // Flametongue Totem
                            m_spellmod = new SpellModifier(SPELLMOD_EFFECT1,SPELLMOD_PCT,m_modifier.m_amount,GetId(),UI64LIT(0x00400000000));
                            break;
                        default: return;
                    }
                }

                ((Player*)m_target)->AddSpellMod(m_spellmod, apply);
                return;
            }
            break;
        }
    }

    // pet auras
    if (PetAura const* petSpell = sSpellMgr.GetPetAura(GetId()))
    {
        if (apply)
            m_target->AddPetAura(petSpell);
        else
            m_target->RemovePetAura(petSpell);
        return;
    }

    if (GetEffIndex() == EFFECT_INDEX_0 && m_target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellAreaForAreaMapBounds saBounds = sSpellMgr.GetSpellAreaForAuraMapBounds(GetId());
        if (saBounds.first != saBounds.second)
        {
            uint32 zone, area;
            m_target->GetZoneAndAreaId(zone, area);

            for(SpellAreaForAreaMap::const_iterator itr = saBounds.first; itr != saBounds.second; ++itr)
            {
                // some auras remove at aura remove
                if (!itr->second->IsFitToRequirements((Player*)m_target, zone, area))
                    m_target->RemoveAurasDueToSpell(itr->second->spellId);
                // some auras applied at aura apply
                else if (itr->second->autocast)
                {
                    if (!m_target->HasAura(itr->second->spellId, EFFECT_INDEX_0))
                        m_target->CastSpell(m_target, itr->second->spellId, true);
                }
            }
        }
    }

    // script has to "handle with care", only use where data are not ok to use in the above code.
    if (m_target->GetTypeId() == TYPEID_UNIT)
        Script->EffectAuraDummy(this, apply);
}

void Aura::HandleAuraMounted(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    if(apply)
    {
        CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
        if(!ci)
        {
            sLog.outErrorDb("AuraMounted: `creature_template`='%u' not found in database (only need it modelid)", m_modifier.m_miscvalue);
            return;
        }

        uint32 team = 0;
        if (m_target->GetTypeId()==TYPEID_PLAYER)
            team = ((Player*)m_target)->GetTeam();

        uint32 display_id = sObjectMgr.ChooseDisplayId(team,ci);
        CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
        if (minfo)
            display_id = minfo->modelid;

        m_target->Mount(display_id);
    }
    else
    {
        m_target->Unmount();
    }
}

void Aura::HandleAuraWaterWalk(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_WATER_WALK, 8+4);
    else
        data.Initialize(SMSG_MOVE_LAND_WALK, 8+4);
    data << m_target->GetPackGUID();
    data << uint32(0);
    m_target->SendMessageToSet(&data, true);
}

void Aura::HandleAuraFeatherFall(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_FEATHER_FALL, 8+4);
    else
        data.Initialize(SMSG_MOVE_NORMAL_FALL, 8+4);
    data << m_target->GetPackGUID();
    data << uint32(0);
    m_target->SendMessageToSet(&data, true);

    // start fall from current height
    if(!apply && m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->SetFallInformation(0, m_target->GetPositionZ());
}

void Aura::HandleAuraHover(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    WorldPacket data;
    if(apply)
        data.Initialize(SMSG_MOVE_SET_HOVER, 8+4);
    else
        data.Initialize(SMSG_MOVE_UNSET_HOVER, 8+4);
    data << m_target->GetPackGUID();
    data << uint32(0);
    m_target->SendMessageToSet(&data, true);
}

void Aura::HandleWaterBreathing(bool /*apply*/, bool /*Real*/)
{
    // update timers in client
    if(m_target->GetTypeId()==TYPEID_PLAYER)
        ((Player*)m_target)->UpdateMirrorTimers();
}

void Aura::HandleAuraModShapeshift(bool apply, bool Real)
{
    if(!Real)
        return;

    uint32 modelid = 0;
    Powers PowerType = POWER_MANA;
    ShapeshiftForm form = ShapeshiftForm(m_modifier.m_miscvalue);
    switch(form)
    {
        case FORM_CAT:
            if(Player::TeamForRace(m_target->getRace()) == ALLIANCE)
                modelid = 892;
            else
                modelid = 8571;
            PowerType = POWER_ENERGY;
            break;
        case FORM_TRAVEL:
            modelid = 632;
            break;
        case FORM_AQUA:
            if(Player::TeamForRace(m_target->getRace()) == ALLIANCE)
                modelid = 2428;
            else
                modelid = 2428;
            break;
        case FORM_BEAR:
            if(Player::TeamForRace(m_target->getRace()) == ALLIANCE)
                modelid = 2281;
            else
                modelid = 2289;
            PowerType = POWER_RAGE;
            break;
        case FORM_GHOUL:
            if(Player::TeamForRace(m_target->getRace()) == ALLIANCE)
                modelid = 10045;
            break;
        case FORM_DIREBEAR:
            if(Player::TeamForRace(m_target->getRace()) == ALLIANCE)
                modelid = 2281;
            else
                modelid = 2289;
            PowerType = POWER_RAGE;
            break;
        case FORM_CREATUREBEAR:
            modelid = 902;
            break;
        case FORM_GHOSTWOLF:
            modelid = 4613;
            break;
        case FORM_MOONKIN:
            if(Player::TeamForRace(m_target->getRace()) == ALLIANCE)
                modelid = 15374;
            else
                modelid = 15375;
            break;
        case FORM_AMBIENT:
        case FORM_SHADOW:
        case FORM_STEALTH:
            break;
        case FORM_TREE:
            modelid = 864;
            break;
        case FORM_BATTLESTANCE:
        case FORM_BERSERKERSTANCE:
        case FORM_DEFENSIVESTANCE:
            PowerType = POWER_RAGE;
            break;
        case FORM_SPIRITOFREDEMPTION:
            modelid = 16031;
            break;
        default:
            sLog.outError("Auras: Unknown Shapeshift Type: %u", m_modifier.m_miscvalue);
    }

    // remove polymorph before changing display id to keep new display id
    switch ( form )
    {
        case FORM_CAT:
        case FORM_TREE:
        case FORM_TRAVEL:
        case FORM_AQUA:
        case FORM_BEAR:
        case FORM_DIREBEAR:
        case FORM_MOONKIN:
        {
            // remove movement affects
            m_target->RemoveSpellsCausingAura(SPELL_AURA_MOD_ROOT);
            Unit::AuraList const& slowingAuras = m_target->GetAurasByType(SPELL_AURA_MOD_DECREASE_SPEED);
            for (Unit::AuraList::const_iterator iter = slowingAuras.begin(); iter != slowingAuras.end();)
            {
                SpellEntry const* aurSpellInfo = (*iter)->GetSpellProto();

                uint32 aurMechMask = GetAllSpellMechanicMask(aurSpellInfo);

                // If spell that caused this aura has Croud Control or Daze effect
                if((aurMechMask & MECHANIC_NOT_REMOVED_BY_SHAPESHIFT) ||
                    // some Daze spells have these parameters instead of MECHANIC_DAZE (skip snare spells)
                    aurSpellInfo->SpellIconID == 15 && aurSpellInfo->Dispel == 0 &&
                    (aurMechMask & (1 << (MECHANIC_SNARE-1)))==0)
                {
                    ++iter;
                    continue;
                }

                // All OK, remove aura now
                m_target->RemoveAurasDueToSpellByCancel(aurSpellInfo->Id);
                iter = slowingAuras.begin();
            }

            // and polymorphic affects
            if(m_target->IsPolymorphed())
                m_target->RemoveAurasDueToSpell(m_target->getTransForm());

            break;
        }
        default:
           break;
    }

    if(apply)
    {
        // remove other shapeshift before applying a new one
        if(m_target->m_ShapeShiftFormSpellId)
            m_target->RemoveAurasDueToSpell(m_target->m_ShapeShiftFormSpellId, this);

        m_target->SetByteValue(UNIT_FIELD_BYTES_1, 2, form);

        if(modelid > 0)
            m_target->SetDisplayId(modelid);

        if(PowerType != POWER_MANA)
        {
            // reset power to default values only at power change
            if(m_target->getPowerType() != PowerType)
                m_target->setPowerType(PowerType);

            switch(form)
            {
                case FORM_CAT:
                case FORM_BEAR:
                case FORM_DIREBEAR:
                {
                    // get furor proc chance
                    int32 furorChance = 0;
                    Unit::AuraList const& mDummy = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                    for(Unit::AuraList::const_iterator i = mDummy.begin(); i != mDummy.end(); ++i)
                    {
                        if ((*i)->GetSpellProto()->SpellIconID == 238)
                        {
                            furorChance = (*i)->GetModifier()->m_amount;
                            break;
                        }
                    }

                    if (m_modifier.m_miscvalue == FORM_CAT)
                    {
                        m_target->SetPower(POWER_ENERGY, 0);
                        if(irand(1,100) <= furorChance)
                            m_target->CastSpell(m_target, 17099, true, NULL, this);
                    }
                    else
                    {
                        m_target->SetPower(POWER_RAGE, 0);
                        if(irand(1,100) <= furorChance)
                            m_target->CastSpell(m_target, 17057, true, NULL, this);
                    }
                    break;
                }
                case FORM_BATTLESTANCE:
                case FORM_DEFENSIVESTANCE:
                case FORM_BERSERKERSTANCE:
                {
                    uint32 Rage_val = 0;
                    //Tactical mastery
                    if(m_target->GetTypeId() == TYPEID_PLAYER)
                    {
                        Unit::AuraList const& aurasOverrideClassScripts = m_target->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                        for(Unit::AuraList::const_iterator iter = aurasOverrideClassScripts.begin(); iter != aurasOverrideClassScripts.end(); ++iter)
                        {
                            // select by script id
                            switch((*iter)->GetModifier()->m_miscvalue)
                            {
                                case 831: Rage_val =  50; break;
                                case 832: Rage_val = 100; break;
                                case 833: Rage_val = 150; break;
                                case 834: Rage_val = 200; break;
                                case 835: Rage_val = 250; break;
                            }
                            if(Rage_val!=0)
                                break;
                        }
                    }
                    if (m_target->GetPower(POWER_RAGE) > Rage_val)
                        m_target->SetPower(POWER_RAGE, Rage_val);
                    break;
                }
                default:
                    break;
            }
        }

        m_target->m_ShapeShiftFormSpellId = GetId();
        m_target->m_form = form;
    }
    else
    {
        if(modelid > 0)
            m_target->SetDisplayId(m_target->GetNativeDisplayId());
        m_target->SetByteValue(UNIT_FIELD_BYTES_1, 2, FORM_NONE);
        if(m_target->getClass() == CLASS_DRUID)
            m_target->setPowerType(POWER_MANA);
        m_target->m_ShapeShiftFormSpellId = 0;
        m_target->m_form = FORM_NONE;

        switch(form)
        {
            // Nordrassil Harness - bonus
            case FORM_BEAR:
            case FORM_DIREBEAR:
            case FORM_CAT:
                if(Aura* dummy = m_target->GetDummyAura(37315) )
                    m_target->CastSpell(m_target, 37316, true, NULL, dummy);
                break;
            // Nordrassil Regalia - bonus
            case FORM_MOONKIN:
                if(Aura* dummy = m_target->GetDummyAura(37324) )
                    m_target->CastSpell(m_target, 37325, true, NULL, dummy);
                break;
            default:
                break;
        }
    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(apply);

    if(m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->InitDataForForm();
}

void Aura::HandleAuraTransform(bool apply, bool Real)
{
    if (apply)
    {
        // special case (spell specific functionality)
        if (m_modifier.m_miscvalue == 0)
        {
            // player applied only
            if (m_target->GetTypeId() != TYPEID_PLAYER)
                return;

            switch (GetId())
            {
                // Orb of Deception
                case 16739:
                {
                    uint32 orb_model = m_target->GetNativeDisplayId();
                    switch(orb_model)
                    {
                        // Troll Female
                        case 1479: m_target->SetDisplayId(10134); break;
                        // Troll Male
                        case 1478: m_target->SetDisplayId(10135); break;
                        // Tauren Male
                        case 59:   m_target->SetDisplayId(10136); break;
                        // Human Male
                        case 49:   m_target->SetDisplayId(10137); break;
                        // Human Female
                        case 50:   m_target->SetDisplayId(10138); break;
                        // Orc Male
                        case 51:   m_target->SetDisplayId(10139); break;
                        // Orc Female
                        case 52:   m_target->SetDisplayId(10140); break;
                        // Dwarf Male
                        case 53:   m_target->SetDisplayId(10141); break;
                        // Dwarf Female
                        case 54:   m_target->SetDisplayId(10142); break;
                        // NightElf Male
                        case 55:   m_target->SetDisplayId(10143); break;
                        // NightElf Female
                        case 56:   m_target->SetDisplayId(10144); break;
                        // Undead Female
                        case 58:   m_target->SetDisplayId(10145); break;
                        // Undead Male
                        case 57:   m_target->SetDisplayId(10146); break;
                        // Tauren Female
                        case 60:   m_target->SetDisplayId(10147); break;
                        // Gnome Male
                        case 1563: m_target->SetDisplayId(10148); break;
                        // Gnome Female
                        case 1564: m_target->SetDisplayId(10149); break;
                        default: break;
                    }
                    break;
                }
                default: break;
            }
        }
        else
        {
            CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(m_modifier.m_miscvalue);
            if (!ci)
            {
                                                            //pig pink ^_^
                m_target->SetDisplayId(16358);
                sLog.outError("Auras: unknown creature id = %d (only need its modelid) Form Spell Aura Transform in Spell ID = %d", m_modifier.m_miscvalue, GetId());
            }
            else
            {
                                                            // Will use the default model here
                m_target->SetDisplayId(ci->DisplayID[0]);
            }
        }

        // update active transform spell only not set or not overwriting negative by positive case
        if (!m_target->getTransForm() || !IsPositiveSpell(GetId()) || IsPositiveSpell(m_target->getTransForm()))
            m_target->setTransForm(GetId());

        // polymorph case
        if (Real && m_target->GetTypeId() == TYPEID_PLAYER && m_target->IsPolymorphed())
        {
            // [-ZERO] maybe we wanna remove it
            // for players, start regeneration after 1s (in polymorph fast regeneration case)
            // only if caster is Player (after patch 2.4.2)
            if (IS_PLAYER_GUID(GetCasterGUID()) )
                ((Player*)m_target)->setRegenTimer(1*IN_MILISECONDS);

            //dismount polymorphed target (after patch 2.4.2)
            if (m_target->IsMounted())
                m_target->RemoveSpellsCausingAura(SPELL_AURA_MOUNTED);
        }
    }
    else
    {
        // ApplyModifier(true) will reapply it if need
        m_target->setTransForm(0);
        m_target->SetDisplayId(m_target->GetNativeDisplayId());

        // re-aplly some from still active with preference negative cases
        Unit::AuraList const& otherTransforms = m_target->GetAurasByType(SPELL_AURA_TRANSFORM);
        if (!otherTransforms.empty())
        {
            // look for other transform auras
            Aura* handledAura = *otherTransforms.begin();
            for(Unit::AuraList::const_iterator i = otherTransforms.begin();i != otherTransforms.end(); ++i)
            {
                // negative auras are preferred
                if (!IsPositiveSpell((*i)->GetSpellProto()->Id))
                {
                    handledAura = *i;
                    break;
                }
            }
            handledAura->ApplyModifier(true);
        }

        // Dragonmaw Illusion (restore mount model)
        if (GetId() == 42016 && m_target->GetMountID() == 16314)
        {
            if (!m_target->GetAurasByType(SPELL_AURA_MOUNTED).empty())
            {
                uint32 cr_id = m_target->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetModifier()->m_miscvalue;
                if (CreatureInfo const* ci = ObjectMgr::GetCreatureTemplate(cr_id))
                {
                    uint32 team = 0;
                    if (m_target->GetTypeId() == TYPEID_PLAYER)
                        team = ((Player*)m_target)->GetTeam();

                    uint32 display_id = sObjectMgr.ChooseDisplayId(team, ci);
                    CreatureModelInfo const *minfo = sObjectMgr.GetCreatureModelRandomGender(display_id);
                    if (minfo)
                        display_id = minfo->modelid;

                    m_target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, display_id);
                }
            }
        }
    }
}

void Aura::HandleForceReaction(bool apply, bool Real)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(!Real)
        return;

    Player* player = (Player*)m_target;

    uint32 faction_id = m_modifier.m_miscvalue;
    ReputationRank faction_rank = ReputationRank(m_modifier.m_amount);

    player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
    player->GetReputationMgr().SendForceReactions();

    // stop fighting if at apply forced rank friendly or at remove real rank friendly
    if (apply && faction_rank >= REP_FRIENDLY || !apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY)
        player->StopAttackFaction(faction_id);
}

void Aura::HandleAuraModSkill(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 prot=GetSpellProto()->EffectMiscValue[m_effIndex];
    int32 points = GetModifier()->m_amount;

    ((Player*)m_target)->ModifySkillBonus(prot, (apply ? points: -points), m_modifier.m_auraname == SPELL_AURA_MOD_SKILL_TALENT);
    if(prot == SKILL_DEFENSE)
        ((Player*)m_target)->UpdateDefenseBonusesMod();
}

void Aura::HandleChannelDeathItem(bool apply, bool Real)
{
    if(Real && !apply)
    {
        if(m_removeMode != AURA_REMOVE_BY_DEATH)
            return;

        SpellEntry const *spellInfo = GetSpellProto();
        if(spellInfo->EffectItemType[m_effIndex] == 0)
            return;

        Unit* victim = GetTarget();
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;

        // Soul Shard only from non-grey units
        if( spellInfo->EffectItemType[m_effIndex] == 6265 &&
            (victim->getLevel() <= MaNGOS::XP::GetGrayLevel(caster->getLevel()) ||
             victim->GetTypeId()==TYPEID_UNIT && !((Player*)caster)->isAllowedToLoot((Creature*)victim)) )
            return;
        ItemPosCountVec dest;
        uint8 msg = ((Player*)caster)->CanStoreNewItem( NULL_BAG, NULL_SLOT, dest, spellInfo->EffectItemType[m_effIndex], 1 );
        if( msg != EQUIP_ERR_OK )
        {
            ((Player*)caster)->SendEquipError( msg, NULL, NULL );
            return;
        }

        Item* newitem = ((Player*)caster)->StoreNewItem(dest, spellInfo->EffectItemType[m_effIndex], true);
        ((Player*)caster)->SendNewItem(newitem, 1, true, false);
    }
}

void Aura::HandleBindSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)caster)->SetFarSightGUID(apply ? m_target->GetGUID() : 0);
}

void Aura::HandleFarSight(bool apply, bool /*Real*/)
{
    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)caster)->SetFarSightGUID(apply ? m_target->GetGUID() : 0);
}

void Aura::HandleAuraTrackCreatures(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        m_target->RemoveNoStackAurasDueToAura(this);
    m_target->SetUInt32Value(PLAYER_TRACK_CREATURES, apply ? ((uint32)1)<<(m_modifier.m_miscvalue-1) : 0 );
}

void Aura::HandleAuraTrackResources(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        m_target->RemoveNoStackAurasDueToAura(this);
    m_target->SetUInt32Value(PLAYER_TRACK_RESOURCES, apply ? ((uint32)1)<<(m_modifier.m_miscvalue-1): 0 );
}

void Aura::HandleAuraTrackStealthed(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId()!=TYPEID_PLAYER)
        return;

    if(apply)
        m_target->RemoveNoStackAurasDueToAura(this);

    m_target->ApplyModFlag(PLAYER_FIELD_BYTES, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void Aura::HandleAuraModScale(bool apply, bool /*Real*/)
{
    m_target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, float(m_modifier.m_amount), apply);
}

void Aura::HandleModPossess(bool apply, bool Real)
{
    if(!Real)
        return;

    // not possess yourself
    if(GetCasterGUID() == m_target->GetGUID())
        return;

    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* p_caster = (Player*)caster;


    if( apply )
    {
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        m_target->SetCharmerGUID(p_caster->GetGUID());
        m_target->setFaction(p_caster->getFaction());

        p_caster->SetCharm(m_target);

        p_caster->SetFarSightGUID(m_target->GetGUID());
        p_caster->SetClientControl(m_target, 1);

        m_target->CombatStop();
        m_target->DeleteThreatList();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            m_target->StopMoving();
            m_target->GetMotionMaster()->Clear();
            m_target->GetMotionMaster()->MoveIdle();
            CharmInfo *charmInfo = ((Creature*)m_target)->InitCharmInfo(m_target);
            charmInfo->InitPossessCreateSpells();
        }

        p_caster->PossessSpellInitialize();
    }
    else
    {
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

        m_target->SetCharmerGUID(0);

        p_caster->InterruptSpell(CURRENT_CHANNELED_SPELL);  // the spell is not automatically canceled when interrupted, do it now

        if(m_target->GetTypeId() == TYPEID_PLAYER)
            ((Player*)m_target)->setFactionForRace(m_target->getRace());
        else if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            CreatureInfo const *cinfo = ((Creature*)m_target)->GetCreatureInfo();
            m_target->setFaction(cinfo->faction_A);
        }

        p_caster->SetCharm(NULL);

        p_caster->SetFarSightGUID(0);
        p_caster->SetClientControl(m_target, 0);

        p_caster->RemovePetActionBar();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)m_target)->AIM_Initialize();

            if (((Creature*)m_target)->AI())
                ((Creature*)m_target)->AI()->AttackedBy(caster);
        }
    }
    if(caster->GetTypeId() == TYPEID_PLAYER)
        ((Player*)caster)->SetFarSightGUID(apply ? m_target->GetGUID() : 0);
}

void Aura::HandleModPossessPet(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if(!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Pet *pet = caster->GetPet();
    if(!pet || pet != m_target)
        return;

    Player* p_caster = (Player*)caster;

    if(apply)
        pet->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);
    else
        pet->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED);

    p_caster->SetFarSightGUID(apply ? pet->GetGUID() : 0);
    p_caster->SetCharm(apply ? pet : NULL);
    p_caster->SetClientControl(pet, apply ? 1 : 0);

    if(apply)
    {
        pet->StopMoving();
        pet->GetMotionMaster()->Clear();
        pet->GetMotionMaster()->MoveIdle();
    }
    else
    {
        pet->AttackStop();
        pet->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        pet->AddSplineFlag(SPLINEFLAG_WALKMODE);
    }
}

void Aura::HandleModCharm(bool apply, bool Real)
{
    if(!Real)
        return;

    // not charm yourself
    if(GetCasterGUID() == m_target->GetGUID())
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    if( apply )
    {
        if (m_target->GetCharmerGUID())
        {
            m_target->RemoveSpellsCausingAura(SPELL_AURA_MOD_CHARM);
            m_target->RemoveSpellsCausingAura(SPELL_AURA_MOD_POSSESS);
        }

        m_target->SetCharmerGUID(GetCasterGUID());
        m_target->setFaction(caster->getFaction());
        m_target->CastStop(m_target == caster ? GetId() : 0);
        caster->SetCharm(m_target);

        m_target->CombatStop();
        m_target->DeleteThreatList();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)m_target)->AIM_Initialize();
            CharmInfo *charmInfo = m_target->InitCharmInfo(m_target);
            charmInfo->InitCharmCreateSpells();
            charmInfo->SetReactState( REACT_DEFENSIVE );

            if(caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK)
            {
                CreatureInfo const *cinfo = ((Creature*)m_target)->GetCreatureInfo();
                if(cinfo && cinfo->type == CREATURE_TYPE_DEMON)
                {
                    // creature with pet number expected have class set
                    if(m_target->GetByteValue(UNIT_FIELD_BYTES_0, 1)==0)
                    {
                        CreatureDataAddon const *cainfo = ((Creature*)m_target)->GetCreatureAddon();
                        if(!cainfo || (cainfo->bytes0 & 0x0000FF00) == 0)
                            sLog.outErrorDb("Creature (Entry: %u) not have creature addon or have bytes0 class byte = 0 but used in charmed spell, that will be result client crash.",cinfo->Entry);
                        else
                            sLog.outError("Creature (Entry: %u) have creature addon with bytes0 = %u but at charming have class 0!!! that will be result client crash.",cinfo->Entry,cainfo->bytes0);

                        m_target->SetByteValue(UNIT_FIELD_BYTES_0, 1, CLASS_MAGE);
                    }

                    //just to enable stat window
                    charmInfo->SetPetNumber(sObjectMgr.GeneratePetNumber(), true);
                    //if charmed two demons the same session, the 2nd gets the 1st one's name
                    m_target->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL)));
                }
            }
        }

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->CharmSpellInitialize();
    }
    else
    {
        m_target->SetCharmerGUID(0);

        if(m_target->GetTypeId() == TYPEID_PLAYER)
            ((Player*)m_target)->setFactionForRace(m_target->getRace());
        else
        {
            CreatureInfo const *cinfo = ((Creature*)m_target)->GetCreatureInfo();

            // restore faction
            if(((Creature*)m_target)->isPet())
            {
                if(Unit* owner = m_target->GetOwner())
                    m_target->setFaction(owner->getFaction());
                else if(cinfo)
                    m_target->setFaction(cinfo->faction_A);
            }
            else if(cinfo)                              // normal creature
                m_target->setFaction(cinfo->faction_A);

            // restore UNIT_FIELD_BYTES_0
            if(cinfo && caster->GetTypeId() == TYPEID_PLAYER && caster->getClass() == CLASS_WARLOCK && cinfo->type == CREATURE_TYPE_DEMON)
            {
                // DB must have proper class set in field at loading, not req. restore, including workaround case at apply
                //if(CreatureDataAddon const *cainfo = ((Creature*)m_target)->GetCreatureAddon())
                //    m_target->SetByteValue(UNIT_FIELD_BYTES_0, 1, (cainfo->bytes0 & 0x0000FF00) >> 8);

                if(m_target->GetCharmInfo())
                    m_target->GetCharmInfo()->SetPetNumber(0, true);
                else
                    sLog.outError("Aura::HandleModCharm: target (GUID: %u TypeId: %u) has a charm aura but no charm info!", m_target->GetGUIDLow(), m_target->GetTypeId());
            }
        }

        caster->SetCharm(NULL);

        if(caster->GetTypeId() == TYPEID_PLAYER)
            ((Player*)caster)->RemovePetActionBar();

        if(m_target->GetTypeId() == TYPEID_UNIT)
        {
            ((Creature*)m_target)->AIM_Initialize();
            if (((Creature*)m_target)->AI())
                ((Creature*)m_target)->AI()->AttackedBy(caster);
        }
    }
}

void Aura::HandleModConfuse(bool apply, bool Real)
{
    if(!Real)
        return;

    m_target->SetConfused(apply, GetCasterGUID(), GetId());
}

void Aura::HandleModFear(bool apply, bool Real)
{
    if (!Real)
        return;

    m_target->SetFeared(apply, GetCasterGUID(), GetId());
}

void Aura::HandleFeignDeath(bool apply, bool Real)
{
    if(!Real)
        return;

    m_target->SetFeignDeath(apply, GetCasterGUID(), GetId());
}

void Aura::HandleAuraModDisarm(bool apply, bool Real)
{
    if(!Real)
        return;

    if(!apply && m_target->HasAuraType(SPELL_AURA_MOD_DISARM))
        return;

    // not sure for it's correctness
    if(apply)
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED);
    else
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISARMED);

    // only at real add/remove aura
    if (m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // main-hand attack speed already set to special value for feral form already and don't must change and reset at remove.
    if (m_target->IsInFeralForm())
        return;

    if (apply)
        m_target->SetAttackTime(BASE_ATTACK,BASE_ATTACK_TIME);
    else
        ((Player *)m_target)->SetRegularAttackTime();

    m_target->UpdateDamagePhysical(BASE_ATTACK);
}

void Aura::HandleAuraModStun(bool apply, bool Real)
{
    if(!Real)
        return;

    if (apply)
    {
        m_target->addUnitState(UNIT_STAT_STUNNED);
        m_target->SetTargetGUID(0);

        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
        m_target->CastStop(m_target->GetGUID() == GetCasterGUID() ? GetId() : 0);

        // Creature specific
        if(m_target->GetTypeId() != TYPEID_PLAYER)
            m_target->StopMoving();
        else
        {
            ((Player*)m_target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
            m_target->SetStandState(UNIT_STAND_STATE_STAND);// in 1.5 client
        }

        WorldPacket data(SMSG_FORCE_MOVE_ROOT, 8);
        data << m_target->GetPackGUID();
        data << uint32(0);
        m_target->SendMessageToSet(&data, true);

        // Summon the Naj'entus Spine GameObject on target if spell is Impaling Spine
        if(GetId() == 39837)
        {
            GameObject* pObj = new GameObject;
            if(pObj->Create(sObjectMgr.GenerateLowGuid(HIGHGUID_GAMEOBJECT), 185584, m_target->GetMap(),
                m_target->GetPositionX(), m_target->GetPositionY(), m_target->GetPositionZ(), m_target->GetOrientation(), 0.0f, 0.0f, 0.0f, 0.0f, 100, GO_STATE_READY))
            {
                pObj->SetRespawnTime(GetAuraDuration()/IN_MILISECONDS);
                pObj->SetSpellId(GetId());
                m_target->AddGameObject(pObj);
                m_target->GetMap()->Add(pObj);
            }
            else
                delete pObj;
        }
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if(m_target->HasAuraType(SPELL_AURA_MOD_STUN))
            return;

        m_target->clearUnitState(UNIT_STAT_STUNNED);
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);

        if(!m_target->hasUnitState(UNIT_STAT_ROOT))         // prevent allow move if have also root effect
        {
            if(m_target->getVictim() && m_target->isAlive())
                m_target->SetTargetGUID(m_target->getVictim()->GetGUID() );

            WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 8+4);
            data << m_target->GetPackGUID();
            data << uint32(0);
            m_target->SendMessageToSet(&data, true);
        }

        // Wyvern Sting
        if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellIconID == 1721)
        {
            Unit* caster = GetCaster();
            if( !caster || caster->GetTypeId()!=TYPEID_PLAYER )
                return;

            uint32 spell_id = 0;

            switch(GetId())
            {
                case 19386: spell_id = 24131; break;
                case 24132: spell_id = 24134; break;
                case 24133: spell_id = 24135; break;
                case 27068: spell_id = 27069; break;
                default:
                    sLog.outError("Spell selection called for unexpected original spell %u, new spell for this spell family?",GetId());
                    return;
            }

            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spell_id);

            if(!spellInfo)
                return;

            caster->CastSpell(m_target,spellInfo,true,NULL,this);
            return;
        }
    }
}

void Aura::HandleModStealth(bool apply, bool Real)
{
    if (apply)
    {
        // drop flag at stealth in bg
         m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // only at real aura add
        if (Real)
        {
            m_target->SetStandFlags(UNIT_BYTE1_FLAGS_CREEP);
            if(m_target->GetTypeId()==TYPEID_PLAYER)
                m_target->SetFlag(PLAYER_FIELD_BYTES2, 0x2000);

            // apply only if not in GM invisibility (and overwrite invisibility state)
            if (m_target->GetVisibility()!=VISIBILITY_OFF)
            {
                m_target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                m_target->SetVisibility(VISIBILITY_GROUP_STEALTH);
            }

            // for RACE_NIGHTELF stealth
            if(m_target->GetTypeId()==TYPEID_PLAYER && GetId()==20580)
                m_target->CastSpell(m_target, 21009, true, NULL, this);

            // apply full stealth period bonuses only at first stealth aura in stack
            if(m_target->GetAurasByType(SPELL_AURA_MOD_STEALTH).size()<=1)
            {
                Unit::AuraList const& mDummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = mDummyAuras.begin();i != mDummyAuras.end(); ++i)
                {
                    // Master of Subtlety
                    if ((*i)->GetSpellProto()->SpellIconID == 2114)
                    {
                        m_target->RemoveAurasDueToSpell(31666);
                        int32 bp = (*i)->GetModifier()->m_amount;
                        m_target->CastCustomSpell(m_target,31665,&bp,NULL,NULL,true);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // for RACE_NIGHTELF stealth
        if(Real && m_target->GetTypeId()==TYPEID_PLAYER && GetId()==20580)
            m_target->RemoveAurasDueToSpell(21009);

        // only at real aura remove of _last_ SPELL_AURA_MOD_STEALTH
        if (Real && !m_target->HasAuraType(SPELL_AURA_MOD_STEALTH))
        {
            // if no GM invisibility
            if (m_target->GetVisibility()!=VISIBILITY_OFF)
            {
                m_target->RemoveStandFlags(UNIT_BYTE1_FLAGS_CREEP);
                if(m_target->GetTypeId()==TYPEID_PLAYER)
                    m_target->RemoveFlag(PLAYER_FIELD_BYTES2, 0x2000);

                // restore invisibility if any
                if (m_target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
                {
                    m_target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
                    m_target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
                }
                else
                    m_target->SetVisibility(VISIBILITY_ON);
            }

            // apply delayed talent bonus remover at last stealth aura remove
            Unit::AuraList const& mDummyAuras = m_target->GetAurasByType(SPELL_AURA_DUMMY);
            for(Unit::AuraList::const_iterator i = mDummyAuras.begin();i != mDummyAuras.end(); ++i)
            {
                // Master of Subtlety
                if ((*i)->GetSpellProto()->SpellIconID == 2114)
                {
                    m_target->CastSpell(m_target,31666,true);
                    break;
                }
            }
        }
    }
}

void Aura::HandleInvisibility(bool apply, bool Real)
{
    if(apply)
    {
        m_target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);

         m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        if(Real && m_target->GetTypeId()==TYPEID_PLAYER)
        {
            // apply glow vision
            m_target->SetFlag(PLAYER_FIELD_BYTES2,PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

        }

        // apply only if not in GM invisibility and not stealth
        if(m_target->GetVisibility() == VISIBILITY_ON)
        {
            // Aura not added yet but visibility code expect temporary add aura
            m_target->SetVisibility(VISIBILITY_GROUP_NO_DETECT);
            m_target->SetVisibility(VISIBILITY_GROUP_INVISIBILITY);
        }
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        m_target->m_invisibilityMask = 0;
        Unit::AuraList const& auras = m_target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            m_target->m_invisibilityMask |= (1 << m_modifier.m_miscvalue);

        // only at real aura remove and if not have different invisibility auras.
        if(Real && m_target->m_invisibilityMask == 0)
        {
            // remove glow vision
            if(m_target->GetTypeId() == TYPEID_PLAYER)
                m_target->RemoveFlag(PLAYER_FIELD_BYTES2,PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW);

            // apply only if not in GM invisibility & not stealthed while invisible
            if(m_target->GetVisibility() != VISIBILITY_OFF)
            {
                // if have stealth aura then already have stealth visibility
                if(!m_target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                    m_target->SetVisibility(VISIBILITY_ON);
            }
        }
    }
}

void Aura::HandleInvisibilityDetect(bool apply, bool Real)
{
    if(apply)
    {
        m_target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    }
    else
    {
        // recalculate value at modifier remove (current aura already removed)
        m_target->m_detectInvisibilityMask = 0;
        Unit::AuraList const& auras = m_target->GetAurasByType(SPELL_AURA_MOD_INVISIBILITY_DETECTION);
        for(Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            m_target->m_detectInvisibilityMask |= (1 << m_modifier.m_miscvalue);
    }
    if(Real && m_target->GetTypeId()==TYPEID_PLAYER)
        ((Player*)m_target)->UpdateVisibilityForPlayer();
}

void Aura::HandleAuraModRoot(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    if (apply)
    {
        m_target->addUnitState(UNIT_STAT_ROOT);
        m_target->SetTargetGUID(0);

        //Save last orientation
        if( m_target->getVictim() )
            m_target->SetOrientation(m_target->GetAngle(m_target->getVictim()));

        if(m_target->GetTypeId() == TYPEID_PLAYER)
        {
            WorldPacket data(SMSG_FORCE_MOVE_ROOT, 10);
            data << m_target->GetPackGUID();
            data << (uint32)2;
            m_target->SendMessageToSet(&data, true);

            //Clear unit movement flags
            ((Player*)m_target)->m_movementInfo.SetMovementFlags(MOVEFLAG_NONE);
        }
        else
            m_target->StopMoving();
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if(m_target->HasAuraType(SPELL_AURA_MOD_ROOT))
            return;

        m_target->clearUnitState(UNIT_STAT_ROOT);

        if(!m_target->hasUnitState(UNIT_STAT_STUNNED))      // prevent allow move if have also stun effect
        {
            if(m_target->getVictim() && m_target->isAlive())
                m_target->SetTargetGUID(m_target->getVictim()->GetGUID());

            if(m_target->GetTypeId() == TYPEID_PLAYER)
            {
                WorldPacket data(SMSG_FORCE_MOVE_UNROOT, 10);
                data << m_target->GetPackGUID();
                data << (uint32)2;
                m_target->SendMessageToSet(&data, true);
            }
        }
    }
}

void Aura::HandleAuraModSilence(bool apply, bool Real)
{
    // only at real add/remove aura
    if(!Real)
        return;

    if(apply)
    {
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = m_target->GetCurrentSpell(CurrentSpellTypes(i)))
                if(spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    m_target->InterruptSpell(CurrentSpellTypes(i), false);

        switch (GetId())
        {
            // Arcane Torrent (Energy)
            case 25046:
            {
                Unit * caster = GetCaster();
                if (!caster)
                    return;

                // Search Mana Tap auras on caster
                int32 energy = 0;
                Unit::AuraList const& m_dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
                for(Unit::AuraList::const_iterator i = m_dummyAuras.begin(); i != m_dummyAuras.end(); ++i)
                    if ((*i)->GetId() == 28734)
                        ++energy;
                if (energy)
                {
                    energy *= 10;
                    caster->CastCustomSpell(caster, 25048, &energy, NULL, NULL, true);
                    caster->RemoveAurasDueToSpell(28734);
                }
            }
        }
    }
    else
    {
        // Real remove called after current aura remove from lists, check if other similar auras active
        if(m_target->HasAuraType(SPELL_AURA_MOD_SILENCE))
            return;

        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void Aura::HandleModThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    if (!m_target->isAlive())
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    int level_diff = 0;
    int multiplier = 0;
    switch (GetId())
    {
        // Arcane Shroud
        case 26400:
            level_diff = m_target->getLevel() - 60;
            multiplier = 2;
            break;
        // The Eye of Diminution
        case 28862:
            level_diff = m_target->getLevel() - 60;
            multiplier = 1;
            break;
    }

    if (level_diff > 0)
        m_modifier.m_amount += multiplier * level_diff;

    if (m_target->GetTypeId() == TYPEID_PLAYER)
        for(int8 x=0;x < MAX_SPELL_SCHOOL;x++)
            if (m_modifier.m_miscvalue & int32(1<<x))
                ApplyPercentModFloatVar(m_target->m_threatModifier[x], float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModTotalThreat(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    if (!m_target->isAlive() || m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    float threatMod = apply ? float(m_modifier.m_amount) : float(-m_modifier.m_amount);

    m_target->getHostileRefManager().threatAssist(caster, threatMod, GetSpellProto());
}

void Aura::HandleModTaunt(bool apply, bool Real)
{
    // only at real add/remove aura
    if (!Real)
        return;

    if (!m_target->isAlive() || !m_target->CanHaveThreatList())
        return;

    Unit* caster = GetCaster();

    if (!caster || !caster->isAlive())
        return;

    if (apply)
        m_target->TauntApply(caster);
    else
    {
        // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
        m_target->TauntFadeOut(caster);
    }
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void Aura::HandleAuraModIncreaseSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseMountedSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_RUN, true);
}

void Aura::HandleAuraModIncreaseSwimSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModDecreaseSpeed(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    if (apply)
    {
        // Gronn Lord's Grasp, becomes stoned
        if (GetId() == 33572)
        {
            if (m_target->GetAuras().count(Unit::spellEffectPair(GetId(),GetEffIndex())) >= 5 &&
                !m_target->HasAura(33652))
                m_target->CastSpell(m_target, 33652, true);
        }
    }

    m_target->UpdateSpeed(MOVE_RUN, true);
    m_target->UpdateSpeed(MOVE_SWIM, true);
}

void Aura::HandleAuraModUseNormalSpeed(bool /*apply*/, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->UpdateSpeed(MOVE_RUN, true);
    m_target->UpdateSpeed(MOVE_SWIM, true);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void Aura::HandleModMechanicImmunity(bool apply, bool /*Real*/)
{
    uint32 misc  = m_modifier.m_miscvalue;

    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        uint32 mechanic = 1 << (misc-1);

        //immune movement impairment and loss of control
        if(GetId()==42292)
            mechanic=IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;

        m_target->RemoveAurasAtMechanicImmunity(mechanic,GetId());
    }

    m_target->ApplySpellImmune(GetId(),IMMUNITY_MECHANIC,misc,apply);

    // Bestial Wrath
    if (GetSpellProto()->SpellFamilyName == SPELLFAMILY_HUNTER && GetSpellProto()->SpellIconID == 1680)
    {
        // The Beast Within cast on owner if talent present
        if (Unit* owner = m_target->GetOwner())
        {
            // Search talent
            Unit::AuraList const& dummyAuras = owner->GetAurasByType(SPELL_AURA_DUMMY);
            for(Unit::AuraList::const_iterator i = dummyAuras.begin(); i != dummyAuras.end(); ++i)
            {
                if ((*i)->GetSpellProto()->SpellIconID == 2229)
                {
                    if (apply)
                        owner->CastSpell(owner, 34471, true, NULL, this);
                    else
                        owner->RemoveAurasDueToSpell(34471);
                    break;
                }
            }
        }
    }
}

void Aura::HandleModMechanicImmunityMask(bool apply, bool /*Real*/)
{
    uint32 mechanic  = m_modifier.m_miscvalue;

    if(apply && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
        m_target->RemoveAurasAtMechanicImmunity(mechanic,GetId());

    // check implemented in Unit::IsImmunedToSpell and Unit::IsImmunedToSpellEffect
}

//this method is called whenever we add / remove aura which gives m_target some imunity to some spell effect
void Aura::HandleAuraModEffectImmunity(bool apply, bool /*Real*/)
{
    // when removing flag aura, handle flag drop
    if( !apply && m_target->GetTypeId() == TYPEID_PLAYER
        && (GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION) )
    {
        if( BattleGround *bg = ((Player*)m_target)->GetBattleGround() )
            bg->EventPlayerDroppedFlag(((Player*)m_target));
    }

    m_target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModStateImmunity(bool apply, bool Real)
{
    if(apply && Real && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY)
    {
        Unit::AuraList const& auraList = m_target->GetAurasByType(AuraType(m_modifier.m_miscvalue));
        for(Unit::AuraList::const_iterator itr = auraList.begin(); itr != auraList.end();)
        {
            if (auraList.front() != this)                   // skip itself aura (it already added)
            {
                m_target->RemoveAurasDueToSpell(auraList.front()->GetId());
                itr = auraList.begin();
            }
            else
                ++itr;
        }
    }

    m_target->ApplySpellImmune(GetId(), IMMUNITY_STATE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModSchoolImmunity(bool apply, bool Real)
{
    m_target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, m_modifier.m_miscvalue, apply);

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if( this->GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
        && this->GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_DAMAGE_REDUCED_SHIELD )
        m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if( Real && apply
        && GetSpellProto()->AttributesEx & SPELL_ATTR_EX_DISPEL_AURAS_ON_IMMUNITY
        && IsPositiveSpell(GetId()) )                       //Only positive immunity removes auras
    {
        uint32 school_mask = m_modifier.m_miscvalue;
        Unit::AuraMap& Auras = m_target->GetAuras();
        for(Unit::AuraMap::iterator iter = Auras.begin(), next; iter != Auras.end(); iter = next)
        {
            next = iter;
            ++next;
            SpellEntry const *spell = iter->second->GetSpellProto();
            if((GetSpellSchoolMask(spell) & school_mask)//Check for school mask
                && !( spell->Attributes & SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY)   //Spells unaffected by invulnerability
                && !iter->second->IsPositive()          //Don't remove positive spells
                && spell->Id != GetId() )               //Don't remove self
            {
                m_target->RemoveAurasDueToSpell(spell->Id);
                if(Auras.empty())
                    break;
                else
                    next = Auras.begin();
            }
        }
    }
    if( Real && GetSpellProto()->Mechanic == MECHANIC_BANISH )
    {
        if( apply )
            m_target->addUnitState(UNIT_STAT_ISOLATED);
        else
            m_target->clearUnitState(UNIT_STAT_ISOLATED);
    }
}

void Aura::HandleAuraModDmgImmunity(bool apply, bool /*Real*/)
{
    m_target->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, m_modifier.m_miscvalue, apply);
}

void Aura::HandleAuraModDispelImmunity(bool apply, bool Real)
{
    // all applied/removed only at real aura add/remove
    if(!Real)
        return;

    m_target->ApplySpellDispelImmunity(m_spellProto, DispelType(m_modifier.m_miscvalue), apply);
}

void Aura::HandleAuraProcTriggerSpell(bool apply, bool Real)
{
    if(!Real)
        return;

    if(apply)
    {
        // some spell have charges by functionality not have its in spell data
        switch (GetId())
        {
            case 28200:                                     // Ascendance (Talisman of Ascendance trinket)
                m_procCharges = 6;
                UpdateAuraCharges();
                break;
            default: break;
        }
    }
}

void Aura::HandleAuraModStalked(bool apply, bool /*Real*/)
{
    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if(apply)
        m_target->SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    else
        m_target->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
}

/*********************************************************/
/***                   PERIODIC                        ***/
/*********************************************************/

void Aura::HandlePeriodicTriggerSpell(bool apply, bool /*Real*/)
{
    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;
    m_isTrigger = apply;

    if (!apply)
    {
        switch(m_spellProto->Id)
        {
            case 29213:                                     // Curse of the Plaguebringer
                if (m_removeMode != AURA_REMOVE_BY_DISPEL)
                    // Cast Wrath of the Plaguebringer if not dispelled
                    m_target->CastSpell(m_target, 29214, true, 0, this);
                return;
            default:
                break;
        }
    }
}

void Aura::HandlePeriodicEnergize(bool apply, bool /*Real*/)
{
    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;
}

void Aura::HandlePeriodicHeal(bool apply, bool Real)
{
    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;

    // only at real apply
    if (Real && apply && GetSpellProto()->Mechanic == MECHANIC_BANDAGE)
    {
        // provided m_target as original caster to prevent apply aura caster selection for this negative buff
        m_target->CastSpell(m_target,11196,true,NULL,this,m_target->GetGUID());
    }

    // For prevent double apply bonuses
    bool loading = (m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading());

    if(!loading && apply)
    {
        switch (m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_DRUID:
            {
                // Rejuvenation
                if(m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000010))
                {
                    if(Unit* caster = GetCaster())
                    {
                        Unit::AuraList const& classScripts = caster->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                        for(Unit::AuraList::const_iterator k = classScripts.begin(); k != classScripts.end(); ++k)
                        {
                            int32 tickcount = GetSpellDuration(m_spellProto) / m_spellProto->EffectAmplitude[m_effIndex];
                            switch((*k)->GetModifier()->m_miscvalue)
                            {
                                case 4953:                          // Increased Rejuvenation Healing - Harold's Rejuvenating Broach Aura
                                case 4415:                          // Increased Rejuvenation Healing - Idol of Rejuvenation Aura
                                {
                                    m_modifier.m_amount += (*k)->GetModifier()->m_amount / tickcount;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Aura::HandlePeriodicDamage(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;

    // For prevent double apply bonuses
    bool loading = (m_target->GetTypeId() == TYPEID_PLAYER && ((Player*)m_target)->GetSession()->PlayerLoading());

    // Custom damage calculation after
    if (apply)
    {
        if(loading)
            return;

        Unit *caster = GetCaster();
        if (!caster)
            return;

        switch (m_spellProto->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                // Pounce Bleed
                if ( m_spellProto->SpellIconID == 147 && m_spellProto->SpellVisual == 0 )
                {
                    // $AP*0.18/6 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 3 / 100);
                    return;
                }
                break;
            }
            case SPELLFAMILY_WARRIOR:
            {
                // Rend
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000020))
                {
                    // 0.00743*(($MWB+$mwb)/2+$AP/14*$MWS) bonus per tick
                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    int32 mws = caster->GetAttackTime(BASE_ATTACK);
                    float mwb_min = caster->GetWeaponDamageRange(BASE_ATTACK,MINDAMAGE);
                    float mwb_max = caster->GetWeaponDamageRange(BASE_ATTACK,MAXDAMAGE);
                    // WARNING! in 3.0 multiplier 0.00743f change to 0.6
                    m_modifier.m_amount+=int32(((mwb_min+mwb_max)/2+ap*mws/14000)*0.00743f);
                    return;
                }
                break;
            }
            case SPELLFAMILY_DRUID:
            {
                // Rake
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000001000))
                {
                    // $AP*0.06/3 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 2 / 100);
                    return;
                }
                // Lacerate
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000010000000000))
                {
                    // $AP*0.05/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) / 100);
                    return;
                }
                // Rip
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000000000800000))
                {
                    // $AP * min(0.06*$cp, 0.24)/6 [Yes, there is no difference, whether 4 or 5 CPs are being used]
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        uint8 cp = ((Player*)caster)->GetComboPoints();

                        // Idol of Feral Shadows. Cant be handled as SpellMod in SpellAura:Dummy due its dependency from CPs
                        Unit::AuraList const& dummyAuras = caster->GetAurasByType(SPELL_AURA_DUMMY);
                        for(Unit::AuraList::const_iterator itr = dummyAuras.begin(); itr != dummyAuras.end(); ++itr)
                        {
                            if((*itr)->GetId()==34241)
                            {
                                m_modifier.m_amount += cp * (*itr)->GetModifier()->m_amount;
                                break;
                            }
                        }

                        if (cp > 4) cp = 4;
                        m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                    }
                    return;
                }
                break;
            }
            case SPELLFAMILY_ROGUE:
            {
                // Rupture
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000000000100000))
                {
                    // Dmg/tick = $AP*min(0.01*$cp, 0.03) [Like Rip: only the first three CP increase the contribution from AP]
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        uint8 cp = ((Player*)caster)->GetComboPoints();
                        if (cp > 3) cp = 3;
                        m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * cp / 100);
                    }
                    return;
                }
                // Garrote
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x000000000000000100))
                {
                    // $AP*0.18/6 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(BASE_ATTACK) * 3 / 100);
                    return;
                }
                break;
            }
            case SPELLFAMILY_HUNTER:
            {
                // Serpent Sting
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000004000))
                {
                    // $RAP*0.1/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 10 / 500);
                    return;
                }
                // Immolation Trap
                if ((m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000004)) && m_spellProto->SpellIconID == 678)
                {
                    // $RAP*0.1/5 bonus per tick
                    m_modifier.m_amount += int32(caster->GetTotalAttackPowerValue(RANGED_ATTACK) * 10 / 500);
                    return;
                }
                break;
            }
            case SPELLFAMILY_PALADIN:
            {
                // Consecration
                if (m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000020))
                {
                    Unit::AuraList const& classScripts = caster->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                    for(Unit::AuraList::const_iterator k = classScripts.begin(); k != classScripts.end(); ++k)
                    {
                        int32 tickcount = GetSpellDuration(m_spellProto) / m_spellProto->EffectAmplitude[m_effIndex];
                        switch((*k)->GetModifier()->m_miscvalue)
                        {
                        case 5147:                  // Improved Consecration - Libram of the Eternal Rest
                            {
                                m_modifier.m_amount += (*k)->GetModifier()->m_amount / tickcount;
                                break;
                            }
                        }
                    }
                    return;
                }
                break;
            }
            default:
                break;
        }
    }
    // remove time effects
    else
    {
        // Parasitic Shadowfiend - handle summoning of two Shadowfiends on DoT expire
        if(m_spellProto->Id == 41917)
            m_target->CastSpell(m_target, 41915, true);
    }
}

void Aura::HandlePeriodicDamagePCT(bool apply, bool /*Real*/)
{
    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;
}

void Aura::HandlePeriodicLeech(bool apply, bool /*Real*/)
{
    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;
}

void Aura::HandlePeriodicManaLeech(bool apply, bool /*Real*/)
{
    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

void Aura::HandleAuraModResistanceExclusive(bool apply, bool /*Real*/)
{
    for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
    {
        if(m_modifier.m_miscvalue & int32(1<<x))
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)m_target)->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleAuraModResistance(bool apply, bool /*Real*/)
{
    for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
    {
        if(m_modifier.m_miscvalue & int32(1<<x))
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)m_target)->ApplyResistanceBuffModsMod(SpellSchools(x), m_positive, float(m_modifier.m_amount), apply);
        }
    }

    // Faerie Fire (druid versions)
    if( m_spellProto->SpellIconID == 109 &&
        m_spellProto->SpellFamilyName == SPELLFAMILY_DRUID &&
        m_spellProto->SpellFamilyFlags & UI64LIT(0x0000000000000400))
    {
        m_target->ApplySpellDispelImmunity(m_spellProto, DISPEL_STEALTH, apply);
        m_target->ApplySpellDispelImmunity(m_spellProto, DISPEL_INVISIBILITY, apply);
    }
}

void Aura::HandleAuraModBaseResistancePCT(bool apply, bool /*Real*/)
{
    // only players have base stats
    if(m_target->GetTypeId() != TYPEID_PLAYER)
    {
        //pets only have base armor
        if(((Creature*)m_target)->isPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            m_target->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(m_modifier.m_amount), apply);
    }
    else
    {
        for(int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL;x++)
        {
            if(m_modifier.m_miscvalue & int32(1<<x))
                m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleModResistancePercent(bool apply, bool /*Real*/)
{
    for(int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
    {
        if(m_modifier.m_miscvalue & int32(1<<i))
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER)
            {
                ((Player*)m_target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, float(m_modifier.m_amount), apply);
                ((Player*)m_target)->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, float(m_modifier.m_amount), apply);
            }
        }
    }
}

void Aura::HandleModBaseResistance(bool apply, bool /*Real*/)
{
    // only players have base stats
    if(m_target->GetTypeId() != TYPEID_PLAYER)
    {
        //only pets have base stats
        if(((Creature*)m_target)->isPet() && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
            m_target->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
    else
    {
        for(int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
            if(m_modifier.m_miscvalue & (1<<i))
                m_target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
    }
}

/********************************/
/***           STAT           ***/
/********************************/

void Aura::HandleAuraModStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -2 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Spell %u effect %u have unsupported misc value (%i) for SPELL_AURA_MOD_STAT ",GetId(),GetEffIndex(),m_modifier.m_miscvalue);
        return;
    }

    for(int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        // -1 or -2 is all stats ( misc < -2 checked in function beginning )
        if (m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue == i)
        {
            //m_target->ApplyStatMod(Stats(i), m_modifier.m_amount,apply);
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)m_target)->ApplyStatBuffMod(Stats(i), float(m_modifier.m_amount), apply);
        }
    }
}

void Aura::HandleModPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_modifier.m_amount), apply);
    }
}

void Aura::HandleModSpellDamagePercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    // Recalculate bonus
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModSpellHealingPercentFromStat(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModHealingDone(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;
    // implemented in Unit::SpellHealingBonus
    // this information is for client side only
    ((Player*)m_target)->UpdateSpellDamageAndHealingBonus();
}

void Aura::HandleModTotalPercentStat(bool apply, bool /*Real*/)
{
    if (m_modifier.m_miscvalue < -1 || m_modifier.m_miscvalue > 4)
    {
        sLog.outError("WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    //save current and max HP before applying aura
    uint32 curHPValue = m_target->GetHealth();
    uint32 maxHPValue = m_target->GetMaxHealth();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if(m_modifier.m_miscvalue == i || m_modifier.m_miscvalue == -1)
        {
            m_target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(m_modifier.m_amount), apply);
            if(m_target->GetTypeId() == TYPEID_PLAYER)
                ((Player*)m_target)->ApplyStatPercentBuffMod(Stats(i), float(m_modifier.m_amount), apply );
        }
    }

    //recalculate current HP/MP after applying aura modifications (only for spells with 0x10 flag)
    if ((m_modifier.m_miscvalue == STAT_STAMINA) && (maxHPValue > 0) && (m_spellProto->Attributes & 0x10))
    {
        // newHP = (curHP / maxHP) * newMaxHP = (newMaxHP * curHP) / maxHP -> which is better because no int -> double -> int conversion is needed
        uint32 newHPValue = (m_target->GetMaxHealth() * curHPValue) / maxHPValue;
        m_target->SetHealth(newHPValue);
    }
}

void Aura::HandleAuraModResistenceOfStatPercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(m_modifier.m_miscvalue != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog.outError("Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) need adding support for non-armor resistances!");
        return;
    }

    // Recalculate Armor
    m_target->UpdateArmor();
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void Aura::HandleAuraModTotalHealthPercentRegen(bool apply, bool /*Real*/)
{
    /*
    Need additional checking for auras who reduce or increase healing, magic effect like Dumpen Magic,
    so this aura not fully working.
    */
    if(apply)
    {
        if(!m_target->isAlive())
            return;

        if((GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED) && !m_target->IsSitState())
            m_target->SetStandState(UNIT_STAND_STATE_SIT);

        if(m_periodicTimer <= 0)
        {
            m_periodicTimer += m_modifier.periodictime;

            if(m_target->GetHealth() < m_target->GetMaxHealth())
            {
                // PeriodicTick can cast triggered spells with stats changes
                PeriodicTick();
            }
        }
    }

    m_isPeriodic = apply;
}

void Aura::HandleAuraModTotalManaPercentRegen(bool apply, bool /*Real*/)
{
    if((GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED) && apply  && !m_target->IsSitState())
        m_target->SetStandState(UNIT_STAND_STATE_SIT);
    if(apply)
    {
        if(m_modifier.periodictime == 0)
            m_modifier.periodictime = 1000;
        if(m_periodicTimer <= 0 && m_target->getPowerType() == POWER_MANA)
        {
            m_periodicTimer += m_modifier.periodictime;

            if(m_target->GetPower(POWER_MANA) < m_target->GetMaxPower(POWER_MANA))
            {
                // PeriodicTick can cast triggered spells with stats changes
                PeriodicTick();
            }
        }
    }

    m_isPeriodic = apply;
}

void Aura::HandleModRegen(bool apply, bool /*Real*/)        // eating
{
    if(apply)
    {
        if(!m_target->isAlive())
            return;

        if ((GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)  && !m_target->IsSitState())
            m_target->SetStandState(UNIT_STAND_STATE_SIT);

        if(m_periodicTimer <= 0)
        {
            m_periodicTimer += 5000;
            int32 gain = m_target->ModifyHealth(m_modifier.m_amount);
            Unit *caster = GetCaster();
            if (caster)
            {
                SpellEntry const *spellProto = GetSpellProto();
                if (spellProto)
                    m_target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, spellProto);
            }
        }
    }

    m_isPeriodic = apply;
}

void Aura::HandleModPowerRegen(bool apply, bool Real)       // drinking
{
    if ((GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED) && apply && !m_target->IsSitState())
        m_target->SetStandState(UNIT_STAND_STATE_SIT);

    if(apply && m_periodicTimer <= 0)
    {
        m_periodicTimer += 2000;

        Powers pt = m_target->getPowerType();
        if(int32(pt) != m_modifier.m_miscvalue)
            return;

        if ( GetSpellProto()->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED )
        {
            // eating anim
            m_target->HandleEmoteCommand(EMOTE_ONESHOT_EAT);
        }
        else if( GetId() == 20577 )
        {
            // cannibalize anim
            m_target->HandleEmoteCommand(EMOTE_STATE_CANNIBALIZE);
        }

        // Warrior talent, gain 1 rage every 3 seconds while in combat
        if(pt == POWER_RAGE && m_target->isInCombat())
        {
            m_target->ModifyPower(pt, m_modifier.m_amount*10/17);
            m_periodicTimer += 1000;
        }
    }
    m_isPeriodic = apply;
    if (Real && m_target->GetTypeId() == TYPEID_PLAYER && m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)m_target)->UpdateManaRegen();
}

void Aura::HandleModPowerRegenPCT(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if (m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Update manaregen value
    if (m_modifier.m_miscvalue == POWER_MANA)
        ((Player*)m_target)->UpdateManaRegen();
}

void Aura::HandleAuraModIncreaseHealth(bool apply, bool Real)
{
    // Special case with temporary increase max/current health
    switch(GetId())
    {
        case 12976:                                         // Warrior Last Stand triggered spell
        case 28726:                                         // Nightmare Seed ( Nightmare Seed )
        case 34511:                                         // Valor (Bulwark of Kings, Bulwark of the Ancient Kings)
        case 44055:                                         // Tremendous Fortitude (Battlemaster's Alacrity)
        {
            if(Real)
            {
                if(apply)
                {
                    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                    m_target->ModifyHealth(m_modifier.m_amount);
                }
                else
                {
                    if (int32(m_target->GetHealth()) > m_modifier.m_amount)
                        m_target->ModifyHealth(-m_modifier.m_amount);
                    else
                        m_target->SetHealth(1);
                    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
                }
            }
            return;
        }
    }

    // generic case
    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergy(bool apply, bool /*Real*/)
{
    Powers powerType = m_target->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    m_target->HandleStatModifier(unitMod, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseEnergyPercent(bool apply, bool /*Real*/)
{
    Powers powerType = m_target->getPowerType();
    if(int32(powerType) != m_modifier.m_miscvalue)
        return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    m_target->HandleStatModifier(unitMod, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModIncreaseHealthPercent(bool apply, bool /*Real*/)
{
    m_target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void Aura::HandleAuraModParryPercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateParryPercentage();
}

void Aura::HandleAuraModDodgePercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateDodgePercentage();
    //sLog.outError("BONUS DODGE CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModBlockPercent(bool /*apply*/, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateBlockPercentage();
    //sLog.outError("BONUS BLOCK CHANCE: + %f", float(m_modifier.m_amount));
}

void Aura::HandleAuraModRegenInterrupt(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    ((Player*)m_target)->UpdateManaRegen();
}

void Aura::HandleAuraModCritPercent(bool apply, bool Real)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    // apply item specific bonuses for already equipped weapon
    if(Real)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)m_target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)m_target)->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);
    }

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if (GetSpellProto()->EquippedItemClass == -1)
    {
        ((Player*)m_target)->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)m_target)->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (m_modifier.m_amount), apply);
        ((Player*)m_target)->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (m_modifier.m_amount), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void Aura::HandleModHitChance(bool apply, bool /*Real*/)
{
    m_target->m_modMeleeHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
    m_target->m_modRangedHitChance += apply ? m_modifier.m_amount : (-m_modifier.m_amount);
}

void Aura::HandleModSpellHitChance(bool apply, bool /*Real*/)
{
    m_target->m_modSpellHitChance += apply ? m_modifier.m_amount: (-m_modifier.m_amount);
}

void Aura::HandleModSpellCritChance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() == TYPEID_PLAYER)
    {
        ((Player*)m_target)->UpdateAllSpellCritChances();
    }
    else
    {
        m_target->m_baseSpellCritChance += apply ? m_modifier.m_amount:(-m_modifier.m_amount);
    }
}

void Aura::HandleModSpellCritChanceShool(bool /*apply*/, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    for(int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (m_modifier.m_miscvalue & (1<<school))
            ((Player*)m_target)->UpdateSpellCritChance(school);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void Aura::HandleModCastingSpeed(bool apply, bool /*Real*/)
{
    m_target->ApplyCastTimePercentMod(float(m_modifier.m_amount),apply);
}

void Aura::HandleModAttackSpeed(bool apply, bool /*Real*/)
{
    if(!m_target->isAlive() )
        return;

    m_target->ApplyAttackTimePercentMod(BASE_ATTACK,float(m_modifier.m_amount),apply);
}

void Aura::HandleHaste(bool apply, bool /*Real*/)
{
    m_target->ApplyAttackTimePercentMod(BASE_ATTACK, float(m_modifier.m_amount), apply);
    m_target->ApplyAttackTimePercentMod(OFF_ATTACK, float(m_modifier.m_amount), apply);
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedHaste(bool apply, bool /*Real*/)
{
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

void Aura::HandleRangedAmmoHaste(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;
    m_target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(m_modifier.m_amount), apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void Aura::HandleAuraModAttackPower(bool apply, bool /*Real*/)
{
    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPower(bool apply, bool /*Real*/)
{
    if((m_target->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModAttackPowerPercent(bool apply, bool /*Real*/)
{
    //UNIT_FIELD_ATTACK_POWER_MULTIPLIER = multiplier - 1
    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraModRangedAttackPowerPercent(bool apply, bool /*Real*/)
{
    if((m_target->getClassMask() & CLASSMASK_WAND_USERS)!=0)
        return;

    //UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER = multiplier - 1
    m_target->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void Aura::HandleModDamageDone(bool apply, bool Real)
{
    // apply item specific bonuses for already equipped weapon
    if(Real && m_target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)m_target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)m_target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || m_target->GetTypeId() != TYPEID_PLAYER)
        {
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }

        if(m_target->GetTypeId() == TYPEID_PLAYER)
        {
            if(m_positive)
                m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, m_modifier.m_amount, apply);
            else
                m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, m_modifier.m_amount, apply);
        }
    }

    // Skip non magic case for speedup
    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if( GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0 )
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    if(m_target->GetTypeId() == TYPEID_PLAYER)
    {
        if(m_positive)
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i, m_modifier.m_amount, apply);
            }
        }
        else
        {
            for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if((m_modifier.m_miscvalue & (1<<i)) != 0)
                    m_target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i, m_modifier.m_amount, apply);
            }
        }
        Pet* pet = m_target->GetPet();
        if(pet)
            pet->UpdateAttackPowerAndDamage();
    }
}

void Aura::HandleModDamagePercentDone(bool apply, bool Real)
{
    sLog.outDebug("AURA MOD DAMAGE type:%u negative:%u", m_modifier.m_miscvalue, m_positive ? 0 : 1);

    // apply item specific bonuses for already equipped weapon
    if(Real && m_target->GetTypeId() == TYPEID_PLAYER)
    {
        for(int i = 0; i < MAX_ATTACK; ++i)
            if(Item* pItem = ((Player*)m_target)->GetWeaponForAttack(WeaponAttackType(i),true,false))
                ((Player*)m_target)->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // m_modifier.m_miscvalue is bitmask of spell schools
    // 1 ( 0-bit ) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wand
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // m_modifier.m_miscvalue comparison with item generated damage types

    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellProto()->EquippedItemClass == -1 || m_target->GetTypeId() != TYPEID_PLAYER)
        {
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
            m_target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_PCT, float(m_modifier.m_amount), apply);
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
        // For show in client
        if(m_target->GetTypeId() == TYPEID_PLAYER)
            m_target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, m_modifier.m_amount/100.0f, apply);
    }

    // Skip non magic case for speedup
    if((m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if( GetSpellProto()->EquippedItemClass != -1 || GetSpellProto()->EquippedItemInventoryTypeMask != 0 )
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage percent modifiers implemented in Unit::SpellDamageBonus
    // Send info to client
    if(m_target->GetTypeId() == TYPEID_PLAYER)
        for(int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            m_target->ApplyModSignedFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT + i, m_modifier.m_amount/100.0f, apply);
}

void Aura::HandleModOffhandDamagePercent(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    sLog.outDebug("AURA MOD OFFHAND DAMAGE");

    m_target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(m_modifier.m_amount), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void Aura::HandleModPowerCostPCT(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    float amount = m_modifier.m_amount/100.0f;
    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if(m_modifier.m_miscvalue & (1<<i))
            m_target->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);
}

void Aura::HandleModPowerCost(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    for(int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if(m_modifier.m_miscvalue & (1<<i))
            m_target->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER + i, m_modifier.m_amount, apply);
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void Aura::HandleShapeshiftBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 HotWSpellId = 0;

    uint32 form = GetModifier()->m_miscvalue;

    switch(form)
    {
        case FORM_CAT:
            spellId1 = 3025;
            HotWSpellId = 24900;
            break;
        case FORM_TREE:
            spellId1 = 5420;
            break;
        case FORM_TRAVEL:
            spellId1 = 5419;
            break;
        case FORM_AQUA:
            spellId1 = 5421;
            break;
        case FORM_BEAR:
            spellId1 = 1178;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_DIREBEAR:
            spellId1 = 9635;
            spellId2 = 21178;
            HotWSpellId = 24899;
            break;
        case FORM_BATTLESTANCE:
            spellId1 = 21156;
            break;
        case FORM_DEFENSIVESTANCE:
            spellId1 = 7376;
            break;
        case FORM_BERSERKERSTANCE:
            spellId1 = 7381;
            break;
        case FORM_MOONKIN:
            spellId1 = 24905;
            // aura from effect trigger spell
            spellId2 = 24907;
            break;
        case FORM_SPIRITOFREDEMPTION:
            spellId1 = 27792;
            spellId2 = 27795;                               // must be second, this important at aura remove to prevent to early iterator invalidation.
            break;
        case FORM_GHOSTWOLF:
        case FORM_AMBIENT:
        case FORM_GHOUL:
        case FORM_SHADOW:
        case FORM_STEALTH:
        case FORM_CREATURECAT:
        case FORM_CREATUREBEAR:
            spellId1 = 0;
            break;
    }

    if(apply)
    {
        if (spellId1) m_target->CastSpell(m_target, spellId1, true, NULL, this );
        if (spellId2) m_target->CastSpell(m_target, spellId2, true, NULL, this);

        if (m_target->GetTypeId() == TYPEID_PLAYER)
        {
            const PlayerSpellMap& sp_list = ((Player *)m_target)->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second.state == PLAYERSPELL_REMOVED) continue;
                if (itr->first==spellId1 || itr->first==spellId2) continue;
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(itr->first);
                if (!spellInfo || !(spellInfo->Attributes & (SPELL_ATTR_PASSIVE | (1<<7))))
                    continue;
                if (spellInfo->Stances & (1<<(form-1)))
                    m_target->CastSpell(m_target, itr->first, true, NULL, this);
            }
            //LotP
            if (((Player*)m_target)->HasSpell(17007))
            {
                SpellEntry const *spellInfo = sSpellStore.LookupEntry(24932);
                if (spellInfo && spellInfo->Stances & (1<<(form-1)))
                    m_target->CastSpell(m_target, 24932, true, NULL, this);
            }
            // Heart of the Wild
            if (HotWSpellId)
            {
                Unit::AuraList const& mModTotalStatPct = m_target->GetAurasByType(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE);
                for(Unit::AuraList::const_iterator i = mModTotalStatPct.begin(); i != mModTotalStatPct.end(); ++i)
                {
                    if ((*i)->GetSpellProto()->SpellIconID == 240 && (*i)->GetModifier()->m_miscvalue == 3)
                    {
                        int32 HotWMod = (*i)->GetModifier()->m_amount;
                        if(GetModifier()->m_miscvalue == FORM_CAT)
                            HotWMod /= 2;

                        m_target->CastCustomSpell(m_target, HotWSpellId, &HotWMod, NULL, NULL, true, NULL, this);
                        break;
                    }
                }
            }
        }
    }
    else
    {
        m_target->RemoveAurasDueToSpell(spellId1);
        m_target->RemoveAurasDueToSpell(spellId2);

        Unit::AuraMap& tAuras = m_target->GetAuras();
        for (Unit::AuraMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            if (itr->second->IsRemovedOnShapeLost())
            {
                m_target->RemoveAurasDueToSpell(itr->second->GetId());
                itr = tAuras.begin();
            }
            else
            {
                ++itr;
            }
        }
    }
}

void Aura::HandleSpellSpecificBoosts(bool apply)
{
    uint32 spellId1 = 0;
    uint32 spellId2 = 0;
    uint32 spellId3 = 0;
    uint32 spellId4 = 0;

    switch(GetSpellProto()->SpellFamilyName)
    {
        case SPELLFAMILY_WARRIOR:
        {
            if(!apply)
            {
                // Remove Blood Frenzy only if target no longer has any Deep Wound or Rend (applying is handled by procs)
                if (GetSpellProto()->Mechanic != MECHANIC_BLEED)
                    return;

                // If target still has one of Warrior's bleeds, do nothing
                Unit::AuraList const& PeriodicDamage = m_target->GetAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
                for(Unit::AuraList::const_iterator i = PeriodicDamage.begin(); i != PeriodicDamage.end(); ++i)
                    if( (*i)->GetCasterGUID() == GetCasterGUID() &&
                        (*i)->GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARRIOR &&
                        (*i)->GetSpellProto()->Mechanic == MECHANIC_BLEED)
                        return;

                spellId1 = 30069;
                spellId2 = 30070;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            // The Beast Within and Bestial Wrath - immunity
            if (GetId() == 19574 || GetId() == 34471)
            {
                spellId1 = 24395;
                spellId2 = 24396;
                spellId3 = 24397;
                spellId4 = 26592;
            }
            else
                return;

            break;
        }
        default:
            return;
    }

    // prevent aura deletion, specially in multi-boost case
    SetInUse(true);

    if (apply)
    {
        if (spellId1)
            m_target->CastSpell(m_target, spellId1, true, NULL, this);
        if (spellId2 && !IsDeleted())
            m_target->CastSpell(m_target, spellId2, true, NULL, this);
        if (spellId3 && !IsDeleted())
            m_target->CastSpell(m_target, spellId3, true, NULL, this);
        if (spellId4 && !IsDeleted())
            m_target->CastSpell(m_target, spellId4, true, NULL, this);
    }
    else
    {
        if (spellId1)
            m_target->RemoveAurasByCasterSpell(spellId1, GetCasterGUID());
        if (spellId2)
            m_target->RemoveAurasByCasterSpell(spellId2, GetCasterGUID());
        if (spellId3)
            m_target->RemoveAurasByCasterSpell(spellId3, GetCasterGUID());
        if (spellId4)
            m_target->RemoveAurasByCasterSpell(spellId4, GetCasterGUID());
    }

    SetInUse(false);
}

void Aura::HandleAuraEmpathy(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_UNIT)
        return;

    CreatureInfo const * ci = ObjectMgr::GetCreatureTemplate(m_target->GetEntry());
    if(ci && ci->type == CREATURE_TYPE_BEAST)
        m_target->ApplyModUInt32Value(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void Aura::HandleAuraUntrackable(bool apply, bool /*Real*/)
{
    if(apply)
        m_target->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
    else
        m_target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_UNTRACKABLE);
}

void Aura::HandleAuraModPacify(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(apply)
        m_target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
        m_target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
}

void Aura::HandleAuraModPacifyAndSilence(bool apply, bool Real)
{
    HandleAuraModPacify(apply, Real);
    HandleAuraModSilence(apply, Real);
}

void Aura::HandleAuraGhost(bool apply, bool /*Real*/)
{
    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    if(apply)
    {
        m_target->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
    else
    {
        m_target->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    }
}

void Aura::HandleModTargetResistance(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;
    // applied to damage as HandleNoImmediateEffect in Unit::CalcAbsorbResist and Unit::CalcArmorReducedDamage

    // show armor penetration
   /*[-ZERO] to rewrite
    if (m_target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_NORMAL))
        m_target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, m_modifier.m_amount, apply);

    // show as spell penetration only full spell penetration bonuses (all resistances except armor and holy
    if (m_target->GetTypeId() == TYPEID_PLAYER && (m_modifier.m_miscvalue & SPELL_SCHOOL_MASK_SPELL)==SPELL_SCHOOL_MASK_SPELL)
        m_target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, m_modifier.m_amount, apply); */
}

void Aura::HandleShieldBlockValue(bool apply, bool /*Real*/)
{
    BaseModType modType = FLAT_MOD;
    if(m_modifier.m_auraname == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if(m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(m_modifier.m_amount), apply);
}

void Aura::HandleAuraRetainComboPoints(bool apply, bool Real)
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    if(m_target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player *target = (Player*)m_target;

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if( !apply && m_duration==0 && target->GetComboTarget())
        if(Unit* unit = ObjectAccessor::GetUnit(*m_target,target->GetComboTarget()))
            target->AddComboPoints(unit, -m_modifier.m_amount);
}

void Aura::HandleModUnattackable( bool Apply, bool Real )
{
    if(Real && Apply)
     {
        m_target->CombatStop();
        m_target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
     }
    m_target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE,Apply);
}

void Aura::HandleSpiritOfRedemption( bool apply, bool Real )
{
    // spells required only Real aura add/remove
    if(!Real)
        return;

    // prepare spirit state
    if(apply)
    {
        if(m_target->GetTypeId()==TYPEID_PLAYER)
        {
            // disable breath/etc timers
            ((Player*)m_target)->StopMirrorTimers();

            // set stand state (expected in this form)
            if(!m_target->IsStandState())
                m_target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        m_target->SetHealth(1);
    }
    // die at aura end
    else
        m_target->DealDamage(m_target, m_target->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, GetSpellProto(), false);
}

void Aura::CleanupTriggeredSpells()
{
    uint32 tSpellId = m_spellProto->EffectTriggerSpell[GetEffIndex()];
    if(!tSpellId)
        return;

    SpellEntry const* tProto = sSpellStore.LookupEntry(tSpellId);
    if(!tProto)
        return;

    if(GetSpellDuration(tProto) != -1)
        return;

    // needed for spell 43680, maybe others
    // TODO: is there a spell flag, which can solve this in a more sophisticated way?
    if(m_spellProto->EffectApplyAuraName[GetEffIndex()] == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
            GetSpellDuration(m_spellProto) == m_spellProto->EffectAmplitude[GetEffIndex()])
        return;
    m_target->RemoveAurasDueToSpell(tSpellId);
}

void Aura::HandleAuraPowerBurn(bool apply, bool Real)
{
    if (m_periodicTimer <= 0)
        m_periodicTimer += m_modifier.periodictime;

    m_isPeriodic = apply;
}

void Aura::HandleSchoolAbsorb(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit* caster = GetCaster();
    if(!caster)
        return;

    if (apply)
    {
        // prevent double apply bonuses
        if(m_target->GetTypeId()!=TYPEID_PLAYER || !((Player*)m_target)->GetSession()->PlayerLoading())
        {
            float DoneActualBenefit = 0.0f;
            switch(m_spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_PRIEST:
                    if(m_spellProto->SpellFamilyFlags == UI64LIT(0x1)) //PW:S
                    {
                        //+30% from +healing bonus
                        DoneActualBenefit = caster->SpellBaseHealingBonus(GetSpellSchoolMask(m_spellProto)) * 0.3f;
                        break;
                    }
                    break;
                case SPELLFAMILY_MAGE:
                    if (m_spellProto->SpellFamilyFlags == UI64LIT(0x80100) ||
                        m_spellProto->SpellFamilyFlags == UI64LIT(0x8) ||
                        m_spellProto->SpellFamilyFlags == UI64LIT(0x100000000))
                    {
                        //frost ward, fire ward, ice barrier
                        //+10% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.1f;
                        break;
                    }
                    break;
                case SPELLFAMILY_WARLOCK:
                    if(m_spellProto->SpellFamilyFlags == UI64LIT(0x00))
                    {
                        //shadow ward
                        //+10% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.1f;
                        break;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

void Aura::PeriodicTick()
{
    switch(m_modifier.m_auraname)
    {
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(m_target, GetSpellProto(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            // some auras remove at specific health level or more
            if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
            {
                switch(GetId())
                {
                    case 43093: case 31956: case 38801:
                    case 35321: case 38363: case 39215:
                        if(m_target->GetHealth() == m_target->GetMaxHealth() )
                        {
                            m_target->RemoveAurasDueToSpell(GetId());
                            return;
                        }
                        break;
                    case 38772:
                    {
                        uint32 percent =
                            GetEffIndex() < EFFECT_INDEX_2 && GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_DUMMY ?
                            pCaster->CalculateSpellDamage(GetSpellProto(), SpellEffectIndex(GetEffIndex() + 1), GetSpellProto()->EffectBasePoints[GetEffIndex() + 1], m_target) :
                            100;
                        if(m_target->GetHealth() * 100 >= m_target->GetMaxHealth() * percent )
                        {
                            m_target->RemoveAurasDueToSpell(GetId());
                            return;
                        }
                        break;
                    }
                    default:
                        break;
                }
            }

            uint32 absorb = 0;
            uint32 resist = 0;
            CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage;

            if(m_modifier.m_auraname == SPELL_AURA_PERIODIC_DAMAGE)
            {
                pdamage = amount;

                // Calculate armor mitigation if it is a physical spell
                // But not for bleed mechanic spells
                if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_NORMAL &&
                    GetEffectMechanic(GetSpellProto(), m_effIndex) != MECHANIC_BLEED)
                {
                    uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(m_target, pdamage);
                    cleanDamage.damage += pdamage - pdamageReductedArmor;
                    pdamage = pdamageReductedArmor;
                }

                pdamage = pCaster->SpellDamageBonus(m_target,GetSpellProto(),pdamage,DOT);

                // Curse of Agony damage-per-tick calculation
                if (GetSpellProto()->SpellFamilyName==SPELLFAMILY_WARLOCK && (GetSpellProto()->SpellFamilyFlags & UI64LIT(0x0000000000000400)) && GetSpellProto()->SpellIconID==544)
                {
                    // 1..4 ticks, 1/2 from normal tick damage
                    if (GetAuraTicks() <= 4)
                        pdamage = pdamage/2;
                    // 9..12 ticks, 3/2 from normal tick damage
                    else if(GetAuraTicks() >= 9)
                        pdamage += (pdamage + 1) / 2;       // +1 prevent 0.5 damage possible lost at 1..4 ticks
                    // 5..8 ticks have normal tick damage
                }
            }
            else
                pdamage = uint32(m_target->GetMaxHealth()*amount/100);

            pCaster->CalcAbsorbResist(m_target, GetSpellSchoolMask(GetSpellProto()), DOT, pdamage, &absorb, &resist, !(GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) attacked %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId(),absorb);

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, absorb, resist, 0.0f);
            m_target->SendPeriodicAuraLog(&pInfo);

            pCaster->DealDamage(m_target, (pdamage <= absorb+resist) ? 0 : (pdamage-absorb-resist), &cleanDamage, DOT, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), true);

            pCaster->ProcDamageAndSpell(m_target, PROC_FLAG_PERIODIC_TICK, PROC_FLAG_TAKE_DAMAGE, (pdamage <= absorb+resist) ? 0 : (pdamage-absorb-resist), GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
            break;
        }
        case SPELL_AURA_PERIODIC_LEECH:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if(!pCaster->isAlive())
                return;

            if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(m_target, GetSpellProto(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            uint32 absorb=0;
            uint32 resist=0;
            CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );

            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            //Calculate armor mitigation if it is a physical spell
            if (GetSpellSchoolMask(GetSpellProto()) & SPELL_SCHOOL_MASK_NORMAL)
            {
                uint32 pdamageReductedArmor = pCaster->CalcArmorReducedDamage(m_target, pdamage);
                cleanDamage.damage += pdamage - pdamageReductedArmor;
                pdamage = pdamageReductedArmor;
            }

            pdamage = pCaster->SpellDamageBonus(m_target, GetSpellProto(), pdamage, DOT);

            // talent Soul Siphon add bonus to Drain Life spells
            if( GetSpellProto()->SpellFamilyName == SPELLFAMILY_WARLOCK && (GetSpellProto()->SpellFamilyFlags & 0x8) )
            {
                // find talent max bonus percentage
                Unit::AuraList const& mClassScriptAuras = pCaster->GetAurasByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for(Unit::AuraList::const_iterator i = mClassScriptAuras.begin(); i != mClassScriptAuras.end(); ++i)
                {
                    if ((*i)->GetModifier()->m_miscvalue == 4992 || (*i)->GetModifier()->m_miscvalue == 4993)
                    {
                        if((*i)->GetEffIndex()!=1)
                        {
                            sLog.outError("Expected spell %u structure change, need code update",(*i)->GetId());
                            break;
                        }

                        // effect 1 m_amount
                        int32 maxPercent = (*i)->GetModifier()->m_amount;
                        // effect 0 m_amount
                        int32 stepPercent = pCaster->CalculateSpellDamage((*i)->GetSpellProto() ,EFFECT_INDEX_0,(*i)->GetSpellProto()->EffectBasePoints[0],pCaster);

                        // count affliction effects and calc additional damage in percentage
                        int32 modPercent = 0;
                        Unit::AuraMap const& victimAuras = m_target->GetAuras();
                        for (Unit::AuraMap::const_iterator itr = victimAuras.begin(); itr != victimAuras.end(); ++itr)
                        {
                            Aura* aura = itr->second;
                            if (aura->IsPositive())continue;
                            SpellEntry const* m_spell = aura->GetSpellProto();
                            if (m_spell->SpellFamilyName != SPELLFAMILY_WARLOCK)
                                continue;

                            SkillLineAbilityMapBounds bounds = sSpellMgr.GetSkillLineAbilityMapBounds(m_spell->Id);

                            for(SkillLineAbilityMap::const_iterator _spell_idx = bounds.first; _spell_idx != bounds.second; ++_spell_idx)
                            {
                                if(_spell_idx->second->skillId == SKILL_AFFLICTION)
                                {
                                    modPercent += stepPercent;
                                    if (modPercent >= maxPercent)
                                    {
                                        modPercent = maxPercent;
                                        break;
                                    }
                                }
                            }
                        }
                        pdamage += (pdamage*modPercent/100);
                        break;
                    }
                }
            }

            pCaster->CalcAbsorbResist(m_target, GetSpellSchoolMask(GetSpellProto()), DOT, pdamage, &absorb, &resist, !(GetSpellProto()->AttributesEx2 & SPELL_ATTR_EX2_CANT_REFLECTED));

            if(m_target->GetHealth() < pdamage)
                pdamage = uint32(m_target->GetHealth());

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) health leech of %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId(),absorb);

            pCaster->SendSpellNonMeleeDamageLog(m_target, GetId(), pdamage, GetSpellSchoolMask(GetSpellProto()), absorb, resist, false, 0);

            float multiplier = GetSpellProto()->EffectMultipleValue[GetEffIndex()] > 0 ? GetSpellProto()->EffectMultipleValue[GetEffIndex()] : 1;

            uint32 new_damage = pCaster->DealDamage(m_target, (pdamage <= absorb+resist) ? 0 : (pdamage-absorb-resist), &cleanDamage, DOT, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), false);

            pCaster->ProcDamageAndSpell(m_target, PROC_FLAG_PERIODIC_TICK, PROC_FLAG_TAKE_DAMAGE, new_damage, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
            if (!m_target->isAlive() && pCaster->IsNonMeleeSpellCasted(false))
                for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
                    if (Spell* spell = pCaster->GetCurrentSpell(CurrentSpellTypes(i)))
                        if (spell->m_spellInfo->Id == GetId())
                            spell->cancel();


            if(Player *modOwner = pCaster->GetSpellModOwner())
                modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, multiplier);

            uint32 heal = pCaster->SpellHealingBonus(GetSpellProto(), uint32(new_damage * multiplier), DOT, pCaster);

            int32 gain = pCaster->DealHeal(pCaster, heal, GetSpellProto());
            pCaster->getHostileRefManager().threatAssist(pCaster, gain * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            // heal for caster damage (must be alive)
            if(m_target != pCaster && GetSpellProto()->SpellVisual==163 && !pCaster->isAlive())
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage;

            if(m_modifier.m_auraname==SPELL_AURA_OBS_MOD_HEALTH)
                pdamage = uint32(m_target->GetMaxHealth() * amount / 100);
            else
                pdamage = amount;

            pdamage = pCaster->SpellHealingBonus(GetSpellProto(), pdamage, DOT, m_target);

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) heal of %u (TypeId: %u) for %u health inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain = m_target->ModifyHealth(pdamage);

            //Do check before because m_modifier.auraName can be invalidate by DealDamage.
            bool procSpell = (m_modifier.m_auraname == SPELL_AURA_PERIODIC_HEAL && m_target != pCaster);

            m_target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, GetSpellProto());

            Unit* target = m_target;                        // aura can be deleted in DealDamage
            SpellEntry const* spellProto = GetSpellProto();
            bool haveCastItem = GetCastItemGUID()!=0;

            // heal for caster damage
            if(m_target != pCaster && spellProto->SpellVisual==163)
            {
                uint32 dmg = spellProto->manaPerSecond;
                if(pCaster->GetHealth() <= dmg && pCaster->GetTypeId()==TYPEID_PLAYER)
                {
                    pCaster->RemoveAurasDueToSpell(GetId());

                    // finish current generic/channeling spells, don't affect autorepeat
                    pCaster->FinishSpell(CURRENT_GENERIC_SPELL);
                    pCaster->FinishSpell(CURRENT_CHANNELED_SPELL);
                }
                else
                {
                    pCaster->SendSpellNonMeleeDamageLog(pCaster, GetId(), gain, GetSpellSchoolMask(GetSpellProto()), 0, 0, false, 0, false);

                    CleanDamage cleanDamage =  CleanDamage(0, BASE_ATTACK, MELEE_HIT_NORMAL );
                    pCaster->DealDamage(pCaster, gain, &cleanDamage, NODAMAGE, GetSpellSchoolMask(GetSpellProto()), GetSpellProto(), true);
                }
            }

            // ignore item heals
            if(procSpell && !haveCastItem)
                pCaster->ProcDamageAndSpell(target,PROC_FLAG_PERIODIC_TICK, PROC_FLAG_HEALED, pdamage, SPELL_SCHOOL_MASK_NONE, spellProto);
            break;
        }
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            if(!pCaster->isAlive())
                return;

            if( GetSpellProto()->Effect[GetEffIndex()] == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
                pCaster->SpellHitResult(m_target, GetSpellProto(), false) != SPELL_MISS_NONE)
                return;

            // Check for immune (not use charges)
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) power leech of %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                break;

            Powers power = Powers(m_modifier.m_miscvalue);

            // power type might have changed between aura applying and tick (druid's shapeshift)
            if(m_target->getPowerType() != power)
                break;

            int32 drain_amount = m_target->GetPower(power) > pdamage ? pdamage : m_target->GetPower(power);

            m_target->ModifyPower(power, -drain_amount);

            float gain_multiplier = 0;

            if(pCaster->GetMaxPower(power) > 0)
            {
                gain_multiplier = GetSpellProto()->EffectMultipleValue[GetEffIndex()];

                if(Player *modOwner = pCaster->GetSpellModOwner())
                    modOwner->ApplySpellMod(GetId(), SPELLMOD_MULTIPLE_VALUE, gain_multiplier);
            }

            SpellPeriodicAuraLogInfo pInfo(this, drain_amount, 0, 0, gain_multiplier);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain_amount = int32(drain_amount * gain_multiplier);

            if(gain_amount)
            {
                int32 gain = pCaster->ModifyPower(power, gain_amount);
                m_target->AddThreat(pCaster, float(gain) * 0.5f, false, GetSpellSchoolMask(GetSpellProto()), GetSpellProto());
            }
            break;
        }
        case SPELL_AURA_PERIODIC_ENERGIZE:
        {
            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u dmg inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            if(m_modifier.m_miscvalue < 0 || m_modifier.m_miscvalue >= MAX_POWERS)
                break;

            Powers power = Powers(m_modifier.m_miscvalue);

            if(m_target->GetMaxPower(power) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain = m_target->ModifyPower(power,pdamage);

            if(Unit* pCaster = GetCaster())
                m_target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_OBS_MOD_MANA:
        {
            // ignore non positive values (can be result apply spellmods to aura damage
            uint32 amount = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            uint32 pdamage = uint32(m_target->GetMaxPower(POWER_MANA) * amount / 100);

            sLog.outDetail("PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u mana inflicted by %u",
                GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), m_target->GetGUIDLow(), m_target->GetTypeId(), pdamage, GetId());

            if(m_target->GetMaxPower(POWER_MANA) == 0)
                break;

            SpellPeriodicAuraLogInfo pInfo(this, pdamage, 0, 0, 0.0f);
            m_target->SendPeriodicAuraLog(&pInfo);

            int32 gain = m_target->ModifyPower(POWER_MANA, pdamage);

            if(Unit* pCaster = GetCaster())
                m_target->getHostileRefManager().threatAssist(pCaster, float(gain) * 0.5f, GetSpellProto());
            break;
        }
        case SPELL_AURA_POWER_BURN_MANA:
        {
            Unit *pCaster = GetCaster();
            if(!pCaster)
                return;

            // Check for immune (not use charges)
            if(m_target->IsImmunedToDamage(GetSpellSchoolMask(GetSpellProto())))
                return;

            int32 pdamage = m_modifier.m_amount > 0 ? m_modifier.m_amount : 0;

            Powers powerType = Powers(m_modifier.m_miscvalue);

            if(!m_target->isAlive() || m_target->getPowerType() != powerType)
                return;

            uint32 gain = uint32(-m_target->ModifyPower(powerType, -pdamage));

            gain = uint32(gain * GetSpellProto()->EffectMultipleValue[GetEffIndex()]);

            //maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
            pCaster->SpellNonMeleeDamageLog(m_target, GetId(), gain);
            break;
        }
        // Here tick dummy auras
        case SPELL_AURA_DUMMY:                              // some spells have dummy aura
        {
            PeriodicDummyTick();
            break;
        }
        default:
            break;
    }
}

void Aura::PeriodicDummyTick()
{
    SpellEntry const* spell = GetSpellProto();
    switch (spell->Id)
    {
        case 7057:                                  // Haunting Spirits
            if (roll_chance_i(33))
                m_target->CastSpell(m_target,7067,true,NULL,this);
            return;
        default:
            break;
    }
}

void Aura::HandlePreventFleeing(bool apply, bool Real)
{
    if(!Real)
        return;

    Unit::AuraList const& fearAuras = m_target->GetAurasByType(SPELL_AURA_MOD_FEAR);
    if( !fearAuras.empty() )
    {
        if (apply)
            m_target->SetFeared(false, fearAuras.front()->GetCasterGUID());
        else
            m_target->SetFeared(true);
    }
}

void Aura::HandleManaShield(bool apply, bool Real)
{
    if(!Real)
        return;

    // prevent double apply bonuses
    if(apply && (m_target->GetTypeId()!=TYPEID_PLAYER || !((Player*)m_target)->GetSession()->PlayerLoading()))
    {
        if(Unit* caster = GetCaster())
        {
            float DoneActualBenefit = 0.0f;
            switch(m_spellProto->SpellFamilyName)
            {
                case SPELLFAMILY_MAGE:
                    if(m_spellProto->SpellFamilyFlags & UI64LIT(0x8000))
                    {
                        // Mana Shield
                        // +50% from +spd bonus
                        DoneActualBenefit = caster->SpellBaseDamageBonus(GetSpellSchoolMask(m_spellProto)) * 0.5f;
                        break;
                    }
                    break;
                default:
                    break;
            }

            DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellProto());

            m_modifier.m_amount += (int32)DoneActualBenefit;
        }
    }
}

void Aura::UnregisterSingleCastAura()
{
    if (IsSingleTarget())
    {
        if(Unit* caster = GetCaster())
        {
            caster->GetSingleCastAuras().remove(this);
        }
        else
        {
            sLog.outError("Couldn't find the caster of the single target aura (SpellId %u), may crash later!", GetId());
            assert(false);
        }
        m_isSingleTargetAura = false;
    }
}

void Aura::HandleAuraSafeFall( bool Apply, bool Real )
{
    // implemented in WorldSession::HandleMovementOpcodes

    // only special case
    if(Apply && Real && GetId() == 32474 && m_target->GetTypeId() == TYPEID_PLAYER)
        ((Player*)m_target)->ActivateTaxiPathTo(506, GetId());
}
