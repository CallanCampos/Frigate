#include "competition_internal.h"
#include "dwc/ext.h"

// The report that the game builds: its header, its roster and results, and the game's, players' and teams' values. Its
// numbers are big-endian, as the service reads them; its first 0x44 bytes are the header. The file and the names in it
// are GameSpy's own, as its sciReport.c (UniSpySDK) has them: sciCreateReport to sciReportAddByteValue are its
// functions in its order. The ROM doesn't name the file.

static int sciReportEndEntity(SCIReport *report);

int sciCreateReport(const char *csid, int version, int playerNum, int teamNum, SCIReport **report) {
    SCIReport *created;
    SCIReportHeader *header;
    gsi_u32 playerSize;
    gsi_u32 teamSize;

    teamSize = playerNum * 0x10;
    playerSize = teamSize + playerNum * 4;

    created = gsimalloc(sizeof(SCIReport));
    if (created == NULL) {
        return 5;
    }
    memset(created, 0, sizeof(SCIReport));
    header = gsimalloc(0x10000);
    if (header == NULL) {
        gsifree(created);
        return 5;
    }
    memset(header, 0, 0x10000);
    created->mBuffer.unk_00 = 0;
    created->mBuffer.mCapacity = 0x10000;
    memset(header, 0, sizeof(SCIReportHeader));
    header->mProtocolVersion = 0x2000000;
    header->mDeveloperVersion = Sc_SwapU32(version);
    header->mRosterSectionLength = playerSize;
    header->mAuthSectionLength = teamSize;
    header->mResultsSectionLength = (playerNum + teamNum) * 4;
    header->mPlayerCount = playerNum;
    header->mTeamCount = teamNum;
    created->mReportState = SCIReportState_ROSTER;
    created->mBuffer.mData = (gsi_u8 *)header;
    created->mCurEntityStartPos = -1;
    created->keyNum = 0;
    *report = created;
    return 0;
}

int sciDestroyReport(SCIReport *report) {
    report->mReportState = 0;
    report->mCurEntityKeyCount = 0;
    report->mCurEntityStartPos = 0;
    report->keyNum = 0;
    report->unk_0c = 0;
    gsifree(report->mBuffer.mData);
    report->mBuffer.mData = NULL;
    report->mBuffer.mCapacity = 0;
    report->mBuffer.unk_04 = 0;
    report->mBuffer.mPos = 0;
    gsifree(report);
    return 0;
}

// Each player's record after the header is 0x14 bytes: the 0x10-byte GUID of its connection, then its key
int sciReportSetPlayerConnectionId(SCIReport *report, int index, const char *id) {
    gsi_u32 offset = sizeof(SCIReportHeader);

    offset += index * 0x14;
    sciSerializeGUID(report->mBuffer.mData + offset, id);
    return 0;
}

int sciReportSetPlayerTeamIndex(SCIReport *report, int index, int key) {
    gsi_u32 offset = sizeof(SCIReportHeader);

    offset += index * 0x14;
    offset += 0x10;
    sciSerializeInt32(report->mBuffer.mData + offset, key);
    return 0;
}

int sciReportSetPlayerGameResult(SCIReport *report, int index, int result) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    sciSerializeInt32(report->mBuffer.mData + (header->mRosterSectionLength + sizeof(SCIReportHeader) +
                                               header->mAuthSectionLength + index * 4),
                      result);
    return 0;
}

int sciReportSetPlayerAuthInfo(SCIReport *report, int index, const GSLoginCertificate *certificate, void *unused) {
    return 0;
}

int sciReportBeginNewPlayer(SCIReport *report) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    report->mCurEntityKeyCount = 0;
    report->mCurEntityStartPos = report->mBuffer.mPos;
    report->mBuffer.mPos += 2;
    sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    header->mPlayerSectionLength += 2;
    return 0;
}

