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

#define PINGASCOUNTER D2
#define DEBOUNCINGTIME 60000 //in microseconds
#define INTERVAL 60 //Interval for publishing measured and calculated values. Keep in mind that the current flow is also calculated using the interval
#define COUNTERUNIT "m3" //unit in which the counter meter publishes values
#define FLOWUNIT "m3/h" //unit in which the flow meter publishes values. This value is related to the INTERVAL!
#define COUNTERFACTOR 0.01 //factor to translate pulses to the counter unit stated above: meter spec says 1 pulse = 0.01m3
#define DEBUGGING //Comment out this line if you don't want debug messages. If kept, there will be an open telnet server available!

#ifdef DEBUGGING
 #include <RemoteDebug.h>
 RemoteDebug Debug;
#endif

HomieNode GasMeter("gasmeter", "meter");
Ticker PublishTicker;
volatile unsigned int PulseCounter = 0;
unsigned int PreviousPulseCounter = 0;
volatile bool TimeToPublish = 0;
float Flow = 0;
float Counterh[60] = {0.0};
float Counter = 0;

//-------------------------------------------------------------------
//Pulse counting for gas meter
void CountPulse(){
  static volatile unsigned long LastMicros = 0;
  if (micros() - LastMicros > DEBOUNCINGTIME || LastMicros == 0) {
    PulseCounter++;
    LastMicros = micros();
    }
  }
//-------------------------------------------------------------------
//Ticker handler for publisher
void PublishValues() {
  TimeToPublish = true;
  }
//-------------------------------------------------------------------
//Set total counter for gas
bool SetCounter(const HomieRange& range, const String& value) {
  PulseCounter = value.toFloat() / COUNTERFACTOR;
  #ifdef DEBUGGING
    Homie.getLogger() << "New input gas counter:" << value << endl;
    Homie.getLogger() << "New gas pulse counter: " << PulseCounter << endl;
  #endif
  return true;
  }
//-------------------------------------------------------------------
//Homie setup function
void SetupHandler() {
  GasMeter.setProperty("unit_counter").send(COUNTERUNIT);
  GasMeter.setProperty("unit_hcounter").send(COUNTERUNIT);
  GasMeter.setProperty("unit_flow").send(FLOWUNIT);
  #ifdef DEBUGGING
    Homie.getLogger() << "Homie setup done." << endl;
  #endif
  }
//-------------------------------------------------------------------
//Homie loop handler
void LoopHandler() {
  if(TimeToPublish) { //only publish when Ticker has said so
    Counter = (float)PulseCounter*COUNTERFACTOR;
    Flow = (float)(PulseCounter - PreviousPulseCounter)*COUNTERFACTOR*INTERVAL;
    static int IndexHCounter = 0;
    Counterh[IndexHCounter] = (float)(PulseCounter - PreviousPulseCounter)*COUNTERFACTOR;
    PreviousPulseCounter = PulseCounter;
    if(IndexHCounter<60)IndexHCounter++;
    else IndexHCounter = 0;
    int i = 0;;
    float Sum = 0.0;
    for(i=0;i<60;i++) Sum = Sum + Counterh[i];
    GasMeter.setProperty("counter").send(String(Counter));
    GasMeter.setProperty("hcounter").send(String(Sum));
    GasMeter.setProperty("flow").send(String(Flow));
    TimeToPublish = 0;
    #ifdef DEBUGGING
      Homie.getLogger() << "Total counter: " << Counter << COUNTERUNIT << endl;
      Homie.getLogger() << "Hourly counter: " << Sum << COUNTERUNIT << endl;
      Homie.getLogger() << "Flow: " << Flow << FLOWUNIT << endl;
      Homie.getLogger() << "Free heap: " << ESP.getFreeHeap() << endl;
    #endif
    }
  }
//-------------------------------------------------------------------
//std ESP8266/Arduino functions
void setup() {
  PublishTicker.attach(INTERVAL, PublishValues);
  pinMode(PINGASCOUNTER, INPUT_PULLUP);
  attachInterrupt(PINGASCOUNTER, CountPulse, FALLING);
  #ifdef DEBUGGING
    Debug.begin("GasMeter");
    Debug.setResetCmdEnabled(true);
    Homie.setLoggingPrinter(&Debug);
  #else
    Homie.disableLogging();
  #endif
  Homie_setBrand ("VHSOMIE");
  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setSetupFunction(SetupHandler).setLoopFunction(LoopHandler);
  Homie.getMqttClient().setKeepAlive(300);
  Homie.disableResetTrigger();
  GasMeter.advertise("unit_counter");
  GasMeter.advertise("unit_hcounter");
  GasMeter.advertise("hcounter");
  GasMeter.advertise("counter").settable(SetCounter);
  GasMeter.advertise("unit_flow");
  GasMeter.advertise("flow");
  Homie.setup();
  }
void loop() {
  Homie.loop();
  Debug.handle();
  }

