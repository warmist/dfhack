/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/


#include "Internal.h"

#include <algorithm>
#include <cstring>
#include <map>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stddef.h>
#include <string>
#include <vector>
using namespace std;

#include "VersionInfo.h"
#include "MemAccess.h"
#include "Error.h"
#include "Types.h"

// we connect to those
#include "modules/Units.h"
#include "modules/Items.h"
#include "modules/Materials.h"
#include "modules/Translation.h"
#include "ModuleFactory.h"
#include "Core.h"
#include "MiscUtils.h"

#include "df/assumed_identity.h"
#include "df/body_appearance_modifier.h"
#include "df/bp_appearance_modifier.h"
#include "df/burrow.h"
#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/curse_attr_change.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/entity_raw.h"
#include "df/entity_raw_flags.h"
#include "df/game_mode.h"
#include "df/general_ref_is_nemesisst.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/histfig_entity_link_memberst.h"
#include "df/historical_entity.h"
#include "df/historical_figure.h"
#include "df/historical_figure_info.h"
#include "df/history_event_add_hf_entity_linkst.h"
#include "df/job.h"
#include "df/squad.h"
#include "df/temperaturest.h"
#include "df/nemesis_record.h"
#include "df/ui.h"
#include "df/unit_inventory_item.h"
#include "df/entity_position.h"
#include "df/entity_position_assignment.h"
#include "df/histfig_entity_link_positionst.h"
#include "df/identity.h"
#include "df/burrow.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "df/game_mode.h"
#include "df/unit_misc_trait.h"
#include "df/unit_skill.h"
#include "df/unit_soul.h"
#include "df/world.h"

using namespace DFHack;
using namespace df::enums;
using df::global::world;
using df::global::ui;
using df::global::gamemode;

bool Units::isValid()
{
    return (world->units.all.size() > 0);
}

int32_t Units::getNumCreatures()
{
    return world->units.all.size();
}

df::unit * Units::GetCreature (const int32_t index)
{
    if (!isValid()) return NULL;

    // read pointer from vector at position
    if(size_t(index) > world->units.all.size())
        return 0;
    return world->units.all[index];
}

// returns index of creature actually read or -1 if no creature can be found
int32_t Units::GetCreatureInBox (int32_t index, df::unit ** furball,
                                const uint16_t x1, const uint16_t y1, const uint16_t z1,
                                const uint16_t x2, const uint16_t y2, const uint16_t z2)
{
    if (!isValid())
        return -1;

    size_t size = world->units.all.size();
    while (size_t(index) < size)
    {
        // read pointer from vector at position
        df::unit * temp = world->units.all[index];
        if (temp->pos.x >= x1 && temp->pos.x < x2)
        {
            if (temp->pos.y >= y1 && temp->pos.y < y2)
            {
                if (temp->pos.z >= z1 && temp->pos.z < z2)
                {
                    *furball = temp;
                    return index;
                }
            }
        }
        index++;
    }
    *furball = NULL;
    return -1;
}

void Units::CopyCreature(df::unit * source, t_unit & furball)
{
    if(!isValid()) return;
    // read pointer from vector at position
    furball.origin = source;

    //read creature from memory
    // name
    Translation::readName(furball.name, &source->name);

    // basic stuff
    furball.id = source->id;
    furball.x = source->pos.x;
    furball.y = source->pos.y;
    furball.z = source->pos.z;
    furball.race = source->race;
    furball.civ = source->civ_id;
    furball.sex = source->sex;
    furball.caste = source->caste;
    furball.flags1.whole = source->flags1.whole;
    furball.flags2.whole = source->flags2.whole;
    furball.flags3.whole = source->flags3.whole;
    // custom profession
    furball.custom_profession = source->custom_profession;
    // profession
    furball.profession = source->profession;
    // happiness
    furball.happiness = 100;//source->status.happiness;
    // physical attributes
    memcpy(&furball.strength, source->body.physical_attrs, sizeof(source->body.physical_attrs));

    // mood stuff
    furball.mood = source->mood;
    furball.mood_skill = source->job.mood_skill; // FIXME: really? More like currently used skill anyway.
    Translation::readName(furball.artifact_name, &source->status.artifact_name);

    // labors
    memcpy(&furball.labors, &source->status.labors, sizeof(furball.labors));

    furball.birth_year = source->relations.birth_year;
    furball.birth_time = source->relations.birth_time;
    furball.pregnancy_timer = source->relations.pregnancy_timer;
    // appearance
    furball.nbcolors = source->appearance.colors.size();
    if(furball.nbcolors>MAX_COLORS)
        furball.nbcolors = MAX_COLORS;
    for(uint32_t i = 0; i < furball.nbcolors; i++)
    {
        furball.color[i] = source->appearance.colors[i];
    }

    //likes. FIXME: where do they fit in now? The soul?
    /*
    DfVector <uint32_t> likes(d->p, temp + offs.creature_likes_offset);
    furball.numLikes = likes.getSize();
    for(uint32_t i = 0;i<furball.numLikes;i++)
    {
        uint32_t temp2 = *(uint32_t *) likes[i];
        p->read(temp2,sizeof(t_like),(uint8_t *) &furball.likes[i]);
    }
    */
    /*
    if(d->Ft_soul)
    {
        uint32_t soul = p->readDWord(addr_cr + offs.default_soul_offset);
        furball.has_default_soul = false;

        if(soul)
        {
            furball.has_default_soul = true;
            // get first soul's skills
            DfVector <uint32_t> skills(soul + offs.soul_skills_vector_offset);
            furball.defaultSoul.numSkills = skills.size();

            for (uint32_t i = 0; i < furball.defaultSoul.numSkills;i++)
            {
                uint32_t temp2 = skills[i];
                // a byte: this gives us 256 skills maximum.
                furball.defaultSoul.skills[i].id = p->readByte (temp2);
                furball.defaultSoul.skills[i].rating =
                    p->readByte (temp2 + offsetof(t_skill, rating));
                furball.defaultSoul.skills[i].experience =
                    p->readWord (temp2 + offsetof(t_skill, experience));
            }

            // mental attributes are part of the soul
            p->read(soul + offs.soul_mental_offset,
                sizeof(t_attrib) * NUM_CREATURE_MENTAL_ATTRIBUTES,
                (uint8_t *)&furball.defaultSoul.analytical_ability);

            // traits as well
            p->read(soul + offs.soul_traits_offset,
                sizeof (uint16_t) * NUM_CREATURE_TRAITS,
                (uint8_t *) &furball.defaultSoul.traits);
        }
    }
    */
    if(source->job.current_job == NULL)
    {
        furball.current_job.active = false;
    }
    else
    {
        furball.current_job.active = true;
        furball.current_job.jobType = source->job.current_job->job_type;
        furball.current_job.jobId = source->job.current_job->id;
    }
}

