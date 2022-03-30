/*
    Title: IoT Project Arduino UNO
    Author: HSG RnD
    Date: 2021-12-05
*/

#include <EEPROM.h>                                                     // EEPROM 사용
#include <LiquidCrystal_I2C.h>                                          // I2C LCD Display 사용
#include <DHT.h>                                                       // DHT 센서 사용
#include <SoftwareSerial.h>                                             // 시리얼 통신 채널

#define EEPROM_ADDRESS_INTERVAL 0                                       // 측정 주기 값이 저장되는 eeprom 주소
#define EEPROM_ADDRESS_TEMPERATURE 10                                   // 온도 제한 값이 저장되는 eeprom 주소
#define EEPROM_ADDRESS_HUMIDITY 15                                      // 습도 제한 값이 저장되는 eeprom 주소

const int dhtPin = 2;                                                   // DHT 센서 디지털 출력 핀 우노 보드 2번 핀 연결
const int rxPin = 8;                                                    // 소프트웨어 시리얼 통신 채널 RX 핀 우노 보드 8번 핀 연결
const int txPin = 9;                                                    // 소프트웨어 시리얼 통신 채널 TX 핀 우노 보드 9번 핀 연결
const int ledPin = 4;                                                   // led 핀 우노 보드 4번 핀 연결
const int buzzerPin = 3;                                                // buzzer 핀 우노 보드 3번 핀 연결

DHT dht(dhtPin, DHT11);                                                 // DHT 객체 선언 핀 번호와 사용하는 DHT 버전을 알림
SoftwareSerial serialToEsp32(rxPin, txPin);                             // 소프트웨어 시리얼 채널 객체 생성

LiquidCrystal_I2C lcd(0x27, 16, 2);                                     // 0x27 I2C 주소를 가지고 있는 16x2 LCD객체를 생성합니다.(I2C 주소는 LCD에 맞게 수정해야 합니다.)

bool firstSendFlag;                                                     // 첫 시작 시 데이터 전송 확인용
unsigned long prev;                                                     // non-blocking delay 시간 저장

unsigned long intervalS;                                                // 센싱 주기: 초(s) 단위
unsigned long intervalMs;                                               // 센싱 주기 밀리초 단위로 변환 한것
float tempLimit;                                                        // 온도 제한 값
float humiLimit;                                                        // 습도 제한 값

void SetSystemInterval(unsigned long _interval_s)                       // 주기를 입력받아 저장한다.
{
    intervalS = _interval_s;
    intervalMs = (unsigned long)intervalS * 1000;                       // 초 단위 시간을 밀리 초 단위로 변환한다.

    Serial.print("Set New Interval(s): ");
    Serial.println(intervalS);

    EEPROM.put(EEPROM_ADDRESS_INTERVAL, intervalS);                     // EEPROM의 0번 주소 부터 4바이트에 intervalS를 저장한다.
}

unsigned long GetSystemInterval()
{
    intervalS = EEPROM.get(EEPROM_ADDRESS_INTERVAL, intervalS);         // EEPROM의 0번 주소 부터 4바이트를 읽어서 intervalS에 저장한다.

    if (intervalS < 60 || intervalS > 3600)                             // 범위 외의 값이면
    {
        intervalS = 60;                                                 // 60로 설정
    }

//    intervalS = 5;

    Serial.print("Get Interval(s): ");
    Serial.println(intervalS);

    return intervalS;
}

void SetTempLimit(float limit)
{
    Serial.print("Set New Temperature Limit: ");
    Serial.println(limit);

    EEPROM.put(EEPROM_ADDRESS_TEMPERATURE, limit);                      // EEPROM의 5번 주소 부터 4바이트에 온도 제한 값을 저장한다.
}

float GetTempLimit()
{
    tempLimit = EEPROM.get(EEPROM_ADDRESS_TEMPERATURE, tempLimit);      // EEPROM의 5번 주소 부터 4바이트를 읽어서 tempLimit에 저장한다.
    
    if (tempLimit < 0 || tempLimit > 50)                                // 범위 외의 값이면
    {
        tempLimit = 25;                                                 // 25로 설정
    }
    
    Serial.print("Get Temperature Limit: ");
    Serial.println(tempLimit);

    return tempLimit;
}

void SetHumiLimit(float limit)
{
    Serial.print("Set New Humidity Limit: ");
    Serial.println(limit);

    EEPROM.put(EEPROM_ADDRESS_HUMIDITY, limit);                         // EEPROM의 10번 주소 부터 4바이트에 습도 제한 값을 저장한다.
}

