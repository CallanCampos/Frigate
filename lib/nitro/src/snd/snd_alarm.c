// NitroSDK's SND (snd_alarm.c, a guess from the SDK's file names): the handlers of the sound driver's alarms. The ARM7
// tells the ARM9 when an alarm goes off with the alarm's number and its ID, and the ID tells a call that is still on
// its way after the alarm was stopped or set up again from a call for the alarm as it is now.
#include "nitro/snd.h"

typedef struct SNDAlarm {
    SNDAlarmHandler handler;
    void *arg;
    u8 id;
} SNDAlarm;

static SNDAlarm sAlarms[SND_ALARM_MAX];

// NitroSDK's SND_AlarmInit
void func_0207db34(void) {
    int i;

    for (i = 0; i < SND_ALARM_MAX; i++) {
        sAlarms[i].handler = NULL;
        sAlarms[i].arg = NULL;
        sAlarms[i].id = 0;
    }
}

// NitroSDK's SNDi_IncAlarmId
void func_0207db58(int alarmNo) {
    SNDAlarm *alarm = &sAlarms[alarmNo];

    alarm->id++;
}

// NitroSDK's SNDi_SetAlarmHandler: returns the alarm's new ID
u8 func_0207db6c(int alarmNo, SNDAlarmHandler handler, void *arg) {
    SNDAlarm *alarm = &sAlarms[alarmNo];

    alarm->handler = handler;
    alarm->arg = arg;
    alarm->id++;
    return alarm->id;
}

// NitroSDK's SNDi_CallAlarmHandler: calls the handler of the alarm that the ARM7 reports, if the ID is still the one it
// was set with
void func_0207db8c(int data) {
    int alarmNo = (u8)data;
    int id = (u8)(data >> 8);
    SNDAlarm *alarm = &sAlarms[alarmNo];

    if (id == alarm->id && alarm->handler != NULL) {
        alarm->handler(alarm->arg);
    }
}
