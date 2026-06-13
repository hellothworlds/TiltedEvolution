#include <Services/DebugService.h>

#include <Components.h>
#include <World.h>

#include <Messages/NotifyQuestUpdate.h>
#include <Messages/NotifyActorValueChanges.h>
#include <Messages/NotifyDrawWeapon.h>

#include <imgui.h>
#include <inttypes.h>

// -- Log ----------------------------------------------------------------------

static Vector<String> s_log;

static void Log(const char* aFmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, aFmt);
    vsnprintf(buf, sizeof(buf), aFmt, args);
    va_end(args);
    s_log.emplace_back(buf);
    if (s_log.size() > 200)
        s_log.erase(s_log.begin());
}

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
    {
        Log("[Quest] Bad form ID: %s", s_questFormIdHex);
        return;
    }

    NotifyQuestUpdate msg{};
    msg.Id.BaseId = baseId;
    msg.Id.ModId = 0;
    msg.Stage = static_cast<uint16_t>(s_questStage);
    msg.Status = static_cast<uint8_t>(s_questStatus);
    msg.ClientQuestType = 2;  // STORY — passes the misc quest filter

    aDispatcher.trigger(msg);
    Log("[Quest] Injected: formId=%X stage=%d status=%d", baseId, s_questStage, s_questStatus);
}

// -- Actor Values (Health) ----------------------------------------------------

static int s_avIndex = 24;   // 24 = Health
static float s_avValue = 100.f;

static void DoActorValueInject(entt::dispatcher& aDispatcher, World& aWorld)
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
        Log("[AV] No remote player entity found.");
        return;
    }

    NotifyActorValueChanges msg{};
    msg.Id = remoteServerId;
    msg.Values[static_cast<uint32_t>(s_avIndex)] = s_avValue;

    aDispatcher.trigger(msg);
    Log("[AV] Injected: serverId=%u av=%d value=%.1f", remoteServerId, s_avIndex, s_avValue);
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
        Log("[Weapon] Injected: formId=%X drawn=%s", formId.Id, s_weaponDrawn ? "true" : "false");
        break;
    }
}

// -- Main view ----------------------------------------------------------------

void DebugService::DrawSyncTestView() noexcept
{
    ImGui::SetNextWindowSize(ImVec2(420, 560), ImGuiCond_FirstUseEver);
    ImGui::Begin("Sync Tests");

    // ---- Quest Sync ----
    if (ImGui::CollapsingHeader("Quest Sync", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputText("Form ID (hex)", s_questFormIdHex, sizeof(s_questFormIdHex));
        ImGui::InputInt("Stage", &s_questStage);
        ImGui::Combo("Status", &s_questStatus, kStatusNames, 3);

        if (ImGui::Button("Inject Quest Update"))
            DoQuestInject(m_dispatcher);

        ImGui::SameLine();
        ImGui::TextDisabled("(e.g. 3372B=MQ101, 4B2D4=Bleak Falls)");
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
            DoActorValueInject(m_dispatcher, m_world);
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
            DoWeaponDrawInject(m_dispatcher, m_world);
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    // ---- Log ----
    ImGui::Text("Log");
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear"))
        s_log.clear();

    ImGui::BeginChild("##synclog", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : s_log)
        ImGui::TextUnformatted(line.c_str());

    // auto-scroll to bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.f);

    ImGui::EndChild();

    ImGui::End();
}
