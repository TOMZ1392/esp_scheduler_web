

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <Arduino_JSON.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
typedef unsigned long ulong_dly;
typedef uint8_t (*taskFptr)(uint8_t pos);
// Replace with your network credentials
const char *ssid = "fitz_2G";
const char *password = "tz81299895";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output5State = "off";
String output4State = "off";
String output13State = "off";
String output14State = "off";

// Assign output variables to GPIO pins
const int output14 = 14;
const int output13 = 13;
const int output5 = 5;
const int output4 = 4;

void setup()
{
    Serial.begin(115200);
    // Initialize the output variables as outputs
    pinMode(output5, OUTPUT);
    pinMode(output4, OUTPUT);
    // Set outputs to LOW
    digitalWrite(output5, LOW);
    digitalWrite(output4, LOW);

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    pinMode(14, OUTPUT);   //d5
    pinMode(12, OUTPUT);   //d6
    pinMode(13, OUTPUT);   //d7
    pinMode(0, OUTPUT);    //d3
    pinMode(2, OUTPUT);    //d4
    pinMode(4, OUTPUT);    //d2
    pinMode(5, OUTPUT);    //d1
    digitalWrite(5, LOW);  // pwm
    digitalWrite(12, LOW); // pwm
    server.begin();
}

String hrDay_g = "----";
bool validateOperationTime(String data);
bool validateOperationTime(String data)
{
    // Serial.print("Inside validate: ");
    // Serial.println(data);
    JSONVar locDataWeb = JSON.parse(data);
    if (JSON.typeof(locDataWeb) == "undefined")
    {
        Serial.println("Parsing input failed!");
    }

    if (locDataWeb.hasOwnProperty("datetime"))
    {
        Serial.print("CurrentDatetime: ");
        Serial.println(locDataWeb["datetime"]);
        String datetime = (const char *)locDataWeb["datetime"];
        uint8_t posT = 0;
        uint8_t posColn = 0;
        uint8_t i = 0;
        for (; i < (uint8_t)(sizeof(datetime)); i++)
        {
            if (datetime[i] == 'T')
            {
                //Serial.print(datetime[i+1]);
                posT = i;
                break;
            }
        }
        char hrs[5] = {'-', '-', '-', '-', '\0'};
        Serial.println("HR ARY:");

        hrs[0] = datetime[posT + 1];
        hrs[1] = datetime[posT + 2];
        hrs[2] = datetime[posT + 4];
        hrs[3] = datetime[posT + 5];
        hrs[4] = '\0';
        uint16_t hrday = (uint16_t)(atoi(hrs));
        Serial.print("hrofday:");
        Serial.println(hrday);
        strcpy((char *)&hrDay_g, hrs);
    }
}

typedef uint8_t (*taskFptr)(uint8_t pos);
typedef struct
{
    uint8_t id;
    bool isActive;
    uint8_t (*taskFptr)(uint8_t pos);
    volatile ulong_dly duration;
    volatile ulong_dly startTime;
} task_dynSt;

task_dynSt taskQueue[5] = {{255, false, 0, 0}, {255, false, 0, 0}, {255, false, 0, 0}, {255, false, 0, 0}, {255, false, 0, 0}};

bool isPumpActive = false;

void removeTask(uint8_t pos)
{
     if(taskQueue[pos].id !=255)
     {
        taskQueue[pos].id = 255;
        taskQueue[pos].isActive = false;
        taskQueue[pos].taskFptr = (taskFptr)(void *)(0);
        taskQueue[pos].duration = 0;
        taskQueue[pos].startTime = 0;
     }
}


