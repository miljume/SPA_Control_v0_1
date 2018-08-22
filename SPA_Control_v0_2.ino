

#include <PubSubClient.h>
#include <stdint.h>
#include <math.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <ESP32WebServer.h>


#define idx_on_off 32
#define idx_man_auto 33
#define idx_temp  31
#define idx_heater  37
#define idx_filter  35
#define idx_ozone  36
#define idx_setpoint  34
#define idx_bubble  38

String power = "off";
String heater = "off";
String bubble = "off";
String ozone = "off";
String last_power = "off";
String last_heater = "off";
String startup_status = "off";

ESP32WebServer server(80);

/* change it with your ssid-password */
const char* ssid = "AKKTUSTAKKI";
const char* password = "mickeljunggrensnetwork";
/* this is the IP of PC/raspberry where you installed MQTT Server
on Wins use "ipconfig"
on Linux use "ifconfig" to get its IP address */
const char* mqtt_server = "192.168.0.51";

int temperature = 38;
int difftemp = 0;

/* EEPROM adress */
//int temp_addr = 0;

/* create an instance of PubSubClient client */
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

/*DB GPIO pin*/
const char DB0 = 25; //ON_OFF
const char DB1 = 32; //HEATER
const char DB2 = 27; //TEMP UPP
const char DB3 = 26; //TEMP NER
const char DB4 = 33; //FILTER
const char DB5 = 18; //O3
const char DB6 = 19; //BUBBLE
const char DB7 = 21; //HEATER?

char res[5000] =
"<!DOCTYPE html>\
<html>\
<head>\
	<meta name='viewport' content='width=device-width, initial-scale=1'>\
	<link rel='stylesheet' href='http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.css' />\
	<script src='http://code.jquery.com/jquery-1.11.1.min.js'></script>\
	<script src='http://code.jquery.com/mobile/1.4.5/jquery.mobile-1.4.5.min.js'></script>\
<script>\
$(function() {\
$('#sliVal').html('Temp: 38');\
$('#slider').slider({\
    orientation:'vertical',value:38,min: 20,max: 42,step: 1\
});\
$('#slider').slider().bind({\
slidestop: function(e,ui){\
    $('#sliVal').html('Temp: '+ui.value);\
    $.get('/temp?val=' + ui.value, function(d, s){\
    }); \
$('#button').button().bind({\
slidestop: function(e,ui){\
    $('#sliVal').html('Temp: '+ui.value);\
    $.get('/temp?val=' + ui.value, function(d, s){\
    }); \
}\
});\
});\
</script>\
</head>\
<body>\
	<div data-role='header'>\
		<h1>MSPA CAMARO</h1>\
	</div>\
	<div role='main' class='ui-content'>\
    <form>\
	<select name = 'select-native-s' id = 'select-native-s'>\
	<option value = 'small'>On</option>\
	<option value = 'medium'>Off</option>\
	</select>\
	<label for = 'slider-s'>Temp: </label>\
	<input type = 'range' name = 'slider-s' id = 'slider-s' value = '38' min = '0' max = '100' data - highlight = 'true'>\
	<a href='#' class='ui - btn ui - corner - all'>Anchor</a>\
	<button class = 'ui-btn ui-corner-all'>Button</button>\
	</select>\
	</form>\
	</div>\
	</body>\
	</html>";

//char res[1200] =
//"<!DOCTYPE html>\
//<html>\
//<head>\
//<meta charset='utf-8' name='viewport' content='width=device-width, initial-scale=1'>\
//<style>\
//.html{ font - family: Helvetica; display: inline - block; margin: 0px auto; text - align: center; }\
//.button{ background - color: #4CAF50; border: none; color: white; padding: 16px 40px;\
//text - decoration: none; font - size: 30px; margin: 2px; cursor: pointer; }\
//.button2{ background - color: #555555; }\
//</style>\
//<H1>MSpa Camaro</H1>\
//<link href='https://code.jquery.com/ui/1.10.4/themes/ui-lightness/jquery-ui.css' rel='stylesheet'>\
//<script src='https://code.jquery.com/jquery-1.10.2.js'></script>\
//<script src='https://code.jquery.com/ui/1.10.4/jquery-ui.js'></script>\
//<script>\
//$( function() {\
//$('.widget input[type=submit], .widget a, .widget button').button();\
//$('button, input, a').click(function(event) {\
//event.preventDefault();\
//}\
//});\
//});\
//</script>\
//</head>\
//<body>\
//<input class='ui - button ui - widget ui - corner - all' type='submit' value='SPA On'>\
//</body>\
//</html>";

/* topics */
#define DOMOTICZ_OUT    "domoticz/out"
#define DOMOTICZ_IN     "domoticz/in"

