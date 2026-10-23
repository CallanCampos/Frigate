// NitroSDK's SND (snd_alarm.c, as ntrtwl's reconstruction of NitroSDK has it): the handlers of the sound driver's
// alarms. The ARM7 tells the ARM9 when an alarm goes off with the alarm's number and its ID, and the ID tells a call
// that is still on its way after the alarm was stopped or set up again from a call for the alarm as it is now.
#include "nitro/snd.h"

typedef struct SNDAlarm {
    SNDAlarmHandler func;
    void *arg;
    u8 id;
} SNDAlarm;

static SNDAlarm sCallbackTable[SND_ALARM_NUM];

void SND_AlarmInit(void) {
    int i;

    for (i = 0; i < SND_ALARM_NUM; i++) {
        sCallbackTable[i].func = NULL;
        sCallbackTable[i].arg = NULL;
        sCallbackTable[i].id = 0;
    }
}

void SNDi_IncAlarmId(int alarmNo) {
    SNDAlarm *alarm = &sCallbackTable[alarmNo];

    alarm->id++;
}

// NitroSDK's SNDi_SetAlarmHandler: returns the alarm's new ID
u8 SNDi_SetAlarmHandler(int alarmNo, SNDAlarmHandler handler, void *arg) {
    SNDAlarm *alarm = &sCallbackTable[alarmNo];

    alarm->func = handler;
    alarm->arg = arg;
    alarm->id++;
    return alarm->id;
}

// NitroSDK's SNDi_CallAlarmHandler: calls the handler of the alarm that the ARM7 reports, if the ID is still the one it
// was set with
void SNDi_CallAlarmHandler(int data) {
    int alarmNo = (u8)data;
    int id = (u8)(data >> 8);
    SNDAlarm *alarm = &sCallbackTable[alarmNo];

    if (id == alarm->id && alarm->func != NULL) {
        alarm->func(alarm->arg);
    }
}
