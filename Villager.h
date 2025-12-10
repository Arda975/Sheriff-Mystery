#pragma once
#include "Sprite.h"
#include <vector>
#include <cmath> // atan2, sqrt, cos, etc.
#include <string>
#include <sstream>
enum VillagerState { WALKING, DYING, ARRESTED, ISOLATED, ANALYZING, EXECUTED, FLEEING };
enum Direction { UP, DOWN, LEFT, RIGHT };

class Villager {
private:
    Sprite* _sprite;
    Direction _dir;
    VillagerState _state;
    int _frameIndex;
    int _rageLevel;
    int _cooldown;
    bool _isInfected;
    int _infectionTimer;
    bool _isKiller;
    int _suspicionLevel;

    // Yeni özellikler
    int _fleeTimer;          // Kaçma süresi
    bool _isScared;          // Korkmuş mu?

    int _targetHouseId = -1; // -1: hedef yok
    int _focusTimer = 0;     // Hedefe odaklanma süresi

    int _id;
    int _resistance;  // Kaç frame sonra kudurukluk seviyesi artacak


    int _prisonTimer; // hapiste kalma süresi

    int _hunger; // 0-10 arası, 10 ile başlar
    int _hungerTick = 0;

    int _moveTimer;      // Hareket süresi sayacı
    int _pauseTimer;     // Durma süresi sayacı
    bool _isPaused;      // Şu an duruyor mu?
    bool _isLying = false;           // Bu tanık yalan mı söylüyor?
    Villager* _lieAnswer = nullptr;
public:
    Villager(Sprite* sprite);  // Yeni constructor tanımı
    void SpreadToNearby(std::vector<Villager*>& others);
    void Update();
    void Draw(HDC hDC);
    void InfectIfClose(Villager* other);
    void SetIsScared(bool val) { _isScared = val; }
    void SetState(VillagerState state);
    void SetFleeTimer(int val) { _fleeTimer = val; }
    void SetIsLying(bool val) { _isLying = val; }
    void SetLieAnswer(Villager* v) { _lieAnswer = v; }
    VillagerState GetState() const;
    void SetRageLevel(int level);
    int GetRageLevel() const;
    Sprite* GetSprite();
    void TryKillNearby(std::vector<Villager*>& others);
    Direction GetDirection() const;
    bool CanSee(const Villager* other) const;
    bool IsInfected() const { return _isInfected; }
    bool IsKiller() const { return _isKiller; }
    int GetSuspicionLevel() const { return _suspicionLevel; }
    void SetSuspicionLevel(int level) { _suspicionLevel = level; }
    void Arrest();
    void Isolate();
    void Analyze();
    void Execute();
    void WitnessMurder(Villager* killer);  // Cinayete tanık olma
    void FleeFrom(Villager* threat);       // Tehlikeden kaçma
    bool KnowsKiller() const;
    Villager* GetLastKiller() const;
    int GetID() const;
    void SetID(int id);
    int GetResistance() const;
    float GetTruthChance() const;
    bool _showSpeech = false;
    std::wstring _speechText = L"";
    int _speechTimer = 0;
    int _memoryTimer;
    void SetTargetHouse(int houseId, int focusTime);
    void ClearTargetHouse();
    int GetTargetHouse() const;
    Villager* _lastKiller;    // Son gördüğü katili hatırlasın
    int _witnessedKillerID = -1;        // Gördüğü veya uydurduğu katilin ID'si
    bool _isLyingAboutKiller = false;
    int GetWitnessedKillerID() const;
    int GetMemoryTimer() const;
    float GetMemoryConfidence() const;
    int GetRandomWrongID(int correctID);
    bool IsLying() const { return _isLying; }
    Villager* GetLieAnswer() const { return _lieAnswer; }
    void AssignWitnessesByDistance(std::vector<Villager*>& villagers, Villager* killer, Sprite* sheriffSprite);

};
#pragma once