int32_t Units::FindIndexById(int32_t creature_id)
{
    return df::unit::binsearch_index(world->units.all, creature_id);
}
/*
bool Creatures::WriteLabors(const uint32_t index, uint8_t labors[NUM_CREATURE_LABORS])
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;

    p->write(temp + d->creatures.labors_offset, NUM_CREATURE_LABORS, labors);
    uint32_t pickup_equip;
    p->readDWord(temp + d->creatures.pickup_equipment_bit, pickup_equip);
    pickup_equip |= 1u;
    p->writeDWord(temp + d->creatures.pickup_equipment_bit, pickup_equip);
    return true;
}

bool Creatures::WriteHappiness(const uint32_t index, const uint32_t happinessValue)
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord (temp + d->creatures.happiness_offset, happinessValue);
    return true;
}

bool Creatures::WriteFlags(const uint32_t index,
                           const uint32_t flags1,
                           const uint32_t flags2)
{
    if(!d->Started || !d->Ft_basic) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord (temp + d->creatures.flags1_offset, flags1);
    p->writeDWord (temp + d->creatures.flags2_offset, flags2);
    return true;
}

bool Creatures::WriteFlags(const uint32_t index,
                           const uint32_t flags1,
                           const uint32_t flags2,
                           const uint32_t flags3)
{
    if(!d->Started || !d->Ft_basic) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord (temp + d->creatures.flags1_offset, flags1);
    p->writeDWord (temp + d->creatures.flags2_offset, flags2);
    p->writeDWord (temp + d->creatures.flags3_offset, flags3);
    return true;
}

bool Creatures::WriteSkills(const uint32_t index, const t_soul &soul)
{
    if(!d->Started || !d->Ft_soul) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    uint32_t souloff = p->readDWord(temp + d->creatures.default_soul_offset);

    if(!souloff)
    {
        return false;
    }

    DfVector<uint32_t> skills(souloff + d->creatures.soul_skills_vector_offset);

    for (uint32_t i=0; i<soul.numSkills; i++)
    {
        uint32_t temp2 = skills[i];
        p->writeByte(temp2 + offsetof(t_skill, rating), soul.skills[i].rating);
        p->writeWord(temp2 + offsetof(t_skill, experience), soul.skills[i].experience);
    }

    return true;
}

bool Creatures::WriteAttributes(const uint32_t index, const t_creature &creature)
{
    if(!d->Started || !d->Ft_advanced || !d->Ft_soul) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    uint32_t souloff = p->readDWord(temp + d->creatures.default_soul_offset);

    if(!souloff)
    {
        return false;
    }

    // physical attributes
    p->write(temp + d->creatures.physical_offset,
        sizeof(t_attrib) * NUM_CREATURE_PHYSICAL_ATTRIBUTES,
        (uint8_t *)&creature.strength);

    // mental attributes are part of the soul
    p->write(souloff + d->creatures.soul_mental_offset,
        sizeof(t_attrib) * NUM_CREATURE_MENTAL_ATTRIBUTES,
        (uint8_t *)&creature.defaultSoul.analytical_ability);

    return true;
}

bool Creatures::WriteSex(const uint32_t index, const uint8_t sex)
{
    if(!d->Started || !d->Ft_basic ) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeByte (temp + d->creatures.sex_offset, sex);

    return true;
}

bool Creatures::WriteTraits(const uint32_t index, const t_soul &soul)
{
    if(!d->Started || !d->Ft_soul) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    uint32_t souloff = p->readDWord(temp + d->creatures.default_soul_offset);

    if(!souloff)
    {
        return false;
    }

    p->write(souloff + d->creatures.soul_traits_offset,
            sizeof (uint16_t) * NUM_CREATURE_TRAITS,
            (uint8_t *) &soul.traits);

    return true;
}

bool Creatures::WriteMood(const uint32_t index, const uint16_t mood)
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeWord(temp + d->creatures.mood_offset, mood);
    return true;
}

bool Creatures::WriteMoodSkill(const uint32_t index, const uint16_t moodSkill)
{
    if(!d->Started || !d->Ft_advanced) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeWord(temp + d->creatures.mood_skill_offset, moodSkill);
    return true;
}

bool Creatures::WriteJob(const t_creature * furball, std::vector<t_material> const& mat)
{
    if(!d->Inited || !d->Ft_job_materials) return false;
    if(!furball->current_job.active) return false;

    unsigned int i;
    Process * p = d->owner;
    Private::t_offsets & off = d->creatures;
    DfVector <uint32_t> cmats(furball->current_job.occupationPtr + off.job_materials_vector);

    for(i=0;i<cmats.size();i++)
    {
        p->writeWord(cmats[i] + off.job_material_itemtype_o, mat[i].itemType);
        p->writeWord(cmats[i] + off.job_material_subtype_o, mat[i].itemSubtype);
        p->writeWord(cmats[i] + off.job_material_subindex_o, mat[i].subIndex);
        p->writeDWord(cmats[i] + off.job_material_index_o, mat[i].index);
        p->writeDWord(cmats[i] + off.job_material_flags_o, mat[i].flags);
    }
    return true;
}

bool Creatures::WritePos(const uint32_t index, const t_creature &creature)
{
    if(!d->Started) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->write (temp + d->creatures.pos_offset, 3 * sizeof (uint16_t), (uint8_t *) & (creature.x));
    return true;
}

bool Creatures::WriteCiv(const uint32_t index, const int32_t civ)
{
    if(!d->Started) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord(temp + d->creatures.civ_offset, civ);
    return true;
}

bool Creatures::WritePregnancy(const uint32_t index, const uint32_t pregTimer)
{
    if(!d->Started) return false;

    uint32_t temp = d->p_cre->at (index);
    Process * p = d->owner;
    p->writeDWord(temp + d->creatures.pregnancy_offset, pregTimer);
    return true;
}
*/
uint32_t Units::GetDwarfRaceIndex()
{
    return ui->race_id;
}

int32_t Units::GetDwarfCivId()
{
    return ui->civ_id;
}
/*
bool Creatures::getCurrentCursorCreature(uint32_t & creature_index)
{
    if(!d->cursorWindowInited) return false;
    Process * p = d->owner;
    creature_index = p->readDWord(d->current_cursor_creature_offset);
    return true;
}
*/
/*
bool Creatures::ReadJob(const t_creature * furball, vector<t_material> & mat)
{
    unsigned int i;
    if(!d->Inited || !d->Ft_job_materials) return false;
    if(!furball->current_job.active) return false;

    Process * p = d->owner;
    Private::t_offsets & off = d->creatures;
    DfVector <uint32_t> cmats(furball->current_job.occupationPtr + off.job_materials_vector);
    mat.resize(cmats.size());
    for(i=0;i<cmats.size();i++)
    {
        mat[i].itemType = p->readWord(cmats[i] + off.job_material_itemtype_o);
        mat[i].itemSubtype = p->readWord(cmats[i] + off.job_material_subtype_o);
        mat[i].subIndex = p->readWord(cmats[i] + off.job_material_subindex_o);
        mat[i].index = p->readDWord(cmats[i] + off.job_material_index_o);
        mat[i].flags = p->readDWord(cmats[i] + off.job_material_flags_o);
    }
    return true;
}
*/
bool Units::ReadInventoryByIdx(const uint32_t index, std::vector<df::item *> & item)
{
    if(index >= world->units.all.size()) return false;
    df::unit * temp = world->units.all[index];
    return ReadInventoryByPtr(temp, item);
}

bool Units::ReadInventoryByPtr(const df::unit * unit, std::vector<df::item *> & items)
{
    if(!isValid()) return false;
    if(!unit) return false;
    items.clear();
    for (size_t i = 0; i < unit->inventory.size(); i++)
        items.push_back(unit->inventory[i]->item);
    return true;
}

