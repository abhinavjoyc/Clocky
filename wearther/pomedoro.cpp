// ============================================================================
// Pomodoro Timer Implementation
// Simple C++ ImGui application for time management using Pomodoro technique
// ============================================================================

#include "Weather.h"
#include <glad/glad.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <cmath>

#include "http.h"
#include "time.h"

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================

// Button States
static bool startButtonState = false;    // true = timer running, false = paused
static bool resumeButtonstate = true;    // true = show logo screen, false = show timer screen

// Time Cache (for UI display only)
static std::string currentTime1 = "00:00:00";
static double lastTimeUpdate1 = 0.0;

// Timer State Machine
enum TimerState {
    TIMER_FOCUS,        // Focus/work session
    TIMER_SHORT_BREAK,  // Short break between focus sessions
    TIMER_LONG_BREAK,   // Long break after every 4 rounds
    SESSION_ENDED
};

// Pomodoro Configuration Structure
struct pomedero {
    int rounds;         // Number of rounds to complete
    int focusTime;      // Focus session duration (minutes)
    int shortBreak;     // Short break duration (minutes)
    int longBreak;      // Long break duration (minutes)
};

// Default pomodoro settings
static pomedero pom = { 1, 25, 5, 0 };  // longBreak = 0 initially since rounds = 1

// Timer State Variables (persist between function calls)
static TimerState currentState = TIMER_FOCUS;
static int currentRound = 1;                    // Current round number (1 to rounds)
static int countdownSeconds = 25 * 60;          // Countdown in seconds
static double lastTick = 0.0;                   // Last time we updated the countdown
static bool timerInitialized = false;           // Has timer been initialized?

// Button Scaling and Positioning (for bottom control buttons)
static float stopScale = 0.5f;
static float startScale = 0.5f;
static float resetScale = 0.5f;

static ImVec2 stopOffset = ImVec2(0, 0);
static ImVec2 startOffset = ImVec2(20, 10);
static ImVec2 resetOffset = ImVec2(-20, -30);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// ----------------------------------------------------------------------------
// Get scaled texture size to fit within a maximum box while keeping aspect ratio
// ----------------------------------------------------------------------------
ImVec2 GetScaledSizeFromGLTexture(GLuint glTex, const ImVec2& maxBox)
{
    // If texture is invalid, return the max box size
    if (glTex == 0) {
        return maxBox;
    }

    // Save current texture binding so we can restore it later
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    // Bind our texture to query its properties
    glBindTexture(GL_TEXTURE_2D, glTex);

    // Get the actual width and height of the texture
    int width = 0;
    int height = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    // Restore previous texture binding
    glBindTexture(GL_TEXTURE_2D, previousTexture);

    // If texture dimensions are invalid, return max box size
    if (width <= 0 || height <= 0) {
        return maxBox;
    }

    // Calculate scale factor to fit within max box while keeping aspect ratio
    float scaleX = maxBox.x / (float)width;
    float scaleY = maxBox.y / (float)height;
    float scale = std::min(scaleX, scaleY);  // Use smaller scale to fit in box

    // Return scaled dimensions
    return ImVec2(width * scale, height * scale);
}

// ----------------------------------------------------------------------------
// Get raw (unscaled) texture dimensions
// ----------------------------------------------------------------------------
ImVec2 GetRawTexSize(GLuint tex)
{
    // If texture is invalid, return zero size
    if (tex == 0) {
        return ImVec2(0, 0);
    }

    // Save current texture binding
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    // Bind texture and query its size
    glBindTexture(GL_TEXTURE_2D, tex);

    int width = 0;
    int height = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    // Restore previous texture
    glBindTexture(GL_TEXTURE_2D, previousTexture);

    return ImVec2((float)width, (float)height);
}

// ----------------------------------------------------------------------------
// Reset pomodoro settings to default values
// ----------------------------------------------------------------------------
void ApplyPomodoroDefaults()
{
    pom.rounds = 1;
    pom.focusTime = 25;
    pom.shortBreak = 5;
    pom.longBreak = 0;  // 0 because rounds < 4
}