#define EEPROM_SIZE 64

byte ctrl_startByte, ctrl_statusByte1, ctrl_statusByte2, ctrl_statusSeq;
byte main_startByte, main_statusByte1, main_statusByte2, main_statusSeq;

/* Debug Serial*/
HardwareSerial Main_debug(1); //GPIO 2 TxD, GPIO 4 RxD
HardwareSerial Ctrl_debug(2); //GPIO 17 TxD, GPIO 16 RxD

long lastMsg = 0;

void receivedCallback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message received: ");
	Serial.println(topic);

	Serial.print("payload: ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();

	switch ((char)payload[0]) {
	case '0': // SPA ON/OFF
		spa_on_off();
		break;
	case '1': // HEATER ON/OFF
		heater_on_off();
		break;
	case '2': // TEMP -> 20 GRADER
		set_temp(20);
		break;
	case '3': // TEMP -> 38 GRADER
		set_temp(38);
		break;
	case '4': // TEMP -> 40 GRADER
		set_temp(40);
		break;
	case '5': // AUTO
		mode_auto();
		break;
	case '6': // MANUAL
		mode_manual();
		break;
	case '7': // FILTER
		filter_on_off();
		break;
	case '8': // O3
		o3_on_off();
		break;
	case '9': // BUBBLE ON
		bubble_on();
		break;
	case 'B': // BUBBLE OFF
		bubble_off();
		break;
	case 'J': // JET
		jet();
		break;
	case 'S': // START
		start_sequence();
		break;
	}

}

void handleRoot() {
	server.send(200, "text/html", res);
}

void handleNotFound() {
	String message = "File Not Found\n\n";
	server.send(404, "text/plain", message);
}

void WiFiReset() {
	WiFi.persistent(false);
	WiFi.disconnect();
	WiFi.mode(WIFI_OFF);
	WiFi.mode(WIFI_STA);
}

void mqttconnect() {
	/* Loop until reconnected */
	while (!mqtt_client.connected()) {
		Serial.print("MQTT connecting ...");
		/* client ID */
		String mqtt_clientId = "ESP32Client";
		/* connect now */
		if (mqtt_client.connect(mqtt_clientId.c_str())) {
			Serial.println("connected");
			/* subscribe topic with default QoS 0*/
			mqtt_client.subscribe(DOMOTICZ_OUT);
		}
		else {
			Serial.print("failed, status code =");
			Serial.print(mqtt_client.state());
			Serial.println("try again in 5 seconds");
			/* Wait 5 seconds before retrying */
			delay(10000);
		}
	}
}

void setup() {
	Serial.begin(115200);
	Ctrl_debug.begin(9708, SERIAL_8N1, 16, 17); //Initialize Ctrl debug
	Main_debug.begin(9708, SERIAL_8N1, 4, 2); //Initialize Main debug

///* initialize EEPROM */
//if (!EEPROM.begin(EEPROM_SIZE))
//{
//	Serial.println("failed to initialise EEPROM"); delay(1000);
//}
//EEPROM.write(temp_addr, byte(temperature));
//temperature = EEPROM.read(temp_addr);
//Serial.println("Lagrad Temperatur");
//Serial.println(temperature);

	Serial.println();
	Serial.println();
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFiReset();
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
	if (MDNS.begin("esp32")) {
		Serial.println("MDNS responder started");
	}

	server.on("/", handleRoot);
	server.on("/temp", handleTemp);
	server.onNotFound(handleNotFound);

	server.begin();
	/* configure the MQTT server with IPaddress and port */
	mqtt_client.setServer(mqtt_server, 1883);
	/* this receivedCallback function will be invoked when client received subscribed topic */
	mqtt_client.setCallback(receivedCallback);

	/*Connect to MQTT*/
	if (!mqtt_client.connected()) {
		mqttconnect();
	}
}

void handleTemp() {
	int newTemp = server.arg(0).toInt();
	Serial.println("Request from SPA CTRL");
	Serial.println(newTemp);
	server.send(200, "text/html", "ok");
}
void mode_manual() {
	pinMode(DB0, INPUT);
	pinMode(DB1, INPUT);
	pinMode(DB2, INPUT);
	pinMode(DB3, INPUT);
	pinMode(DB4, INPUT);
	pinMode(DB5, INPUT);
	pinMode(DB6, INPUT);
	pinMode(DB7, INPUT);
}

