#pragma once
#include <Arduino.h>
#include <time.h>

void timeServiceInit();
void timeServiceTick();

bool timeIsValid();
time_t nowLocal();
String timeFormatHHMM();
String dateFormatDMY();
String timeFormatFull();

void timeSetManual(int year, int month, int day, int hour, int minute, int second);
void timeSetNtpServer(const String& server);
String timeGetNtpServer();
void timeRequestNtpSync();
bool timeNtpSynced();
void timeSetTimezoneMinutes(int32_t minutes);
int32_t timeGetTimezoneMinutes();