#include <WiFi.h>                           // WiFi 라이브러리 포함
#include <time.h>                           // time 라이브러리 포함

const char* ssid = "";                      // 무선 공유기 SSID 확인 후 입력
const char* password = "";                  // 무선 공유기 비밀번호 확인 후 입력

const char* ntpServer = "pool.ntp.org";     // NTP 서버 주소

// 세계협정시 오프셋을 초 단위로 환산한 것으로 우리나라는 세계협정시보다 9시간 빠름
const long gmtOffSetSec = 32400;

// 서머타임 보정 시간으로 우리나라는 시행하지 않으므로 0으로 설정
const int dayLightOffsetSec = 0;

void printCurrentTime()                     // 현재 시간을 출력하는 함수
{
    struct tm timeinfo;                     // 시간 정보를 저장하는 구조체 변수 선언
    if(!getLocalTime(&timeinfo))            // 시스템 내부 시간 정보를 가져오지 못하면
    {
        Serial.println("Failed to get time info");  // 에러 메시지 출력
        return;                             // 함수 종료
    }
    // 시간 정보를 "년 월 일, 요일 시:분:초" 형식으로 출력
    Serial.println(&timeinfo, "%Y %B %d, %A %H:%M:%S");
}

void setup()
{
    Serial.begin(115200);           

    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);             // 무선 공유기 연결

    // 무선 공유기와의 연결 상태가 WL_CONNECTED일 때까지 반복
    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);                         // 재시도 대기 시간
        Serial.print(".");
    }

    Serial.println("WiFi Connected.");
    // NTP 서버와 ESP32 보드의 내부 시간 동기화
    configTime(gmtOffSetSec, dayLightOffsetSec, ntpServer);
}

void loop()
{
    printCurrentTime();
    delay(1000);                    
}