void mode_auto() {
	pinMode(DB0, OUTPUT);
	pinMode(DB1, OUTPUT);
	pinMode(DB2, OUTPUT);
	pinMode(DB3, OUTPUT);
	pinMode(DB4, OUTPUT);
	pinMode(DB5, OUTPUT);
	pinMode(DB6, OUTPUT);
	pinMode(DB7, OUTPUT);
	digitalWrite(DB0, HIGH);
	digitalWrite(DB1, HIGH);
	digitalWrite(DB2, HIGH);
	digitalWrite(DB3, HIGH);
	digitalWrite(DB4, HIGH);
	digitalWrite(DB5, HIGH);
	digitalWrite(DB6, HIGH);
	digitalWrite(DB7, HIGH);
}

void spa_on_off() {
	digitalWrite(DB0, LOW);
	delay(500);
	digitalWrite(DB0, HIGH);
}

void heater_on_off() {
	digitalWrite(DB1, LOW);
	delay(500);
	digitalWrite(DB1, HIGH);
}

void jet() {
	digitalWrite(DB7, LOW);
	delay(500);
	digitalWrite(DB7, HIGH);
}

void filter_on_off() {
	digitalWrite(DB4, LOW);
	delay(500);
	digitalWrite(DB4, HIGH);
}

void o3_on_off() {
	digitalWrite(DB5, LOW);
	delay(500);
	digitalWrite(DB5, HIGH);
}

void bubble_on() {
	digitalWrite(DB6, LOW);
	delay(500);
	digitalWrite(DB6, HIGH);
}

void bubble_off() {
	digitalWrite(DB6, LOW);
	delay(2000);
	digitalWrite(DB6, HIGH);
}
void set_temp(int set_temp) {
	int difftemp = (set_temp - temperature); // Negativ difftemp -> minska temp, positiv difftemp -> �ka temp
	Serial.println("Tempskillnad: ");
	Serial.println(abs(difftemp));
	if (difftemp<0) {
		temp_down(abs(difftemp));
	}
	else if (difftemp>0) {
		temp_up(abs(difftemp));
	}
	else {

	}
	temperature = set_temp;
	// EEPROM.write(temp_addr, byte(set_temp));
}

void temp_up(int difftemp) {
	Serial.println("Temp Upp");
	Serial.println(difftemp);
	for (int i = -1;i < difftemp;i++) {
		digitalWrite(DB2, LOW);
		delay(500);
		digitalWrite(DB2, HIGH);
		delay(500);
	}
}

void temp_down(int difftemp) {
	Serial.println("Temp Ner");
	Serial.println(difftemp);
	for (int i = -1;i < difftemp;i++) {
		digitalWrite(DB3, LOW);
		delay(500);
		digitalWrite(DB3, HIGH);
		delay(500);
	}
}

void start_sequence() {
	startup_status = "on";
	Serial.println("Startar SPA");
	update_log("SPA startar!");
	mode_auto();
	update_log("SPA Auto mode set");
	delay(500);
	update_selector(idx_man_auto, 10); // Set mode auto
	delay(500);
	update_switch(idx_ozone, 0); // Set O3 to OFF in Domoticz
	delay(500);
	update_switch(idx_bubble, 0); // Set bubble to OFF in Domoticz
	delay(500);
	update_selector(idx_temp, 20); // Set start temp 38 degrees in Domoticz
	delay(500);
	//heater_on_off(); //Switch heater ON
	//delay(500);
	//update_switch(idx_heater, 1); // Set heater to ON in Domoticz
}

void update_switch(int idx, int nvalue)
{
	StaticJsonBuffer<300> JSONbuffer;
	JsonObject& JSONencoder = JSONbuffer.createObject();
	JSONencoder["idx"] = idx;
	JSONencoder["nvalue"] = nvalue;
	char JSONmessageBuffer[100];
	JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
	Serial.println("Sending message to MQTT topic..");
	Serial.println(JSONmessageBuffer);

	if (mqtt_client.publish(DOMOTICZ_IN, JSONmessageBuffer) == true) {
		Serial.println("Success sending message");
	}
	else {
		Serial.println("Error sending message");
	}
}

void update_log(char message[50])
{
	StaticJsonBuffer<300> JSONbuffer;
	JsonObject& JSONencoder = JSONbuffer.createObject();
	JSONencoder["command"] = "addlogmessage";
	JSONencoder["message"] = message;
	char JSONmessageBuffer[100];
	JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
	Serial.println("Sending message to MQTT topic..");
	Serial.println(JSONmessageBuffer);

	if (mqtt_client.publish(DOMOTICZ_IN, JSONmessageBuffer) == true) {
		Serial.println("Success sending message");
	}
	else {
		Serial.println("Error sending message");
	}
}

