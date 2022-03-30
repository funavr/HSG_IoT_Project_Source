/*
    Title: IoT Project ESP32 with Firebase
    Author: HSG RnD
    Date: 2021-12-05
*/

#include <SPIFFS.h>                                                                 // SPIFFS 사용
#include <WiFi.h>                                                                   // WiFI 공유기 연결
#include <AsyncTCP.h>                                                               // 비동기 연결 기능 지원
#include <ESPAsyncWebServer.h>                                                      // 비동기 웹서버 사용
#include <time.h>                                                                   // NTP 이용

#include <Firebase_ESP_Client.h>                                                    // Firebase Client 라이브러리
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define API_KEY ""                                                                  // 자신의 API Key 입력

#define DATABASE_URL ""                                                             // 자신의 URL 입력

FirebaseData fbdo;     //파이어베이스 객체 생성
FirebaseAuth auth;     // 파이어베이스 인증 객체 생성
FirebaseConfig config; // 파이어베이스 설정 객체 생성

AsyncWebServer server(80);                                                          // 웹 서버 객체 셍성, 웹 서버 접근 포트: 80번

const unsigned long timeOut = 1000;                                                 // 시리얼 통신 시 응답 대기 시간 설정(1초)

const char* ssid = "";                                                 // WiFi 공유기 연결 정보
const char* password = "";

unsigned int interval;                                                              // ms 단위
float humidity;                                                                     // 우노 보드에서 보내오는 습도 값 저장
float temperature;                                                                  // 우노 보드에서 보내오는 온도 값 저장

const char* ntpServer = "pool.ntp.org";                                             // Network Time Procotol Server URL
const long  gmtOffset_sec = 32400;                                                  // 3600초는 1시간, 우리나라는 GMT+9 이므로 3600(초/시간) x 9시간 = 32400초로 설정
const int daylightOffset_sec = 0;                                                   // 섬머타임 적용 시 3600 (시간을 1시간 빠르게 함)초를 추가, 우리나라는 시행 안하므로 0

String result;

void SendInterval(unsigned long _interval_s)
{
    Serial2.write(0x4F);                                                            // 총 5byte 의 데이터 패킷을 보낸다.

    Serial2.write(_interval_s >> 24);                                               // 4바이트의 길이의 변수를 전송하기 위해
    Serial2.write(_interval_s >> 16);                                               // 상위 바이트 부터 전송한다.
    Serial2.write(_interval_s >> 8);
    Serial2.write(_interval_s);
}

unsigned long GetSystemInterval()
{
    Serial.println("Request Interval to the Arduino Uno");
    Serial2.write(0x4E);                                                            // 아두이노 우노에게 주기 값을 요청하는 코드를 전송한다.

    unsigned long prev = millis();                                                  // 1초 타임아웃 확인 하기 위해 시작 시간 기록

    while (true)
    {
        if (Serial2.available())
        {
            byte code = Serial2.read();

            if (code == 0x4E)
            {
                while (!(Serial2.available() >= 4))                                 // 아두이노 우노가 4바이트를 전송할 때 까지 대기
                {
                    if (millis() - prev >= timeOut)                                 // 1초 동안 아무런 응답을 받지 못하면 종료
                    {
                        Serial.println("Fail to get Interval");
                        return 0;                                                   // 실패 시 0을 리턴
                    }
                }

                unsigned long temp = Serial2.read() << 24;                          // 1byte 씩 수신하여
                temp |= Serial2.read() << 16;                                       // 최상위 바이트부터 채워간다.
                temp |= Serial2.read() << 8;
                temp |= Serial2.read();

                Serial.print("receive from uno: ");
                Serial.println(temp);

                return temp;
            }
        }

        if (millis() - prev >= timeOut)                                             // 1초 동안 아무런 응답을 받지 못하면 종료
        {
            Serial.println("Fail to get Interval");
            return 0;                                                               // 실패 시 0을 리턴
        }
    }

    return 0;
}

