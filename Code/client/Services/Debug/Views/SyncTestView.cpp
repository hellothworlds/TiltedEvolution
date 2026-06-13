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

static void DoQuestInject(entt::dispatcher& aDispatcher, DebugService& aSelf)
{
    uint32_t baseId = 0;
    sscanf_s(s_questFormIdHex, "%X", &baseId);
    if (!baseId)
    {
        aSelf.SyncLog("[INJECT][Quest] Bad form ID: %s", s_questFormIdHex);
        return;
    }

    NotifyQuestUpdate msg{};
    msg.Id.BaseId = baseId;
    msg.Id.ModId = 0;
    msg.Stage = static_cast<uint16_t>(s_questStage);
    msg.Status = static_cast<uint8_t>(s_questStatus);
    msg.ClientQuestType = 2;  // STORY — passes the misc quest filter

    aSelf.SyncLog("[INJECT][Quest] formId=%X stage=%d status=%d", baseId, s_questStage, s_questStatus);
    aDispatcher.trigger(msg);
    // [RECV] log line will also appear from the DebugService dispatcher listener
}

// -- Actor Values (Health) ----------------------------------------------------

static int s_avIndex = 24;   // 24 = Health
static float s_avValue = 100.f;

static void DoActorValueInject(entt::dispatcher& aDispatcher, World& aWorld, DebugService& aSelf)
{
    uint32_t remoteServerId = 0;
    bool found = false;

    auto view = aWorld.view<RemoteComponent>();
    for (auto entity : view)
    {
        auto& remote = view.get<RemoteComponent>(entity);
        remoteServerId = remote.Id;
        found = true;
        break;
    }

    if (!found)
    {
        aSelf.SyncLog("[INJECT][AV] No remote player entity found.");
        return;
    }

    NotifyActorValueChanges msg{};
    msg.Id = remoteServerId;
    msg.Values[static_cast<uint32_t>(s_avIndex)] = s_avValue;

    aSelf.SyncLog("[INJECT][AV] serverId=%u av=%d value=%.1f", remoteServerId, s_avIndex, s_avValue);
    aDispatcher.trigger(msg);
}

// -- Weapon Draw --------------------------------------------------------------

static bool s_weaponDrawn = true;

static void DoWeaponDrawInject(entt::dispatcher& aDispatcher, World& aWorld, DebugService& aSelf)
{
    auto view = aWorld.view<RemoteComponent, FormIdComponent>();
    for (auto entity : view)
    {
        auto& formId = view.get<FormIdComponent>(entity);

        NotifyDrawWeapon msg{};
        msg.Id = formId.Id;
        msg.IsWeaponDrawn = s_weaponDrawn;

        aSelf.SyncLog("[INJECT][Weapon] formId=%X drawn=%s", formId.Id, s_weaponDrawn ? "true" : "false");
        aDispatcher.trigger(msg);
        break;
    }
}

// -- Main view ----------------------------------------------------------------

void DebugService::DrawSyncTestView() noexcept
{
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    ImGui::Begin("Sync Tests");

    // ---- Quest Sync ----
    if (ImGui::CollapsingHeader("Quest Sync", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputText("Form ID (hex)", s_questFormIdHex, sizeof(s_questFormIdHex));
        ImGui::InputInt("Stage", &s_questStage);
        ImGui::Combo("Status", &s_questStatus, kStatusNames, 3);

        if (ImGui::Button("Inject Quest Update"))
            DoQuestInject(m_dispatcher, *this);

        ImGui::SameLine();
        ImGui::TextDisabled("e.g. 3372B=MQ101  4B2D4=Bleak Falls");
    }

    ImGui::Separator();

    // ---- Actor Values / Health ----
    if (ImGui::CollapsingHeader("Actor Values (needs 2nd player)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputInt("AV Index  (24=Health  25=Magicka  26=Stamina)", &s_avIndex);
        ImGui::InputFloat("Value", &s_avValue, 1.f, 10.f, "%.1f");

        bool hasRemote = !m_world.view<RemoteComponent>().empty();
        if (!hasRemote)
            ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "No remote player found.");

        ImGui::BeginDisabled(!hasRemote);
        if (ImGui::Button("Inject Actor Value"))
            DoActorValueInject(m_dispatcher, m_world, *this);
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    // ---- Weapon Draw ----
    if (ImGui::CollapsingHeader("Weapon Draw (needs 2nd player)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Weapon Drawn", &s_weaponDrawn);

        bool hasRemoteWeapon = false;
        for (auto _ : m_world.view<RemoteComponent, FormIdComponent>()) { hasRemoteWeapon = true; break; }
        if (!hasRemoteWeapon)
            ImGui::TextColored({1.f, 0.4f, 0.4f, 1.f}, "No remote player found.");

        ImGui::BeginDisabled(!hasRemoteWeapon);
        if (ImGui::Button("Inject Draw State"))
            DoWeaponDrawInject(m_dispatcher, m_world, *this);
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    // ---- Log ----------------------------------------------------------------
    // Captures all injected messages AND real Notify* messages from the server.
    // [INJECT] = sent by a button above. [RECV] = arrived from the network.
    ImGui::Text("Log");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear"))
        m_syncLog.clear();

    ImGui::BeginChild("##synclog", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : m_syncLog)
        ImGui::TextUnformatted(line.c_str());

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.f);

    ImGui::EndChild();

    ImGui::End();
}