// ----------------------------------------------------------------------------
// Start a new timer session (Focus, Short Break, or Long Break)
// ----------------------------------------------------------------------------
void StartTimerSession(TimerState newState)
{
    // Set the new state
    currentState = newState;

    // Set countdown time based on session type
    if (newState == TIMER_FOCUS) {
        countdownSeconds = 10;
    }
    else if (newState == TIMER_SHORT_BREAK) {
        countdownSeconds = 5;
    }
    else if (newState == TIMER_LONG_BREAK) {
        countdownSeconds = 5;
    }
    else if (newState == SESSION_ENDED) {
        countdownSeconds = 5;  // Show 00:00
    }

    // Reset the tick timer
    lastTick = ImGui::GetTime();
}

// ----------------------------------------------------------------------------
// Advance to next session when current session finishes
// FIXED LOGIC:
// - After focus session, check if we just completed the last round
// - If yes, go to SESSION_ENDED
// - If no, give appropriate break, then increment round after break
// ----------------------------------------------------------------------------
void AdvanceToNextSession()
{
    if (currentState == TIMER_FOCUS) {
        // Check if we just finished the last round
        if (currentRound >= pom.rounds) {
            // All rounds complete - show session ended
            StartTimerSession(SESSION_ENDED);
            return;
        }

        // We still have more rounds to do - give a break
        bool shouldGiveLongBreak = false;

        // Only consider long break if total rounds is 4 or more
        if (pom.rounds >= 4) {
            // Give long break after every 4th round (4, 8, 12, etc.)
            if (currentRound % 4 == 0) {
                shouldGiveLongBreak = true;
            }
        }

        // Decide which break to give
        if (shouldGiveLongBreak) {
            StartTimerSession(TIMER_LONG_BREAK);
        }
        else {
            StartTimerSession(TIMER_SHORT_BREAK);
        }
    }
    else if (currentState == TIMER_SHORT_BREAK) {
        // After short break, increment round and start next focus
        currentRound++;
        StartTimerSession(TIMER_FOCUS);
    }
    else if (currentState == TIMER_LONG_BREAK) {
        // After long break, increment round and start next focus
        currentRound++;
        StartTimerSession(TIMER_FOCUS);
    }
    else if (currentState == SESSION_ENDED) {
        // After showing "Session Ended", reset everything like stop button
        startButtonState = false;

        // Reset all settings to defaults
        pom.rounds = 1;
        pom.focusTime = 25;
        pom.shortBreak = 5;
        pom.longBreak = 0;

        // Reset timer state
        currentRound = 1;
        currentState = TIMER_FOCUS;
        countdownSeconds = pom.focusTime * 60;
        timerInitialized = false;
        lastTick = ImGui::GetTime();

        // Return to logo screen
        resumeButtonstate = true;
    }
}

