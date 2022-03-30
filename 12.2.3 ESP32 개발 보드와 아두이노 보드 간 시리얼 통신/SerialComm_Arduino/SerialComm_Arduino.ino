/*
Arduino Code
*/

#include <SoftwareSerial.h>        // 소프트웨어 시리얼 라이브러리 포함

#define RX  8               // Rx 8번 핀 
#define TX  9               // Tx 9번 핀

SoftwareSerial serialToEsp32(RX, TX);  // 소프트웨어 시리얼 객체 생성

void setup() 
{
    Serial.begin(9600);         // 아두이노 보드와 PC 간 통신 속도 설정
    serialToEsp32.begin(9600);      // 아두이노와 ESP32 보드 간 통신 속도 설정

    Serial.println("Start serial communication with ESP32\n");
}

void loop() 
{
    if(serialToEsp32.available())       // ESP32 보드로부터 데이터가 수신되면,
    {
        // 시리얼 버퍼에 저장된 '\n' 까지의 문자열을 Rxdata 변수에 저장 
        String Rxdata = serialToEsp32.readStringUntil('\n');
        Serial.print("ESP32: ");        
        Serial.println(Rxdata);         // ESP32 보드에서 보낸 문자열 출력
    }

    if(Serial.available())          // PC로부터 데이터가 수신되면,
    {
        // 시리얼 버퍼에 저장된 '\n' 까지의 문자열을 Txdata 변수에 저장
        String Txdata = Serial.readStringUntil('\n');
        Serial.print("Arduino: ");
        Serial.println(Txdata);     // ESP32 보드로 전송할 문자열 출력
        serialToEsp32.println(Txdata);  // ESP32 보드로 저장된 문자열 전송
    }
}