void SendTempLimit(float limit)
{
    Serial2.write(0x4D);                                                            // 총 5byte 의 데이터 패킷을 보낸다.

    Serial2.write((int)limit >> 24);                                                // 4바이트의 길이의 변수를 전송하기 위해
    Serial2.write((int)limit >> 16);                                                // 상위 바이트 부터 전송한다.
    Serial2.write((int)limit >> 8);                                                 // float 형은 비트 연산을 할 수 없으므로
    Serial2.write((int)limit);                                                      // int 형으로 자료형을 변경한다.
}

float GetTempLimit()
{
    Serial.println("Request Temperature Limit to the Arduino Uno");
    Serial2.write(0x4C);                                                            // 아두이노 우노에게 주기 값을 요청하는 코드를 전송한다.

    unsigned long prev = millis();                                                  // 1초 타임아웃 확인 하기 위해 시작 시간 기록

    while (true)
    {
        if (Serial2.available())
        {
            byte code = Serial2.read();

            if (code == 0x4C)
            {
                while (!(Serial2.available() >= 4))                                 // 아두이노 우노가 4바이트를 전송할 때 까지 대기
                {
                    if (millis() - prev >= 10000)                                   // 1초 동안 아무런 응답을 받지 못하면 종료
                    {
                        Serial.println("Fail to get Interval");
                        return 0;                                                   // 실패 시 0을 리턴
                    }
                }

                unsigned long temp = Serial2.read() << 24;                          // float형 데이터를 1바이트씩 받아서
                temp |= Serial2.read() << 16;                                       // 4바이트 형 변수에 채워 놓는다.
                temp |= Serial2.read() << 8;
                temp |= Serial2.read();

                float _tempLimit = (float)temp;                                     // 시리얼 통신으로 수신한 데이터를 float로 변환한다.

                Serial.print("receive from uno: ");
                Serial.println(_tempLimit);

                return _tempLimit;
            }
        }

        if (millis() - prev >= 10000)                                               // 1초 동안 아무런 응답을 받지 못하면 종료
        {
            Serial.println("Fail to get Interval");
            return 0;                                                               // 실패 시 0을 리턴
        }
    }

    return 0;
}

void SendHumiLimit(float limit)
{
    Serial2.write(0x4B);                                                            // 총 5byte 의 데이터 패킷을 보낸다.

    Serial2.write((int)limit >> 24);                                                // 4바이트의 길이의 변수를 전송하기 위해
    Serial2.write((int)limit >> 16);                                                // 상위 바이트 부터 전송한다.
    Serial2.write((int)limit >> 8);                                                 // float 형은 비트 연산을 할 수 없으므로
    Serial2.write((int)limit);                                                      // int 형으로 자료형을 변경한다.
}

float GetHumiLimit()
{
    Serial.println("Request Humidity Limit to the Arduino Uno");
    Serial2.write(0x4A);                                                            // 아두이노 우노에게 주기 값을 요청하는 코드를 전송한다.

    unsigned long prev = millis();                                                  // 1초 타임아웃 확인 하기 위해 시작 시간 기록

    while (true)
    {
        if (Serial2.available())
        {
            byte code = Serial2.read();

            if (code == 0x4A)
            {
                while (!(Serial2.available() >= 4))                                 // 아두이노 우노가 4바이트를 전송할 때 까지 대기
                {
                    if (millis() - prev >= 10000)                                   // 1초 동안 아무런 응답을 받지 못하면 종료
                    {
                        Serial.println("Fail to get Interval");
                        return 0;                                                   // 실패 시 0을 리턴
                    }
                }

                unsigned long temp = Serial2.read() << 24;                          // float형 데이터를 1바이트씩 받아서
                temp |= Serial2.read() << 16;                                       // 4바이트 형 변수에 채워 놓는다.
                temp |= Serial2.read() << 8;
                temp |= Serial2.read();

                float _tempLimit = (float)temp;                                     // 시리얼 통신으로 수신한 데이터를 float로 변환한다.

                Serial.print("receive from uno: ");
                Serial.println(_tempLimit);

                return _tempLimit;
            }
        }

        if (millis() - prev >= 10000)                                               // 1초 동안 아무런 응답을 받지 못하면 종료
        {
            Serial.println("Fail to get Interval");
            return 0;                                                               // 실패 시 0을 리턴
        }
    }

    return 0;
}

