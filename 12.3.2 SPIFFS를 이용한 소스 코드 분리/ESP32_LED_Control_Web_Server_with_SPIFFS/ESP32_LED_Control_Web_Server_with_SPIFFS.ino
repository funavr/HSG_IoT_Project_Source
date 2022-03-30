/*
* Title: ESP32 LED Constrol Async Web Server With SPIFFS
* Author: HSG RnD
* Date: 2021-12-05
*/

#include <SPIFFS.h>                                                                 // SPIFFS 라이브러리 포함

#include <WiFi.h>                                                                   // WiFi 연결 기능을 제공
#include <AsyncTCP.h>                                                               // ESPAsyncWebServer.h에서 필요한 기능을 제공하는 라이브러리
#include <ESPAsyncWebServer.h>                                                      // 비동기 웹 서버를 구현하는 라이브러리

AsyncWebServer server(80);                                                          // 웹 서버 객체 생성

                                                                                    // ssid와 pasword 공유기 접속 정보 확인 후 입력
const char* ssid = "";
const char* password = "";

String indexHtml;                                                                   // 문자열 클래스의 인스턴스 생성
const char * controlParam = "led";                                                  // HTTP GET 요쳥시 사용할 키값

const int ledPin = 18;                                                              // LED 핀 번호

String GetLedState(int pin)                                                         // 해당 IO핀의 상태를 반환
{
    if (digitalRead(pin) == HIGH)
    {
        return "checked";
    }
    else
    {
        return "";
    }
}

String LEDStateDisplay(const String& contentId)                                     // 초기 LED 상태를 웹 페이지에 포함시키는 함수
{
    if (contentId == "LED_STATE_PLACE_HOLDER")                                      // 웹 페이지의 전처리구문
    {
        String buttons = "";
        buttons += "<h4>LED State</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + GetLedState(ledPin) + "><span class=\"slider\"></span></label>";
        return buttons;
    }
    return String();
}


void setup()
{
    Serial.begin(9600);                                                             // 시리얼 통신 속도 설정

    if (!SPIFFS.begin(true))                                                        // SPIFFS 기능 사용여부 확인
    {
        Serial.println("Error: Mounting SPIFFS");
        return;                                                                     // 시스템 종료
    }

    File indexFile = SPIFFS.open("/index.html");                                    // index.html 파일 오픈

    while (indexFile.available())                                                   // index.html 파일 내용 읽기
    {
        indexHtml += indexFile.readString();
    }

    Serial.println(indexHtml);                                                      // 읽은 파일 내용 출력

    pinMode(ledPin, OUTPUT);                                                        // led핀 출력 모드 설정
    digitalWrite(ledPin, LOW);                                                      // led핀 초기화

    Serial.println("Start connecting WiFi");

    WiFi.mode(WIFI_STA);                                                            // 공유기 접속 모드로 설정
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)                                           // 공유기와 연결될 떄 까지 시도
    {
        delay(1000);                                                                // 1초 간격으로 재연결을 시도한다.
        Serial.print(millis());
        Serial.println(" Connecting WiFi...");
    }

    Serial.println("WiFi Connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());                                                 // 공유기로부터 할당 받은 아이피 주소 출력

                                                                                    // 웹 브라우저도 서버로 최초 접근 시 보여줄 페이지를 생성한다.
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        AsyncClient *pClient = request -> client();                                 // 서버로 접속한 클라이언트 정보를 가져온다.
        IPAddress clientIP = pClient -> remoteIP();                                 // 클라이언트의 IP주소를 알아낸다.

        Serial.println();
        Serial.print("Client IP : ");
        Serial.println(request->client()->remoteIP());

        request -> send_P(200, "text/html", indexHtml.c_str(), LEDStateDisplay);    // 전처리 함수를 거쳐서 웹 페이지 제공하기
    });

                                                                                    // 웹 브라우저에서 LED 제어를 요청할 때 수행할 작업을 등록
    server.on("/control", HTTP_GET, [](AsyncWebServerRequest * request) {
        String controlInput;

        if (request -> hasParam(controlParam))
        {
            controlInput = request -> getParam(controlParam) -> value();

            if (controlInput == "on")
            {
                digitalWrite(ledPin, HIGH);
            }
            else if (controlInput == "off")
            {
                digitalWrite(ledPin, LOW);
            }

            Serial.println("LED " + controlInput);
        }
    });

    server.begin();                                                                 // 웹 서버 시작
}

void loop()
{

}
