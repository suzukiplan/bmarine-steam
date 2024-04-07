#include "../sdk/public/steam/steam_api.h"
#include <ctype.h>
#include <vector>

class CSteam
{
  private:
    bool initialized;
    bool leaderboardFound;
    bool overlay;
    void (*putlog)(const char*, ...);
    InputActionSetHandle_t act;
    InputAnalogActionHandle_t actMove;
    InputDigitalActionHandle_t actFire;
    InputDigitalActionHandle_t actJump;
    InputDigitalActionHandle_t actPause;
    STEAM_CALLBACK_MANUAL(CSteam, onGameOverlayActivated, GameOverlayActivated_t, callbackGameOverlayActivated);
    SteamLeaderboard_t currentLeaderboard;
    void onFindLeaderboard(LeaderboardFindResult_t* callback, bool failed);
    CCallResult<CSteam, LeaderboardFindResult_t> callResultFindLeaderboard;
    void onUploadScore(LeaderboardScoreUploaded_t* callback, bool failed);
    CCallResult<CSteam, LeaderboardScoreUploaded_t> callResultUploadLeaderboardScore;
    void onDownloadLeaderboardScore(LeaderboardScoresDownloaded_t* callback, bool failed);
    CCallResult<CSteam, LeaderboardScoresDownloaded_t> callResultDownloadLeaderboardScore;
    int lastSentScore;

  public:
    class FriendScoreRecord {
      public:
        int score;
        int globalRank;
        char name[13];
        FriendScoreRecord(int score, int globalRank, const char* srcName) {
            this->score = score;
            this->globalRank = globalRank;
            memset(this->name, 0, sizeof(this->name));
            strncpy(this->name, srcName, 12);
            for (int i = 0; i < (int)strlen(this->name); i++) {
                if (name[i] & 0x80) {
                    name[i] = '?';
                } else if (isalpha(name[i])) {
                    name[i] = toupper(name[i]);
                }
            }
        }
    };

    void (*downloadLeaderboardScore)(std::vector<FriendScoreRecord*>* ranking);

    CSteam(void (*putlog)(const char*, ...))
    {
        this->putlog = putlog;
        this->initialized = false;
        this->leaderboardFound = false;
        this->overlay = false;
        this->deactivate();
        this->lastSentScore = 0;
        this->downloadLeaderboardScore = nullptr;
    }

    ~CSteam()
    {
        if (this->initialized) {
            putlog("Teminating Steam...");
            SteamAPI_Shutdown();
        }
    }

    bool init()
    {
        putlog("Initializing Steam...");
        if (!SteamAPI_Init()) {
            putlog("SteamAPI_Init failed");
        } else {
            this->initialized = true;
            if (!SteamUserStats()->RequestCurrentStats()) {
                putlog("SteamUserStats::RequestCurrentStats failed!");
            } else {
                auto hdl = SteamUserStats()->FindLeaderboard("score_ranking_friend");
                this->callResultFindLeaderboard.Set(hdl, this, &CSteam::onFindLeaderboard);
            }
            callbackGameOverlayActivated.Register(this, &CSteam::onGameOverlayActivated);
            if (!SteamInput()->Init(true)) {
                putlog("SteamInput::Init failed!");
            }
        }
        return this->initialized;
    }

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

    void score(int score)
    {
        if (!this->initialized) {
            return;
        }
        if (!this->leaderboardFound) {
            putlog("Score was not send to the leadboard (leadboard not found)");
            return;
        }
        if (score <= lastSentScore) {
            return;
        }
        auto hdl = SteamUserStats()->UploadLeaderboardScore(this->currentLeaderboard, k_ELeaderboardUploadScoreMethodKeepBest, score, nullptr, 0);
        this->callResultUploadLeaderboardScore.Set(hdl, this, &CSteam::onUploadScore);
    }

    void refreshLeaderboard()
    {
        auto hdl = SteamUserStats()->DownloadLeaderboardEntries(this->currentLeaderboard, k_ELeaderboardDataRequestFriends, 0, 0);
        this->callResultDownloadLeaderboardScore.Set(hdl, this, &CSteam::onDownloadLeaderboardScore);
    }

    void runLoop()
    {
        if (initialized) {
            SteamAPI_RunCallbacks();
        }
    }

    uint8_t getJoypad(bool* connected)
    {
        if (!initialized) {
            return 0;
        }
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
        auto fire = SteamInput()->GetDigitalActionData(inputHandle, actFire);
        auto jump = SteamInput()->GetDigitalActionData(inputHandle, actJump);
        auto pause = SteamInput()->GetDigitalActionData(inputHandle, actPause);
        auto move = SteamInput()->GetAnalogActionData(inputHandle, actMove);
        result |= fire.bState ? VGS0_JOYPAD_T2 : 0;
        result |= jump.bState ? VGS0_JOYPAD_T1 : 0;
        result |= pause.bState ? VGS0_JOYPAD_ST : 0;
        result |= move.x < 0 ? VGS0_JOYPAD_LE : 0;
        result |= 0 < move.x ? VGS0_JOYPAD_RI : 0;
        result |= move.y < 0 ? VGS0_JOYPAD_DW : 0;
        result |= 0 < move.y ? VGS0_JOYPAD_UP : 0;
        return result;
    }

    inline bool isOverlay() { return this->overlay; }

  private:
    void deactivate()
    {
        this->act = 0;
        this->actFire = 0;
        this->actJump = 0;
        this->actPause = 0;
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
        if (!this->actFire) {
            this->actFire = SteamInput()->GetDigitalActionHandle("fire");
            if (!this->actFire) {
                return false;
            }
        }
        if (!this->actJump) {
            this->actJump = SteamInput()->GetDigitalActionHandle("jump");
            if (!this->actJump) {
                return false;
            }
        }
        if (!this->actPause) {
            this->actPause = SteamInput()->GetDigitalActionHandle("pause");
            if (!this->actPause) {
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

void CSteam::onFindLeaderboard(LeaderboardFindResult_t* callback, bool failed)
{
    if (!callback->m_bLeaderboardFound || failed) {
        putlog("onFindLeaderboard: leaderboard not found or error");
    } else {
        this->currentLeaderboard = callback->m_hSteamLeaderboard;
        this->leaderboardFound = true;
        putlog("Leadboard found");
        this->refreshLeaderboard();
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
        lastSentScore = callback->m_nScore;
        this->refreshLeaderboard();
    }
}

void CSteam::onGameOverlayActivated(GameOverlayActivated_t* args)
{
    this->overlay = args->m_bActive;
}

void CSteam::onDownloadLeaderboardScore(LeaderboardScoresDownloaded_t* callback, bool failed)
{
    if (failed || !callback) {
        putlog("onDownloadLeaderboardScore: failed to download theleaderboard");
        return;
    }
    putlog("Leaderboard Entry Count: %d", callback->m_cEntryCount);
    std::vector<FriendScoreRecord*> ranking;
    for (int i = 0; i < callback->m_cEntryCount; i++) {
        LeaderboardEntry_t entry;
        SteamUserStats()->GetDownloadedLeaderboardEntry(callback->m_hSteamLeaderboardEntries, i, &entry, nullptr, 0);
        ranking.push_back(new FriendScoreRecord(entry.m_nScore, entry.m_nGlobalRank, SteamFriends()->GetFriendPersonaName(entry.m_steamIDUser)));
    }
    if (this->downloadLeaderboardScore) {
        this->downloadLeaderboardScore(&ranking);
    }
    for (auto record : ranking) {
        delete record;
    }
}
