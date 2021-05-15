/* using ESP32 nodeMCU Board: DOIT ESP32 DEVKIT V1 + UBTech uKit Explore Controller */
/* version 3 - DHT + Telegram + Email w Threshold + Authentication (login & Logout) + Servo (Webcam Sweeping) */

// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "ESP32_MailClient.h"

// Replace with your network credentials
const char* ssid = "network SSID"; //  your network SSID (name)
const char* password = "network password";    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;

unsigned long myChannelNumber = 9999999; // your Thingspeak Channel Number
const char * myWriteAPIKey = "WriteAPIKey";  // your Thingspeak WriteAPI key

const char* http_username = "admin";
const char* http_password = "admin";

const char* PARAM_INPUT_1 = "state";

const int output = 2;

// To send Email using Gmail use port 465 (SSL) and SMTP Server smtp.gmail.com
// YOU MUST ENABLE less secure app option https://myaccount.google.com/lesssecureapps?pli=1
#define emailSenderAccount   "email account"
#define emailSenderPassword  "email password"
#define emailRecipient  "email account"
#define smtpServer      "smtp.gmail.com"
#define smtpServerPort  465
#define emailSubject  "[ESP32 ALERT] HIGH TEMPERATURE @COMPUTER ROOM"

// Default Threshold Temperature Value
String TempThreshold = "25.00";

// The Email Sending data object contains config and data to send
SMTPData smtpData;

// Callback function to get the Email sending status
void sendCallback(SendStatus info);

// Flag variable to keep track if email notification was sent or not
bool emailSent = false;

// Initialize Telegram BOT
#define BOTtoken "BOTtoken"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "-999999999"

WiFiClientSecure clientsecure;
UniversalTelegramBot bot(BOTtoken, clientsecure);

WiFiClient client;

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11

// DHT Sensor
uint8_t DHTPin = 32; 
               
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Checks for new messages every 1 minutes.
int botRequestDelay = 60000;
unsigned long lastTimeBotRan;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String readDHTTemperature() {

    float c = dht.readTemperature();
    if (isnan(c)) {    
      Serial.println("Failed to read temperature!");
      return "--";
    }
    else {
      Serial.print("");
      Serial.print("Temperature: ");
      Serial.print(c);
      Serial.println(" °C\t");
      return String(c);
    }
}

String readDHTHumidity() {

    float h = dht.readHumidity();
    if (isnan(h)) {
      Serial.println("Failed to read Humidity!");
      return "--";
    }
    else {
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.println(" %\t");
      return String(h);
    }
}

String outputState(){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Computer Room Surveillance Monitoring System</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    body {margin: 0; padding: 0}
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 2.0rem; }
    h3 { font-size: 1.2rem; }
    p { font-size: 2.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 5px;
    }
    .main-wrap {display: flex; flex-direction: row}
    .main-wrap > div {flex: 1}
    .switch {position: relative; display: inline-block; width: 80px; height: 48px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 24px}
    .slider:before {position: absolute; content: ""; height: 32px; width: 32px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 48px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(32px); -ms-transform: translateX(32px); transform: translateX(32px)}
  </style>
</head>
<body style="background-color:#F0F8FF">
  <h2>Computer Room Surveillance Monitoring System</h2>
  <button onclick="logoutButton()">Logout</button><hr>
  <p><h3>Webcam Sweeping <span id="state">%STATE%</span></h3></p>
     %BUTTONPLACEHOLDER%
<script>

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ 
    xhr.open("GET", "/update?state=1", true); 
    document.getElementById("state").innerHTML = "ON";
  }
  else { 
    xhr.open("GET", "/update?state=0", true); 
    document.getElementById("state").innerHTML = "OFF";
  }
  xhr.send();
}

function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 60000 ) ;
 
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 60000 ) ;

</script>
  
  <div class="main-wrap">
    <div style="background-color:#F0F8FF">
      <p>
        <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
        <span class="dht-labels">Temperature</span> 
        <span id="temperature">%TEMPERATURE%</span>
        <sup class="units">&deg;C</sup>
      </p>
      <p>
        <i class="fas fa-tint" style="color:#00add6;"></i> 
        <span class="dht-labels">Humidity</span>
        <span id="humidity">%HUMIDITY%</span>
        <sup class="units">%</sup>
      </p>
    </div>
    <div style="background-color:#F0F8FF">
       <iframe width="644" height="484" src="http://192.168.43.99:8080/video"></iframe>    
    </div>
</div>

<body>
</html>
)rawliteral";

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <p>Logged out or <a href="/">return to homepage</a>.</p>
  <p><strong>Note:</strong> close all web browser tabs to complete the logout process.</p>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  if (var == "STATE"){
    if(digitalRead(output)){
      return "ON";
    }
    else {
      return "OFF";
    }
  }
  if(var == "TEMPERATURE"){
    return readDHTTemperature();
  }
  else if(var == "HUMIDITY"){
    return readDHTHumidity();
  }
 return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 25, 26);
  Serial.print("Start!");
  delay(100);

  pinMode(DHTPin, INPUT);
  dht.begin();              

  // Attempt to connect to Wifi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }

  // Print ESP Local IP Address
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "Bot started up", "");
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html, processor);
  });
 
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(401);
  });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", logout_html, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      if (inputMessage == "1") {
        Serial2.write("Y");
      }
      else {
        Serial2.write("N");
      }
 //     digitalWrite(output, inputMessage.toInt());
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println("inputMessage: " + inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();
}

bool sendEmailNotification(String emailMessage){
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // For library version 1.2.0 and later which STARTTLS protocol was supported,the STARTTLS will be 
  // enabled automatically when port 587 was used, or enable it manually using setSTARTTLS function.
  //smtpData.setSTARTTLS(true);

  // Set the sender name and Email
  smtpData.setSender("ESP32", emailSenderAccount);

  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("High");

  // Set the subject
  smtpData.setSubject(emailSubject);

  // Set the message with HTML format
  smtpData.setMessage(emailMessage, true);

  // Add recipients, you can add more than one recipient
  smtpData.addRecipient(emailRecipient);

  smtpData.setSendCallback(sendCallback);

  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)) {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    return false;
  }
  // Clear all data from Email object to free memory
  smtpData.empty();
  return true;
}

// Callback function to get the Email sending status
void sendCallback(SendStatus msg) {
  // Print the current status
  Serial.println(msg.info());

  // Do something when complete
  if (msg.success()) {
    Serial.println("----------------");
  }
}

void loop() {
  
  if (millis() > lastTimeBotRan + botRequestDelay) {

    float c = dht.readTemperature();
    float h = dht.readHumidity();
    String statusText = "";
      
    statusText = "Temperature: " + String(c) + " °C" + "\nHumidity: " + String(h) + " %";

    bot.sendMessage(CHAT_ID, statusText, "");

    // Check if temperature is above threshold and if it needs to send the Email alert
    if(c > TempThreshold.toFloat() && !emailSent){
      String emailMessage = String("Temperature above threshold. Current temperature: ") + 
                            String(c) + " °C";
      if(sendEmailNotification(emailMessage)) {
        Serial.println(emailMessage);
        emailSent = false;
      }
      else {
        Serial.println("Email failed to send");
      }    
    }
    lastTimeBotRan = millis();
  }
}