float GetHumiLimit()
{
    humiLimit = EEPROM.get(EEPROM_ADDRESS_HUMIDITY, humiLimit);         // EEPROM의 10번 주소 부터 4바이트를 읽어서 humiLimit에 저장한다.

    if (humiLimit < 20 || humiLimit > 90)                                // 범위 외의 값이면
    {
        humiLimit = 50;                                                 // 50로 설정
    }
    
    Serial.print("Get Humidity Limit: ");
    Serial.println(humiLimit);

    return humiLimit;
}

void LEDInit()                                                          // LED 핀 출력 모드 설정 및 초기화 설정
{
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
}

void setup()
{
    Serial.begin(9600);                                                 // 시리얼 통신 속도 설정
    serialToEsp32.begin(9600);                                          // 시리얼 통신 속도 설정

    Serial.println("ESP32 IoT Project - Arduino Data Collector\n");

    while (!serialToEsp32)                                              // 소프트웨어 시리얼 통신 객체 생성 여부 검사
    {
        Serial.println("esp32 serial is not open.");                     // 생성될 때 까지 대기
        delay(1000);
    }

    Serial.println("Start system initialization.");

    Serial.println("EEPROM initialization.");
    EEPROM.begin();                                                     // EEPROM 초기화

    Serial.println("LED initialization.");
    LEDInit();                                                          // LED 초기화

    Serial.println("DHT-11 sensor initialization.");
    dht.begin();                                                        // DHT 센서 시작

    Serial.println("I2C C-LCD initialization.");
    lcd.init();                                                         // I2C LCD 초기화
    lcd.backlight();                                                    // I2C LCD 백라이트 켜기

    Serial.println("Get measurement interval value from EEPROM");
    GetSystemInterval();                                                // 센서 측정 주기 값 불러오기
    intervalMs = intervalS * 1000;                                      // 주기 값 단위 변환 초 -> 밀리초

    Serial.println("Get sensor limit value from eeprom");
    GetTempLimit();                                                     // 온도 제한 값 가져오기
    GetHumiLimit();                                                     // 습도 제한 값 가져오기

    Serial.println("System initialization complete\n");

    prev = millis();                                                    // 딜레이 시작 시간 설정
    firstSendFlag = true;                                               // 초기 센서 값 측정 플레그 설정
}