void Units::CopyNameTo(df::unit * creature, df::language_name * target)
{
    Translation::copyName(&creature->name, target);
}

df::coord Units::getPosition(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    if (unit->flags1.bits.caged)
    {
        auto cage = getContainer(unit);
        if (cage)
            return Items::getPosition(cage);
    }

    return unit->pos;
}

df::general_ref *Units::getGeneralRef(df::unit *unit, df::general_ref_type type)
{
    CHECK_NULL_POINTER(unit);

    return findRef(unit->general_refs, type);
}

df::specific_ref *Units::getSpecificRef(df::unit *unit, df::specific_ref_type type)
{
    CHECK_NULL_POINTER(unit);

    return findRef(unit->specific_refs, type);
}

df::item *Units::getContainer(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return findItemRef(unit->general_refs, general_ref_type::CONTAINED_IN_ITEM);
}

static df::identity *getFigureIdentity(df::historical_figure *figure)
{
    if (figure && figure->info && figure->info->reputation)
        return df::identity::find(figure->info->reputation->cur_identity);

    return NULL;
}

df::identity *Units::getIdentity(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    df::historical_figure *figure = df::historical_figure::find(unit->hist_figure_id);

    return getFigureIdentity(figure);
}

void Units::setNickname(df::unit *unit, std::string nick)
{
    CHECK_NULL_POINTER(unit);

    // There are >=3 copies of the name, and the one
    // in the unit is not the authoritative one.
    // This is the reason why military units often
    // lose nicknames set from Dwarf Therapist.
    Translation::setNickname(&unit->name, nick);

    if (unit->status.current_soul)
        Translation::setNickname(&unit->status.current_soul->name, nick);

    df::historical_figure *figure = df::historical_figure::find(unit->hist_figure_id);
    if (figure)
    {
        Translation::setNickname(&figure->name, nick);

        if (auto identity = getFigureIdentity(figure))
        {
            auto id_hfig = df::historical_figure::find(identity->histfig_id);

            if (id_hfig)
            {
                // Even DF doesn't do this bit, because it's apparently
                // only used for demons masquerading as gods, so you
                // can't ever change their nickname in-game.
                Translation::setNickname(&id_hfig->name, nick);
            }
            else
                Translation::setNickname(&identity->name, nick);
        }
    }
}

df::language_name *Units::getVisibleName(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    if (auto identity = getIdentity(unit))
    {
        auto id_hfig = df::historical_figure::find(identity->histfig_id);

        if (id_hfig)
            return &id_hfig->name;

        return &identity->name;
    }

    return &unit->name;
}

df::nemesis_record *Units::getNemesis(df::unit *unit)
{
    if (!unit)
        return NULL;

    for (unsigned i = 0; i < unit->general_refs.size(); i++)
    {
        df::nemesis_record *rv = unit->general_refs[i]->getNemesis();
        if (rv && rv->unit == unit)
            return rv;
    }

    return NULL;
}


bool Units::isHidingCurse(df::unit *unit)
{
    if (!unit->job.hunt_target)
    {
        auto identity = Units::getIdentity(unit);
        if (identity && identity->unk_4c == 0)
            return true;
    }

    return false;
}

int Units::getPhysicalAttrValue(df::unit *unit, df::physical_attribute_type attr)
{
    auto &aobj = unit->body.physical_attrs[attr];
    int value = std::max(0, aobj.value - aobj.soft_demotion);

    if (auto mod = unit->curse.attr_change)
    {
        int mvalue = (value * mod->phys_att_perc[attr] / 100) + mod->phys_att_add[attr];

        if (isHidingCurse(unit))
            value = std::min(value, mvalue);
        else
            value = mvalue;
    }

    return std::max(0, value);
}

int Units::getMentalAttrValue(df::unit *unit, df::mental_attribute_type attr)
{
    auto soul = unit->status.current_soul;
    if (!soul) return 0;

    auto &aobj = soul->mental_attrs[attr];
    int value = std::max(0, aobj.value - aobj.soft_demotion);

    if (auto mod = unit->curse.attr_change)
    {
        int mvalue = (value * mod->ment_att_perc[attr] / 100) + mod->ment_att_add[attr];

        if (isHidingCurse(unit))
            value = std::min(value, mvalue);
        else
            value = mvalue;
    }

    return std::max(0, value);
}

static bool casteFlagSet(int race, int caste, df::caste_raw_flags flag)
{
    auto creature = df::creature_raw::find(race);
    if (!creature)
        return false;

    auto craw = vector_get(creature->caste, caste);
    if (!craw)
        return false;

    return craw->flags.is_set(flag);
}

bool Units::isCrazed(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->flags3.bits.scuttle)
        return false;
    if (unit->curse.rem_tags1.bits.CRAZED)
        return false;
    if (unit->curse.add_tags1.bits.CRAZED)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::CRAZED);
}

bool Units::isOpposedToLife(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.OPPOSED_TO_LIFE)
        return false;
    if (unit->curse.add_tags1.bits.OPPOSED_TO_LIFE)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::OPPOSED_TO_LIFE);
}

bool Units::hasExtravision(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.EXTRAVISION)
        return false;
    if (unit->curse.add_tags1.bits.EXTRAVISION)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::EXTRAVISION);
}

bool Units::isBloodsucker(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.BLOODSUCKER)
        return false;
    if (unit->curse.add_tags1.bits.BLOODSUCKER)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::BLOODSUCKER);
}

bool Units::isMischievous(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);
    if (unit->curse.rem_tags1.bits.MISCHIEVOUS)
        return false;
    if (unit->curse.add_tags1.bits.MISCHIEVOUS)
        return true;
    return casteFlagSet(unit->race, unit->caste, caste_raw_flags::MISCHIEVOUS);
}

df::unit_misc_trait *Units::getMiscTrait(df::unit *unit, df::misc_trait_type type, bool create)
{
    CHECK_NULL_POINTER(unit);

    auto &vec = unit->status.misc_traits;
    for (size_t i = 0; i < vec.size(); i++)
        if (vec[i]->id == type)
            return vec[i];

    if (create)
    {
        auto obj = new df::unit_misc_trait();
        obj->id = type;
        vec.push_back(obj);
        return obj;
    }

    return NULL;
}

bool DFHack::Units::isDead(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->flags1.bits.dead ||
           unit->flags3.bits.ghostly;
}

bool DFHack::Units::isAlive(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return !unit->flags1.bits.dead &&
           !unit->flags3.bits.ghostly &&
           !unit->curse.add_tags1.bits.NOT_LIVING;
}

bool DFHack::Units::isSane(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    if (unit->flags1.bits.dead ||
        unit->flags3.bits.ghostly ||
        isOpposedToLife(unit) ||
        unit->enemy.undead)
        return false;

    if (unit->enemy.normal_race == unit->enemy.were_race && isCrazed(unit))
        return false;

    switch (unit->mood)
    {
    case mood_type::Melancholy:
    case mood_type::Raving:
    case mood_type::Berserk:
        return false;
    default:
        break;
    }

    return true;
}

