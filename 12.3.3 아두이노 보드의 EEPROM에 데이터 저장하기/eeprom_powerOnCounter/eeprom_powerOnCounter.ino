#include <EEPROM.h>                                     // EEPROM 라이브러리 포함

const int powerOnCntAddr = 5;                           // 리셋 횟수를 저장해 둘 EEPROM 주소 지정

void setup() 
{
    Serial.begin(9600);

    // 주소 5에 저장된 값을 읽어 powerOnCnt 변수에 저장
    int powerOnCnt = EEPROM.read(powerOnCntAddr);       

    Serial.println("System Start");
    Serial.print("Power ON Count: ");
    Serial.println(powerOnCnt);                         // powerOnCnt 변숫값 출력

    // powerOnCnt 변숫값에 1씩 더한 값을 주소 5에 씀
    EEPROM.write(powerOnCntAddr, powerOnCnt + 1);
}

void loop() 
{

}