// ============================================================================
// MAIN UI FUNCTION
// ============================================================================
void PomederoTab(ImGuiIO& io, std::vector<GLuint>& textures, ImFont* bigFont)
{
    // ------------------------------------------------------------------------
    // VALIDATE POMODORO SETTINGS
    // ------------------------------------------------------------------------
    // Make sure settings have valid values
    if (pom.focusTime <= 0 || pom.shortBreak <= 0) {
        ApplyPomodoroDefaults();
    }

    // ------------------------------------------------------------------------
    // INITIALIZE TIMER ON FIRST ENTRY TO TIMER SCREEN
    // ------------------------------------------------------------------------
    if (!resumeButtonstate && !timerInitialized) {
        // User pressed START from logo screen - initialize timer
        currentRound = 1;
        StartTimerSession(TIMER_FOCUS);
        timerInitialized = true;
    }

    // ------------------------------------------------------------------------
    // BACKGROUND IMAGE (full screen)
    // ------------------------------------------------------------------------
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    // Remove padding and borders for clean background
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));  // Transparent background

    ImGui::Begin("BG", nullptr,
        ImGuiWindowFlags_NoDecoration |       // No title bar, scrollbar, etc
        ImGuiWindowFlags_NoInputs |           // Doesn't receive input
        ImGuiWindowFlags_NoMove |             // Cannot be moved
        ImGuiWindowFlags_NoResize |           // Cannot be resized
        ImGuiWindowFlags_NoScrollbar |        // No scrollbar
        ImGuiWindowFlags_NoSavedSettings);    // Don't save settings

    // Draw background image if available (textures[0])
    if (textures.size() > 0) {
        ImGui::Image((ImTextureID)(intptr_t)textures[0], io.DisplaySize);
    }

    ImGui::End();

    // Restore original style
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    // ========================================================================
    // LOGO SCREEN (shown when resumeButtonstate == true)
    // ========================================================================
    if (resumeButtonstate)
    {
        // --------------------------------------------------------------------
        // LOGO IMAGE (centered)
        // --------------------------------------------------------------------
        ImVec2 logoMaxSize = ImVec2(500, 500);
        ImVec2 logoScaled = GetScaledSizeFromGLTexture(
            textures.size() > 2 ? textures[2] : 0,
            logoMaxSize
        );

        // Calculate position to center logo
        float logoX = (io.DisplaySize.x - logoScaled.x) * 0.5f;
        float logoY = (io.DisplaySize.y - logoScaled.y) * 0.5f - 90;  // Offset up a bit

        ImGui::SetNextWindowPos(ImVec2(logoX, logoY));
        ImGui::SetNextWindowSize(logoScaled);

        ImGui::Begin("Logo", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs);      // No input - logo is just for display

        if (textures.size() > 2) {
            ImGui::Image((ImTextureID)(intptr_t)textures[2], logoScaled);
        }

        ImGui::End();

        // --------------------------------------------------------------------
        // ARROW BUTTONS (increase/decrease rounds)
        // --------------------------------------------------------------------
        ImVec2 arrowMaxSize = ImVec2(150, 150);
        ImVec2 arrowScaled = GetScaledSizeFromGLTexture(
            textures.size() > 3 ? textures[3] : 0,
            arrowMaxSize
        );

        // RIGHT ARROW - Increase rounds
        {
            ImVec2 pos = ImVec2(logoX + logoScaled.x + 50.0f, logoY + logoScaled.y * 0.2f);
            ImGui::SetNextWindowPos(pos);
            ImGui::SetNextWindowSize(ImVec2(arrowScaled.x + 10, arrowScaled.y + 10));

            ImGui::Begin("ArrowRight", nullptr,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoBackground);

            // Transparent button with subtle hover effect
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

            ImGui::SetCursorPos(ImVec2(5, 5));
            if (ImGui::ImageButton("##ArrowInc", (ImTextureID)(intptr_t)textures[3], arrowScaled))
            {
                // Increase round count
                pom.rounds++;

                // Keep default time values
                pom.focusTime = 25 * pom.rounds;
                pom.longBreak = ((pom.rounds) / 4) * 40;
                pom.shortBreak = ((pom.rounds) - (pom.longBreak / 40)) * 5;
            }

            ImGui::PopStyleColor(3);
            ImGui::End();
        }

        // LEFT ARROW - Decrease rounds
        {
            ImVec2 pos = ImVec2(logoX - arrowScaled.x - 70.0f, logoY + logoScaled.y * 0.2f);
            ImGui::SetNextWindowPos(pos);
            ImGui::SetNextWindowSize(ImVec2(arrowScaled.x + 10, arrowScaled.y + 10));

            ImGui::Begin("ArrowLeft", nullptr,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoBackground);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

            ImGui::SetCursorPos(ImVec2(5, 5));

            // Flip arrow image horizontally by swapping UV coordinates
            if (ImGui::ImageButton("##ArrowDec", (ImTextureID)(intptr_t)textures[3], arrowScaled,
                ImVec2(1, 0), ImVec2(0, 1)))
            {
                // Decrease rounds, minimum 1
                if (pom.rounds > 1) {
                    pom.rounds--;
                }

                // Keep default time values
                pom.focusTime = 25 * pom.rounds;
                pom.longBreak = ((pom.rounds) / 4) * 40;
                pom.shortBreak = ((pom.rounds) - (pom.longBreak / 40)) * 5;
            }

            ImGui::PopStyleColor(3);
            ImGui::End();
        }

        // --------------------------------------------------------------------
        // STATS TEXT (display current settings)
        // Only show LongBreak if rounds >= 4
        // --------------------------------------------------------------------
        ImGui::PushFont(bigFont);

        // Build stats string - only include long break if rounds >= 4
        std::string statsText =
            "Rounds: " + std::to_string(pom.rounds) +
            "    FocusTime: " + std::to_string(pom.focusTime) +
            "    ShortBreak: " + std::to_string(pom.shortBreak) +
            "    LongBreak: " + std::to_string(pom.longBreak);

        // Calculate text size for centering
        ImGui::SetWindowFontScale(6.0f);
        ImVec2 textSize = ImGui::CalcTextSize(statsText.c_str());
        ImGui::SetWindowFontScale(1.0f);

        // Position text centered, below logo
        ImGui::SetNextWindowPos(ImVec2(
            (io.DisplaySize.x - textSize.x) * 0.5f,
            io.DisplaySize.y * 0.5f + 120
        ));
        ImGui::SetNextWindowSize(ImVec2(textSize.x + 20, textSize.y + 20));

        ImGui::Begin("Stats", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs);

        // Draw black text
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));
        ImGui::SetWindowFontScale(6.0f);
        ImGui::TextUnformatted(statsText.c_str());
        ImGui::PopStyleColor();

        ImGui::PopFont();
        ImGui::End();
    }
    // ========================================================================
    // TIMER SCREEN (shown when resumeButtonstate == false)
    // ========================================================================
    else
    {
        // --------------------------------------------------------------------
        // TIMER UPDATE LOGIC
        // --------------------------------------------------------------------
        // Make sure timer is initialized
        if (!timerInitialized) {
            currentRound = 1;
            currentState = TIMER_FOCUS;
            countdownSeconds = 25 * 60;
            lastTick = ImGui::GetTime();
            timerInitialized = true;
        }

        // Update countdown every second when timer is running
        double currentTime = ImGui::GetTime();
        if (startButtonState && (currentTime - lastTick) >= 1.0) {
            if (countdownSeconds > 0) {
                // Decrease countdown
                countdownSeconds--;
            }
            else {
                // Session finished - move to next session
                AdvanceToNextSession();
            }
            lastTick = currentTime;
        }

        // --------------------------------------------------------------------
        // FORMAT TIMER DISPLAY
        // --------------------------------------------------------------------
        // Convert seconds to MM:SS format
        int minutes = countdownSeconds / 60;
        int seconds = countdownSeconds % 60;
        char timerText[16];
        sprintf_s(timerText, sizeof(timerText), "%02d:%02d", minutes, seconds);

        // Determine what type of session we're in
        const char* sessionLabel = "";
        if (currentState == TIMER_FOCUS) {
            sessionLabel = "Focus Round";
        }
        else if (currentState == TIMER_SHORT_BREAK) {
            sessionLabel = "Short Break";
        }
        else if (currentState == TIMER_LONG_BREAK) {
            sessionLabel = "Long Break";
        }
        else if (currentState == SESSION_ENDED) {
            sessionLabel = "Session Ended";
        }

        // --------------------------------------------------------------------
        // DISPLAY TIMER (full screen window)
        // --------------------------------------------------------------------
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGui::Begin("Time", nullptr,
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoSavedSettings);

        // Draw session label (centered, above timer)
        ImGui::SetWindowFontScale(6.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));  // Black text

        // Build label with round number if in Focus mode
        char labelText[64];
        if (currentState == TIMER_FOCUS) {
            sprintf_s(labelText, sizeof(labelText), "%s %d", sessionLabel, currentRound);
        }
        else if (currentState == SESSION_ENDED) {
            sprintf_s(labelText, sizeof(labelText), "%s!", sessionLabel);
        }
        else {
            sprintf_s(labelText, sizeof(labelText), "%s", sessionLabel);
        }

        // Calculate label position (centered, above timer)
        ImVec2 labelSize = ImGui::CalcTextSize(labelText);
        float labelX = (io.DisplaySize.x - labelSize.x) * 0.5f;
        float labelY = (io.DisplaySize.y * 0.5f) - 150.0f;
        ImGui::SetCursorPos(ImVec2(labelX, labelY));
        ImGui::TextUnformatted(labelText);

        // Draw timer (big centered text) - always show, will be 00:00 when session ended
        ImGui::SetWindowFontScale(8.0f);
        ImVec2 timerSize = ImGui::CalcTextSize(timerText);
        float timerX = (io.DisplaySize.x - timerSize.x) * 0.5f;
        float timerY = (io.DisplaySize.y - timerSize.y) * 0.5f;
        ImGui::SetCursorPos(ImVec2(timerX, timerY));
        ImGui::Text("%s", timerText);

        ImGui::PopStyleColor();
        ImGui::End();
    }

    // ========================================================================
    // BOTTOM CONTROL BUTTONS (STOP | START | RESET)
    // ========================================================================

    // Get texture IDs for each button
    // NOTE: Textures are swapped - stop icon is used for reset and vice versa
    GLuint stopTexture = textures.size() > 7 ? textures[7] : 0;      // Reset icon
    GLuint resetTexture = textures.size() > 5 ? textures[5] : 0;     // Stop icon

    // Start button changes image based on state (play vs pause)
    GLuint startTexture = 0;
    if (startButtonState) {
        startTexture = textures.size() > 6 ? textures[6] : 0;  // Pause icon
    }
    else {
        startTexture = textures.size() > 4 ? textures[4] : 0;  // Play icon
    }

    // Get raw texture sizes
    ImVec2 stopRawSize = GetRawTexSize(stopTexture);
    ImVec2 startRawSize = GetRawTexSize(startTexture);
    ImVec2 resetRawSize = GetRawTexSize(resetTexture);

    // Default size if texture is invalid
    if (stopRawSize.x <= 0) stopRawSize = ImVec2(120, 120);
    if (startRawSize.x <= 0) startRawSize = ImVec2(120, 120);
    if (resetRawSize.x <= 0) resetRawSize = ImVec2(120, 120);

    // Apply scaling to each button
    ImVec2 stopImageSize = ImVec2(stopRawSize.x * stopScale, stopRawSize.y * stopScale);
    ImVec2 startImageSize = ImVec2(startRawSize.x * startScale, startRawSize.y * startScale);
    ImVec2 resetImageSize = ImVec2(resetRawSize.x * resetScale, resetRawSize.y * resetScale);

    // Add padding around each button
    float padding = 6.0f;
    ImVec2 stopWindowSize = ImVec2(stopImageSize.x + padding * 2, stopImageSize.y + padding * 2);
    ImVec2 startWindowSize = ImVec2(startImageSize.x + padding * 2, startImageSize.y + padding * 2);
    ImVec2 resetWindowSize = ImVec2(resetImageSize.x + padding * 2, resetImageSize.y + padding * 2);

    // Calculate button positions (centered at bottom of screen)
    const float buttonSpacing = 28.0f;
    float centerX = io.DisplaySize.x * 0.5f;
    float baseY = io.DisplaySize.y - std::max({ stopWindowSize.y, startWindowSize.y, resetWindowSize.y }) - 30.0f;

    // Start button is in center
    float startX = centerX - startWindowSize.x * 0.5f + startOffset.x;
    float startY = baseY + startOffset.y;

    // Stop button is to the left
    float stopX = startX - buttonSpacing - stopWindowSize.x + stopOffset.x;
    float stopY = baseY + stopOffset.y;

    // Reset button is to the right
    float resetX = startX + startWindowSize.x + buttonSpacing + resetOffset.x;
    float resetY = baseY + resetOffset.y;

    // ------------------------------------------------------------------------
    // STOP BUTTON (left button - resets everything and returns to logo)
    // ------------------------------------------------------------------------
    ImGui::SetNextWindowPos(ImVec2(stopX, stopY));
    ImGui::SetNextWindowSize(stopWindowSize);

    ImGui::Begin("StopBtnWindow", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground);

    // Transparent button with subtle hover effect
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

    // Center image in window
    ImGui::SetCursorPos(ImVec2(
        (stopWindowSize.x - stopImageSize.x) * 0.5f,
        (stopWindowSize.y - stopImageSize.y) * 0.5f
    ));

    if (ImGui::ImageButton("##StopButton", (ImTextureID)(intptr_t)stopTexture, stopImageSize))
    {
        std::cout << "STOP/RESET BUTTON CLICKED\n";

        // Stop the timer
        startButtonState = false;

        // Reset all settings to defaults
        pom.rounds = 1;
        pom.focusTime = 25;
        pom.shortBreak = 5;
        pom.longBreak = 0;  // 0 because rounds = 1 (< 4)

        // Reset timer state
        currentRound = 1;
        currentState = TIMER_FOCUS;
        countdownSeconds = pom.focusTime * 60;
        timerInitialized = false;
        lastTick = ImGui::GetTime();

        // Return to logo screen
        resumeButtonstate = true;
    }

    ImGui::PopStyleColor(3);
    ImGui::End();

    // ------------------------------------------------------------------------
    // START/PAUSE BUTTON (center button - toggles timer)
    // ------------------------------------------------------------------------
    ImGui::SetNextWindowPos(ImVec2(startX, startY));
    ImGui::SetNextWindowSize(startWindowSize);

    ImGui::Begin("StartBtnWindow", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

    ImGui::SetCursorPos(ImVec2(
        (startWindowSize.x - startImageSize.x) * 0.5f,
        (startWindowSize.y - startImageSize.y) * 0.5f
    ));

    if (ImGui::ImageButton("##StartButton", (ImTextureID)(intptr_t)startTexture, startImageSize))
    {
        // If on logo screen, enter timer screen and start playing
        if (resumeButtonstate) {
            resumeButtonstate = false;
            timerInitialized = false;  // Will be initialized on next frame
            startButtonState = true;   // Start playing immediately
        }
        else {
            // If on timer screen, toggle play/pause
            startButtonState = !startButtonState;
        }

        std::cout << "START BUTTON CLICKED -> Playing = " << (startButtonState ? "YES" : "NO") << "\n";
    }

    ImGui::PopStyleColor(3);
    ImGui::End();

    // ------------------------------------------------------------------------
    // RESET/RESUME BUTTON (right button - returns to logo screen)
    // ------------------------------------------------------------------------
    ImGui::SetNextWindowPos(ImVec2(resetX, resetY));
    ImGui::SetNextWindowSize(resetWindowSize);

    ImGui::Begin("ResetBtnWindow", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoBackground);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.2f));

    ImGui::SetCursorPos(ImVec2(
        (resetWindowSize.x - resetImageSize.x) * 0.5f,
        (resetWindowSize.y - resetImageSize.y) * 0.5f
    ));

    if (ImGui::ImageButton("##ResetButton", (ImTextureID)(intptr_t)resetTexture, resetImageSize))
    {
        std::cout << "RESUME BUTTON CLICKED\n";

        // Return to logo screen
        resumeButtonstate = true;

        // Reset timer initialization flag
        timerInitialized = false;

        // Toggle start button if it was running
        if (startButtonState) {
            startButtonState = !startButtonState;
        }
    }

    ImGui::PopStyleColor(3);
    ImGui::End();
}