bool DFHack::Units::isCitizen(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    // Copied from the conditions used to decide game over,
    // except that the game appears to let melancholy/raving
    // dwarves count as citizens.

    if (!isDwarf(unit) || !isSane(unit))
        return false;

    if (unit->flags1.bits.marauder ||
        unit->flags1.bits.invader_origin ||
        unit->flags1.bits.active_invader ||
        unit->flags1.bits.forest ||
        unit->flags1.bits.merchant ||
        unit->flags1.bits.diplomat)
        return false;

    if (unit->flags1.bits.tame)
        return true;

    return unit->civ_id == ui->civ_id &&
           unit->civ_id != -1 &&
           !unit->flags2.bits.underworld &&
           !unit->flags2.bits.resident &&
           !unit->flags2.bits.visitor_uninvited &&
           !unit->flags2.bits.visitor;
}

bool DFHack::Units::isDwarf(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    return unit->race == ui->race_id ||
           unit->enemy.normal_race == ui->race_id;
}

double DFHack::Units::getAge(df::unit *unit, bool true_age)
{
    using df::global::cur_year;
    using df::global::cur_year_tick;

    CHECK_NULL_POINTER(unit);

    if (!cur_year || !cur_year_tick)
        return -1;

    double year_ticks = 403200.0;
    double birth_time = unit->relations.birth_year + unit->relations.birth_time/year_ticks;
    double cur_time = *cur_year + *cur_year_tick / year_ticks;

    if (!true_age && unit->relations.curse_year >= 0)
    {
        if (auto identity = getIdentity(unit))
        {
            if (identity->histfig_id < 0)
                birth_time = identity->birth_year + identity->birth_second/year_ticks;
        }
    }

    return cur_time - birth_time;
}

inline void adjust_skill_rating(int &rating, bool is_adventure, int value, int dwarf3_4, int dwarf1_2, int adv9_10, int adv3_4, int adv1_2)
{
    if  (is_adventure)
    {
        if (value >= adv1_2) rating >>= 1;
        else if (value >= adv3_4) rating = rating*3/4;
        else if (value >= adv9_10) rating = rating*9/10;
    }
    else
    {
        if (value >= dwarf1_2) rating >>= 1;
        else if (value >= dwarf3_4) rating = rating*3/4;
    }
}

int Units::getNominalSkill(df::unit *unit, df::job_skill skill_id, bool use_rust)
{
    CHECK_NULL_POINTER(unit);

    /*
     * This is 100% reverse-engineered from DF code.
     */

    if (!unit->status.current_soul)
        return 0;

    // Retrieve skill from unit soul:

    auto skill = binsearch_in_vector(unit->status.current_soul->skills, &df::unit_skill::id, skill_id);

    if (skill)
    {
        int rating = int(skill->rating);
        if (use_rust)
            rating -= skill->rusty;
        return std::max(0, rating);
    }

    return 0;
}

int Units::getExperience(df::unit *unit, df::job_skill skill_id, bool total)
{
    CHECK_NULL_POINTER(unit);

    if (!unit->status.current_soul)
        return 0;

    auto skill = binsearch_in_vector(unit->status.current_soul->skills, &df::unit_skill::id, skill_id);
    if (!skill)
        return 0;

    int xp = skill->experience;
    // exact formula used by the game:
    if (total && skill->rating > 0)
        xp += 500*skill->rating + 100*skill->rating*(skill->rating - 1)/2;
    return xp;
}

int Units::getEffectiveSkill(df::unit *unit, df::job_skill skill_id)
{
    /*
     * This is 100% reverse-engineered from DF code.
     */

    int rating = getNominalSkill(unit, skill_id, true);

    // Apply special states

    if (unit->counters.soldier_mood == df::unit::T_counters::None)
    {
        if (unit->counters.nausea > 0) rating >>= 1;
        if (unit->counters.winded > 0) rating >>= 1;
        if (unit->counters.stunned > 0) rating >>= 1;
        if (unit->counters.dizziness > 0) rating >>= 1;
        if (unit->counters2.fever > 0) rating >>= 1;
    }

    if (unit->counters.soldier_mood != df::unit::T_counters::MartialTrance)
    {
        if (!unit->flags3.bits.ghostly && !unit->flags3.bits.scuttle &&
            !unit->flags2.bits.vision_good && !unit->flags2.bits.vision_damaged &&
            !hasExtravision(unit))
        {
            rating >>= 2;
        }
        if (unit->counters.pain >= 100 && unit->mood == -1)
        {
            rating >>= 1;
        }
        if (unit->counters2.exhaustion >= 2000)
        {
            rating = rating*3/4;
            if (unit->counters2.exhaustion >= 4000)
            {
                rating = rating*3/4;
                if (unit->counters2.exhaustion >= 6000)
                    rating = rating*3/4;
            }
        }
    }

    // Hunger etc timers

    bool is_adventure = (gamemode && *gamemode == game_mode::ADVENTURE);

    if (!unit->flags3.bits.scuttle && isBloodsucker(unit))
    {
        using namespace df::enums::misc_trait_type;

        if (auto trait = getMiscTrait(unit, TimeSinceSuckedBlood))
        {
            adjust_skill_rating(
                rating, is_adventure, trait->value,
                302400, 403200,           // dwf 3/4; 1/2
                1209600, 1209600, 2419200 // adv 9/10; 3/4; 1/2
            );
        }
    }

    adjust_skill_rating(
        rating, is_adventure, unit->counters2.thirst_timer,
        50000, 50000, 115200, 172800, 345600
    );
    adjust_skill_rating(
        rating, is_adventure, unit->counters2.hunger_timer,
        75000, 75000, 172800, 1209600, 2592000
    );
    if (is_adventure && unit->counters2.sleepiness_timer >= 846000)
        rating >>= 2;
    else
        adjust_skill_rating(
            rating, is_adventure, unit->counters2.sleepiness_timer,
            150000, 150000, 172800, 259200, 345600
        );

    return rating;
}

inline void adjust_speed_rating(int &rating, bool is_adventure, int value, int dwarf100, int dwarf200, int adv50, int adv75, int adv100, int adv200)
{
    if  (is_adventure)
    {
        if (value >= adv200) rating += 200;
        else if (value >= adv100) rating += 100;
        else if (value >= adv75) rating += 75;
        else if (value >= adv50) rating += 50;
    }
    else
    {
        if (value >= dwarf200) rating += 200;
        else if (value >= dwarf100) rating += 100;
    }
}

static int calcInventoryWeight(df::unit *unit)
{
    int armor_skill = Units::getEffectiveSkill(unit, job_skill::ARMOR);
    int armor_mul = 15 - std::min(15, armor_skill);

    int inv_weight = 0, inv_weight_fraction = 0;

    for (size_t i = 0; i < unit->inventory.size(); i++)
    {
        auto item = unit->inventory[i]->item;
        if (!item->flags.bits.weight_computed)
            continue;

        int wval = item->weight;
        int wfval = item->weight_fraction;
        auto mode = unit->inventory[i]->mode;

        if ((mode == df::unit_inventory_item::Worn ||
             mode == df::unit_inventory_item::WrappedAround) &&
             item->isArmor() && armor_skill > 1)
        {
            wval = wval * armor_mul / 16;
            wfval = wfval * armor_mul / 16;
        }

        inv_weight += wval;
        inv_weight_fraction += wfval;
    }

    return inv_weight*100 + inv_weight_fraction/10000;
}

