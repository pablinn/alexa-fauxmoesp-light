//esp-01 casi al limite no agregar mas o tener en cuenta la memoria limitada de esta placa
//RAM:   [====      ]  35.8% (used 29368 bytes from 81920 bytes)
//Flash: [=======   ]  71.8% (used 311728 bytes from 434160 bytes)


#include <fauxmoESP.h>
#include <ESPAsyncWebServer.h>
#include "pass.h"   //Incluir las password de tu router wifi..:)


fauxmoESP fauxmo;
AsyncWebServer server(80);

#define LED   2 
#define BUTTON_PIN 0  



bool myPowerState = false;
unsigned long lastBtnPress = 0;


//maneja los eventos de pulsar un boton en el PIN 0 lo cual enciende y apaga la salida 2 o la lampara
void handleButtonPress() {

  unsigned long actualMillis = millis(); // get actual millis() and keep it in variable actualMillis
  if (digitalRead(BUTTON_PIN) == LOW && actualMillis - lastBtnPress > 1000)  { // is button pressed (inverted logic! button pressed = LOW) and debounced?
    if (myPowerState) {     // flip myPowerState: if it was true, set it to false, vice versa
      myPowerState = false;
    } else {
      myPowerState = true;
    }
    digitalWrite(LED, myPowerState?LOW:HIGH); // if myPowerState indicates device turned on: turn on led (builtin led uses inverted logic: LOW = LED ON / HIGH = LED OFF)
    lastBtnPress = actualMillis;  // update last button press variable
  } 
}

// HTML WEP SERVER esto es un pequeño webserver dinamico que agrega los botones o sea lo pueden manejar desde la red local si se quedan sin alexa

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";


String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}


//botones dinamicos placeholder 
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Output - GPIO 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
//    buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
//    buttons += "<h4>Output - GPIO 33</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"33\" " + outputState(33) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}


//setup del wifi printf comentados para ahorrar espacio usar solo como debug
void wifiSetup() {
     WiFi.mode(WIFI_STA);
     Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
     WiFi.begin(WIFI_SSID, WIFI_PASS);
     while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    //Serial.println();
    // Connected!
    //Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

//request compartido asyncwebserver para alexa y metodos locales
void serverSetup() {

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
    });
   
    server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    //Serial.print("GPIO: ");
    //Serial.print(inputMessage1);
    //Serial.print(" - Set to: ");
    //Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });


     //usado por alexa
    // These two callbacks are required for gen1 and gen3 compatibility
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data))) return;
        // Handle any other body request here...
    });
    server.onNotFound([](AsyncWebServerRequest *request) {
        String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
        if (fauxmo.process(request->client(), request->method() == HTTP_GET, request->url(), body)) return;
        // Handle not found request here...
    });

    // Start the server
    server.begin();

}


void setup() {

    Serial.begin(115200);
    Serial.println();
    Serial.println();
    pinMode(LED, OUTPUT); 
    digitalWrite(LED, HIGH); //pin 2 negado problemas al bootear
    wifiSetup();     
    serverSetup();
    fauxmo.createServer(false);
    fauxmo.setPort(80);
    fauxmo.enable(true);
    fauxmo.addDevice("luz cocina");
    
    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        //Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
        digitalWrite(LED, !state);
    });
    
}

void loop() {
	handleButtonPress();
    fauxmo.handle();
    static unsigned long last = millis();
    if (millis() - last > 5000) {
        last = millis();
        //Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
    }

}
