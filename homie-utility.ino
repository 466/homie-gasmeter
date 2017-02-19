#include <Arduino.h>
#include <Homie.h>
#include <Ticker.h>

//hardware setting, change according needs
#define FW_NAME "HGasMeter"
#define FW_VERSION "0.8.0"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

#define PINGASCOUNTER D2
#define DEBOUNCINGTIME 10 //can be low because of internal debouncing in A3213 Hall sensor IC

//software settings, change according needs
int PublishInterval = 60; //this is also the base time for calculating the flow measurement --> 60sec = for example litres per minute
String GasCounterUnit = "m3";
String GasFlowUnit = "m3/h";
bool DoDebug = 0;
float GasCounterFactor = 0.01; //the spec of the meter says 1 pulse is 0.01m3
float GasFlowFactor = 0.01*60; //the spec of the meter says 1 pulse is 0.01m3; take publish interval into account!
unsigned long GasPulseCounter = 0; //set starting value, can be set by MQTT; CAUTION Take conversion factor into account

HomieNode GasMeterNode("gasmeter", "meter");
Ticker PublishTicker;
unsigned long SnapshotGasPulses = 0;
unsigned long GasDiff = 0;
float GasFlow = 0;
float GasConsumption[60] = {0.0};
float GasCounter = 0;
unsigned long LastMicrosGas = 0;
bool MQTTIsConnected = 0;
bool TimeToPublish = 0;
int IndexConsumption = 0;

//-------------------------------------------------------------------
//Pulse counting for gas meter
void CountGasPulse() {
  if((micros() - LastMicrosGas) >= (unsigned long)DEBOUNCINGTIME * 1000) {
    GasPulseCounter++;
    LastMicrosGas = micros();
    if(DoDebug){
      Serial.println("pulse gas counted");
      Serial.println(String(GasPulseCounter));
    }
  }
}
//-------------------------------------------------------------------
//Ticker handler for publisher
void PublishValues() {
  TimeToPublish = 1;
  if(DoDebug){
    Serial.println("taking snapshot of counters");
  }
  GasDiff = GasPulseCounter - SnapshotGasPulses;
  SnapshotGasPulses = GasPulseCounter;
}
//-------------------------------------------------------------------
//Homie event handler
void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_CONNECTED:
      MQTTIsConnected = true;
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      MQTTIsConnected = false;
      break;
  }
}
//-------------------------------------------------------------------
//Set total counter for gas
bool SetGasCounter(const HomieRange& range, const String& value) {
 float newvalue = value.toFloat();
 if(DoDebug){
  Serial.print("input gas counter:" );
  Serial.println(newvalue);
 }
 GasPulseCounter = newvalue / GasCounterFactor;
 if(DoDebug){
  Serial.print("New gas pulse counter:" );
  Serial.println(GasPulseCounter);
 }
 return true;
}
//-------------------------------------------------------------------
//Homie setup function
void SetupHandler() {
  GasMeterNode.setProperty("unit_counter").send(GasCounterUnit);
  GasMeterNode.setProperty("conversionfactor_counter").send(String(GasCounterFactor));
  GasMeterNode.setProperty("unit_counter_h").send(GasCounterUnit);
  GasMeterNode.setProperty("conversionfactor_counterh").send(String(GasCounterFactor));
  GasMeterNode.setProperty("unit_flow").send(GasFlowUnit);
  GasMeterNode.setProperty("conversionfactor_flow").send(String(GasFlowFactor));
}
//-------------------------------------------------------------------
//Homie loop handler
void LoopHandler() {
  if(TimeToPublish) { //only publish when Tickers has said so
    GasCounter = (float)SnapshotGasPulses*GasCounterFactor;
    GasFlow = (float)GasDiff*GasFlowFactor;
    GasConsumption[IndexConsumption] = (float)GasDiff*GasCounterFactor;
    if(IndexConsumption<60)IndexConsumption++;
    else IndexConsumption = 0;
    int i;
    float Sum = 0.0;
    for(i=0;i<60;i++) Sum = Sum + GasConsumption[i];
    if(MQTTIsConnected){
      Homie.getLogger() << "Gas Counter: " << GasCounter << GasCounterUnit << endl;
      GasMeterNode.setProperty("counter").send(String(GasCounter));
      Homie.getLogger() << "Gas Counter 1h: " << Sum << GasCounterUnit << endl;
      GasMeterNode.setProperty("counterh").send(String(Sum));
      Homie.getLogger() << "Gas Flow: " << GasFlow << GasFlowUnit << endl;
      GasMeterNode.setProperty("flow").send(String(GasFlow));
      }
    TimeToPublish = 0;
  }
}
//-------------------------------------------------------------------
//std ESP8266/Arduino functions
void setup() {
  if(DoDebug){
    Serial.begin(115200);
    Serial << endl << endl;
  }

  //initial firmware
  Homie_setBrand ("VHSOMIE");
  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setSetupFunction(SetupHandler).setLoopFunction(LoopHandler);
  Homie.onEvent(onHomieEvent);

  //publish nodes to broker
  GasMeterNode.advertise("unit_counter");
  GasMeterNode.advertise("conversionfactor_counter");
  GasMeterNode.advertise("counterh");
  GasMeterNode.advertise("conversionfactor_counterh");
  GasMeterNode.advertise("counter").settable(SetGasCounter);
  GasMeterNode.advertise("unit_flow");
  GasMeterNode.advertise("conversionfactor_flow");
  GasMeterNode.advertise("flow");
  Homie.setup();

  noInterrupts();
  //setup ticker to publish values according interval
  PublishTicker.attach(PublishInterval, PublishValues);
  //setup interrupts for counting
  pinMode(PINGASCOUNTER, INPUT_PULLUP);
  attachInterrupt(PINGASCOUNTER, CountGasPulse, FALLING);
  interrupts();
}
void loop() {
  Homie.loop();
}

