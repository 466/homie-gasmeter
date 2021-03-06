#include <Arduino.h>
#include <Homie.h>
#include <Ticker.h>

//hardware setting, change according needs
#define FW_NAME "HGasMeter"
#define FW_VERSION "1.0.0"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

#define PINGASCOUNTER D3
#define DEBUGGING //Comment out this line if you don't want debug messages. If kept, there will be an open telnet server available!
#ifdef DEBUGGING
 #include <WifiPrinter.h>
 WifiPrinter debug;
#endif

const unsigned int debounceTime = 60000; //in microseconds
const unsigned int interval = 60; //Interval for publishing measured and calculated values. Keep in mind that the current flow is also calculated using the interval
const char* counterUnit = "m3"; //unit in which the counter meter publishes values
const char* flowUnit = "m3/h"; //unit in which the flow meter publishes values. This value is related to the INTERVAL!
const float counterFactor = 0.01; //factor to translate pulses to the counter unit stated above: meter spec says 1 pulse = 0.01m3

HomieNode gasMeter("gasmeter", "meter");
Ticker publishTicker;
volatile unsigned int pulseCounter = 0;
unsigned int previousPulseCounter = 0;
volatile bool addPulse = false;
volatile bool doPublish = false;
float flow = 0;
float counterh[60] = {0.0};
float counter = 0;


//-------------------------------------------------------------------
//Pulse counting for gas meter
void countPulse() {
  addPulse = true;
}


//-------------------------------------------------------------------
//Ticker handler for publisher
void publishValues() {
  doPublish = true;
}


//-------------------------------------------------------------------
//Set total counter for gas
bool setCounter(const HomieRange& range, const String& value) {
  pulseCounter = value.toFloat() / counterFactor;
  #ifdef DEBUGGING
    Homie.getLogger() << "New input gas counter:" << value << endl;
    Homie.getLogger() << "New gas pulse counter: " << pulseCounter << endl;
  #endif
  return true;
}


//-------------------------------------------------------------------
//Homie setup function
void setupHandler() {
  gasMeter.setProperty("unit_counter").send(counterUnit);
  gasMeter.setProperty("unit_hcounter").send(counterUnit);
  gasMeter.setProperty("unit_flow").send(flowUnit);
  #ifdef DEBUGGING
    Homie.getLogger() << "Homie setup done." << endl;
  #endif
}


//-------------------------------------------------------------------
//Homie loop handler
void loopHandler() {
  if(addPulse){ //pulse detected
    static volatile unsigned long lastMicros = 0;
    if (micros() - lastMicros > debounceTime || lastMicros == 0) {
      pulseCounter++;
      addPulse = false;
      lastMicros = micros();
    }
  }
  if(doPublish) { //only publish when Ticker has said so
    counter = (float)pulseCounter * counterFactor;
    flow = (float)(pulseCounter - previousPulseCounter) * counterFactor * interval;
    static int indexHCounter = 0;
    counterh[indexHCounter] = (float)(pulseCounter - previousPulseCounter) * counterFactor;
    previousPulseCounter = pulseCounter;
    if(indexHCounter < 60) indexHCounter++;
    else indexHCounter = 0;
    int i = 0;
    float sum = 0.0;
    for(i = 0; i < 60; i++) sum = sum + counterh[i];
    if(Homie.isConnected()) {
      gasMeter.setProperty("counter").send(String(counter));
      gasMeter.setProperty("hcounter").send(String(sum));
      gasMeter.setProperty("flow").send(String(flow));
    }
    doPublish = false;
    #ifdef DEBUGGING
      Homie.getLogger() << "Total counter: " << counter << counterUnit << endl;
      Homie.getLogger() << "Hourly counter: " << sum << counterUnit << endl;
      Homie.getLogger() << "Flow: " << flow << flowUnit << endl;
      Homie.getLogger() << "Free heap: " << ESP.getFreeHeap() << endl;
    #endif
  }
}


//-------------------------------------------------------------------
//std ESP8266/Arduino functions
void setup() {
  publishTicker.attach(interval, publishValues);
  pinMode(PINGASCOUNTER, INPUT_PULLUP);
  attachInterrupt(PINGASCOUNTER, countPulse, FALLING);
  #ifdef DEBUGGING
    debug.begin();
    debug.enableReset(true);
    Homie.setLoggingPrinter(&debug);
  #else
    Homie.disableLogging();
  #endif
  Homie_setBrand ("VHSOMIE");
  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);
  Homie.getMqttClient().setKeepAlive(300);
  Homie.disableResetTrigger();
  gasMeter.advertise("unit_counter");
  gasMeter.advertise("unit_hcounter");
  gasMeter.advertise("hcounter");
  gasMeter.advertise("counter").settable(setCounter);
  gasMeter.advertise("unit_flow");
  gasMeter.advertise("flow");
  Homie.setup();
}


void loop() {
  Homie.loop();
  debug.loop();
}