int Units::computeMovementSpeed(df::unit *unit)
{
    using namespace df::enums::physical_attribute_type;

    CHECK_NULL_POINTER(unit);

    /*
     * Pure reverse-engineered computation of unit _slowness_,
     * i.e. number of ticks to move * 100.
     */

    // Base speed

    auto creature = df::creature_raw::find(unit->race);
    if (!creature)
        return 0;

    auto craw = vector_get(creature->caste, unit->caste);
    if (!craw)
        return 0;

    int speed = craw->misc.speed;

    if (unit->flags3.bits.ghostly)
        return speed;

    // Curse multiplier

    if (unit->curse.speed_mul_percent != 100)
    {
        speed *= 100;
        if (unit->curse.speed_mul_percent != 0)
            speed /= unit->curse.speed_mul_percent;
    }

    speed += unit->curse.speed_add;

    // Swimming

    auto cur_liquid = unit->status2.liquid_type.bits.liquid_type;
    bool in_magma = (cur_liquid == tile_liquid::Magma);

    if (unit->flags2.bits.swimming)
    {
        speed = craw->misc.swim_speed;
        if (in_magma)
            speed *= 2;

        if (craw->flags.is_set(caste_raw_flags::SWIMS_LEARNED))
        {
            int skill = Units::getEffectiveSkill(unit, job_skill::SWIMMING);

            // Originally a switch:
            if (skill > 1)
                speed = speed * std::max(6, 21-skill) / 20;
        }
    }
    else
    {
        int delta = 150*unit->status2.liquid_depth;
        if (in_magma)
            delta *= 2;
        speed += delta;
    }

    // General counters and flags

    if (unit->profession == profession::BABY)
        speed += 3000;

    if (unit->flags3.bits.unk15)
        speed /= 20;

    if (unit->counters2.exhaustion >= 2000)
    {
        speed += 200;
        if (unit->counters2.exhaustion >= 4000)
        {
            speed += 200;
            if (unit->counters2.exhaustion >= 6000)
                speed += 200;
        }
    }

    if (unit->flags2.bits.gutted) speed += 2000;

    if (unit->counters.soldier_mood == df::unit::T_counters::None)
    {
        if (unit->counters.nausea > 0) speed += 1000;
        if (unit->counters.winded > 0) speed += 1000;
        if (unit->counters.stunned > 0) speed += 1000;
        if (unit->counters.dizziness > 0) speed += 1000;
        if (unit->counters2.fever > 0) speed += 1000;
    }

    if (unit->counters.soldier_mood != df::unit::T_counters::MartialTrance)
    {
        if (unit->counters.pain >= 100 && unit->mood == -1)
            speed += 1000;
    }

    // Hunger etc timers

    bool is_adventure = (gamemode && *gamemode == game_mode::ADVENTURE);

    if (!unit->flags3.bits.scuttle && Units::isBloodsucker(unit))
    {
        using namespace df::enums::misc_trait_type;

        if (auto trait = Units::getMiscTrait(unit, TimeSinceSuckedBlood))
        {
            adjust_speed_rating(
                speed, is_adventure, trait->value,
                302400, 403200,                    // dwf 100; 200
                1209600, 1209600, 1209600, 2419200 // adv 50; 75; 100; 200
            );
        }
    }

    adjust_speed_rating(
        speed, is_adventure, unit->counters2.thirst_timer,
        50000, 0x7fffffff, 172800, 172800, 172800, 345600
    );
    adjust_speed_rating(
        speed, is_adventure, unit->counters2.hunger_timer,
        75000, 0x7fffffff, 1209600, 1209600, 1209600, 2592000
    );
    adjust_speed_rating(
        speed, is_adventure, unit->counters2.sleepiness_timer,
        57600, 150000, 172800, 259200, 345600, 864000
    );

    // Activity state

    if (unit->relations.draggee_id != -1) speed += 1000;

    if (unit->flags1.bits.on_ground)
        speed += 2000;
    else if (unit->flags3.bits.on_crutch)
    {
        int skill = Units::getEffectiveSkill(unit, job_skill::CRUTCH_WALK);
        speed += 2000 - 100*std::min(20, skill);
    }

    if (unit->flags1.bits.hidden_in_ambush && !Units::isMischievous(unit))
    {
        int skill = Units::getEffectiveSkill(unit, job_skill::SNEAK);
        speed += 2000 - 100*std::min(20, skill);
    }

    if (unsigned(unit->counters2.paralysis-1) <= 98)
        speed += unit->counters2.paralysis*10;
    if (unsigned(unit->counters.webbed-1) <= 8)
        speed += unit->counters.webbed*100;

    // Muscle and fat weight vs expected size

    auto &s_info = unit->body.size_info;
    speed = std::max(speed*3/4, std::min(speed*3/2, int(int64_t(speed)*s_info.size_cur/s_info.size_base)));

    // Attributes

    int strength_attr = Units::getPhysicalAttrValue(unit, STRENGTH);
    int agility_attr = Units::getPhysicalAttrValue(unit, AGILITY);

    int total_attr = std::max(200, std::min(3800, strength_attr + agility_attr));
    speed = ((total_attr-200)*(speed/2) + (3800-total_attr)*(speed*3/2))/3600;

    // Stance

    if (!unit->flags1.bits.on_ground && unit->status2.limbs_stand_max > 2)
    {
        // WTF
        int as = unit->status2.limbs_stand_max;
        int x = (as-1) - (as>>1);
        int y = as - unit->status2.limbs_stand_count;
        if (unit->flags3.bits.on_crutch) y--;
        y = y * 500 / x;
        if (y > 0) speed += y;
    }

    // Mood

    if (unit->mood == mood_type::Melancholy) speed += 8000;

    // Inventory encumberance

    int total_weight = calcInventoryWeight(unit);
    int free_weight = std::max(1, s_info.size_cur/10 + strength_attr*3);

    if (free_weight < total_weight)
    {
        int delta = (total_weight - free_weight)/10 + 1;
        if (!is_adventure)
            delta = std::min(5000, delta);
        speed += delta;
    }

    // skipped: unknown loop on inventory items that amounts to 0 change

    if (is_adventure)
    {
        auto player = vector_get(world->units.active, 0);
        if (player && player->id == unit->relations.group_leader_id)
            speed = std::min(speed, computeMovementSpeed(player));
    }

    return std::min(10000, std::max(0, speed));
}

static bool entityRawFlagSet(int civ_id, df::entity_raw_flags flag)
{
    auto entity = df::historical_entity::find(civ_id);

    return entity && entity->entity_raw && entity->entity_raw->flags.is_set(flag);
}

float Units::computeSlowdownFactor(df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    /*
     * These slowdowns are actually done by skipping a move if random(x) != 0, so
     * it follows the geometric distribution. The mean expected slowdown is x.
     */

    float coeff = 1.0f;

    if (!unit->job.hunt_target && (!gamemode || *gamemode == game_mode::DWARF))
    {
        if (!unit->flags1.bits.marauder &&
            casteFlagSet(unit->race, unit->caste, caste_raw_flags::MEANDERER) &&
            !(unit->relations.following && isCitizen(unit)) &&
            linear_index(unit->inventory, &df::unit_inventory_item::mode,
                         df::unit_inventory_item::Hauled) < 0)
        {
            coeff *= 4.0f;
        }

        if (unit->relations.group_leader_id < 0 &&
            unit->flags1.bits.active_invader &&
            !unit->job.current_job && !unit->flags3.bits.no_meandering &&
            unit->profession != profession::THIEF && unit->profession != profession::MASTER_THIEF &&
            !entityRawFlagSet(unit->civ_id, entity_raw_flags::ITEM_THIEF))
        {
            coeff *= 3.0f;
        }
    }

    if (unit->flags3.bits.floundering)
    {
        coeff *= 3.0f;
    }

    return coeff;
}


