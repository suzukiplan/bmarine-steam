/**
 * VGS-Zero SDK for Steam
 * License under GPLv3: https://github.com/suzukiplan/vgszero/blob/master/LICENSE-VGS0.txt
 * (C)2024, SUZUKI PLAN
 */
#include "../sdk/public/steam/steam_api.h"

class CSteam
{
  private:
    bool initialized;
    bool overlay;
    bool leaderboardFound;
    void (*putlog)(const char*, ...);
    InputActionSetHandle_t act;
    InputAnalogActionHandle_t actMove;
    InputDigitalActionHandle_t actA;
    InputDigitalActionHandle_t actB;
    InputDigitalActionHandle_t actStart;
    InputDigitalActionHandle_t actSelect;
    STEAM_CALLBACK_MANUAL(CSteam, onGameOverlayActivated, GameOverlayActivated_t, callbackGameOverlayActivated);
    SteamLeaderboard_t currentLeaderboard;
    void onFindLeaderboard(LeaderboardFindResult_t* callback, bool failed);
    CCallResult<CSteam, LeaderboardFindResult_t> callResultFindLeaderboard;
    void onUploadScore(LeaderboardScoreUploaded_t* callback, bool failed);
    CCallResult<CSteam, LeaderboardScoreUploaded_t> callResultUploadLeaderboardScore;

  public:
    CSteam(void (*putlog)(const char*, ...))
    {
        this->putlog = putlog;
        this->initialized = false;
        this->overlay = false;
        this->leaderboardFound = false;
        this->deactivate();
    }

    ~CSteam()
    {
        if (this->initialized) {
            putlog("Teminating Steam...");
            SteamAPI_Shutdown();
        }
    }

    void init(const char* leaderboard = nullptr)
    {
        putlog("Initializing Steam...");
        if (!SteamAPI_Init()) {
            putlog("SteamAPI_Init failed");
        } else {
            this->initialized = true;
            if (!SteamUserStats()->RequestCurrentStats()) {
                putlog("SteamUserStats::RequestCurrentStats failed!");
            }
            callbackGameOverlayActivated.Register(this, &CSteam::onGameOverlayActivated);
            if (!SteamInput()->Init(true)) {
                putlog("SteamInput::Init failed!");
            }
            if (leaderboard) {
                auto hdl = SteamUserStats()->FindLeaderboard(leaderboard);
                this->callResultFindLeaderboard.Set(hdl, this, &CSteam::onFindLeaderboard);
            }
        }
    }

    uint8_t getJoypad(bool* connected)
    {
        SteamInput()->RunFrame();
        InputHandle_t inputHandles[STEAM_INPUT_MAX_COUNT];
        int num = SteamInput()->GetConnectedControllers(inputHandles);
        *connected = 0 < num;
        if (!*connected) {
            return 0;
        }
        auto inputHandle = inputHandles[0];
        if (!this->activate(inputHandle)) {
            return 0;
        }
        uint8_t result = 0;
        auto a = SteamInput()->GetDigitalActionData(inputHandle, actA);
        auto b = SteamInput()->GetDigitalActionData(inputHandle, actB);
        auto start = SteamInput()->GetDigitalActionData(inputHandle, actStart);
        auto select = SteamInput()->GetDigitalActionData(inputHandle, actSelect);
        auto move = SteamInput()->GetAnalogActionData(inputHandle, actMove);
        result |= a.bState ? VGS0_JOYPAD_T1 : 0;
        result |= b.bState ? VGS0_JOYPAD_T2 : 0;
        result |= start.bState ? VGS0_JOYPAD_ST : 0;
        result |= select.bState ? VGS0_JOYPAD_SE : 0;
        result |= move.x < 0 ? VGS0_JOYPAD_LE : 0;
        result |= 0 < move.x ? VGS0_JOYPAD_RI : 0;
        result |= move.y < 0 ? VGS0_JOYPAD_DW : 0;
        result |= 0 < move.y ? VGS0_JOYPAD_UP : 0;
        return result;
    }

    inline bool isOverlay() { return this->overlay; }

    void unlock(const char* name)
    {
        if (!this->initialized) {
            return;
        }
        if (!SteamUserStats()->SetAchievement(name)) {
            putlog("SteamUserStats::SetAchievement(%s) failed!", name);
        } else {
            if (!SteamUserStats()->StoreStats()) {
                putlog("SteamUserStats::StoreStats failed!");
            }
        }
    }

    void sendScore(int score)
    {
        if (!this->initialized) {
            return;
        }
        if (!this->leaderboardFound) {
            putlog("Score was not send to the leadboard (leadboard not found)");
            return;
        }
        auto hdl = SteamUserStats()->UploadLeaderboardScore(this->currentLeaderboard, k_ELeaderboardUploadScoreMethodKeepBest, score, nullptr, 0);
        this->callResultUploadLeaderboardScore.Set(hdl, this, &CSteam::onUploadScore);
    }

  private:
    void deactivate()
    {
        this->act = 0;
        this->actA = 0;
        this->actB = 0;
        this->actStart = 0;
        this->actSelect = 0;
        this->actMove = 0;
    }

    bool activate(InputHandle_t inputHandle)
    {
        if (!this->act) {
            this->act = SteamInput()->GetActionSetHandle("InGameControls");
            if (!this->act) {
                return false;
            }
        }
        if (!this->actA) {
            this->actA = SteamInput()->GetDigitalActionHandle("A");
            if (!this->actA) {
                return false;
            }
        }
        if (!this->actB) {
            this->actB = SteamInput()->GetDigitalActionHandle("B");
            if (!this->actB) {
                return false;
            }
        }
        if (!this->actStart) {
            this->actStart = SteamInput()->GetDigitalActionHandle("Start");
            if (!this->actStart) {
                return false;
            }
        }
        if (!this->actSelect) {
            this->actSelect = SteamInput()->GetDigitalActionHandle("Select");
            if (!this->actSelect) {
                return false;
            }
        }
        if (!this->actMove) {
            this->actMove = SteamInput()->GetAnalogActionHandle("Move");
            if (!this->actMove) {
                return false;
            }
        }
        return true;
    }
};

void CSteam::onGameOverlayActivated(GameOverlayActivated_t* args)
{
    this->overlay = args->m_bActive;
}

void CSteam::onFindLeaderboard(LeaderboardFindResult_t* callback, bool failed)
{
    if (failed || !callback || !callback->m_bLeaderboardFound) {
        putlog("onFindLeaderboard: leaderboard not found or error");
    } else {
        this->currentLeaderboard = callback->m_hSteamLeaderboard;
        this->leaderboardFound = true;
        putlog("Leadboard found");
    }
}

void CSteam::onUploadScore(LeaderboardScoreUploaded_t* callback, bool failed)
{
    if (failed || !callback || !callback->m_bSuccess) {
        putlog("onUploadScore: cannot register to the leaderboard");
    } else {
        if (callback->m_bScoreChanged) {
            putlog("score: %d, ranking: %d -> %d", callback->m_nScore, callback->m_nGlobalRankPrevious, callback->m_nGlobalRankNew);
        }
    }
}