int sciReportBeginGlobalData(SCIReport *report) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    report->mReportState = SCIReportState_GLOBALDATA;
    report->mBuffer.mPos = header->mRosterSectionLength + sizeof(SCIReportHeader) + header->mAuthSectionLength +
                           header->mResultsSectionLength;
    return 0;
}

int sciReportBeginPlayerData(SCIReport *report) {
    report->mReportState = SCIReportState_PLAYERDATA;
    return 0;
}

int sciReportBeginTeamData(SCIReport *report) {
    report->mReportState = SCIReportState_TEAMDATA;
    return 0;
}

static int sciReportEndEntity(SCIReport *report) {
    return 0;
}

int sciReportEnd(SCIReport *report, int flag, int value) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    sciReportEndEntity(report);
    if (flag != 0) {
        header->mFlags |= 1;
    }
    header->mGameStatus = Sc_SwapU32(value);
    header->mFlags = Sc_SwapU32(header->mFlags);
    header->mPlayerCount = Sc_SwapU16(header->mPlayerCount);
    header->mTeamCount = Sc_SwapU16(header->mTeamCount);
    header->mReserved = 0;
    header->mGameKeyCount = Sc_SwapU16(header->mGameKeyCount);
    header->mPlayerKeyCount = Sc_SwapU16(header->mPlayerKeyCount);
    header->mTeamKeyCount = Sc_SwapU16(header->mTeamKeyCount);
    header->mRosterSectionLength = Sc_SwapU32(header->mRosterSectionLength);
    header->mAuthSectionLength = Sc_SwapU32(header->mAuthSectionLength);
    header->mResultsSectionLength = Sc_SwapU32(header->mResultsSectionLength);
    header->mGameSectionLength = Sc_SwapU32(header->mGameSectionLength);
    header->mPlayerSectionLength = Sc_SwapU32(header->mPlayerSectionLength);
    header->mTeamSectionLength = Sc_SwapU32(header->mTeamSectionLength);
    return 0;
}

int sciReportAddIntValue(SCIReport *report, int key, int value) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    if (report->mReportState == SCIReportState_GLOBALDATA) {
        header->mGameKeyCount++;
        header->mGameSectionLength += 8;
    } else if (report->mReportState == SCIReportState_PLAYERDATA) {
        header->mPlayerKeyCount++;
        header->mPlayerSectionLength += 8;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    } else if (report->mReportState == SCIReportState_TEAMDATA) {
        header->mTeamKeyCount++;
        header->mTeamSectionLength += 8;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    }
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, key);
    report->mBuffer.mPos += 2;
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, 0);
    report->mBuffer.mPos += 2;
    sciSerializeInt32(report->mBuffer.mData + report->mBuffer.mPos, value);
    report->mBuffer.mPos += 4;
    return 0;
}

int sciReportAddByteValue(SCIReport *report, int key, int value) {
    SCIReportHeader *header = (SCIReportHeader *)report->mBuffer.mData;

    if (report->mReportState == SCIReportState_GLOBALDATA) {
        header->mGameKeyCount++;
        header->mGameSectionLength += 5;
    } else if (report->mReportState == SCIReportState_PLAYERDATA) {
        header->mPlayerKeyCount++;
        header->mPlayerSectionLength += 5;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    } else if (report->mReportState == SCIReportState_TEAMDATA) {
        header->mTeamKeyCount++;
        header->mTeamSectionLength += 5;
        report->mCurEntityKeyCount++;
        sciSerializeInt16(report->mBuffer.mData + report->mCurEntityStartPos, report->mCurEntityKeyCount);
    }
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, key);
    report->mBuffer.mPos += 2;
    sciSerializeInt16(report->mBuffer.mData + report->mBuffer.mPos, 2);
    report->mBuffer.mPos += 2;
    sciSerializeInt8(report->mBuffer.mData + report->mBuffer.mPos, value);
    report->mBuffer.mPos += 1;
    return 0;
}
