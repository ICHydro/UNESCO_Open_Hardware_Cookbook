
#include <TimeLib.h>
#include <RTCZero.h>

// variables needed in interrupt should be of type volatile.

volatile uint16_t interruptCount = 0;
volatile bool interruptFlag = false;

void InterruptServiceRoutine() {
  interruptCount++;
  interruptFlag = true;
}

RTCZero rtc;

void setup() {
  rtc.begin();
  rtc.setEpoch(cvt_date(__DATE__, __TIME__) + 5);
  rtc.setAlarmSeconds(0);
  rtc.enableAlarm(rtc.MATCH_SS);                //once a minute
  rtc.attachInterrupt(InterruptServiceRoutine);
  while(!SerialUSB);
  SerialUSB.println("Starting");
}


void loop () {
  SerialUSB.print(rtc.getHours());
  SerialUSB.print(":");
  SerialUSB.print(rtc.getMinutes());
  SerialUSB.print(":");
  SerialUSB.print(rtc.getSeconds());

  SerialUSB.println();

  // we only want to show time every 10 seconds
  // but we want to show response to the interupt firing
  for (int timeCount = 0; timeCount < 20; timeCount++) {

    if (interruptFlag) {
      SerialUSB.print(">>Interrupt Count: ");
      SerialUSB.print(interruptCount);
      SerialUSB.println("<<");
      interruptFlag = false;
    }

    delay(500);
  }
}




time_t cvt_date(char const *date, char const *time)
{
    char s_month[5];
    int year, day;
    int hour, minute, second;
    tmElements_t t;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(date, "%s %d %d", s_month, &day, &year);
    //sscanf(time, "%2hhd %*c %2hhd %*c %2hhd", &t.Hour, &t.Minute, &t.Second);
    sscanf(time, "%2d %*c %2d %*c %2d", &hour, &minute, &second);
    // Find where is s_month in month_names. Deduce month value.
    t.Month = (strstr(month_names, s_month) - month_names) / 3 + 1;
    t.Day = day;
    // year can be given as '2010' or '10'. It is converted to years since 1970
    if (year > 99) t.Year = year - 1970;
    else t.Year = year + 50;
    t.Hour = hour;
    t.Minute = minute;
    t.Second = second;
    return makeTime(t);
}
