#ifndef COMPAT_NETWORK_STUB_H
#define COMPAT_NETWORK_STUB_H

#ifndef MINECRAFT_NET_MAX_PLAYERS
#define MINECRAFT_NET_MAX_PLAYERS 8
#endif

#ifndef XUSER_NAME_SIZE
#define XUSER_NAME_SIZE 16
#endif

typedef struct _XOVERLAPPED {
    void*         hEvent;
    unsigned long Internal;
    unsigned long InternalHigh;
    unsigned long Offset;
    unsigned long OffsetHigh;
} XOVERLAPPED, *PXOVERLAPPED, *LPXOVERLAPPED;

typedef unsigned long long SessionID_t;
#ifndef SessionID
typedef unsigned long long SessionID;
#endif
#ifndef GameSessionUID
typedef unsigned long long GameSessionUID;
#endif

#ifdef __cplusplus
class IQNetPlayer {
public:
    virtual ~IQNetPlayer() {}
    virtual unsigned char GetSmallId() { return 0; }
    virtual void SendData(IQNetPlayer*, const void*, unsigned long, unsigned long) {}
    virtual bool IsSameSystem(IQNetPlayer*) { return true; }
    virtual unsigned long GetSendQueueSize(IQNetPlayer*, unsigned long) { return 0; }
    virtual unsigned long GetCurrentRtt() { return 0; }
    virtual bool IsHost() { return false; }
    virtual bool IsGuest() { return false; }
    virtual bool IsLocal() { return true; }
    virtual PlayerUID GetXuid() { return 0ULL; }
    virtual const wchar_t* GetGamertag() { return L"stub"; }
    virtual int GetSessionIndex() { return 0; }
    virtual bool IsTalking() { return false; }
    virtual bool IsMutedByLocalUser(unsigned long) { return false; }
    virtual bool HasVoice() { return false; }
    virtual bool HasCamera() { return false; }
    virtual int GetUserIndex() { return 0; }
    virtual void SetCustomDataValue(unsigned long long) {}
    virtual unsigned long long GetCustomDataValue() { return 0; }
};

class IQNet {
public:
    IQNetPlayer m_player[MINECRAFT_NET_MAX_PLAYERS];

    void AddLocalPlayerByUserIndex(unsigned long) {}
    IQNetPlayer* GetHostPlayer() { return &m_player[0]; }
    IQNetPlayer* GetLocalPlayerByUserIndex(unsigned long i) {
        return i < MINECRAFT_NET_MAX_PLAYERS ? &m_player[i] : nullptr;
    }
    IQNetPlayer* GetPlayerByIndex(int i) {
        return (i >= 0 && i < MINECRAFT_NET_MAX_PLAYERS) ? &m_player[i] : nullptr;
    }
    IQNetPlayer* GetPlayerBySmallId(unsigned char) { return &m_player[0]; }
    IQNetPlayer* GetPlayerByXuid(PlayerUID) { return &m_player[0]; }
    unsigned long GetPlayerCount() { return 1; }
    bool IsHost() { return true; }
};
#endif /* __cplusplus */

#endif /* COMPAT_NETWORK_STUB_H */