uint8_t scheduleTask(uint8_t id, ulong_dly duration, taskFptr taskRoutine)
{
    uint8_t pos = 0;
    for (pos = 0; pos < 5; pos++)
    {
        if (taskQueue[pos].id == 255)
        {
            taskQueue[pos].id = id;
            taskQueue[pos].taskFptr = taskRoutine;
            taskQueue[pos].duration = duration;
            taskQueue[pos].startTime = millis();
            taskQueue[pos].taskFptr(pos);
            break;
        }
    }
    if (pos == 5)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void updateTaskState()
{
    uint8_t pos = 0;
    for (pos = 0; pos < 5; pos++)
    {
         
        if (taskQueue[pos].id != 255)
        {
            ulong_dly pastTime =0;
             if(taskQueue[pos].isActive)
             { 
                 pastTime= millis() - taskQueue[pos].startTime;
             }
             else
             {
                taskQueue[pos].startTime=millis();
                pastTime= millis() - taskQueue[pos].startTime;
             }
            if (pastTime > taskQueue[pos].duration)
            {
                taskQueue[pos].duration = 0;
                taskQueue[pos].startTime = 0;
                 taskQueue[pos].taskFptr(pos);
                 removeTask(pos);
            }
            else
            {
                taskQueue[pos].taskFptr(pos);
            }
            
        }
    }
}

uint8_t drivePump(uint8_t pos)
{
    if (taskQueue[pos].duration == 0)
    {
        digitalWrite(output5, LOW);
        removeTask(pos);
        return 0;
    }
    else
    {
        if (!taskQueue[pos].isActive)
        {
            digitalWrite(output5, HIGH);
            taskQueue[pos].isActive = true;
        }
    }
    return 1;
}

    volatile unsigned long uold = 0;

    void loop()
    {
        updateTaskState();
        WiFiClient client = server.available(); // Listen for incoming clients
        client.setTimeout(5000);
        if (client)
        {                                  // If a new client connects,
            Serial.println("New Client."); // print a message out in the serial port
            String currentLine = "";       // make a String to hold incoming data from the client
            while (client.connected())
            { // loop while the client's connected
                if (client.available())
                {                           // if there's bytes to read from the client,
                    char c = client.read(); // read a byte, then
                    Serial.write(c);        // print it out the serial monitor
                    header += c;
                    if (c == '\n')
                    { // if the byte is a newline character
                        // if the current line is blank, you got two newline characters in a row.
                        // that's the end of the client HTTP request, so send a response:
                        if (currentLine.length() == 0)
                        {
                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                            // and a content-type so the client knows what's coming, then a blank line:
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println("Connection: close");
                            client.println();

                            if (header.indexOf("GET /5/on") >= 0)
                            {
                                Serial.println("GPIO 5 on");
                                output4State = "on";
                                scheduleTask(1,3000,&drivePump);
                                
                            }
                            else if (header.indexOf("GET /5/off") >= 0)
                            {
                                Serial.println("GPIO 5 off");
                                output4State = "off";
                            }

                            client.println("<!DOCTYPE html><html>");
                            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                            client.println("<link rel=\"icon\" href=\"data:,\">");
                            // CSS to style the on/off buttons
                            // Feel free to change the background-color and font-size attributes to fit your preferences
                            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                            client.println(".button { background-color: #f0e92b; border: none;border-radius:5px; color: white; padding: 16px 40px;");
                            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                            client.println(".button3 { position:relative;}");

                            client.println(".button2 {background-color: #3f4545;}</style></head>");

                            // Web Page Heading
                            client.println("<body><h1>Garden irrigation (&copy; Tomz)</h1>");

                            // Display current state, and ON/OFF buttons for GPIO 4

                            client.println("<p>Hour of day: " + hrDay_g + "</p>");

                            // If the output4State is off, it displays the ON button
                            if (output4State == "off")
                            {
                                client.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
                            }
                            else
                            {
                                client.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
                            }

                            client.println("</body></html>");

                            // The HTTP response ends with another blank line
                            client.println();
                            // Break out of the while loop
                            break;
                        }
                        else
                        { // if you got a newline, then clear currentLine
                            currentLine = "";
                        }
                    }
                    else if (c != '\r')
                    {                     // if you got anything else but a carriage return character,
                        currentLine += c; // add it to the end of the currentLine
                    }
                    // client.stop();
                }
            }
            // Clear the header variable
            header = "";
            // Close the connection
            //client.stop();
            Serial.println("Client disconnected.");
            Serial.println("");
        }
        //client.stop();
        volatile unsigned long curUs = micros();
        if ((curUs - uold) > 10000)
        {
            uold = curUs;
            HTTPClient http2;

            Serial.print("[HTTP] begin...\n");
            if (http2.begin(client, "http://worldtimeapi.org/api/timezone/Asia/Kolkata"))
            { // HTTP

                Serial.print("[HTTP] GET...\n");
                // start connection and send HTTP header
                int httpCode = http2.GET();

                // httpCode will be negative on error
                if (httpCode > 0)
                {
                    // HTTP header has been send and Server response header has been handled
                    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

                    // file found at server
                    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                    {
                        String payload = http2.getString();
                        Serial.println(payload);
                        validateOperationTime(payload);
                    }
                }
                else
                {
                    Serial.printf("[HTTP] GET... failed, error: %s\n", http2.errorToString(httpCode).c_str());
                }

                http2.end();
            }
            else
            {
                Serial.printf("[HTTP} Unable to connect\n");
            }
        }
    }