static bool noble_pos_compare(const Units::NoblePosition &a, const Units::NoblePosition &b)
{
    if (a.position->precedence < b.position->precedence)
        return true;
    if (a.position->precedence > b.position->precedence)
        return false;
    return a.position->id < b.position->id;
}

bool DFHack::Units::getNoblePositions(std::vector<NoblePosition> *pvec, df::unit *unit)
{
    CHECK_NULL_POINTER(unit);

    pvec->clear();

    auto histfig = df::historical_figure::find(unit->hist_figure_id);
    if (!histfig)
        return false;

    for (size_t i = 0; i < histfig->entity_links.size(); i++)
    {
        auto link = histfig->entity_links[i];
        auto epos = strict_virtual_cast<df::histfig_entity_link_positionst>(link);
        if (!epos)
            continue;

        NoblePosition pos;

        pos.entity = df::historical_entity::find(epos->entity_id);
        if (!pos.entity)
            continue;

        pos.assignment = binsearch_in_vector(pos.entity->positions.assignments, epos->assignment_id);
        if (!pos.assignment)
            continue;

        pos.position = binsearch_in_vector(pos.entity->positions.own, pos.assignment->position_id);
        if (!pos.position)
            continue;

        pvec->push_back(pos);
    }

    if (pvec->empty())
        return false;

    std::sort(pvec->begin(), pvec->end(), noble_pos_compare);
    return true;
}

std::string DFHack::Units::getProfessionName(df::unit *unit, bool ignore_noble, bool plural)
{
    CHECK_NULL_POINTER(unit);

    std::string prof = unit->custom_profession;
    if (!prof.empty())
        return prof;

    std::vector<NoblePosition> np;

    if (!ignore_noble && getNoblePositions(&np, unit))
    {
        switch (unit->sex)
        {
        case 0:
            prof = np[0].position->name_female[plural ? 1 : 0];
            break;
        case 1:
            prof = np[0].position->name_male[plural ? 1 : 0];
            break;
        default:
            break;
        }

        if (prof.empty())
            prof = np[0].position->name[plural ? 1 : 0];
        if (!prof.empty())
            return prof;
    }

    return getCasteProfessionName(unit->race, unit->caste, unit->profession, plural);
}

std::string DFHack::Units::getCasteProfessionName(int race, int casteid, df::profession pid, bool plural)
{
    std::string prof, race_prefix;

	if (pid < (df::profession)0 || !is_valid_enum_item(pid))
		return "";
	int16_t current_race = df::global::ui->race_id;
	if (df::global::gamemode && *df::global::gamemode == df::game_mode::ADVENTURE)
		current_race = world->units.active[0]->race;
	bool use_race_prefix = (race >= 0 && race != current_race);

    if (auto creature = df::creature_raw::find(race))
    {
        if (auto caste = vector_get(creature->caste, casteid))
        {
            race_prefix = caste->caste_name[0];

            if (plural)
                prof = caste->caste_profession_name.plural[pid];
            else
                prof = caste->caste_profession_name.singular[pid];

            if (prof.empty())
            {
                switch (pid)
                {
                case profession::CHILD:
                    prof = caste->child_name[plural ? 1 : 0];
                    if (!prof.empty())
                        use_race_prefix = false;
                    break;

                case profession::BABY:
                    prof = caste->baby_name[plural ? 1 : 0];
                    if (!prof.empty())
                        use_race_prefix = false;
                    break;

                default:
                    break;
                }
            }
        }

        if (race_prefix.empty())
            race_prefix = creature->name[0];

        if (prof.empty())
        {
            if (plural)
                prof = creature->profession_name.plural[pid];
            else
                prof = creature->profession_name.singular[pid];

            if (prof.empty())
            {
                switch (pid)
                {
                case profession::CHILD:
                    prof = creature->general_child_name[plural ? 1 : 0];
                    if (!prof.empty())
                        use_race_prefix = false;
                    break;

                case profession::BABY:
                    prof = creature->general_baby_name[plural ? 1 : 0];
                    if (!prof.empty())
                        use_race_prefix = false;
                    break;

                default:
                    break;
                }
            }
        }
    }

    if (race_prefix.empty())
        race_prefix = "Animal";

    if (prof.empty())
    {
        switch (pid)
        {
        case profession::TRAINED_WAR:
            prof = "War " + (use_race_prefix ? race_prefix : "Peasant");
            use_race_prefix = false;
            break;

        case profession::TRAINED_HUNTER:
            prof = "Hunting " + (use_race_prefix ? race_prefix : "Peasant");
            use_race_prefix = false;
            break;

        case profession::STANDARD:
            if (!use_race_prefix)
                prof = "Peasant";
            break;

        default:
            if (auto caption = ENUM_ATTR(profession, caption, pid))
                prof = caption;
            else
                prof = ENUM_KEY_STR(profession, pid);
        }
    }

    if (use_race_prefix)
    {
        if (!prof.empty())
            race_prefix += " ";
        prof = race_prefix + prof;
    }

    return Translation::capitalize(prof, true);
}

int8_t DFHack::Units::getProfessionColor(df::unit *unit, bool ignore_noble)
{
    CHECK_NULL_POINTER(unit);

    std::vector<NoblePosition> np;

    if (!ignore_noble && getNoblePositions(&np, unit))
    {
        if (np[0].position->flags.is_set(entity_position_flags::COLOR))
            return np[0].position->color[0] + np[0].position->color[2] * 8;
    }

    return getCasteProfessionColor(unit->race, unit->caste, unit->profession);
}

int8_t DFHack::Units::getCasteProfessionColor(int race, int casteid, df::profession pid)
{
    // make sure it's an actual profession
    if (pid < 0 || !is_valid_enum_item(pid))
        return 3;

    // If it's not a Peasant, it's hardcoded
    if (pid != profession::STANDARD)
        return ENUM_ATTR(profession, color, pid);

    if (auto creature = df::creature_raw::find(race))
    {
        if (auto caste = vector_get(creature->caste, casteid))
        {
            if (caste->flags.is_set(caste_raw_flags::CASTE_COLOR))
                return caste->caste_color[0] + caste->caste_color[2] * 8;
        }
        return creature->color[0] + creature->color[2] * 8;
    }

    // default to dwarven peasant color
    return 3;
}

std::string DFHack::Units::getSquadName(df::unit *unit)
{
    if (unit->military.squad_id == -1)
        return "";
    df::squad *squad = df::squad::find(unit->military.squad_id);
    if (!squad)
        return "";
    if (squad->alias.size() > 0)
        return squad->alias;
    return Translation::TranslateName(&squad->name, true);
}