void notFound(AsyncWebServerRequest *request)                                       // 클라이언트가 웹 서버에서 원하는 페이지를 찾지 못했을 때 출력되는 함수로 에러 메시지를 출력
{
    request->send(404, "text/plain", "Not found");
}

void setup()
{
    Serial.begin(115200);                                                            // 시리얼 통신 속도 설정
    Serial2.begin(9600);                                                            // 아두이노 우노와 연결, 시리얼 통신 속도 설정

    if (!SPIFFS.begin(true))                                                        // SPIFFS 기능 사용여부 확인
    {
        Serial.println("ESP32 System Error: Can not use SPIFFS");
        return;                                                                     // 시스템 종료
    }

    File root = SPIFFS.open("/");                                                   // SPIFFS 디렉터리 구조 이상 유무 확인

    if (!root)                                                                      // 루트 파일 객체를 얻지 못하면
    {
        Serial.println("ESP32 System Error: Can not use File");
        return;                                                                     // 시스템 종료
    }

    File file = root.openNextFile();                                                // 내부 파일 목록 출력을 위한 첫번째 파일 탐색

    while (file)                                                                    // SPIFFS의 모든 내부 파일을 출력
    {
        Serial.print("File: ");                                                     // 파일 이름과 사이즈 출력
        Serial.print(file.name());
        Serial.print("\tSize: ");
        Serial.println(file.size());

        file = root.openNextFile();                                                 // 다음 파일을 탐색
    }

    root.close();                                                                   // 파일 객체 닫기
    file.close();

    WiFi.begin(ssid, password);                                                     // WiFi 공유기  연결 시도

    if (WiFi.waitForConnectResult() != WL_CONNECTED)                                // WiFi 공유기 연결 실패
    {
        Serial.println("WiFi Error: wWiFi Connection Failed!");
        return;                                                                     // 시스템 종료
    }
    else                                                                            // WiFi 공유기 연결 성공
    {
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());                                             // ESP32에 할당된 IP 주소 출력
    }

    config.api_key = API_KEY; // 파이어베이스 접근 주소 설정

    config.database_url = DATABASE_URL; // 파이어베이스 데이터베이스 주소 설정

    if (Firebase.signUp(&config, &auth, "", "")) // 파이어베이스 사용자 로그인
    {
        Serial.println("Firebase login successful.");
    }
    else
    {
        Serial.print("Fail to log in: ");
        Serial.println(config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback; // 장시간 접속을 위한 토큰 생성 함수 설정

    Firebase.begin(&config, &auth); // 파이어베이스 객체 초기화
    Firebase.reconnectWiFi(true);   // WiFi 끊김 시 재 접속 시도 설정

    server.on                                                                       // 클라이언트가 서버에 접속할 때
    (
        "/",                                                                        // index.html 을 요청하는 경로
        HTTP_GET,
        [](AsyncWebServerRequest * request)
    {
        Serial.println("Send INDEX page");
        request->send(SPIFFS, "/index.html", "text/html");                      // SPIFFS에 저장된 웹 페이지 문서를 보내준다.
    }
    );


    server.on
    (
        "/data",                                                                    // 웹 페이지에 보여줄 온 습도 값을 요청하는 경로
        HTTP_GET,
        [] (AsyncWebServerRequest * request)
    {
        Serial.print(millis());
        Serial.print(" -> client request data: ");
        Serial.println(String(humidity) + " , " + String(temperature));             // 습도와 온도 값을 콤마로 구분하여 보낸다.
        request->send(200, "text/plain", String(humidity) + "," + String(temperature));
    }
    );

    server.on
    (
        "/interval",                                                                // 웹 페이지에 보여줄 센서 값 측정 주기값을 GET 방식으로 요청하는 경로
        HTTP_GET,
        [] (AsyncWebServerRequest * request)
    {
        Serial.println("Request System interval value to Aruino Uno.");
        interval = GetSystemInterval();                                             // Arduino uno로부터 interval 값을 읽어온다.

        if (interval == 0)                                                          // 읽어온 값이 0이면
        {
            Serial.print(millis());
            Serial.println(" -> Fail to get interval value");
            request->send(200, "text/plain", "0");                                  // 실패로 간주하고 0을 보낸다.
            return;
        }

        Serial.print(millis());
        Serial.print(" -> Sensing Interval value(s): ");
        Serial.println(interval);
        request->send(200, "text/plain", String(interval));                         // 주기 값을 보낸다.
    }
    );

    server.on
    (
        "/interval",                                                                // 사용자가 웹 페이지에서 설정한 주기 값를 POST 방식으로 받는 경로
        HTTP_POST,
        [](AsyncWebServerRequest * request)
    {
        String userSettingInterval;                                                 // 사용자가 설정한 값을 저장하는 변수

        if (request->hasParam("interval", true))                                    // JSON형식의 데이터에서 interval을 키로 하는 데이터를 찾는다.
        {
            userSettingInterval = request->getParam("interval", true)->value();     // interval을 키로 하는 데이터의 값을 읽어와서 저장한다.

            Serial.print("Client set interval: ");
            Serial.println(userSettingInterval);

            // 문자열로 받은 수를 정수로 변환
            unsigned long _interval = (unsigned long)atoi(userSettingInterval.c_str());

            if (_interval == 0)                                                     // 정수 변환 결과가 0이면
            {
                request->send(200, "text/plain", "0");                              // 잘못된 데이터로 간주
                return;                                                             // 0을 보내고 종료한다.
            }

            SendInterval(_interval);                                                // 새로운 주기 값을 Arduino uno 에게 전송한다.

            unsigned long ret = GetSystemInterval();                                // 새로운 주기 값으로 설정되었는지 검사하기 위해 주기 값을 읽어온다.

            if (ret != _interval)                                                   // Arduino uno로 부터 읽어온 주기 값과 사용자가 설정한 값이 다르면
            {
                request->send(200, "text/plain", "0");                              // 설정 실패로 간주
                return;                                                             // 0을 보내고 종료한다.
            }
            else                                                                    // 두 값이 같으면
            {
                request->send(200, "text/plain", userSettingInterval);              // 새롭게 설정한 값을 보낸다.
                return;                                                             // 종료
            }
        }

        request->send(200, "text/plain", "0");                                      // interval을 키로 하는 데이터를 찾지 못하면 0을 보낸다.
    }
    );

    server.on
    (
        "/humidity",                                                                // 웹 페이지에 보여줄 습도 제한 값을 GET 방식으로 요청하는 경로
        HTTP_GET,
        [] (AsyncWebServerRequest * request)
    {
        Serial.println("Request System Humidity Limit value to Aruino Uno.");
        float _humidityLimit = GetHumiLimit();                                      // Arduino uno로부터 humidity limit 값을 읽어온다.

        if (_humidityLimit == 0)                                                    // 읽어온 값이 0이면
        {
            Serial.print(millis());
            Serial.println(" -> Fail to get humidity limit value");
            request->send(200, "text/plain", "0");                                  // 실패로 간주하고 0을 보낸다.
            return;
        }

        Serial.print(millis());
        Serial.print(" -> System humidity limit: ");
        Serial.println(_humidityLimit);
        request->send(200, "text/plain", String(_humidityLimit));                   // 습도 제한 값을 보낸다.
    }
    );

    server.on
    (
        "/humidity",                                                                // 사용자가 웹 페이지에서 설정한 습도 제한 값를 POST 방식으로 받는 경로
        HTTP_POST,
        [](AsyncWebServerRequest * request)
    {
        String userSettingHumidity;                                                 // 사용자가 설정한 값을 저장하는 변수

        if (request->hasParam("humidity", true))                                    // JSON형식의 데이터에서 humidity를 키로 하는 데이터를 찾는다.
        {
            userSettingHumidity = request->getParam("humidity", true)->value();     // humidity를 키로 하는 데이터의 값을 읽어와서 저장한다.

            Serial.print("Client set humidity limit: ");
            Serial.println(userSettingHumidity);

            // 문자열로 받은 수를 실수로 변환
            float _humidityLimit = (float)atof(userSettingHumidity.c_str());

            if (_humidityLimit == 0)                                                // 실수 변환 결과가 0이면
            {
                request->send(200, "text/plain", "0");                              // 잘못된 데이터로 간주
                return;                                                             // 0을 보내고 종료한다.
            }

            SendHumiLimit(_humidityLimit);                                          // 새로운 습도 제한 값를 Arduino uno 에게 전송한다.

            float ret = GetHumiLimit();                                             // 새로운 습도 제한 값으로 설정되었는지 검사하기 위해 습도 제한 값을 읽어온다.

            if (ret != _humidityLimit)                                              // Arduino uno로 부터 읽어온 습도 제한 값과 사용자가 설정한 값이 다르면
            {
                request->send(200, "text/plain", "0");                              // 설정 실패로 간주
                return;                                                             // 0을 보내고 종료한다.
            }
            else                                                                    // 두 값이 같으면
            {
                request->send(200, "text/plain", userSettingHumidity);              // 새롭게 설정한 값을 보낸다.
                return;                                                             // 종료
            }
        }

        request->send(200, "text/plain", "0");                                      // humidity을 키로 하는 데이터를 찾지 못하면 0을 보낸다.
    }
    );

    server.on
    (
        "/temperature",                                                             // 웹 페이지에 보여줄 온도 제한 값을 GET 방식으로 요청하는 경로
        HTTP_GET,
        [] (AsyncWebServerRequest * request)
    {
        Serial.println("Request System Temperature Limit value to Aruino Uno.");
        float _temperatureLimit = GetTempLimit();                                   // Arduino uno로부터 temperature limit 값을 읽어온다.

        if (_temperatureLimit == 0)                                                 // 읽어온 값이 0이면
        {
            Serial.print(millis());
            Serial.println(" -> Fail to get temperature limit value");
            request->send(200, "text/plain", "0");                                  // 실패로 간주하고 0을 보낸다.
            return;
        }

        Serial.print(millis());
        Serial.print(" -> System temperature limit: ");
        Serial.println(_temperatureLimit);
        request->send(200, "text/plain", String(_temperatureLimit));                // 온도 제한 값을 보낸다.
    }
    );

    server.on
    (
        "/temperature",                                                             // 사용자가 웹 페이지에서 설정한 온도 제한 값를 POST 방식으로 받는 경로
        HTTP_POST,
        [](AsyncWebServerRequest * request)
    {
        String userSettingTemperature;                                              // 사용자가 설정한 값을 저장하는 변수

        if (request->hasParam("temperature", true))                                 // JSON형식의 데이터에서 temperature를 키로 하는 데이터를 찾는다.
        {
            userSettingTemperature = request->getParam("temperature", true)->value();// temperature를 키로 하는 데이터의 값을 읽어와서 저장한다.

            Serial.print("Client set temperature limit: ");
            Serial.println(userSettingTemperature);

            // 문자열로 받은 수를 실수로 변환
            float _tempLimit = (float)atof(userSettingTemperature.c_str());

            if (_tempLimit == 0)                                                    // 실수 변환 결과가 0이면
            {
                request->send(200, "text/plain", "0");                              // 잘못된 데이터로 간주
                return;                                                             // 0을 보내고 종료한다.
            }

            SendTempLimit(_tempLimit);                                              // 새로운 온도 제한 값를 Arduino uno 에게 전송한다.

            float ret = GetTempLimit();                                             // 새로운 온도 제한 값으로 설정되었는지 검사하기 위해 온도 제한 값을 읽어온다.

            if (ret != _tempLimit)                                                  // Arduino uno로 부터 읽어온 온도 제한 값과 사용자가 설정한 값이 다르면
            {
                request->send(200, "text/plain", "0");                              // 설정 실패로 간주
                return;                                                             // 0을 보내고 종료한다.
            }
            else                                                                    // 두 값이 같으면
            {
                request->send(200, "text/plain", userSettingTemperature);           // 새롭게 설정한 값을 보낸다.
                return;                                                             // 종료
            }
        }

        request->send(200, "text/plain", "0");                                      // temperature을 키로 하는 데이터를 찾지 못하면 0을 보낸다.
    }
    );

    server.on
    (
        "/log/temperature",                                                         // 사용자가 웹 페이지에서 설정한 온도 제한 값를 POST 방식으로 받는 경로
        HTTP_GET,
        [] (AsyncWebServerRequest * request)
    {
        String startParam = "";                                                     // 조회 시작 날짜를 저장할 변수
        String endParam = "";                                                       // 조회 종료 날짜를 저장할 변수

        Serial.print(millis());
        Serial.println(" -> Client Request temperature log");

        if (request->hasParam("start"))                                             // JSON형식의 데이터에서 start를 키로 하는 데이터를 찾는다.
        {
            startParam = request->getParam("start")->value();                       // start를 키로 하는 데이터의 값을 읽어와서 저장한다.
        }

        if (request->hasParam("end"))                                               // JSON형식의 데이터에서 end를 키로 하는 데이터를 찾는다.
        {
            endParam = request -> getParam("end") -> value();                       // end를 키로 하는 데이터의 값을 읽어와서 저장한다.
        }

        if (startParam == "" || endParam == "")                                     // start와 end중 하나라도 비어 있으면
        {
            Serial.println("param error");
            return;                                                                 // 조회를 중단한다.
        }

        if (Firebase.ready())
        {
            if (Firebase.RTDB.getJSON(&fbdo, "/temperature"))
            {
                result = fbdo.jsonString();
                Serial.println("Temperature log read success");

                request->send(200, "text/plain", result);
            }
            else
            {
                Serial.println(fbdo.errorReason());
                request->send(200, "text/plain", "error");
            }
        }
    }
    );

    server.on
    (
        "/log/humidity",
        HTTP_GET,
        [] (AsyncWebServerRequest * request)
    {
        String startParam = "";                                                     // 조회 시작 날짜를 저장할 변수
        String endParam = "";                                                       // 조회 종료 날짜를 저장할 변수

        Serial.print(millis());
        Serial.println(" -> Client Request temperature log");

        if (request->hasParam("start"))                                             // JSON형식의 데이터에서 start를 키로 하는 데이터를 찾는다.
        {
            startParam = request->getParam("start")->value();                       // start를 키로 하는 데이터의 값을 읽어와서 저장한다.
        }

        if (request->hasParam("end"))                                               // JSON형식의 데이터에서 end를 키로 하는 데이터를 찾는다.
        {
            endParam = request -> getParam("end") -> value();                       // end를 키로 하는 데이터의 값을 읽어와서 저장한다.
        }

        if (startParam == "" || endParam == "")                                     // start와 end중 하나라도 비어 있으면
        {
            Serial.println("param error");
            return;                                                                 // 조회를 중단한다.
        }

        if (Firebase.ready())
        {
            if (Firebase.RTDB.getJSON(&fbdo, "/humidity"))
            {
                result = fbdo.jsonString();
                Serial.println("Humidity log read success");

                request->send(200, "text/plain", result);
            }
            else
            {
                Serial.println(fbdo.errorReason());
                request->send(200, "text/plain", "error");
            }
        }
    }
    );

    server.onNotFound(notFound);                                                    // 404 에러 시 출력 함수

    server.begin();                                                                 // 서버 시작

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);                       // ESP32 시스템 시간 초기화 및 설정
}