void loop()
{
    if (serialToEsp32.available())
    {
        byte code = serialToEsp32.read();                               // ESP32가 보내는 명령 코드를 읽는다.

        if (code == 0x4F)                                               // ESP32로부터 주기 값(초)을 받는다.
        {
            Serial.println(F("\nReceive sensor measurement cycle value set by user from ESP32.\n"));

            while (!(serialToEsp32.available() >= 4))                   // 4바이트 수신 대기
            {
                ;
            }

            unsigned long intervalTemp = (unsigned long)serialToEsp32.read() << 24;
            intervalTemp |= (unsigned long)serialToEsp32.read() << 16;
            intervalTemp |= (unsigned long)serialToEsp32.read() << 8;
            intervalTemp |= (unsigned long)serialToEsp32.read();

            Serial.print(F("Incomming sensor measurement cycle value: "));
            Serial.println(intervalTemp);

            SetSystemInterval(intervalTemp);                            // EEPROM에 주기값을 저장하고
            GetSystemInterval();                                        // 측정 주기 값 갱신

            prev = millis();                                            // 측정 주기 갱신
        }
        else if (code == 0x4E)                                          // ESP32로부터 주기값 요청을 받음.
        {
            Serial.println(F("Received request to send measurement interval value to user from ESP32."));
            Serial.print("Current Interval: ");
            Serial.print(intervalS);
            Serial.println(" seconds");

            serialToEsp32.write(0x4E);

            serialToEsp32.write(intervalS >> 24);
            serialToEsp32.write(intervalS >> 16);
            serialToEsp32.write(intervalS >> 8);
            serialToEsp32.write(intervalS);
        }
        else if (code == 0x4D)                                          // ESP32로 부터 온도 제한 값을 받는다.
        {
            Serial.println(F("\nReceive temperature limit value from ESP32.\n"));

            while (!(serialToEsp32.available() >= 4))                   // 4바이트 수신 대기
            {
                ;
            }

            unsigned long limitTemp = (unsigned long)serialToEsp32.read() << 24;
            limitTemp |= (unsigned long)serialToEsp32.read() << 16;
            limitTemp |= (unsigned long)serialToEsp32.read() << 8;
            limitTemp |= (unsigned long)serialToEsp32.read();

            float limit = (float)limitTemp;

            Serial.print(F("Incomming temperature limit value: "));
            Serial.println(limit);

            SetTempLimit(limit);

            tempLimit = GetTempLimit();
        }
        else if (code == 0x4C)                                          // ESP32로 부터 온도 제한 값 요청을 받음.
        {
            Serial.println(F("Received request to send temperature limit value to user from ESP32."));
            Serial.print(F("Current temperature limit value: "));
            Serial.println(tempLimit);

            unsigned long temp = (unsigned long)tempLimit;

            serialToEsp32.write(0x4C);

            serialToEsp32.write(temp >> 24);
            serialToEsp32.write(temp >> 16);
            serialToEsp32.write(temp >> 8);
            serialToEsp32.write(temp);
        }
        else if (code == 0x4B)                                          // ESP32로 부터 습도 제한 값을 받는다.
        {
            Serial.println(F("\nReceive humidity limit value from ESP32.\n"));

            while (!(serialToEsp32.available() >= 4))
            {
                ;
            }

            unsigned long limitTemp = (unsigned long)serialToEsp32.read() << 24;
            limitTemp |= (unsigned long)serialToEsp32.read() << 16;
            limitTemp |= (unsigned long)serialToEsp32.read() << 8;
            limitTemp |= (unsigned long)serialToEsp32.read();

            float limit = (float)limitTemp;

            Serial.print(F("Incomming humidity limit value: "));
            Serial.println(limit);

            SetHumiLimit(limit);

            humiLimit = GetHumiLimit();
        }
        else if (code == 0x4A)                                          // ESP32로 부터 습도 제한 값 요청을 받음.
        {
            Serial.println(F("Received request to send humidity limit value to user from ESP32."));
            Serial.print(F("Current humidity limit value: "));
            Serial.println(humiLimit);

            unsigned long temp = (unsigned long)humiLimit;

            serialToEsp32.write(0x4A);

            serialToEsp32.write(temp >> 24);
            serialToEsp32.write(temp >> 16);
            serialToEsp32.write(temp >> 8);
            serialToEsp32.write(temp);
        }
    }

     if (millis() - prev >= intervalMs || firstSendFlag == true)     // 측정 주기를 넘거나 firstSendFlag가 참이면
    {
        firstSendFlag = false;

        Serial.println(F("Read temperature and humidity values."));

        delay(1000);

        float humidity = dht.readHumidity();                            // 습도 측정
        float temperature = dht.readTemperature();                      // 온도 측정

        if (isnan(humidity) || isnan(temperature))                      // 데이터 유효성 검증
        {
            Serial.println("Failed to read sensor data.");
            prev = millis();
            return;                                                     // 유효하지 않으면 함수 종료
        }

        if (humidity >= humiLimit || temperature >= tempLimit)          // 온도와 습도가 제한 값을 넘지 않았는지 검사
        {
            Serial.println(F("Temperature or humidity values ​​exceed limits."));

            digitalWrite(ledPin, HIGH);                                 // 넘었으면 LED를 켜고 부저를 3초동안 울린다.
            tone(buzzerPin, 500, 3000);
        }
        else
        {
            digitalWrite(ledPin, LOW);                                  // 넘지 않으면 LED와 부저를 끈다.
            noTone(buzzerPin);
        }

        Serial.print("humidity: ");                                     // 센서 값 출력
        Serial.print(humidity);
        Serial.print("% \t humidity limit: ");
        Serial.print(humiLimit);
        Serial.println("%");

        Serial.print("temperature: ");
        Serial.print(temperature);
        Serial.print("C \t temperature limit: ");
        Serial.print(tempLimit);
        Serial.println("C");

        Serial.println(F("\nTransmission of temperature and humidity values ​​to ESP32."));

        serialToEsp32.write(0xF0);                                      // 시리얼 통신 시작코드

        serialToEsp32.write((int)humidity >> 24);                       // 4byte 크기의 float 데이터를 1 byte씩 보내기
        serialToEsp32.write((int)humidity >> 16);
        serialToEsp32.write((int)humidity >> 8);
        serialToEsp32.write((int)humidity);

        serialToEsp32.write((int)temperature >> 24);                    // 4byte 크기의 float 데이터를 1 byte씩 보내기
        serialToEsp32.write((int)temperature >> 16);
        serialToEsp32.write((int)temperature >> 8);
        serialToEsp32.write((int)temperature);

        lcd.clear();                                                    // LCD의 모든 내용을 삭제합니다.

        lcd.setCursor(0, 0);                                            // 0번째 줄 0번째 셀부터 입력하게 합니다.

        lcd.print("humi: ");                                            // 아래의 문장을 출력합니다.
        lcd.print(String(humidity).c_str());
        lcd.print("%");

        lcd.setCursor(0, 1);                                            // 1번째 줄 0번째 셀부터 입력하게 합니다.

        lcd.print("temp: ");                                            // 아래의 문장을 출력합니다.
        lcd.print(String(temperature).c_str());
        lcd.print("C");

        prev = millis();
    }
}
