#ifndef MOCK_XRNM_H
#define MOCK_XRNM_H

/* Minimal stub for Xbox 360 XRNM (Xbox Live Rich Network Module).
 * Provides the declarations used by Minecraft.World/Socket.cpp.
 * Real networking is not implemented — Phase 8 work. */

/* M2.5: INetworkPlayer moved to NetworkPlayerInterface.h */

struct XRNM_SEND_BUFFER {
    unsigned char* pbyData;
    unsigned int   dwDataSize;
};

#endif /* MOCK_XRNM_H */
