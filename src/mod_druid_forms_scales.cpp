#include "ScriptMgr.h"
#include "Player.h"
#include "Config.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include <unordered_map>
#include <vector>

using namespace Acore::ChatCommands;

// Caché global en memoria: GUID -> (Forma -> Escala)
static std::unordered_map<uint32, std::unordered_map<uint8, float>> DruidScaleCache;

class DruidFormsScalesScript : public PlayerScript
{
public:
    DruidFormsScalesScript() : PlayerScript("DruidFormsScalesScript") {}

    void OnPlayerLogin(Player* player) override
    {
        if (!sConfigMgr->GetOption<bool>("DruidFormsScales.Enable", true))
            return;

        LoadAndApplyFormScale(player, false);
    }

    void OnPlayerMapChanged(Player* player) override
    {
        if (!sConfigMgr->GetOption<bool>("DruidFormsScales.Enable", true))
            return;

        LoadAndApplyFormScale(player, true);
    }

    // Este hook se dispara justo cuando el cliente termina la pantalla de carga
    void OnPlayerUpdateZone(Player* player, uint32 /*newZone*/, uint32 /*newArea*/) override
    {
        if (!sConfigMgr->GetOption<bool>("DruidFormsScales.Enable", true))
            return;

        ApplyCurrentFormScale(player, true);
    }

    // Respaldo por si el teletransporte ocurre dentro de la misma zona pero cambia de área
    void OnPlayerUpdateArea(Player* player, uint32 /*oldArea*/, uint32 /*newArea*/) override
    {
        if (!sConfigMgr->GetOption<bool>("DruidFormsScales.Enable", true))
            return;

        ApplyCurrentFormScale(player, true);
    }

    void OnPlayerLogout(Player* player) override
    {
        DruidScaleCache.erase(player->GetGUID().GetCounter());
    }

    static void LoadAndApplyFormScale(Player* player, bool force = false)
    {
        if (!player)
            return;

        uint32 guid = player->GetGUID().GetCounter();
        DruidScaleCache.erase(guid);

        QueryResult result = CharacterDatabase.Query(
            "SELECT form, scale FROM character_druid_forms_scales WHERE guid = {}", guid);

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint8 form = fields[0].Get<uint8>();
                float scale = fields[1].Get<float>();
                DruidScaleCache[guid][form] = scale;
            } while (result->NextRow());
        }

        ApplyCurrentFormScale(player, force);
    }

    static void ApplyCurrentFormScale(Player* player, bool force = false)
    {
        if (!player)
            return;

        uint8 currentForm = player->GetShapeshiftForm();
        if (currentForm == FORM_NONE)
            return; // Lo maneja mod-character-scale

        uint32 guid = player->GetGUID().GetCounter();
        auto it = DruidScaleCache.find(guid);
        if (it != DruidScaleCache.end())
        {
            auto formIt = it->second.find(currentForm);
            if (formIt != it->second.end())
            {
                float scale = formIt->second;
                
                // Si forzamos la actualización, ensuciamos el campo temporalmente para obligar
                // al servidor a enviar el paquete SMSG_UPDATE_OBJECT al cliente.
                if (force)
                {
                    player->SetObjectScale(scale == 1.0f ? 1.01f : 1.0f);
                }
                player->SetObjectScale(scale);
            }
        }
    }
};

class DruidFormsScalesUnitScript : public UnitScript
{
public:
    DruidFormsScalesUnitScript() : UnitScript("DruidFormsScalesUnitScript") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!sConfigMgr->GetOption<bool>("DruidFormsScales.Enable", true))
            return;

        Player* player = unit->ToPlayer();
        if (!player)
            return;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (spellInfo && spellInfo->HasAura(SPELL_AURA_MOD_SHAPESHIFT))
        {
            uint8 auraForm = FORM_NONE;
            for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (spellInfo->Effects[i].ApplyAuraName == SPELL_AURA_MOD_SHAPESHIFT)
                {
                    auraForm = spellInfo->Effects[i].MiscValue;
                    break;
                }
            }

            if (auraForm != FORM_NONE)
            {
                uint32 guid = player->GetGUID().GetCounter();
                auto it = DruidScaleCache.find(guid);
                if (it != DruidScaleCache.end())
                {
                    auto formIt = it->second.find(auraForm);
                    if (formIt != it->second.end())
                    {
                        player->SetObjectScale(formIt->second);
                    }
                }
            }
        }
    }
};

