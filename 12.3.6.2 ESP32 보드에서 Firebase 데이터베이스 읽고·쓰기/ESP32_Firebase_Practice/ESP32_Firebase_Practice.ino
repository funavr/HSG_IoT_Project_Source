#include <WiFi.h>                // 인터넷 연결 라이브러리 포함
#include <Firebase_ESP_Client.h>        // Firebase 라이브러리 포함
#include <addons/TokenHelper.h>     // 토큰 생성 프로세스 정보 제공
#include <addons/RTDBHelper.h>      // RTDB 페이로드 정보 및 기타 기능

#define API_KEY " "             // 본인의 웹 API Key 입력
#define DATABASE_URL " "            // 본인의 URL 입력

FirebaseData fbdo;              // Firebase 데이터 객체 생성
FirebaseAuth auth;              // Firebase 인증 데이터 객체 생성
FirebaseConfig config;              // Firebase 구성 데이터 객체 생성

const char* ssid = " ";             // 본인의 무선 공유기 SSID 입력
const char* password = " ";         // 본인의 무선 공유기 패스워드 입력

void setup()
{
    Serial.begin(9600);

    WiFi.begin(ssid, password);         // 무선 공유기 연결 시작

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {   // 무선 공유기 연결 실패 시,
        Serial.println("WiFi Error: wWiFi Connection Failed!");
        return;                 // 실패 메시지 출력 후 종료
    }
    else                        // 무선 공유기 연결되면,
    {
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());     // ESP32 보드에 할당된 IP 주소 출력
    }

    config.api_key = API_KEY;           // Firebase API 키 저장
    config.database_url = DATABASE_URL;     // Firebase 데이터베이스 주소 저장

    if (Firebase.signUp(&config, &auth, "", "")) // Firebase 사용자 로그인
    {
        Serial.println("firebase log in success.");
    }
    else
    {
        Serial.print("Fail to log in: ");
        Serial.println(config.signer.signupError.message.c_str());
    }

    // 장시간 접속을 위한 토큰 생성 함수 설정
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);     // Firebase 인증과 구성으로 초기화
    Firebase.reconnectWiFi(true);       // 무선 공유기 재접속 시도 설정

    if (Firebase.ready())               // Firebase 준비 완료 시,
    {
        String insertURL = String("node");  // 문자열이 설정될 노드의 경로

        // 데이터베이스 실행 결과
        bool resultInsert = Firebase.RTDB.setString(&fbdo, insertURL.c_str(), "hello world");
        if (resultInsert)
        {
            Serial.println("Success to insert test");
        }
        else
        {
            Serial.print("Fail to insert test: ");
            Serial.println(fbdo.errorReason()); // 에러 이유 출력
        }

        Serial.println("READ DB");

        if (Firebase.RTDB.getString(&fbdo, "node"))
        {   // 해당 경로의 데이터 읽기
            String result = fbdo.stringData();  // 문자열로 읽고 저장
            Serial.println(result);
        }
        else
        {
            Serial.print("Fail to get test: ");
            Serial.println(fbdo.errorReason());
        }
    }
    else
    {
        Serial.println("Firebase is not Ready.");
    }
}

void loop()
{

}
