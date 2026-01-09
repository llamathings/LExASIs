#pragma once

#include <array>
#include <optional>
#include <LESDK/Headers.hpp>

namespace SeqActLogEnabler
{

/**
 * @brief   On-screen message logger that displays messages during gameplay.
 * @remarks Adapted from WarrantyVoider's ME3OnTheHook screen logger.
 *          Messages are rendered via the HUD's Canvas during PostRender.
 */
class ScreenLogger
{
public:
    static constexpr size_t MaxMessages = 20;

    explicit ScreenLogger(const wchar_t* name);
    ~ScreenLogger() = default;

    ScreenLogger(const ScreenLogger&) = delete;
    ScreenLogger& operator=(const ScreenLogger&) = delete;

    /**
     * @brief   Add a message to the on-screen log buffer.
     * @param   text The message text to display.
     */
    void LogMessage(const wchar_t* text);

    /**
     * @brief   Render all buffered messages to the screen.
     * @param   hud The BioHUD instance to render through.
     */
    void PostRenderer(ABioHUD* hud) const;

private:
    static void RenderText(FString const& msg, float x, float y, float r, float g, float b, UCanvas* canvas);

    FString m_name;
    std::array<std::optional<FString>, MaxMessages> m_messageBuffer;
    size_t m_head = 0;  // Next write position (overwrites oldest)
};

} // namespace SeqActLogEnabler