void update_selector(int idx, int level)
{
	StaticJsonBuffer<300> JSONbuffer;
	JsonObject& JSONencoder = JSONbuffer.createObject();
	JSONencoder["command"] = "switchlight";
	JSONencoder["idx"] = idx;
	JSONencoder["switchcmd"] = "Set Level";
	JSONencoder["level"] = level;
	char JSONmessageBuffer[100];
	JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
	Serial.println("Sending message to MQTT topic..");
	Serial.println(JSONmessageBuffer);

	if (mqtt_client.publish(DOMOTICZ_IN, JSONmessageBuffer) == true) {
		Serial.println("Success sending message");
	}
	else {
		Serial.println("Error sending message");
	}
}

void update_setpoint(int idx, int temp)
{
	StaticJsonBuffer<300> JSONbuffer;
	JsonObject& JSONencoder = JSONbuffer.createObject();
	JSONencoder["command"] = "udevice";
	JSONencoder["idx"] = idx;
	JSONencoder["nvalue"] = 0;
	JSONencoder["value"] = temp;
	char JSONmessageBuffer[100];
	JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
	Serial.println("Sending message to MQTT topic..");
	Serial.println(JSONmessageBuffer);

	if (mqtt_client.publish(DOMOTICZ_IN, JSONmessageBuffer) == true) {
		Serial.println("Success sending message");
	}
	else {
		Serial.println("Error sending message");
	}
}

void loop() {

	/* CHECK WIFI AND MQTT CONNECTION */
	if ((WiFi.status() != WL_CONNECTED)) {
		while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			Serial.print(".");
		}
		Serial.println("");
		Serial.println("WiFi connected");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
	}
	if ((WiFi.status() == WL_CONNECTED) && !mqtt_client.connected()) {
		mqttconnect();
	}

	mqtt_client.loop();     // internal household function for MQTT

	if (startup_status == "off")
		start_sequence();
	else {}
	if (power == "on" && last_power != "on") {
		update_switch(idx_on_off, 1); // Set SPA to On in Domoticz
		last_power = "on";
	}
	else if (power == "off" && last_power != "off") {
		update_switch(idx_on_off, 0); // Set SPA to Off in Domoticz
		last_power = "off";
	}
	if (heater == "on" && last_heater != "on") {
		update_switch(idx_heater, 1); // Set SPA to On in Domoticz
		last_heater = "on";
	}
	else if (heater == "off" && last_heater != "off") {
		update_switch(idx_heater, 0); // Set SPA to Off in Domoticz
		last_heater = "off";
	}

	server.handleClient();

	/* SERIAL RECEIVE FROM SPA KEYBOARD UNIT */
	if (Ctrl_debug.available()>0) { // there are bytes in the serial buffer to read
		ctrl_startByte = Ctrl_debug.read(); // read in the next byte
		if (ctrl_startByte == 0xA5) { //startOfMessage
			Serial.println("Found start!");
			ctrl_statusSeq = Ctrl_debug.read();
			Serial.println(ctrl_statusSeq, HEX);
			ctrl_statusByte1 = Ctrl_debug.read();
			Serial.println(ctrl_statusByte1, HEX);
			ctrl_statusByte2 = Ctrl_debug.read();
			Serial.println(ctrl_statusByte2, HEX);

			switch (ctrl_statusSeq) {
			case 1: // SPA ON/OFF
				if (ctrl_statusByte1 == 0)
				{
					Serial.println("SPA Off");
					power = "off";
				}
				else if (ctrl_statusByte1 == 1)
				{
					Serial.println("SPA On");
					power = "on";
				}
				break;
			case 2: // SEKVENS 2
				Serial.print("Sekvens 2, Value: ");
				Serial.println(ctrl_statusByte1, HEX);
				break;
			case 3: // HEATER ON/OFF
				if (ctrl_statusByte1 == 0)
				{
					Serial.println("Heater Off");
					heater = "off";
				}
				else if (ctrl_statusByte1 == 1)
				{
					Serial.println("Heater On");
					heater = "on";
				}
				break;
			case 4: // SEKVENS 4
				Serial.print("Sekvens 4, Value: ");
				Serial.println(ctrl_statusByte1, HEX);
				break;
			case 5: // TEMP
				Serial.print("Ny Temp ");
				Serial.println(ctrl_statusByte1, DEC);
				break;
			case 10: // SEKVENS 10
				Serial.print("Sekvens 10, Value: ");
				Serial.println(ctrl_statusByte1, HEX);
				break;
			case 11: // SEKVENS 11
				Serial.print("Sekvens 11, Value: ");
				Serial.println(ctrl_statusByte1, HEX);
				break;
			}
		}
		else {}
	}

	/* SERIAL RECEIVE FROM SPA MAIN UNIT */

	if (Main_debug.available()>0) { // there are bytes in the serial buffer to read
		main_startByte = Main_debug.read(); // read in the next byte
		Serial.print("Main Unit Rx: ");
		Serial.println(main_startByte, HEX);
	}
	else {}

}





