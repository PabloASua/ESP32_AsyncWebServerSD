#include <Arduino.h>

#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h" //https://github.com/me-no-dev/ESPAsyncWebServer/blob/master/examples/CaptivePortal/CaptivePortal.ino

#include <SD.h>
#include <SPI.h>

void printDirectory(File dir, int numTabs);
bool loadFromSdCard(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);

DNSServer dnsServer;
AsyncWebServer server(80);
const int CS_SDcard = 5;


void setup(){

  Serial.begin(9600);
  delay(100);
  Serial.println("");

  Serial.print("Waiting for SD card to initialise...");
  if (!SD.begin(CS_SDcard)){Serial.println("Initialising failed!");}
  else {Serial.println("->OK");}

  File root = SD.open("/");
  root.rewindDirectory();
  printDirectory(root, 0); //Display the card contents
  root.close();




  Serial.println("Setting AP as esp-captive, no password");
  //your other setup stuff...
  WiFi.softAP("esp-captive");
  Serial.print("Local IP Address:"); 
  Serial.println(WiFi.localIP());
  dnsServer.start(53, "*", WiFi.softAPIP());
   
  server.onNotFound(handleNotFound);
  //more handlers...
  server.begin();
}

void loop(){
  dnsServer.processNextRequest();
}




void handleNotFound(AsyncWebServerRequest *request){
  
  Serial.println ("Function: handleNotFound reached!!!!!!!");

  String path = request->url();
  Serial.print("handleNotFound: ");
  Serial.println(path);

  if(loadFromSdCard(request)){return;}


  String message = "\nNo Handler\r\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET)?"GET":"POST";
  message += "\nParameters: ";
  message += request->params();
  message += "\n";
  for (uint8_t i=0; i<request->params(); i++){
    AsyncWebParameter* p = request->getParam(i);
    message += String(p->name().c_str()) + " : " + String(p->value().c_str()) + "\r\n";
  }
  request->send(404, "text/plain", message);
  Serial.print(message);
}




bool loadFromSdCard(AsyncWebServerRequest *request) {
      String path = request->url();
      String dataType = "text/plain";
      struct fileBlk {
        File dataFile;
      };
      fileBlk *fileObj = new fileBlk;
      

      if(path.endsWith("/generate_204")) {path = "/index.html"; Serial.println("¡*************!");}
      if(path.endsWith("/connecttest.txt")) {path = "/index.html"; Serial.println("¡*************!");}
      if(path.endsWith("/redirect")) {path = "/index.html"; Serial.println("¡*************!");}
      if(path.endsWith("/")) path += "index.htm";
      if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
      else if(path.endsWith(".html")) dataType = "text/html";
      else if(path.endsWith(".css")) dataType = "text/css";
      else if(path.endsWith(".js")) dataType = "application/javascript";
      else if(path.endsWith(".png")) dataType = "image/png";
      else if(path.endsWith(".gif")) dataType = "image/gif";
      else if(path.endsWith(".jpg")) dataType = "image/jpeg";
      else if(path.endsWith(".ico")) dataType = "image/x-icon";
      else if(path.endsWith(".xml")) dataType = "text/xml";
      else if(path.endsWith(".pdf")) dataType = "application/pdf";
      else if(path.endsWith(".zip")) dataType = "application/zip";
     
      fileObj->dataFile  = SD.open(path.c_str());
      if(fileObj->dataFile.isDirectory()){
        path += "/index.htm";
        dataType = "text/html";
        fileObj->dataFile = SD.open(path.c_str());
      }
    
      if (!fileObj->dataFile){
        delete fileObj;
        return false;
      }
    
      if (request->hasParam("download")) dataType = "application/octet-stream";

            // Here is the context problem.  If there are multiple downloads active, 
            // we don't have the File handles. So we only allow one active download request
            // at a time and keep the file handle in static.  I'm open to a solution.
    
      request->_tempObject = (void*)fileObj;
      request->send(dataType, fileObj->dataFile.size(), [request](uint8_t *buffer, size_t maxlen, size_t index) -> size_t {
                                                  fileBlk *fileObj = (fileBlk*)request->_tempObject;
                                                  size_t thisSize = fileObj->dataFile.read(buffer, maxlen);
                                                  if((index + thisSize) >= fileObj->dataFile.size()){
                                                    fileObj->dataFile.close();
                                                    request->_tempObject = NULL;
                                                    delete fileObj;
                                                  }
                                                  return thisSize;
                                                });
      return true;
}




void printDirectory(File dir, int numTabs)
{

    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            // no more files
            break;
        }
        if (numTabs > 0)
        {
            for (uint8_t i = 0; i <= numTabs; i++)
            {
                Serial.print('\t');
            }
        }
        Serial.print(entry.name());
        if (entry.isDirectory())
        {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        }
        else
        {
            // files have sizes, directories do not
            Serial.print("\t");
            Serial.println(entry.size(), DEC);
        }
        entry.close();
    }
}
