#include <Services/DebugService.h>

#include <Components.h>
#include <World.h>

#include <Messages/NotifyQuestUpdate.h>
#include <Messages/NotifyActorValueChanges.h>
#include <Messages/NotifyDrawWeapon.h>

#include <imgui.h>
#include <inttypes.h>

// -- Quest Sync ---------------------------------------------------------------

static char s_questFormIdHex[16] = "3372B";  // default: Main Quest (MQ101)
static int s_questStage = 10;
static int s_questStatus = 0;  // 0=StageUpdate 1=Started 2=Stopped

static const char* kStatusNames[] = {"StageUpdate", "Started", "Stopped"};

static void DoQuestInject(entt::dispatcher& aDispatcher)
{
    uint32_t baseId = 0;
    sscanf_s(s_questFormIdHex, "%X", &baseId);
    if (!baseId)
        return;

    NotifyQuestUpdate msg{};
    msg.Id.BaseId = baseId;
    msg.Id.ModId = 0;
    msg.Stage = static_cast<uint16_t>(s_questStage);
    msg.Status = static_cast<uint8_t>(s_questStatus);
    msg.ClientQuestType = 2;  // STORY type — passes the bEnableMiscQuestSync gate

    aDispatcher.trigger(msg);
}

// -- Actor Values (Health) ----------------------------------------------------

static int s_avIndex = 24;   // 24 = Health
static float s_avValue = 100.f;

static void DoActorValueInject(entt::dispatcher& aDispatcher, World& aWorld)
{
    entt::entity firstRemote = entt::null;
    uint32_t remoteServerId = 0;

    auto view = aWorld.view<RemoteComponent>();
    for (auto entity : view)
    {
        auto& remote = view.get<RemoteComponent>(entity);
        firstRemote = entity;
        remoteServerId = remote.Id;
        break;
    }

    if (firstRemote == entt::null)
        return;

    NotifyActorValueChanges msg{};
    msg.Id = remoteServerId;
    msg.Values[static_cast<uint32_t>(s_avIndex)] = s_avValue;

    aDispatcher.trigger(msg);
}

// -- Weapon Draw --------------------------------------------------------------

static bool s_weaponDrawn = true;

static void DoWeaponDrawInject(entt::dispatcher& aDispatcher, World& aWorld)
{
    auto view = aWorld.view<RemoteComponent, FormIdComponent>();
    for (auto entity : view)
    {
        auto& formId = view.get<FormIdComponent>(entity);

        NotifyDrawWeapon msg{};
        msg.Id = formId.Id;
        msg.IsWeaponDrawn = s_weaponDrawn;
        aDispatcher.trigger(msg);
        break;
    }
}

// -- Main view ----------------------------------------------------------------

void DebugService::DrawSyncTestView() noexcept
{
    ImGui::SetNextWindowSize(ImVec2(380, 420), ImGuiCond_FirstUseEver);
    ImGui::Begin("Sync Tests");

    // ---- Quest Sync ----
    if (ImGui::CollapsingHeader("Quest Sync", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextDisabled("Injects a NotifyQuestUpdate into the local dispatcher.");
        ImGui::TextDisabled("Simulates the server telling this client a quest changed.");

        ImGui::InputText("Form ID (hex)", s_questFormIdHex, sizeof(s_questFormIdHex));
        ImGui::InputInt("Stage", &s_questStage);
        ImGui::Combo("Status", &s_questStatus, kStatusNames, 3);

        if (ImGui::Button("Inject Quest Update"))
            DoQuestInject(m_dispatcher);

        ImGui::Spacing();
        ImGui::TextDisabled("Example IDs: 3372B=MQ101, 4B2D4=Bleak Falls, A5B3B=Riverwood quest");
    }

    ImGui::Separator();

    // ---- Actor Values / Health ----
    if (ImGui::CollapsingHeader("Actor Values (needs 2nd player)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextDisabled("Sets an actor value on the first remote player entity.");
        ImGui::TextDisabled("Requires a second player connected to the server.");

        ImGui::InputInt("AV Index (24=Health, 25=Magicka, 26=Stamina)", &s_avIndex);
        ImGui::InputFloat("Value", &s_avValue, 1.f, 10.f, "%.1f");

        auto view = m_world.view<RemoteComponent>();
        bool hasRemote = !view.empty();
        if (!hasRemote)
            ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "No remote player found.");

        ImGui::BeginDisabled(!hasRemote);
        if (ImGui::Button("Inject Actor Value"))
            DoActorValueInject(m_dispatcher, m_world);
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    // ---- Weapon Draw ----
    if (ImGui::CollapsingHeader("Weapon Draw (needs 2nd player)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextDisabled("Forces the first remote player's weapon draw state.");

        ImGui::Checkbox("Weapon Drawn", &s_weaponDrawn);

        bool hasRemoteWeapon = false;
        for (auto _ : m_world.view<RemoteComponent, FormIdComponent>()) { hasRemoteWeapon = true; break; }
        if (!hasRemoteWeapon)
            ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "No remote player found.");

        ImGui::BeginDisabled(!hasRemoteWeapon);
        if (ImGui::Button("Inject Draw State"))
            DoWeaponDrawInject(m_dispatcher, m_world);
        ImGui::EndDisabled();
    }

    ImGui::End();
}