df::unit* DFHack::Units::createUnit(int32_t raceId, int32_t casteId, df::coord pos) {
    DFHack::Console& out = Core::getInstance().getConsole();

    CHECK_INVALID_ARGUMENT(pos.x!=-30000);
    
    //TODO: use modules/Random
    df::unit* unit = new df::unit();
    unit->pos=pos;
    df::creature_raw* race = df::creature_raw::find(raceId);
    if ( !race ) {
        delete unit;
        CHECK_INVALID_ARGUMENT(race!=0);
    }
    df::caste_raw* caste = race->caste[casteId];
    if ( !caste ) {
        delete unit;
        CHECK_INVALID_ARGUMENT(caste!=0);
    }
    //create unit
    {
        unit->race = raceId;
        unit->caste = casteId;
        unit->sex = caste->gender;
        unit->relations.birth_year = *(df::global::cur_year);
        if ( caste->misc.maxage_max == -1 ) {
            unit->relations.old_year = -1;
        } else {
            unit->relations.old_year = unit->relations.birth_year;
            int32_t maxage = (int32_t)((rand()/(1+(float)RAND_MAX))*(caste->misc.maxage_max - caste->misc.maxage_min));
            unit->relations.old_year += maxage;
            //TODO: old_time
        }
        unit->relations.birth_time = *(df::global::cur_year_tick);

        size_t num_inter = caste->body_info.interactions.size();
        unit->curse.own_interaction.resize(num_inter);
        unit->curse.own_interaction_delay.resize(num_inter);

        //body stuff
        unit->body.body_plan = &caste->body_info;
        size_t body_part_count = caste->body_info.body_parts.size();
        size_t layer_count = caste->body_info.layer_part.size();
        unit->body.components.body_part_status.resize(body_part_count);
        unit->body.components.numbered_masks.resize(caste->body_info.numbered_masks.size());
        for ( size_t a = 0; a < caste->body_info.numbered_masks.size(); a++ ) {
            unit->body.components.numbered_masks[a] = caste->body_info.numbered_masks[a];
        }
        unit->body.components.layer_status.resize(layer_count);
        unit->body.components.layer_wound_area.resize(layer_count);
        unit->body.components.layer_cut_fraction.resize(layer_count);
        unit->body.components.layer_dent_fraction.resize(layer_count);
        unit->body.components.layer_effect_fraction.resize(layer_count);

        const size_t phys_att_count = df::enum_traits<df::physical_attribute_type>::last_item_value + 1; 
        //shenanigans to avoid magic constants
        const size_t phys_att_range_size = sizeof(caste->attributes.phys_att_range[0]) / sizeof(int32_t);
        for ( size_t a = 0; a < phys_att_count; a++ ) {
            int32_t max_percent = caste->attributes.phys_att_cap_perc[a];
            //pick a random index in the array, then pick a random number between the arr[index] and arr[index+1]
            int32_t index = (int32_t)((rand()/(1+(float)RAND_MAX))*(phys_att_range_size-1)); 
            int32_t cvalue = (int32_t)((rand()/(1+(float)RAND_MAX))*(caste->attributes.phys_att_range[a][index+1] - caste->attributes.phys_att_range[a][index])) + caste->attributes.phys_att_range[a][index];
            unit->body.physical_attrs[a].value = cvalue;
            unit->body.physical_attrs[a].max_value = (cvalue*max_percent) / 100;
        }

        unit->body.blood_max = caste->body_size_1[caste->body_size_1.size()-1]; //TODO: distribute correctly
        unit->body.blood_count = unit->body.blood_max;
        unit->body.infection_level = 0;
        unit->status2.body_part_temperature.resize(body_part_count);
        for ( size_t a = 0; a < body_part_count; a++ ) {
            df::temperaturest* temp = df::allocate<df::temperaturest>();
            temp->whole = 10067; //TODO: look for homeotherm or something?
            temp->fraction = 0;
            unit->status2.body_part_temperature[a] = temp;
        }

        //largely unknown stuff
        unit->enemy.body_part_878.resize(body_part_count);
        unit->enemy.body_part_888.resize(body_part_count);
        unit->enemy.body_part_relsize.resize(body_part_count);

        unit->enemy.were_race = raceId;
        unit->enemy.were_caste = casteId;
        unit->enemy.normal_race = raceId;
        unit->enemy.normal_caste = casteId;
        unit->enemy.body_part_8a8.resize(body_part_count);
        unit->enemy.body_part_base_ins.resize(body_part_count);
        unit->enemy.body_part_clothing_ins.resize(body_part_count);
        unit->enemy.body_part_8d8.resize(body_part_count);

        int32_t size = caste->body_size_2[caste->body_size_2.size()-1];
        unit->body.size_info.size_cur = size;
        unit->body.size_info.size_base = size;
        unit->body.size_info.area_cur = (int32_t)pow(size,0.666);
        unit->body.size_info.area_base = (int32_t)pow(size,0.666);
        unit->body.size_info.length_cur = (int32_t)pow(size*10000,0.333);
        unit->body.size_info.length_base = (int32_t)pow(size*10000,0.333);

        unit->recuperation.healing_rate.resize(layer_count);

        //appearance
        unit->appearance.body_modifiers.resize(caste->body_appearance_modifiers.size());
        for (size_t a = 0; a < caste->body_appearance_modifiers.size(); a++ ) {
            df::body_appearance_modifier* mod = caste->body_appearance_modifiers[a];
            size_t rangeSize = sizeof(mod->ranges) / sizeof(int32_t);
            size_t index = (size_t)((rand()/(1+(float)RAND_MAX))*(-1+rangeSize));
            int32_t value = (int32_t)((rand()/(1+(float)RAND_MAX))*(mod->ranges[index+1] - mod->ranges[index]));
            unit->appearance.body_modifiers[a] = value;
        }
        //size_t idxSize = caste->bp_appearance.modifier_idx.size();
        size_t modifierSize = caste->bp_appearance.modifiers.size();
        unit->appearance.bp_modifiers.resize(modifierSize);
        for ( size_t a = 0; a < unit->appearance.bp_modifiers.size(); a++ ) {
            df::bp_appearance_modifier* mod = caste->bp_appearance.modifiers[a];
            size_t rangeSize = sizeof(mod->ranges) / sizeof(int32_t);
            size_t index = (size_t)((rand()/(1+(float)RAND_MAX))*(-1+rangeSize));
            int32_t value = (int32_t)((rand()/(1+(float)RAND_MAX))*(mod->ranges[index+1] - mod->ranges[index]));
            unit->appearance.bp_modifiers[a] = value;
        }

        unit->appearance.tissue_style.resize(modifierSize);
        unit->appearance.tissue_style_civ_id.resize(modifierSize);
        unit->appearance.tissue_style_id.resize(modifierSize);
        unit->appearance.tissue_style_type.resize(modifierSize);
        unit->appearance.tissue_length.resize(modifierSize);
        
        unit->appearance.genes.appearance.resize(caste->body_appearance_modifiers.size() + caste->bp_appearance.modifiers.size());
        unit->appearance.genes.colors.resize(caste->color_modifiers.size()*2); //???
        unit->appearance.colors.resize(caste->color_modifiers.size()); //3
    }
    
    unit->id = *(df::global::unit_next_id);
    *(df::global::unit_next_id)++;
    df::global::world->units.all.push_back(unit);
    df::global::world->units.active.push_back(unit);
    
    //make soul
    {
        df::unit_soul* soul = new df::unit_soul;
        soul->unit_id = unit->id;
        //soul->name.assign(unit->name);
        soul->race = unit->race;
        soul->sex = unit->sex;
        soul->caste = unit->caste;
        //TODO: preferences.traits.
        const size_t ment_att_count = df::enum_traits<df::mental_attribute_type>::last_item_value + 1; 
        const size_t ment_att_range_size = sizeof(caste->attributes.phys_att_range[0]) / sizeof(int32_t);
        for ( size_t a = 0; a < ment_att_count; a++ ) {
            int32_t max_percent = caste->attributes.ment_att_cap_perc[a];
            int32_t index = (int32_t)((rand()/(1+(float)RAND_MAX))*(ment_att_range_size-1));
            int32_t cvalue = (int32_t)((rand()/(1+(float)RAND_MAX))*(caste->attributes.ment_att_range[a][index+1] - caste->attributes.ment_att_range[a][index]));
            soul->mental_attrs[a].value = cvalue;
            soul->mental_attrs[a].max_value = (cvalue*max_percent) / 100;
        }
        const size_t traitCount = sizeof(soul->traits) / sizeof(soul->traits[0]);
        for ( size_t a = 0; a < traitCount; a++ ) {
            int16_t min = caste->personality.a[a];
            int16_t mean = caste->personality.b[a];
            int16_t max = caste->personality.c[a];
            double sd = sqrt((double)(max-min));
            //normal distribution
            double z = sqrt((-2)*log(rand()/(1+(double)RAND_MAX)))*cos(2*M_PI*rand()/(1+(double)RAND_MAX));
            z *= sd;
            z += mean;
            if ( z < min )
                z = min;
            if ( z > max )
                z = max;
            soul->traits[a] = (int16_t)z;
        }
        
        for ( size_t a = 0; a < caste->natural_skill_id.size(); a++ ) {
            df::unit_skill* skill = new df::unit_skill;
            skill->id = (df::enums::job_skill::job_skill)caste->natural_skill_id[a];
            skill->experience = caste->natural_skill_exp[a];
            skill->rating = (df::enums::skill_rating::skill_rating)caste->natural_skill_lvl[a];
            soul->skills.push_back(skill);
        }
        unit->status.souls.push_back(soul);
        unit->status.current_soul = soul;
    }

    return unit;
}

