#include "ScreenLogger.hpp"
#include <cstring>

namespace SeqActLogEnabler
{

ScreenLogger::ScreenLogger(const wchar_t* name)
    : m_name(name)
    , m_messageBuffer{}
{
}

void ScreenLogger::LogMessage(const wchar_t* text)
{
    m_messageBuffer[m_head] = FString(text);
    m_head = (m_head + 1) % MaxMessages;
}

void ScreenLogger::RenderText(FString const& msg, float x, float y, float r, float g, float b, UCanvas* canvas)
{
    canvas->SetDrawColor(
        static_cast<unsigned char>(r * 255),
        static_cast<unsigned char>(g * 255),
        static_cast<unsigned char>(b * 255),
        255
    );
    canvas->SetPos(x, y);

    FLinearColor drawColor;
    drawColor.R = r;
    drawColor.G = g;
    drawColor.B = b;
    drawColor.A = 1.0f;

    FVector2D glowBorder;
    glowBorder.X = 2;
    glowBorder.Y = 2;

    FFontRenderInfo renderInfo;
    std::memset(&renderInfo, 0, sizeof(renderInfo));
    renderInfo.bClipText = true;
    renderInfo.bEnableShadow = true;
    renderInfo.GlowInfo.bEnableGlow = false;
    renderInfo.GlowInfo.GlowColor = drawColor;
    renderInfo.GlowInfo.GlowInnerRadius = glowBorder;
    renderInfo.GlowInfo.GlowOuterRadius = glowBorder;

    canvas->DrawText(msg, 1, 1.0f, 1.0f, &renderInfo);
}

void ScreenLogger::PostRenderer(ABioHUD* hud) const
{
    if (!hud || !hud->Canvas) return;

    // Render the logger name at the top
    RenderText(m_name, 0, 0, 0, 1, 0, hud->Canvas);

    // Render messages from newest to oldest
    for (size_t i = 0; i < MaxMessages; i++)
    {
        size_t idx = (m_head + MaxMessages - 1 - i) % MaxMessages;
        if (m_messageBuffer[idx].has_value())
        {
            RenderText(m_messageBuffer[idx].value(), 0, static_cast<float>(i * 12 + 12), 0, 1, 0, hud->Canvas);
        }
    }
}

} // namespace SeqActLogEnabler
