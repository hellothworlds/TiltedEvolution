#pragma once

#include "Events/ConnectedEvent.h"
#include "Events/DisconnectedEvent.h"

#include <atomic>
#include <Client.hpp>

struct ImguiService;
struct UpdateEvent;
struct ClientMessage;
struct AuthenticationResponse;
struct NotifySettingsChange;

struct World;

using TiltedPhoques::Client;

/**
 * @brief Handles communication with the server.
 */
struct TransportService : Client
{
    TransportService(World& aWorld, entt::dispatcher& aDispatcher) noexcept;
    ~TransportService() noexcept = default;

    TP_NOCOPYMOVE(TransportService);

    bool Send(const ClientMessage& acMessage) const noexcept;

    // Shadows Client::Connect — saves the address so auto-reconnect can reuse it.
    void Connect(const std::string& acAddress) noexcept;

    void OnConsume(const void* apData, uint32_t aSize) override;
    void OnConnected() override;
    void OnDisconnected(EDisconnectReason aReason) override;
    void OnUpdate() override;

    [[nodiscard]] bool IsOnline() const noexcept { return m_connected; }
    void SetServerPassword(const std::string& acPassword) noexcept { m_serverPassword = acPassword; }
    const uint32_t& GetLocalPlayerId() const noexcept { return m_localPlayerId; }

protected:
    // Event handlers
    void HandleUpdate(const UpdateEvent& acEvent) noexcept;
    void HandleConnected(const ConnectedEvent& acEvent) noexcept;
    void HandleDisconnected(const DisconnectedEvent& acEvent) noexcept;

    // Packet handlers
    void HandleAuthenticationResponse(const AuthenticationResponse& acMessage) noexcept;
    void HandleNotifySettingsChange(const NotifySettingsChange& acMessage) noexcept;

private:
    static constexpr uint32_t kMaxReconnectAttempts  = 5;
    static constexpr uint32_t kReconnectCooldownFrames = 180; // ~3 s at 60 fps

    World& m_world;
    entt::dispatcher& m_dispatcher;
    bool m_connected;
    String m_serverPassword{};
    uint32_t m_localPlayerId;

    String   m_lastServerAddress{};
    uint32_t m_reconnectAttempts{0};
    uint32_t m_reconnectCooldown{0};

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_sendServerMessageConnection;
    entt::scoped_connection m_settingsChangeConnection;
    entt::scoped_connection m_connectedConnection;
    entt::scoped_connection m_disconnectedConnection;
    std::function<void(UniquePtr<ServerMessage>&)> m_messageHandlers[kServerOpcodeMax];
};
