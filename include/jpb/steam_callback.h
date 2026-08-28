#ifndef JPB_STEAM_CALLBACK_H
#define JPB_STEAM_CALLBACK_H

#include <cstdint>

class CCallbackBase;

extern "C" {
void SteamAPI_RegisterCallback(
    CCallbackBase *callback, int callback_id);
void SteamAPI_UnregisterCallback(CCallbackBase *callback);
}

class CCallbackBase {
public:
    enum {
        k_ECallbackFlagsRegistered = 1,
        k_ECallbackFlagsGameServer = 2
    };

    CCallbackBase() : m_nCallbackFlags(0), m_iCallback(0) {}

    virtual void Run(
        void *pvParam,
        bool bIOFailure,
        std::uint64_t hSteamAPICall) = 0;
    virtual void Run(void *pvParam) = 0;
    int GetICallback() const { return m_iCallback; }
    virtual int GetCallbackSizeBytes() = 0;

protected:
    std::uint8_t m_nCallbackFlags;
    int m_iCallback;

    friend void SteamAPI_RegisterCallback(
        CCallbackBase *callback, int callback_id);
    friend void SteamAPI_UnregisterCallback(CCallbackBase *callback);
};

template <int CallbackSize>
class CCallbackImpl : protected CCallbackBase {
public:
    virtual ~CCallbackImpl();

    CCallbackBase *AsCallbackBase() { return this; }

protected:
    void Run(
        void *pvParam,
        bool bIOFailure,
        std::uint64_t hSteamAPICall) override;
    virtual void Run(void *pvParam) override = 0;
    int GetCallbackSizeBytes() override;
};

template <class Object, class Parameter, bool GameServer = false>
class CCallback : public CCallbackImpl<sizeof(Parameter)> {
public:
    typedef void (Object::*func_t)(Parameter *);

    CCallback(Object *object, func_t function)
        : m_pObj(object), m_Func(function)
    {
        if (GameServer) {
            this->m_nCallbackFlags |=
                CCallbackBase::k_ECallbackFlagsGameServer;
        }
        SteamAPI_RegisterCallback(this, Parameter::k_iCallback);
    }

    void Register(Object *object, func_t function)
    {
        m_pObj = object;
        m_Func = function;
        SteamAPI_RegisterCallback(this, Parameter::k_iCallback);
    }

    void Unregister()
    {
        SteamAPI_UnregisterCallback(this);
    }

protected:
    void Run(void *pvParam) override;

    Object *m_pObj;
    func_t m_Func;
};

static_assert(sizeof(CCallbackBase) == 16,
    "CCallbackBase must match the x64 Steam/PDB layout");

#endif