class DruidFormsScalesCommand : public CommandScript
{
public:
    DruidFormsScalesCommand() : CommandScript("DruidFormsScalesCommand") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable commandTable =
        {
            { "bear_scale",    HandleBearScale,    SEC_PLAYER, Console::No },
            { "cat_scale",     HandleCatScale,     SEC_PLAYER, Console::No },
            { "moonkin_scale", HandleMoonkinScale, SEC_PLAYER, Console::No },
            { "tree_scale",    HandleTreeScale,    SEC_PLAYER, Console::No },
            { "flight_scale",  HandleFlightScale,  SEC_PLAYER, Console::No },
            { "aquatic_scale", HandleAquaticScale, SEC_PLAYER, Console::No },
            { "travel_scale",  HandleTravelScale,  SEC_PLAYER, Console::No }
        };
        return commandTable;
    }

    static bool SetFormScale(ChatHandler* handler, Optional<float> scaleOpt, std::vector<uint8> const& forms, const char* cmdName)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player) return false;

        float minScale = sConfigMgr->GetOption<float>("DruidFormsScales.MinScale", 0.5f);
        float maxScale = sConfigMgr->GetOption<float>("DruidFormsScales.MaxScale", 2.0f);

        if (!scaleOpt.has_value())
        {
            handler->SendSysMessage(Acore::StringFormat("Uso: .{} <val> (Rango: {} - {})", cmdName, minScale, maxScale));
            return true;
        }

        float scale = *scaleOpt;
        if (scale < minScale || scale > maxScale)
        {
            handler->SendSysMessage(Acore::StringFormat("Escala fuera de rango. Debe estar entre {} y {}.", minScale, maxScale));
            return true;
        }

        uint32 guid = player->GetGUID().GetCounter();
        uint8 currentForm = player->GetShapeshiftForm();

        for (uint8 form : forms)
        {
            CharacterDatabase.Execute(
                "INSERT INTO character_druid_forms_scales (guid, form, scale) VALUES ({}, {}, {}) ON DUPLICATE KEY UPDATE scale = {}",
                guid, form, scale, scale
            );

            DruidScaleCache[guid][form] = scale;

            if (currentForm == form)
            {
                player->SetObjectScale(scale);
            }
        }

        handler->SendSysMessage(Acore::StringFormat("Escala guardada en {} para el comando .{}", scale, cmdName));
        return true;
    }

    static bool HandleBearScale(ChatHandler* handler, Optional<float> scaleOpt) 
    { 
        return SetFormScale(handler, scaleOpt, { FORM_BEAR, FORM_DIREBEAR }, "bear_scale"); 
    }
    static bool HandleCatScale(ChatHandler* handler, Optional<float> scaleOpt) 
    { 
        return SetFormScale(handler, scaleOpt, { FORM_CAT }, "cat_scale"); 
    }
    static bool HandleMoonkinScale(ChatHandler* handler, Optional<float> scaleOpt) 
    { 
        return SetFormScale(handler, scaleOpt, { FORM_MOONKIN }, "moonkin_scale"); 
    }
    static bool HandleTreeScale(ChatHandler* handler, Optional<float> scaleOpt) 
    { 
        return SetFormScale(handler, scaleOpt, { FORM_TREE }, "tree_scale"); 
    }
    static bool HandleFlightScale(ChatHandler* handler, Optional<float> scaleOpt) 
    { 
        return SetFormScale(handler, scaleOpt, { FORM_FLIGHT, FORM_FLIGHT_EPIC }, "flight_scale"); 
    }
    static bool HandleAquaticScale(ChatHandler* handler, Optional<float> scaleOpt) 
    { 
        return SetFormScale(handler, scaleOpt, { FORM_AQUA }, "aquatic_scale"); 
    }
    static bool HandleTravelScale(ChatHandler* handler, Optional<float> scaleOpt) 
    { 
        return SetFormScale(handler, scaleOpt, { FORM_TRAVEL }, "travel_scale"); 
    }
};

void Addmod_druid_forms_scalesScripts()
{
    new DruidFormsScalesScript();
    new DruidFormsScalesUnitScript();
    new DruidFormsScalesCommand();
}