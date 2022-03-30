void setup() 
{
    Serial.begin(115200);        // ESP32 보드와 PC 간 통신 속도 설정
    Serial2.begin(9600);            // ESP32 UART2와 아두이노 간 통신 속도 설정

    Serial.println("Start serial communication with Arduino\n");
}

void loop() 
{
    if(Serial2.available())         // 아두이노 보드로부터 데이터가 수신되면,
    {
        // 시리얼 버퍼에 저장된 '\n' 까지의 문자열을 Rxdata 변수에 저장 
        String Rxdata = Serial2.readStringUntil('\n');
        Serial.print("Arduino: ");
        Serial.println(Rxdata);     // 아두이노 보드에서 보낸 문자열 출력
    }

    if(Serial.available())          // PC로부터 데이터가 수신되면,
    {   
        // 시리얼 버퍼에 저장된 '\n' 까지의 문자열을 Rxdata 변수에 저장 
        String Txdata = Serial.readStringUntil('\n');
        Serial.print("ESP32: ");
        Serial.println(Txdata);     // 아두이노 보드로 전송할 문자열 출력
        Serial2.println(Txdata);        // 아두이노 보드로 저장된 문자열 전송
    }
}