void loop()
{
    if (Serial2.available() >= 9)                                                   // 아두이노 우노가 보내는 센서 데이터 패킷 수신
    {
        byte code = Serial2.read();                                                 // 첫 번째 데이터를 읽었을 때

        if (code == 0xF0)                                                           // 0xF0 이면 온습도 데이터로 간주
        {
            uint32_t humidityDataPacket = 0;                                        // 4바이트 변수를 저장할 변수

            byte humiData = Serial2.read();                                         // 1바이트씩 읽어서 4바이트 변수에 저장
            humidityDataPacket |= humiData << 24;                                   // 최상단에서부터 8비트씩 채워넣는다.
            humiData = Serial2.read();
            humidityDataPacket |= humiData << 16;
            humiData = Serial2.read();
            humidityDataPacket |= humiData << 8;
            humiData = Serial2.read();
            humidityDataPacket |= humiData;

            uint32_t temperatureDataPacket = 0;                                     // 4바이트 변수를 저장할 변수

            byte tempData = Serial2.read();
            temperatureDataPacket |= tempData << 24;                                // 최상단에서부터 8비트씩 채워넣는다.
            tempData = Serial2.read();
            temperatureDataPacket |= tempData << 16;
            tempData = Serial2.read();
            temperatureDataPacket |= tempData << 8;
            tempData = Serial2.read();
            temperatureDataPacket = tempData;

            humidity = (float)humidityDataPacket;                                   // 첫번째 4바이트는 습도 데이터
            temperature = (float)temperatureDataPacket;                             // 두번째 4바이트는 온도 데이터

            Serial.print("Humidity: ");                                             // 온도 습도 데이터 출력
            Serial.print(humidity);
            Serial.println("%");

            Serial.print("Temperature: ");
            Serial.print(temperature);
            Serial.println("C");

            struct tm timeInfo;// 시간 정보

            if (!getLocalTime(&timeInfo))
            {
                Serial.println("Failed to obtain time");
                return;
            }
            Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S");

            String dateTime = String(timeInfo.tm_year + 1900) + "-" + String(timeInfo.tm_mon + 1) + "-" + String(timeInfo.tm_mday) + " " + String(timeInfo.tm_hour) + ":" + String(timeInfo.tm_min) + ":" + String(timeInfo.tm_sec);

            Serial.print("date time: ");
            Serial.println(dateTime);

            if (Firebase.ready()) // 파이어베이스 준비 완료
            {
                String temperatureInsertQuery = String("/temperature/") + dateTime; // 데이터 베이스 저장 위치 설정
                String humidityInsertQuery = String("/humidity/") + dateTime;

                bool resultTemperature = Firebase.RTDB.setFloat(&fbdo, temperatureInsertQuery.c_str(), temperature); // 온도 값 저장
                if (resultTemperature)
                {
                    Serial.println("Success to insert temperature");
                }
                else
                {
                    Serial.print("Fail to insert temperature: ");
                    Serial.println(fbdo.errorReason());
                }

                bool resultHumidity = Firebase.RTDB.setFloat(&fbdo, humidityInsertQuery.c_str(), humidity); // 습도 값 저장
                if (resultHumidity)
                {
                    Serial.println("Success to insert humidity");
                }
                else
                {
                    Serial.print("Fail to insert humidity: ");
                    Serial.println(fbdo.errorReason());
                }
            }
        }
    }

}