void DFHack::Units::makeHistorical(df::unit* unit, df::historical_entity* civ, df::historical_entity* group) {

    CHECK_NULL_POINTER(unit);
    CHECK_NULL_POINTER(civ);
    CHECK_NULL_POINTER(group);

    df::historical_figure* histfig = new df::historical_figure;
    histfig->id = *df::global::hist_figure_next_id;
    (*df::global::hist_figure_next_id)++;
    histfig->race = unit->race;
    histfig->caste = unit->caste;
    histfig->profession = unit->profession;
    histfig->sex = unit->sex;
    histfig->appeared_year = unit->relations.birth_year;
    histfig->born_year = unit->relations.birth_year;
    histfig->born_seconds = unit->relations.birth_time;
    histfig->curse_year = unit->relations.curse_year;
    histfig->curse_seconds = unit->relations.curse_time;
    histfig->birth_year_bias = unit->relations.birth_year_bias;
    histfig->birth_time_bias = unit->relations.birth_time_bias;
    histfig->old_year = unit->relations.old_year;
    histfig->old_seconds = unit->relations.old_time;
    histfig->died_year = -1;
    histfig->died_seconds = -1;
    histfig->name = unit->name;
    if ( civ )
        histfig->civ_id = civ->id;
    else
        histfig->civ_id = -1;
    histfig->population_id = unit->population_id;
    histfig->breed_id = -1;
    histfig->unit_id = unit->id;

    df::global::world->history.figures.push_back(histfig);

    histfig->info = new df::historical_figure_info;
    histfig->info->unk_14 = new df::historical_figure_info::T_unk_14; //hf state?
    //unk_14.region_id = -1; unk_14.beast_id = -1; unk_14.unk_14 = 0
    histfig->info->unk_14->unk_18 = -1;
    histfig->info->unk_14->unk_1c = -1;
    // set values that seem related to state and do event
    //change_state(hf, dfg.ui.site_id, region_pos)
    
    //TODO: skills
    histfig->info->skills = new df::historical_figure_info::T_skills;
    
    civ->histfig_ids.push_back(histfig->id);
    civ->hist_figures.push_back(histfig);
    if (group) {
        group->histfig_ids.push_back(histfig->id);
        group->hist_figures.push_back(histfig);
        df::histfig_entity_link_memberst* link = df::allocate<df::histfig_entity_link_memberst>();
        link->entity_id = group->id;
        link->link_strength = 100;
        histfig->entity_links.push_back(link);
    }
    unit->flags1.bits.important_historical_figure = true;
    unit->flags2.bits.important_historical_figure = true;
    unit->hist_figure_id = histfig->id;
    unit->hist_figure_id2 = histfig->id;

    {
        df::histfig_entity_link_memberst* link = df::allocate<df::histfig_entity_link_memberst>();
        link->entity_id = unit->civ_id;
        link->link_strength = 100;
        histfig->entity_links.push_back(link);
    }
    
    {
        df::history_event_add_hf_entity_linkst* link = df::allocate<df::history_event_add_hf_entity_linkst>();
        link->year = unit->relations.birth_year;
        link->seconds = unit->relations.birth_time;
        link->id = histfig->id + 1; //TODO: really?
        link->civ = histfig->civ_id;
        link->histfig = histfig->id;
        link->link_type = df::enums::histfig_entity_link_type::MEMBER;
        df::global::world->history.events.push_back(link);
    }
    return;
}

void DFHack::Units::makeNemesis(df::unit* unit) {

    CHECK_NULL_POINTER(unit);

    df::nemesis_record* nem = new df::nemesis_record;
    nem->id = (*df::global::nemesis_next_id)++;
    nem->unit_id = unit->id;
    nem->unit = unit;
    nem->flags.resize(4);
    for ( size_t i = 4; i <= 9; i++ ) {
        nem->flags.set((df::enums::nemesis_flags::nemesis_flags)i); //TODO: I have no idea if this works
        //nem->flags[i] = true;
    }
    nem->unk10 = -1;
    nem->unk11 = -1;
    nem->unk12 = -1;
    
    df::global::world->nemesis.all.push_back(nem);
    df::general_ref_is_nemesisst* nemesisRef = df::allocate<df::general_ref_is_nemesisst>();
    nemesisRef->nemesis_id = nem->id;
    unit->general_refs.push_back(nemesisRef);
    
    nem->save_file_id = -1;
    
    df::historical_entity* civ = df::historical_entity::find(unit->civ_id);
    df::historical_figure* histfig = df::historical_figure::find(unit->hist_figure_id);
    //find group of unit, if any
    for ( size_t a = 0; a < histfig->entity_links.size(); a++ ) {
        df::historical_entity* entity = df::historical_entity::find(histfig->entity_links[a]->entity_id);
        entity->nemesis_ids.push_back(nem->id);
        entity->nemesis.push_back(nem);
    }
    nem->figure = histfig;
    //allocateIds
    if (civ->next_member_idx == 100) {
        //allocate new chunk
        civ->save_file_id = (*df::global::unit_chunk_next_id)++;
        civ->next_member_idx = 0;
    }
    nem->save_file_id = civ->save_file_id;
    nem->member_idx = civ->next_member_idx++;
    return;
}

